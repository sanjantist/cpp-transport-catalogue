#pragma once
#include <map>
#include <ostream>
#include <set>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "domain.h"
#include "geo.h"
#include "svg.h"

namespace renderer {

struct RenderSettings {
    double width;
    double height;

    double padding;

    double line_width;
    double stop_radius;

    int bus_label_font_size;
    svg::Point bus_label_offset;

    int stop_label_font_size;
    svg::Point stop_label_offset;

    svg::Color underlayer_color;
    double underlayer_width;

    std::vector<svg::Color> color_palette;
};

class MapRenderer {
   public:
    MapRenderer() = default;

    void AddBusToMap(std::string_view id, const Bus *bus);
    void AddStopToMap(std::string_view id, const Stop *stop);
    void SetSettings(const RenderSettings &settings);
    svg::Document RenderMap() const;

   private:
    RenderSettings settings_;
    std::map<std::string_view, const Stop *> stops_;
    std::map<std::string_view, const Bus *> buses_;

    void RenderBusRoutes(svg::Document &doc,
                         const std::unordered_map<std::string_view, svg::Point>
                             &stop_to_projected_point) const;
    void RenderBusLabels(svg::Document &doc,
                         const std::unordered_map<std::string_view, svg::Point>
                             &stop_to_projected_point) const;
    void RenderStops(svg::Document &doc,
                     const std::unordered_map<std::string_view, svg::Point>
                         &stop_to_projected_point) const;
    void RenderStopLabels(svg::Document &doc,
                          const std::unordered_map<std::string_view, svg::Point>
                              &stop_to_projected_point) const;
};
}  // namespace renderer
