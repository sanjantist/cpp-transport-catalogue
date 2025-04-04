#pragma once

#include <cassert>
#include <cstddef>
#include <istream>
#include <ostream>
#include <string_view>
#include <unordered_map>

#include "graph.h"
#include "json.h"
#include "request_handler.h"
#include "transport_catalogue.h"

struct RequestsInfo {
  json::Array base_requests;
  json::Array stat_requests;
  json::Dict render_settings;
  json::Dict routing_settings;
};

class JsonReader {
 public:
  JsonReader(std::istream &input, catalogue::TransportCatalogue &catalogue);
  void ParseRequests(std::ostream &out);

 private:
  json::Document document_;
  RequestsInfo requests_;
  catalogue::TransportCatalogue *catalogue_;
  renderer::MapRenderer renderer_;

  double bus_velocity_;
  int bus_wait_time_;
  graph::DirectedWeightedGraph<double> graph_;
  graph::VertexId current_vertex_id_ = 0;
  std::unordered_map<std::string_view, graph::VertexId>
      stop_name_to_in_vertex_id_;
  std::unordered_map<graph::VertexId, std::string_view> vertex_id_to_stop_name_;
  std::unordered_map<graph::EdgeId, std::string_view> edge_to_bus_name_;
  std::unordered_map<graph::EdgeId, size_t> edge_to_span_count_;
  std::unordered_set<graph::EdgeId> waiting_edges_;

  RequestsInfo DivideRequests();
  void ParseBaseRequests();
  json::Document ParseStatRequests();
  void ParseRenderSettings();
  void ParseRoutingSettings();

  size_t GetVertexCount() const;
  void AddStopToGraph(std::string_view stop_name);
  double GetTravelTime(double distance) const;

  svg::Rgb ArrayToRgb(const json::Node &node);
  svg::Rgba ArrayToRgba(const json::Node &node);
};