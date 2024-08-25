#include "transport_catalogue.h"

#include <cstddef>
#include <set>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "geo.h"

void catalogue::TransportCatalogue::AddRoute(
    const std::string& id, const std::vector<std::string_view>& stops) {
    if (!busname_to_buses_.count(id)) {
        Bus bus;
        bus.id = id;
        bus.route.reserve(stops.size());
        for (const auto& stop : stops) {
            if (stopname_to_stop_.count(stop) == 0) {
                return;
            }
            bus.route.push_back(stopname_to_stop_.at(stop));
        }

        buses_.push_back(std::move(bus));
        const Bus* added_bus = &buses_.back();
        busname_to_buses_.emplace(added_bus->id, added_bus);
        for (const Stop* stop : added_bus->route) {
            stop_to_buses_.at(stop).insert(added_bus->id);
        }
    }
}

void catalogue::TransportCatalogue::AddStop(const std::string& id,
                                            const geo::Coordinates& coords) {
    Stop stop{id, coords};

    if (!stopname_to_stop_.count(stop.id)) {
        stops_.push_back(std::move(stop));
        const Stop* added_stop = &stops_.back();
        stopname_to_stop_.emplace(added_stop->id, added_stop);
        stop_to_buses_.emplace(added_stop, std::set<std::string_view>{});
    }
}

void catalogue::TransportCatalogue::AddDistances(
    const std::pair<std::pair<std::string_view, std::string_view>, int>&
        distance) {
    auto from_stop = stopname_to_stop_.find(distance.first.first);
    auto to_stop = stopname_to_stop_.find(distance.first.second);
    if (from_stop != stopname_to_stop_.end() &&
        to_stop != stopname_to_stop_.end()) {
        stops_to_distances_[{from_stop->second->id, to_stop->second->id}] =
            distance.second;
    }
}

const catalogue::Bus* catalogue::TransportCatalogue::FindRoute(
    std::string_view route) const {
    auto bus_pos = busname_to_buses_.find(route);
    if (bus_pos != busname_to_buses_.end()) {
        return bus_pos->second;
    }
    return nullptr;
}

const catalogue::Stop* catalogue::TransportCatalogue::FindStop(
    std::string_view stop) const {
    auto stop_pos = stopname_to_stop_.find(stop);
    if (stop_pos != stopname_to_stop_.end()) {
        return stop_pos->second;
    }
    return nullptr;
}

const catalogue::Bus* catalogue::TransportCatalogue::GetRouteInfo(
    std::string_view route) const {
    return FindRoute(route);
}

const std::set<std::string_view>* catalogue::TransportCatalogue::GetStopInfo(
    std::string_view stop) const {
    auto stop_pos = stopname_to_stop_.find(stop);
    if (stop_pos == stopname_to_stop_.end()) {
        return nullptr;
    }

    return &stop_to_buses_.at(stop_pos->second);
}

const catalogue::RouteStat catalogue::TransportCatalogue::GetRouteStat(
    std::string_view id) const {
    RouteStat result;
    result.id = id;

    auto bus_pos = busname_to_buses_.find(id);
    if (bus_pos == busname_to_buses_.end()) {
        result.is_found = false;
        return result;
    }

    const Bus* bus = busname_to_buses_.at(id);
    std::unordered_set<std::string_view> unique_stops;
    for (auto& stop : bus->route) {
        unique_stops.insert(stop->id);
    }
    result.unique_stops = unique_stops;

    double geo_route_length = 0.0;
    for (size_t i = 0; i < bus->route.size() - 1; ++i) {
        geo_route_length += ComputeDistance(bus->route.at(i)->coords,
                                            bus->route.at(i + 1)->coords);
    }

    double route_length = 0.0;
    for (size_t i = 0; i < bus->route.size() - 1; ++i) {
        std::string_view stop1, stop2;
        stop1 = bus->route.at(i)->id;
        stop2 = bus->route.at(i + 1)->id;
        auto pos = stops_to_distances_.find(std::pair(stop1, stop2));
        if (pos != stops_to_distances_.end()) {
            route_length += pos->second;
        } else {
            route_length += stops_to_distances_.at(std::pair(stop2, stop1));
        }
    }

    result.route_length = route_length;
    result.curvature = route_length / geo_route_length;
    result.stops = bus->route.size();

    return result;
}