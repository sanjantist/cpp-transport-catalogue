#pragma once

#include <deque>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "domain.h"

namespace catalogue {
class TransportCatalogue {
   public:
    using BusPtr = const Bus*;
    using StopPtr = const Stop*;

    std::optional<BusPtr> AddBus(const std::string& id,
                const std::vector<std::string_view>& stops, bool is_roundtrip);
    std::optional<StopPtr> AddStop(const std::string& id, const geo::Coordinates& coords);
    void AddDistances(std::string_view from_stop, std::string_view to_stop,
                      int distance);

    int GetDistance(std::string_view from_stop, std::string_view to_stop) const;
    std::optional<BusStat> GetBusStat(std::string_view bus_name) const;
    const std::set<std::string_view>* GetBusesByStop(
        std::string_view stop_name) const;

    BusPtr FindBus(std::string_view bus) const;
    StopPtr FindStop(std::string_view stop) const;

   private:
    std::deque<Stop> stops_;
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, StopPtr> stopname_to_stop_;
    std::unordered_map<std::string_view, BusPtr> busname_to_buses_;
    std::unordered_map<StopPtr, std::set<std::string_view>> stop_to_buses_;
    std::unordered_map<std::pair<std::string_view, std::string_view>, int,
                       StopsDistanceHasher>
        stops_to_distances_;
};
}  // namespace catalogue