#define _USE_MATH_DEFINES
#include "geo.h"

#include <cmath>
#include <cstdlib>

namespace geo {

bool Coordinates::operator==(const Coordinates &other) const {
    return lat == other.lat && lng == other.lng;
}
bool Coordinates::operator!=(const Coordinates &other) const {
    return !(*this == other);
}

bool IsZero(double value) { return std::abs(value) < EPSILON; }

double ComputeDistance(geo::Coordinates from, geo::Coordinates to) {
    using namespace std;
    if (from == to) {
        return 0;
    }
    static const double dr = 3.1415926535 / 180.;
    return acos(sin(from.lat * dr) * sin(to.lat * dr) +
                cos(from.lat * dr) * cos(to.lat * dr) *
                    cos(abs(from.lng - to.lng) * dr)) *
           EARTH_RADIUS;
}

}  // namespace geo