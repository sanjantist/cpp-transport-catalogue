#include "stat_reader.h"

#include <iostream>
#include <sstream>
#include <string_view>

#include "transport_catalogue.h"

void catalogue::stat_reader::ParseAndPrintStat(
    const catalogue::TransportCatalogue& transport_catalogue,
    std::string_view request, std::ostream& output) {
    size_t id_begin = request.find(' ') + 1;
    std::string_view request_type = request.substr(0, id_begin - 1);
    std::string_view id = request.substr(id_begin);
    std::string route_info;

    if (request_type == "Bus") {
        route_info = ParseRouteStat(id, transport_catalogue);
    } else if (request_type == "Stop") {
        route_info = ParseStopStat(id, transport_catalogue.GetStopInfo(id));
    }
    output << route_info;
}

std::string catalogue::stat_reader::ParseRouteStat(
    std::string_view id, const catalogue::TransportCatalogue& catalogue) {
    std::stringstream result;
    catalogue::RouteStat stat = catalogue.GetRouteStat(id);

    result << "Bus " << id << ": ";
    if (!stat.is_found) {
        result << "not found" << std::endl;
        return result.str();
    }

    result << stat.stops << " stops on route, ";
    result << stat.unique_stops.size() << " unique stops, ";
    result << stat.route_length << " route length" << std::endl;

    return result.str();
}

std::string catalogue::stat_reader::ParseStopStat(
    std::string_view stop, const std::set<std::string_view>* buses) {
    std::stringstream result;
    result << "Stop " << stop << ": ";
    if (buses == nullptr) {
        result << "not found" << std::endl;
        return result.str();
    }

    if (buses->empty()) {
        result << "no buses" << std::endl;
        return result.str();
    }

    result << "buses ";
    for (std::string_view bus : *buses) {
        result << bus << " ";
    }

    result << std::endl;

    return result.str();
}