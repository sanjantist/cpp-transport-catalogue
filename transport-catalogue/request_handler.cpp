#include "request_handler.h"

RequestHandler::RequestHandler(const catalogue::TransportCatalogue& db,
                               const renderer::MapRenderer& renderer)
    : db_(db), renderer_(renderer) {}

std::optional<BusStat> RequestHandler::GetBusStat(
    const std::string_view& bus_name) const {
    return db_.GetBusStat(bus_name);
};

const std::set<std::string_view>* RequestHandler::GetBusesByStop(
    std::string_view stop_name) const {
    return db_.GetBusesByStop(stop_name);
}

svg::Document RequestHandler::RenderMap() const {
    return renderer_.RenderMap();
}