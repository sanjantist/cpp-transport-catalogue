#pragma once
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "graph.h"
#include "json.h"
#include "router.h"
#include "transport_catalogue.h"

namespace router {
struct RouteInfo {
  double total_time;
  std::vector<std::unordered_map<std::string, std::string>> items;
};

class TransportRouter {
 public:
  TransportRouter(double bus_velocity, int bus_wait_time,
                  const catalogue::TransportCatalogue& db);
  std::optional<RouteInfo> GetRouteInfo(std::string_view from_stop,
                                        std::string_view to_stop) const;

 private:
  const catalogue::TransportCatalogue& db_;
  graph::DirectedWeightedGraph<double> graph_;
  std::unique_ptr<graph::Router<double>> router_;

  double bus_velocity_;
  int bus_wait_time_;
  graph::VertexId current_vertex_id_ = 0;
  std::unordered_map<std::string_view, graph::VertexId>
      stop_name_to_in_vertex_id_;
  std::unordered_map<graph::VertexId, std::string_view> vertex_id_to_stop_name_;
  std::unordered_map<graph::EdgeId, std::string_view> edge_to_bus_name_;
  std::unordered_map<graph::EdgeId, size_t> edge_to_span_count_;
  std::unordered_set<graph::EdgeId> waiting_edges_;

  double GetTravelTime(double distance) const;
  void AddStopToGraph(std::string_view stop_name);
  void AddBusToGraph(catalogue::TransportCatalogue::BusPtr bus);
  void BuildRouter();
};
}  // namespace router