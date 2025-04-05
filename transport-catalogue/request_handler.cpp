#include "request_handler.h"

#include "json.h"
#include "json_builder.h"
#include "transport_router.h"

RequestHandler::RequestHandler(const catalogue::TransportCatalogue& db,
                               const renderer::MapRenderer& renderer,
                               const router::TransportRouter& router)
    : db_(db), renderer_(renderer), router_(router) {}

std::optional<BusStat> RequestHandler::GetBusStat(
    const std::string_view& bus_name) const {
  return db_.GetBusStat(bus_name);
};

const std::set<std::string_view>* RequestHandler::GetBusesByStop(
    std::string_view stop_name) const {
  return db_.GetBusesByStop(stop_name);
}

svg::Document RequestHandler::RenderMap() const {
  return renderer_.RenderMap();
}

json::Dict RequestHandler::FindRoute(std::string_view from, std::string_view to,
                                     int request_id) {
  json::Builder builder;
  builder.StartDict();
  builder.Key("request_id").Value(request_id);
  std::optional<router::RouteInfo> route_info = router_.GetRouteInfo(from, to);
  if (!route_info) {
    return builder.Key("error_message")
        .Value("not found")
        .EndDict()
        .Build()
        .AsMap();
  }
  builder.Key("total_time").Value((*route_info).total_time);
  builder.Key("items").StartArray();
  for (const auto& item : (*route_info).items) {
    builder.StartDict();
    for (const auto& [key, value] : item) {
      builder.Key(key);
      if (key == "time") {
        builder.Value(std::stod(value));
      } else if (key == "span_count") {
        builder.Value(std::stoi(value));
      } else {
        builder.Value(value);
      }
    }
    builder.EndDict();
  }
  builder.EndArray().EndDict();
  return builder.Build().AsMap();
}