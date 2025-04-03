#include "json_reader.h"

#include <cassert>
#include <cstddef>
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
    : document_(json::Load(input)),
      requests_(DivideRequests()),
      catalogue_(&catalogue),
      graph_(GetVertexCount()) {}

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
    } else if (request_type == "routing_settings"s) {
      result.routing_settings = node.AsMap();
    }
  }

  return result;
}

void JsonReader::ParseRequests(std::ostream& out) {
  ParseRoutingSettings();
  ParseBaseRequests();
  ParseRenderSettings();
  json::Document stat_result = ParseStatRequests();
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
      auto stop = catalogue_->AddStop(id, {latitude, longitude});
      AddStopToGraph((*stop)->id);
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

      if (bus) {
        renderer_.AddBusToMap(bus.value()->id, *bus);
        auto route_parsing_end =
            (is_roundtrip ? stops_sv.end() - 1
                          : stops_sv.begin() + stops_sv.size() / 2 + 1);
        for (auto it_l = stops_sv.begin(); it_l != route_parsing_end; ++it_l) {
          std::string_view stop_from_name = catalogue_->FindStop(*it_l)->id;
          size_t current_span_count = 0;
          double current_travel_time = 0.0;
          for (auto it_r = it_l + 1; it_r != stops_sv.end(); ++it_r) {
            std::string_view stop_to_name = catalogue_->FindStop(*it_r)->id;
            if (stop_from_name != stop_to_name &&
                !IsEdgeAdded(stop_from_name, stop_to_name)) {
              graph::VertexId stop_from_id, stop_to_id;
              stop_from_id = stop_name_to_in_vertex_id_.at(stop_from_name) + 1;
              stop_to_id = stop_name_to_in_vertex_id_.at(stop_to_name);

              ++current_span_count;
              std::string_view prev_stop =
                  catalogue_->FindStop(*(it_r - 1))->id;
              current_travel_time += GetTravelTime(
                  catalogue_->GetDistance(prev_stop, stop_to_name));
              graph::EdgeId edge = graph_.AddEdge(
                  {stop_from_id, stop_to_id, current_travel_time});
              edge_to_bus_name_[edge] = (*bus)->id;
              edge_to_span_count_[edge] = current_span_count;
            }
          }
          renderer_.AddStopToMap(stop_from_name,
                                 catalogue_->FindStop(stop_from_name));
        }
        auto last_stop = catalogue_->FindStop(stops_sv.back());
        renderer_.AddStopToMap(last_stop->id, last_stop);
      }
    }
  }
}

json::Document JsonReader::ParseStatRequests() {
  GraphData graph_data{stop_name_to_in_vertex_id_, vertex_id_to_stop_name_,
                       edge_to_bus_name_, edge_to_span_count_, waiting_edges_};
  RequestHandler handler{*catalogue_, renderer_, graph_, graph_data};

  json::Builder builder;
  builder.StartArray();

  for (const auto& request : requests_.stat_requests) {
    auto request_as_map = request.AsMap();
    int id = request_as_map.at("id"s).AsInt();
    std::string type = request_as_map.at("type"s).AsString();

    if (type == "Bus"s) {
      std::string name = request_as_map.at("name"s).AsString();
      auto stat = handler.GetBusStat(name);

      builder.StartDict().Key("request_id").Value(id);

      if (!stat.has_value()) {
        builder.Key("error_message").Value("not found");
      } else {
        builder.Key("curvature")
            .Value(stat->curvature)
            .Key("route_length")
            .Value(stat->route_length)
            .Key("stop_count")
            .Value(int(stat->stops))
            .Key("unique_stop_count")
            .Value(int(stat->unique_stops.size()));
      }
      builder.EndDict();
    } else if (type == "Stop"s) {
      std::string name = request_as_map.at("name"s).AsString();
      auto stat = handler.GetBusesByStop(name);

      builder.StartDict().Key("request_id").Value(id);

      if (stat == nullptr) {
        builder.Key("error_message").Value("not found");
      } else {
        builder.Key("buses");
        builder.StartArray();
        for (auto bus : *stat) {
          std::string str_bus(bus);
          builder.Value(str_bus);
        }
        builder.EndArray();
      }
      builder.EndDict();
    } else if (type == "Map"s) {
      builder.StartDict().Key("request_id").Value(id);

      std::ostringstream map_output;
      handler.RenderMap().Render(map_output);
      json::Node val(map_output.str());

      builder.Key("map").Value(val.AsString());
      builder.EndDict();
    } else if (type == "Route"s) {
      builder.StartDict().Key("request_id").Value(id);
      std::string from_stop_raw_name = request_as_map.at("from"s).AsString();
      std::string to_stop_raw_name = request_as_map.at("to"s).AsString();

      const Stop* from_stop = catalogue_->FindStop(from_stop_raw_name);
      const Stop* to_stop = catalogue_->FindStop(to_stop_raw_name);

      json::Dict routing_result = handler.FindRoute(from_stop->id, to_stop->id);
      for (const auto& [key, value] : routing_result) {
        builder.Key(key).Value(value.GetValue());
      }
      builder.EndDict();
    }
  }

  builder.EndArray();

  return json::Document{builder.Build()};
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

void JsonReader::ParseRoutingSettings() {
  bus_velocity_ = requests_.routing_settings.at("bus_velocity"s).AsDouble();
  bus_wait_time_ = requests_.routing_settings.at("bus_wait_time"s).AsInt();
}

bool JsonReader::IsEdgeAdded(const std::string_view from,
                             const std::string_view to) {
  return added_adges_.count({from, to});
}

size_t JsonReader::GetVertexCount() const {
  std::unordered_set<std::string> stops_sv;
  for (const auto& request : requests_.base_requests) {
    auto request_as_map = request.AsMap();
    std::string type = request_as_map.at("type"s).AsString();
    if (type == "Stop"s) {
      std::string stop_name = request_as_map.at("name"s).AsString();
      stops_sv.insert(stop_name);
    }
  }

  return stops_sv.size() * 2;
}

void JsonReader::AddStopToGraph(std::string_view stop_name) {
  if (!stop_name_to_in_vertex_id_.count(stop_name)) {
    vertex_id_to_stop_name_[current_vertex_id_] = stop_name;
    stop_name_to_in_vertex_id_[stop_name] = current_vertex_id_;
    ++current_vertex_id_;
    vertex_id_to_stop_name_[current_vertex_id_] = stop_name;
    ++current_vertex_id_;
    graph::EdgeId edge =
        graph_.AddEdge({current_vertex_id_ - 2, current_vertex_id_ - 1,
                        (double)bus_wait_time_});
    waiting_edges_.insert(edge);
  }
}

double JsonReader::GetTravelTime(double distance) const {
  return (distance / 1000) / (bus_velocity_ / 60);
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