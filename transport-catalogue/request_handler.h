#pragma once

#include <optional>
#include <string_view>

#include "graph.h"
#include "json.h"
#include "json_builder.h"
#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"

struct GraphData {
  const std::unordered_map<std::string_view, graph::VertexId>&
      stop_name_to_in_vertex_id;
  const std::unordered_map<graph::VertexId, std::string_view>&
      vertex_id_to_stop_name_;
  const std::unordered_map<graph::EdgeId, std::string_view>& edge_to_bus_name_;
  const std::unordered_map<graph::EdgeId, size_t>& edge_to_span_count_;
  const std::unordered_set<graph::EdgeId>& waiting_edges_;
};

class RequestHandler {
 public:
  RequestHandler(const catalogue::TransportCatalogue& db,
                 const renderer::MapRenderer& renderer,
                 const graph::DirectedWeightedGraph<double>& graph,
                 GraphData graph_data);

  std::optional<BusStat> GetBusStat(const std::string_view& bus_name) const;

  const std::set<std::string_view>* GetBusesByStop(
      std::string_view stop_name) const;

  svg::Document RenderMap() const;
  json::Dict FindRoute(std::string_view from, std::string_view to);

 private:
  const catalogue::TransportCatalogue& db_;
  const renderer::MapRenderer& renderer_;
  const graph::DirectedWeightedGraph<double>& graph_;
  router::TranstportRouter router_;
  const GraphData graph_data_;
};