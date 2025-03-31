#include "request_handler.h"

#include "graph.h"
#include "json_builder.h"
#include "transport_router.h"

RequestHandler::RequestHandler(
    const catalogue::TransportCatalogue& db,
    const renderer::MapRenderer& renderer,
    const graph::DirectedWeightedGraph<double>& graph, GraphData graph_data)
    : db_(db), renderer_(renderer), graph_(graph), graph_data_(graph_data) {}

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

json::Dict RequestHandler::FindRoute(std::string_view from,
                                     std::string_view to) {
    json::Builder builder;
    router::TranstportRouter router(graph_);
    auto route =
        router.GetRoute(graph_data_.stop_name_to_in_vertex_id.at(std::string(from)),
                        graph_data_.stop_name_to_in_vertex_id.at(std::string(to)));
    builder.StartDict();
    if (!route) {
        return builder.Key("error_message")
            .Value("not found")
            .EndDict()
            .Build()
            .AsMap();
    }
    builder.Key("total_time").Value(route->weight);
    builder.Key("items").StartArray();
    for (graph::EdgeId edge_id : route->edges) {
        builder.StartDict();
        std::string type =
            (graph_data_.waiting_edges_.count(edge_id) ? "Wait" : "Bus");
        builder.Key("type").Value(type);
        if (type == "Wait") {
            graph::VertexId stop_id = router.GetEdge(edge_id).from;
            std::string stop_name(
                graph_data_.vertex_id_to_stop_name_.at(stop_id));
            builder.Key("stop_name").Value(stop_name);
        } else if (type == "Bus") {
            builder.Key("bus").Value(
                std::string(graph_data_.edge_to_bus_name_.at(edge_id)));
            builder.Key("span_count")
                .Value((int)graph_data_.edge_to_span_count_.at(edge_id));
        }
        builder.Key("time").Value(router.GetEdge(edge_id).weight);
        builder.EndDict();
    }
    builder.EndArray().EndDict();
    return builder.Build().AsMap();
}