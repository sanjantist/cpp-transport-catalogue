#pragma once

#include <cmath>
#include <cstddef>
#include <deque>
#include <functional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "geo.h"

namespace catalogue {
struct Stop {
    std::string id;
    geo::Coordinates coords;
};

struct Bus {
    std::string id;
    std::vector<const Stop*> route;
};

struct RouteStat {
    std::string_view id;
    std::unordered_set<std::string_view> unique_stops;
    double route_length;
    double curvature;
    size_t stops;
    bool is_found = true;
};

struct StopsDistanceHasher {
   public:
    size_t operator()(
        std::pair<std::string_view, std::string_view> stops) const {
        return hasher_(stops.first.data()) * 31 + hasher_(stops.second.data());
    }

   private:
    std::hash<const void*> hasher_;
};

class TransportCatalogue {
   public:
    void AddRoute(const std::string& id,
                  const std::vector<std::string_view>& stops);
    void AddStop(const std::string& id, const geo::Coordinates& coords);
    void AddDistances(std::string_view from_stop, std::string_view to_stop,
                      int distance);
    const Bus* FindRoute(std::string_view route) const;
    const Stop* FindStop(std::string_view stop) const;
    const Bus* GetRouteInfo(std::string_view route) const;
    const std::set<std::string_view>* GetStopInfo(std::string_view stop) const;
    const RouteStat GetRouteStat(std::string_view id) const;

   private:
    std::deque<Stop> stops_;
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, const Stop*> stopname_to_stop_;
    std::unordered_map<std::string_view, const Bus*> busname_to_buses_;
    std::unordered_map<const Stop*, std::set<std::string_view>> stop_to_buses_;
    std::unordered_map<std::pair<std::string_view, std::string_view>, int,
                       StopsDistanceHasher>
        stops_to_distances_;
};
}  // namespace catalogue