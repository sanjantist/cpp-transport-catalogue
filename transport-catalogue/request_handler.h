#pragma once

#include <optional>
#include <string_view>

#include "graph.h"
#include "json.h"
#include "json_builder.h"
#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"

class RequestHandler {
 public:
  RequestHandler(const catalogue::TransportCatalogue& db,
                 const renderer::MapRenderer& renderer,
                 const router::TransportRouter& router);

  std::optional<BusStat> GetBusStat(const std::string_view& bus_name) const;

  const std::set<std::string_view>* GetBusesByStop(
      std::string_view stop_name) const;

  svg::Document RenderMap() const;
  json::Dict FindRoute(std::string_view from, std::string_view to, int request_id);

 private:
  const catalogue::TransportCatalogue& db_;
  const renderer::MapRenderer& renderer_;
  const router::TransportRouter& router_;
};