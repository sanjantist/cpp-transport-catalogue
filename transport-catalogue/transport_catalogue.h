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

struct RouteInfo {
    std::string_view id;
};

class TransportCatalogue {
   public:
    void AddRoute(const std::string& id,
                  const std::vector<std::string_view>& stops);
    void AddStop(const std::string& id, const geo::Coordinates& coords);
    const Bus* FindRoute(const std::string_view route) const;
    const Stop* FindStop(const std::string_view stop) const;
    const Bus* GetRouteInfo(std::string_view route) const;
    const std::set<std::string_view>* GetStopInfo(std::string_view stop) const;

   private:
    std::deque<Stop> stops_;
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, const Stop*> stopname_to_stop_;
    std::unordered_map<std::string_view, const Bus*> busname_to_buses_;
    std::unordered_map<const Stop*, std::set<std::string_view>> stop_to_buses_;
};
}  // namespace catalogue