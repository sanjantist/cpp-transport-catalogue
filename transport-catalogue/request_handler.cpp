#include "request_handler.h"

RequestHandler::RequestHandler(const catalogue::TransportCatalogue& db,
                               const renderer::MapRenderer& renderer)
    : db_(db), renderer_(renderer) {}

std::optional<BusStat> RequestHandler::GetBusStat(
    const std::string_view& bus_name) const {
    BusStat result;
    result.id = bus_name;

    const Bus* bus = db_.FindBus(bus_name);
    if (bus == nullptr) {
        return std::nullopt;
    }

    std::unordered_set<std::string_view> unique_stops;
    for (auto& stop : bus->route) {
        unique_stops.insert(stop->id);
    }
    result.unique_stops = unique_stops;

    double geo_route_length = 0.0;
    for (size_t i = 0; i < bus->route.size() - 1; ++i) {
        geo_route_length += geo::ComputeDistance(bus->route.at(i)->coords,
                                                 bus->route.at(i + 1)->coords);
    }

    double route_length = 0.0;
    for (size_t i = 0; i < bus->route.size() - 1; ++i) {
        std::string_view stop1, stop2;
        stop1 = bus->route.at(i)->id;
        stop2 = bus->route.at(i + 1)->id;
        route_length += db_.GetDistance(stop1, stop2);
    }

    result.route_length = route_length;
    result.curvature = route_length / geo_route_length;
    result.stops = bus->route.size();

    return result;
};

const std::set<std::string_view>* RequestHandler::GetBusesByStop(
    std::string_view stop_name) const {
    return db_.GetBusesByStop(stop_name);
}

svg::Document RequestHandler::RenderMap() const {
    return renderer_.RenderMap();
}