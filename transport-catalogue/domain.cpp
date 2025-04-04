#include "domain.h"

size_t StopsDistanceHasher::operator()(std::pair<std::string_view, std::string_view> stops) const {
    return hasher_(stops.first.data()) * 31 + hasher_(stops.second.data());
}
