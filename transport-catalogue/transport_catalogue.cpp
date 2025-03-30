#include "transport_catalogue.h"

#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "domain.h"
#include "geo.h"

std::optional<catalogue::TransportCatalogue::BusPtr>
catalogue::TransportCatalogue::AddBus(
    const std::string& id, const std::vector<std::string_view>& stops,
    bool is_roundtrip) {
    auto bus_pos = busname_to_buses_.find(id);
    if (bus_pos == busname_to_buses_.end()) {
        Bus bus;
        bus.id = id;
        bus.is_roundtrip = is_roundtrip;
        bus.route.reserve(stops.size());
        for (const auto& stop : stops) {
            if (stopname_to_stop_.count(stop) == 0) {
                return std::nullopt;
            }
            bus.route.push_back(stopname_to_stop_.at(stop));
        }

        buses_.push_back(std::move(bus));
        const Bus* added_bus = &buses_.back();
        busname_to_buses_.emplace(added_bus->id, added_bus);
        for (const Stop* stop : added_bus->route) {
            stop_to_buses_.at(stop).insert(added_bus->id);
        }

        return added_bus;
    }

    return std::nullopt;
}

std::optional<catalogue::TransportCatalogue::StopPtr>
catalogue::TransportCatalogue::AddStop(const std::string& id,
                                       const geo::Coordinates& coords) {
    Stop stop{id, coords};

    auto stop_pos = stopname_to_stop_.find(stop.id);
    if (stop_pos == stopname_to_stop_.end()) {
        stops_.push_back(std::move(stop));

        const Stop* added_stop = &stops_.back();
        stopname_to_stop_.emplace(added_stop->id, added_stop);
        stop_to_buses_.emplace(added_stop, std::set<std::string_view>{});

        return added_stop;
    }

    return std::nullopt;
}

void catalogue::TransportCatalogue::AddDistances(std::string_view from_stop,
                                                 std::string_view to_stop,
                                                 int distance) {
    auto from = stopname_to_stop_.find(from_stop);
    auto to = stopname_to_stop_.find(to_stop);
    if (from != stopname_to_stop_.end() && to != stopname_to_stop_.end()) {
        stops_to_distances_[{from->second->id, to->second->id}] = distance;
    }
}

int catalogue::TransportCatalogue::GetDistance(std::string_view from_stop,
                                               std::string_view to_stop) const {
    auto pos = stops_to_distances_.find(std::pair(from_stop, to_stop));
    if (pos != stops_to_distances_.end()) {
        return pos->second;
    } else {
        pos = stops_to_distances_.find(std::pair(to_stop, from_stop));
        if (pos == stops_to_distances_.end()) {
            throw std::runtime_error("no road between stops");
        }
        return stops_to_distances_.at(std::pair(to_stop, from_stop));
    }
}

std::optional<BusStat> catalogue::TransportCatalogue::GetBusStat(
    std::string_view bus_name) const {
    BusStat result;
    result.id = bus_name;

    const Bus* bus = FindBus(bus_name);
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
        route_length += GetDistance(stop1, stop2);
    }

    result.route_length = route_length;
    result.curvature = route_length / geo_route_length;
    result.stops = bus->route.size();

    return result;
}

const Bus* catalogue::TransportCatalogue::FindBus(std::string_view bus) const {
    auto bus_pos = busname_to_buses_.find(bus);
    if (bus_pos != busname_to_buses_.end()) {
        return bus_pos->second;
    }
    return nullptr;
}

const Stop* catalogue::TransportCatalogue::FindStop(
    std::string_view stop) const {
    auto stop_pos = stopname_to_stop_.find(stop);
    if (stop_pos != stopname_to_stop_.end()) {
        return stop_pos->second;
    }
    return nullptr;
}

const std::set<std::string_view>* catalogue::TransportCatalogue::GetBusesByStop(
    std::string_view stop_name) const {
    auto stop = FindStop(stop_name);
    if (stop) {
        return &stop_to_buses_.at(stop);
    }

    return nullptr;
}