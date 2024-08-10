#include "transport_catalogue.h"

#include <cstddef>
#include <ostream>
#include <set>
#include <sstream>
#include <string_view>
#include <vector>

#include "geo.h"

void catalogue::TransportCatalogue::AddRoute(
    const std::string& id, const std::vector<std::string_view>& stops) {
    if (!busname_to_buses_.count(id)) {
        Bus bus;
        bus.id = std::move(id);
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
            stop_to_buses_.at(stop).insert(added_bus);
        }
    }
}

void catalogue::TransportCatalogue::AddStop(const std::string& id,
                                            const geo::Coordinates& coords) {
    Stop stop{std::move(id), std::move(coords)};

    if (!stopname_to_stop_.count(stop.id)) {
        stops_.push_back(std::move(stop));
        const Stop* added_stop = &stops_.back();
        stopname_to_stop_.emplace(added_stop->id, added_stop);
        stop_to_buses_.emplace(added_stop,
                               std::set<const Bus*, BusPtrCompare>{});
    }
}

const catalogue::Bus* catalogue::TransportCatalogue::FindRoute(
    const std::string_view& route) {
    if (busname_to_buses_.count(route)) {
        return busname_to_buses_.at(route);
    }
    return nullptr;
}

const catalogue::Stop* catalogue::TransportCatalogue::FindStop(
    const std::string_view& stop) {
    if (stopname_to_stop_.count(stop)) {
        return stopname_to_stop_.at(stop);
    }
    return nullptr;
}

std::string catalogue::TransportCatalogue::GetRouteInfo(
    std::string_view route) const {
    std::stringstream result;
    result << "Bus " << route << ": ";
    if (!busname_to_buses_.count(route)) {
        result << "not found" << std::endl;
        return result.str();
    }
    const Bus* bus = busname_to_buses_.at(route);
    result << bus->route.size() << " stops on route, ";

    std::unordered_set<std::string_view> unique_stops;
    for (auto& stop : bus->route) {
        unique_stops.insert(stop->id);
    }

    result << unique_stops.size() << " unique stops, ";

    double route_length = 0.0;
    for (size_t i = 0; i < bus->route.size() - 1; ++i) {
        route_length += ComputeDistance(bus->route.at(i)->coords,
                                        bus->route.at(i + 1)->coords);
    }

    result << route_length << " route length" << std::endl;

    return result.str();
}

std::string catalogue::TransportCatalogue::GetStopInfo(
    std::string_view stop) const {
    std::stringstream result;
    result << "Stop " << stop << ": ";
    if (!stopname_to_stop_.count(stop)) {
        result << "not found" << std::endl;
        return result.str();
    }

    const Stop* desired_stop = stopname_to_stop_.at(stop);
    if (stop_to_buses_.at(desired_stop).empty()) {
        result << "no buses" << std::endl;
        return result.str();
    }

    result << "buses ";
    for (const Bus* bus : stop_to_buses_.at(desired_stop)) {
        result << bus->id << " ";
    }

    result << std::endl;

    return result.str();
}