#include "json_reader.h"

#include <cassert>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "domain.h"
#include "json.h"
#include "request_handler.h"

using namespace std::literals;

JsonReader::JsonReader(std::istream& input,
                       catalogue::TransportCatalogue& catalogue)
    : document_(json::Load(input)), catalogue_(&catalogue) {
    requests_ = DivideRequests();
}

RequestsInfo JsonReader::DivideRequests() {
    RequestsInfo result;

    json::Node root = document_.GetRoot();

    for (const auto& [request_type, node] : root.AsMap()) {
        if (request_type == "base_requests"s) {
            for (const auto& base_request : node.AsArray()) {
                result.base_requests.push_back(base_request);
            }
        } else if (request_type == "stat_requests"s) {
            for (const auto& stat_request : node.AsArray()) {
                result.stat_requests.push_back(stat_request);
            }
        } else if (request_type == "render_settings"s) {
            result.render_settings = node.AsMap();
        }
    }

    return result;
}

void JsonReader::ParseRequests(std::ostream& out) {
    ParseBaseRequests();
    ParseRenderSettings();

    std::stringstream output;
    output << "[";
    ParseStatRequests(output);
    output << "]";

    json::Document stat_result = json::Load(output);
    json::Print(stat_result, out);
}

void JsonReader::ParseBaseRequests() {
    // add all stops for correct bus handling
    for (const auto& request : requests_.base_requests) {
        auto request_as_map = request.AsMap();
        std::string type = request_as_map.at("type"s).AsString();
        if (type == "Stop"s) {
            std::string id = request_as_map.at("name"s).AsString();
            double latitude = request_as_map.at("latitude"s).AsDouble();
            double longitude = request_as_map.at("longitude"s).AsDouble();
            catalogue_->AddStop(id, {latitude, longitude});
        }
    }

    // add road distances
    for (const auto& request : requests_.base_requests) {
        auto request_as_map = request.AsMap();
        std::string type = request_as_map.at("type"s).AsString();
        if (type == "Stop") {
            std::string id = request_as_map.at("name"s).AsString();
            auto distances = request_as_map.at("road_distances"s).AsMap();
            for (const auto& [stop, distance] : distances) {
                catalogue_->AddDistances(id, stop, distance.AsInt());
            }
        }
    }

    // add buses
    for (const auto& request : requests_.base_requests) {
        auto request_as_map = request.AsMap();
        std::string type = request_as_map.at("type"s).AsString();
        if (type == "Bus"s) {
            std::string id = request_as_map.at("name"s).AsString();
            bool is_roundtrip = request_as_map.at("is_roundtrip"s).AsBool();
            auto stops = request_as_map.at("stops"s).AsArray();
            std::vector<std::string_view> stops_sv;
            for (const auto& stop : stops) {
                stops_sv.push_back(stop.AsString());
            }
            if (!is_roundtrip) {
                for (auto it = stops.rbegin() + 1; it != stops.rend(); ++it) {
                    stops_sv.push_back(it->AsString());
                }
            }
            auto bus = catalogue_->AddBus(id, stops_sv, is_roundtrip);
            if (bus) {
                renderer_.AddBusToMap(bus.value()->id, *bus);
                for (const auto& stop : bus.value()->route) {
                    renderer_.AddStopToMap(stop->id, stop);
                }
            }
        }
    }
}

void JsonReader::ParseStatRequests(std::ostream& output) {
    bool is_first_request = true;
    RequestHandler handler{*catalogue_, renderer_};

    for (const auto& request : requests_.stat_requests) {
        auto request_as_map = request.AsMap();
        int id = request_as_map.at("id"s).AsInt();
        std::string type = request_as_map.at("type"s).AsString();

        if (is_first_request) {
            is_first_request = false;
        } else {
            output << ", ";
        }

        if (type == "Bus"s) {
            std::string name = request_as_map.at("name"s).AsString();
            auto stat = handler.GetBusStat(name);

            output << "{ " << "\"request_id\": " << id;
            if (!stat) {
                output << ", \"error_message\": \"not found\"}";
            } else {
                output << ", \"curvature\": " << stat->curvature
                       << ", \"route_length\": " << stat->route_length
                       << ", \"stop_count\": " << stat->stops
                       << ", \"unique_stop_count\": "
                       << stat->unique_stops.size() << "}";
            }
        } else if (type == "Stop"s) {
            std::string name = request_as_map.at("name"s).AsString();
            auto stat = handler.GetBusesByStop(name);
            output << "{ \"request_id\": " << id;
            if (stat == nullptr) {
                output << ", \"error_message\": \"not found\"}";
            } else {
                output << ", \"buses\": [";

                bool is_first = true;
                for (auto bus : *stat) {
                    if (is_first) {
                        output << "\"" << bus << "\"";
                        is_first = false;
                    } else {
                        output << ",\"" << bus << "\"";
                    }
                }

                output << "]}";
            }
        } else if (type == "Map"s) {
            output << "{ \"request_id\": " << id << ", \"map\": ";
            std::ostringstream map_output;
            handler.RenderMap().Render(map_output);
            json::Node val(map_output.str());
            json::PrintNode(val, {output});
            output << "}";
        }
    }
}

void JsonReader::ParseRenderSettings() {
    renderer::RenderSettings settings;
    for (const auto& [setting, value] : requests_.render_settings) {
        if (setting == "width"s) {
            settings.width = value.AsDouble();
        }
        if (setting == "height"s) {
            settings.height = value.AsDouble();
        }
        if (setting == "padding"s) {
            settings.padding = value.AsDouble();
        }
        if (setting == "line_width"s) {
            settings.line_width = value.AsDouble();
        }
        if (setting == "stop_radius"s) {
            settings.stop_radius = value.AsDouble();
        }
        if (setting == "bus_label_font_size"s) {
            settings.bus_label_font_size = value.AsInt();
        }
        if (setting == "bus_label_offset"s) {
            auto value_arr = value.AsArray();
            settings.bus_label_offset = {value_arr[0].AsDouble(),
                                         value_arr[1].AsDouble()};
        }
        if (setting == "stop_label_font_size"s) {
            settings.stop_label_font_size = value.AsInt();
        }
        if (setting == "stop_label_offset"s) {
            auto value_arr = value.AsArray();
            settings.stop_label_offset = {value_arr[0].AsDouble(),
                                          value_arr[1].AsDouble()};
        }
        if (setting == "underlayer_color"s) {
            if (value.IsString()) {
                settings.underlayer_color = value.AsString();
            } else if (value.IsArray()) {
                if (value.AsArray().size() == 3) {
                    settings.underlayer_color = ArrayToRgb(value);
                } else {
                    settings.underlayer_color = ArrayToRgba(value);
                }
            }
        }
        if (setting == "underlayer_width"s) {
            settings.underlayer_width = value.AsDouble();
        }
        if (setting == "color_palette"s) {
            for (const auto& color : value.AsArray()) {
                if (color.IsString()) {
                    settings.color_palette.push_back(color.AsString());
                } else if (color.IsArray()) {
                    if (color.AsArray().size() == 3) {
                        settings.color_palette.push_back(ArrayToRgb(color));
                    } else {
                        settings.color_palette.push_back(ArrayToRgba(color));
                    }
                }
            }
        }
    }

    renderer_.SetSettings(settings);
}

svg::Rgb JsonReader::ArrayToRgb(const json::Node& node) {
    auto node_arr = node.AsArray();
    svg::Rgb result;

    result.red = node_arr[0].AsInt();
    result.green = node_arr[1].AsInt();
    result.blue = node_arr[2].AsInt();

    return result;
}
svg::Rgba JsonReader::ArrayToRgba(const json::Node& node) {
    auto node_arr = node.AsArray();
    svg::Rgba result;

    auto rgb = ArrayToRgb(node);
    result.red = rgb.red;
    result.green = rgb.green;
    result.blue = rgb.blue;
    result.opacity = node_arr[3].AsDouble();

    return result;
}