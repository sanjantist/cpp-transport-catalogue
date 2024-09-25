#include "map_renderer.h"

namespace renderer {

void MapRenderer::AddBusToMap(std::string_view id, const Bus* bus) {
    buses_.emplace(id, bus);
}

void MapRenderer::AddStopToMap(std::string_view id, const Stop* stop) {
    if (!stops_.count(id)) {
        stops_.emplace(id, stop);
    }
}

void MapRenderer::SetSettings(const RenderSettings& settings) {
    settings_ = settings;
}

svg::Document MapRenderer::RenderMap() const {
    svg::Document result;
    std::vector<geo::Coordinates> geo_coords;

    for (const auto& [_, bus] : buses_) {
        for (const auto& stop : bus->route) {
            geo_coords.push_back(stop->coords);
        }
    }

    geo::SphereProjector proj{geo_coords.begin(), geo_coords.end(),
                              settings_.width, settings_.height,
                              settings_.padding};

    std::unordered_map<std::string_view, svg::Point> stop_to_projected_point;
    for (const auto& [id, stop] : stops_) {
        stop_to_projected_point[id] = proj(stop->coords);
    }

    // add routes
    int obj_counter = 0;
    for (const auto& [_, bus] : buses_) {
        svg::Polyline route;

        for (const auto& stop : bus->route) {
            route.AddPoint(stop_to_projected_point[stop->id]);
        }

        route.SetFillColor("none")
            .SetStrokeColor(
                settings_.color_palette[obj_counter %
                                        settings_.color_palette.size()])
            .SetStrokeWidth(settings_.line_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        result.Add(route);
        ++obj_counter;
    }

    // add bus labels
    obj_counter = 0;
    for (const auto& [_, bus] : buses_) {
        svg::Text bus_label;
        svg::Text bus_underlayer;

        auto proj_coords = proj(bus->route[0]->coords);

        bus_label.SetPosition(proj_coords)
            .SetOffset(settings_.bus_label_offset)
            .SetFontSize(settings_.bus_label_font_size)
            .SetFontFamily("Verdana")
            .SetFontWeight("bold")
            .SetData(bus->id)
            .SetFillColor(
                settings_.color_palette[obj_counter %
                                        settings_.color_palette.size()]);

        bus_underlayer.SetPosition(proj_coords)
            .SetOffset(settings_.bus_label_offset)
            .SetFontSize(settings_.bus_label_font_size)
            .SetFontFamily("Verdana")
            .SetFontWeight("bold")
            .SetData(bus->id)
            .SetFillColor(settings_.underlayer_color)
            .SetStrokeColor(settings_.underlayer_color)
            .SetStrokeWidth(settings_.underlayer_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        result.Add(bus_underlayer);
        result.Add(bus_label);

        if (!bus->is_roundtrip &&
            bus->route[0] != bus->route[bus->route.size() / 2]) {
            svg::Text bus_end_label;
            svg::Text bus_end_underlayer;

            auto proj_end_coords =
                stop_to_projected_point[bus->route[bus->route.size() / 2]->id];

            bus_end_label.SetPosition(proj_end_coords)
                .SetOffset(settings_.bus_label_offset)
                .SetFontSize(settings_.bus_label_font_size)
                .SetFontFamily("Verdana")
                .SetFontWeight("bold")
                .SetData(bus->id)
                .SetFillColor(
                    settings_.color_palette[obj_counter %
                                            settings_.color_palette.size()]);

            bus_end_underlayer.SetPosition(proj_end_coords)
                .SetOffset(settings_.bus_label_offset)
                .SetFontSize(settings_.bus_label_font_size)
                .SetFontFamily("Verdana")
                .SetFontWeight("bold")
                .SetData(bus->id)
                .SetFillColor(settings_.underlayer_color)
                .SetStrokeColor(settings_.underlayer_color)
                .SetStrokeWidth(settings_.underlayer_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            result.Add(bus_end_underlayer);
            result.Add(bus_end_label);
        }

        ++obj_counter;
    }

    // add stops
    for (const auto& [id, stop] : stops_) {
        svg::Circle stop_obj;

        stop_obj.SetCenter(stop_to_projected_point[id])
            .SetRadius(settings_.stop_radius)
            .SetFillColor("white");

        result.Add(stop_obj);
    }

    // add stop labels
    for (const auto& [id, stop] : stops_) {
        svg::Text stop_label;
        svg::Text stop_underlayer;

        auto proj_label_coords = stop_to_projected_point[id];

        stop_label.SetPosition(proj_label_coords)
            .SetOffset(settings_.stop_label_offset)
            .SetFontSize(settings_.stop_label_font_size)
            .SetFontFamily("Verdana")
            .SetData(stop->id)
            .SetFillColor("black");

        stop_underlayer.SetPosition(proj_label_coords)
            .SetOffset(settings_.stop_label_offset)
            .SetFontSize(settings_.stop_label_font_size)
            .SetFontFamily("Verdana")
            .SetData(stop->id)
            .SetFillColor(settings_.underlayer_color)
            .SetStrokeColor(settings_.underlayer_color)
            .SetStrokeWidth(settings_.underlayer_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        result.Add(stop_underlayer);
        result.Add(stop_label);
    }

    return result;
}

}  // namespace renderer
