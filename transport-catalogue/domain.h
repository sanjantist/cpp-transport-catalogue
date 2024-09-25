#pragma once
#include <string>
#include <unordered_set>
#include <vector>

#include "geo.h"

struct Stop {
    std::string id;
    geo::Coordinates coords;
};

struct Bus {
    std::string id;
    std::vector<const Stop*> route;
    bool is_roundtrip;
};

struct BusStat {
    std::string_view id;
    std::unordered_set<std::string_view> unique_stops;
    double route_length;
    double curvature;
    size_t stops;
};

struct StopsDistanceHasher {
   public:
    size_t operator()(
        std::pair<std::string_view, std::string_view> stops) const;

   private:
    std::hash<const void*> hasher_;
};