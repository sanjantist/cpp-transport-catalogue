#include "transport_router.h"

namespace router {
TranstportRouter::TranstportRouter(
    const graph::DirectedWeightedGraph<double>& graph)
    : graph_(graph), router_(graph) {}

std::optional<graph::Router<double>::RouteInfo> TranstportRouter::GetRoute(
    graph::VertexId from, graph::VertexId to) const {
    return router_.BuildRoute(from, to);
}

const graph::Edge<double>& TranstportRouter::GetEdge(graph::EdgeId id) const {
    return graph_.GetEdge(id);
}
}  // namespace router