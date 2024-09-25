#pragma once

#include <optional>

#include "map_renderer.h"
#include "transport_catalogue.h"

class RequestHandler {
   public:
    RequestHandler(const catalogue::TransportCatalogue& db,
                   const renderer::MapRenderer& renderer);

    std::optional<BusStat> GetBusStat(const std::string_view& bus_name) const;

    const std::set<std::string_view>* GetBusesByStop(
        std::string_view stop_name) const;

    svg::Document RenderMap() const;

   private:
    const catalogue::TransportCatalogue& db_;
    const renderer::MapRenderer& renderer_;
};