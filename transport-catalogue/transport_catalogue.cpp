#include "transport_catalogue.h"

#include <set>
#include <string_view>
#include <vector>

#include "geo.h"

void catalogue::TransportCatalogue::AddRoute(
    const std::string& id, const std::vector<std::string_view>& stops) {
    if (!busname_to_buses_.count(id)) {
        Bus bus;
        bus.id = id;
        bus.route.reserve(stops.size());
        for (auto& stop : stops) {
            if (!stopname_to_stop_.count(stop)) {
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

const catalogue::Bus* catalogue::TransportCatalogue::FindRoute(
    const std::string_view route) const {
    auto bus_pos = busname_to_buses_.find(route);
    if (bus_pos != busname_to_buses_.end()) {
        return bus_pos->second;
    }
    return nullptr;
}

const catalogue::Stop* catalogue::TransportCatalogue::FindStop(
    const std::string_view stop) const {
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