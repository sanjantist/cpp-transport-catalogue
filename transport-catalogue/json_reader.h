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
#include "transport_router.h"

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
  router::TransportRouter router_;

  RequestsInfo DivideRequests();
  void ParseBaseRequests();
  json::Document ParseStatRequests();
  void ParseRenderSettings();

  size_t GetVertexCount() const;

  svg::Rgb ArrayToRgb(const json::Node &node);
  svg::Rgba ArrayToRgba(const json::Node &node);
};