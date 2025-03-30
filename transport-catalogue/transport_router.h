#pragma once
#include <string_view>
#include <unordered_map>

#include "graph.h"
#include "router.h"

namespace router {
class TranstportRouter {
   public:
    TranstportRouter(const graph::DirectedWeightedGraph<double>& graph);
    std::optional<graph::Router<double>::RouteInfo> GetRoute(
        graph::VertexId from, graph::VertexId to) const;
    const graph::Edge<double>& GetEdge(graph::EdgeId id) const;

   private:
    const graph::DirectedWeightedGraph<double>& graph_;
    graph::Router<double> router_;
};
}  // namespace router