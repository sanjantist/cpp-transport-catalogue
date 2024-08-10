#include "stat_reader.h"

#include <iostream>
#include <string_view>

void catalogue::stat_reader::ParseAndPrintStat(
    const catalogue::TransportCatalogue& transport_catalogue,
    std::string_view request, std::ostream& output) {
    size_t id_begin = request.find(' ') + 1;
    std::string_view request_type = request.substr(0, id_begin - 1);
    std::string route_info;

    if (request_type == "Bus") {
        route_info = transport_catalogue.GetRouteInfo(request.substr(id_begin));
    } else if (request_type == "Stop") {
        route_info = transport_catalogue.GetStopInfo(request.substr(id_begin));
    }
    output << route_info;
}