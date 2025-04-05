#pragma once
#include <memory>
#include <string_view>
#include <unordered_map>

#include "graph.h"
#include "json.h"
#include "router.h"
#include "transport_catalogue.h"

namespace router {
class TransportRouter {
 public:
  TransportRouter(size_t vertex_count, double bus_velocity, int bus_wait_time,
                  const catalogue::TransportCatalogue& db);
  void AddStopToGraph(std::string_view stop_name);
  void AddBusToGraph(catalogue::TransportCatalogue::BusPtr bus);
  void BuildRouter();
  json::Dict GetRouteInfo(std::string_view from_stop,
                          std::string_view to_stop) const;

 private:
  graph::DirectedWeightedGraph<double> graph_;
  std::unique_ptr<graph::Router<double>> router_;
  const catalogue::TransportCatalogue& db_;

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
};
}  // namespace router