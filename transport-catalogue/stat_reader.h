#pragma once

#include <iosfwd>
#include <string_view>

#include "transport_catalogue.h"

namespace catalogue {
namespace stat_reader {
void ParseAndPrintStat(const catalogue::TransportCatalogue& transport_catalogue,
                       std::string_view request, std::ostream& output);

std::string ParseRouteStat(std::string_view id, const catalogue::TransportCatalogue& catalogue);
std::string ParseStopStat(std::string_view stop,
                          const std::set<std::string_view>* buses);
}  // namespace stat_reader
}  // namespace catalogue