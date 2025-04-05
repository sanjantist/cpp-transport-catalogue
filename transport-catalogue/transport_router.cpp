#include "transport_router.h"

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "domain.h"
#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

#define MINUTES_PER_HOUR 60
#define METERS_PER_KILOMETER 1'000

namespace router {
TransportRouter::TransportRouter(double bus_velocity, int bus_wait_time,
                                 const catalogue::TransportCatalogue& db)
    : db_(db),
      graph_(db_.GetStopCount() * 2),
      bus_velocity_(bus_velocity),
      bus_wait_time_(bus_wait_time) {
  BuildRouter();
}

void TransportRouter::AddStopToGraph(std::string_view stop_name) {
  if (!stop_name_to_in_vertex_id_.count(stop_name)) {
    vertex_id_to_stop_name_[current_vertex_id_] = stop_name;
    stop_name_to_in_vertex_id_[stop_name] = current_vertex_id_;
    ++current_vertex_id_;
    vertex_id_to_stop_name_[current_vertex_id_] = stop_name;
    ++current_vertex_id_;
    graph::EdgeId edge =
        graph_.AddEdge({current_vertex_id_ - 2, current_vertex_id_ - 1,
                        (double)bus_wait_time_});
    waiting_edges_.insert(edge);
  }
}

void TransportRouter::AddBusToGraph(catalogue::TransportCatalogue::BusPtr bus) {
  auto bus_route = bus->route;
  for (auto it_l = bus_route.begin(); it_l != bus_route.end() - 1; ++it_l) {
    std::string_view stop_from_name = (*it_l)->id;
    size_t current_span_count = 0;
    double current_travel_time = 0.0;
    for (auto it_r = it_l + 1; it_r != bus_route.end(); ++it_r) {
      std::string_view stop_to_name = (*it_r)->id;
      graph::VertexId stop_from_id, stop_to_id;
      stop_from_id = stop_name_to_in_vertex_id_.at(stop_from_name) + 1;
      stop_to_id = stop_name_to_in_vertex_id_.at(stop_to_name);

      ++current_span_count;
      std::string_view prev_stop = (*(it_r - 1))->id;
      current_travel_time +=
          GetTravelTime(db_.GetDistance(prev_stop, stop_to_name));
      graph::EdgeId edge =
          graph_.AddEdge({stop_from_id, stop_to_id, current_travel_time});
      edge_to_bus_name_[edge] = bus->id;
      edge_to_span_count_[edge] = current_span_count;
    }
  }
}

void TransportRouter::BuildRouter() {
  const std::deque<Stop>* all_stops = db_.GetAllStops();
  const std::deque<Bus>* all_buses = db_.GetAllBuses();
  for (const auto& stop : *all_stops) {
    AddStopToGraph(stop.id);
  }
  for (const auto& bus : *all_buses) {
    AddBusToGraph(&bus);
  }
  router_ = std::make_unique<graph::Router<double>>(graph_);
}

std::optional<RouteInfo> TransportRouter::GetRouteInfo(
    std::string_view from_stop, std::string_view to_stop) const {
  graph::VertexId from_in_id = stop_name_to_in_vertex_id_.at(from_stop);
  graph::VertexId to_in_id = stop_name_to_in_vertex_id_.at(to_stop);

  std::optional<graph::Router<double>::RouteInfo> route =
      router_->BuildRoute(from_in_id, to_in_id);
  if (route) {
    RouteInfo result;
    result.total_time = (*route).weight;
    for (const graph::EdgeId edge_id : (*route).edges) {
      std::unordered_map<std::string, std::string> item;
      std::string type = (waiting_edges_.count(edge_id) ? "Wait" : "Bus");
      item["type"] = type;
      if (type == "Wait") {
        graph::VertexId stop_id = graph_.GetEdge(edge_id).from;
        std::string stop_name(vertex_id_to_stop_name_.at(stop_id));
        item["stop_name"] = stop_name;
      } else if (type == "Bus") {
        item["bus"] = std::string(edge_to_bus_name_.at(edge_id));
        item["span_count"] = std::to_string(edge_to_span_count_.at(edge_id));
      }
      item["time"] = std::to_string(graph_.GetEdge(edge_id).weight);
      result.items.emplace_back(std::move(item));
    }
    return result;
  }
  return std::nullopt;
}

double TransportRouter::GetTravelTime(double distance) const {
  return (distance / METERS_PER_KILOMETER) / (bus_velocity_ / MINUTES_PER_HOUR);
}
}  // namespace router