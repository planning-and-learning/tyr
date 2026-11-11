/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_DISTANCE_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_DISTANCE_HPP_

#include "tyr/datalog/lifted/database/semantics/distance_plan.hpp"
#include "tyr/datalog/lifted/database/semantics/operations.hpp"
#include "tyr/datalog/lifted/database/semantics/search/concepts.hpp"
#include "yggdrasil/core/config.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <urd/config.hpp>
#include <urd/repr/pools/pools.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace tyr::datalog::database
{

namespace detail
{
/// Append-only vertex handles remain valid until clear(); adjacency stores handles only.
struct DistanceGraph
{
    size_t tuple_size;  ///< The values of a vertex.
    /// The vertices' tuples, interned in a urd pool.
    VertexSet vertices;
    urd::Store<urd::map<VertexSet::handle_type, VertexSet::handle_type>, urd::multi, urd::retains> outgoing;
    urd::Store<urd::sequence<ygg::Value>> scratch;  ///< A row's values, as read for a lookup or insert.

    explicit DistanceGraph(size_t tuple_size);
    VertexSet::handle_type insert_vertex(std::span<const ygg::Value> tuple);
    VertexSet::handle_type insert_vertex(const ygg::RowPool::row_type& row) { return insert_vertex(vertex_values(row, scratch)); }
    /// The stored vertex of a row, or none.
    [[nodiscard]] VertexSet::handle_type find_vertex(const ygg::RowPool::row_type& row) { return vertices.find(vertex_values(row, scratch)); }
    /// Inserts both endpoint tuples of an edge row; returns their handles.
    std::pair<VertexSet::handle_type, VertexSet::handle_type> insert_endpoints(const ygg::RowPool::row_type& edge);
    [[nodiscard]] bool contains_edge(VertexSet::handle_type from, VertexSet::handle_type to) const;
    void clear();
    [[nodiscard]] size_t memory_usage() const noexcept;
    /// The members it owns memory through (urd::memory_of).
    static constexpr auto memory_members() noexcept { return urd::Members<&DistanceGraph::vertices, &DistanceGraph::outgoing, &DistanceGraph::scratch> {}; }
};
}  // namespace detail

/// Reusable full-evaluation storage for one vertex size. Each call replaces
/// its graph; sequential calls retain capacity. Do not share across concurrent calls.
class DistanceWorkspace
{
    detail::DistanceGraph m_graph;
    urd::Store<urd::sequence<ygg::uint_t, VertexSet::handle_type>> m_distances;
    urd::Store<urd::sequence<VertexSet::handle_type>> m_queue;
    urd::Store<urd::sequence<VertexSet::handle_type>> m_targets;
    urd::Store<urd::sequence<ygg::Value>> m_row;

public:
    explicit DistanceWorkspace(const DistancePlan& plan);

    /// Whether the workspace serves the plan's vertex size.
    [[nodiscard]] bool matches(const DistancePlan& plan) const noexcept;
    /// Appends the distances of the inputs, which match the plan, to out.
    template<RelationViewConcept S, RelationViewConcept E, RelationViewConcept T>
    void append_distances(const S& sources, const E& edges, const T& targets, const DistancePlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out);
    [[nodiscard]] size_t memory_usage() const noexcept;
    /// The members it owns memory through (urd::memory_of).
    static constexpr auto memory_members() noexcept
    {
        return urd::Members<&DistanceWorkspace::m_graph,
                            &DistanceWorkspace::m_distances,
                            &DistanceWorkspace::m_queue,
                            &DistanceWorkspace::m_targets,
                            &DistanceWorkspace::m_row> {};
    }
};

/// Exact directed unit-edge distances for every reachable source/target pair.
/// Includes zero-length paths; unreachable pairs are absent. An output aliasing an input throws before clearing it.
/// Evaluation failures may leave partial output. Inputs must not borrow the workspace's buffers.
template<RelationViewConcept S, RelationViewConcept E, RelationViewConcept T>
void distance(const S& sources,
              const E& edges,
              const T& targets,
              const DistancePlan& plan,
              urd::builder<ygg::Relation<ygg::Schema>>& out,
              DistanceWorkspace& workspace);

/// The smallest distance between any source and target, from distance() into result.
/// uint_t's maximum if no target is reachable.
template<RelationViewConcept S, RelationViewConcept E, RelationViewConcept T>
ygg::uint_t min_distance(const S& sources,
                         const E& edges,
                         const T& targets,
                         const DistancePlan& plan,
                         urd::builder<ygg::Relation<ygg::Schema>>& result,
                         DistanceWorkspace& workspace);

}  // namespace tyr::datalog::database

namespace tyr::datalog::database
{

namespace detail
{
inline constexpr ygg::uint_t distance_infinity = std::numeric_limits<ygg::uint_t>::max();

inline DistanceGraph::DistanceGraph(size_t tuple_size_) : tuple_size(tuple_size_) {}

inline size_t DistanceGraph::memory_usage() const noexcept { return urd::memory_usage(*this); }

void breadth_first_search(const DistanceGraph& graph,
                          VertexSet::handle_type root,
                          urd::Store<urd::sequence<ygg::uint_t, VertexSet::handle_type>>& distances,
                          urd::Store<urd::sequence<VertexSet::handle_type>>& queue);

/// The output row (from, to, length) as values in `row`, which the result borrows.
std::span<const ygg::Value> make_distance_row(const DistanceGraph& graph,
                                              VertexSet::handle_type from,
                                              VertexSet::handle_type to,
                                              ygg::uint_t length,
                                              urd::Store<urd::sequence<ygg::Value>>& row);
}  // namespace detail

template<RelationViewConcept S, RelationViewConcept E, RelationViewConcept T>
void DistanceWorkspace::append_distances(const S& sources,
                                         const E& edges,
                                         const T& targets,
                                         const DistancePlan& plan,
                                         urd::builder<ygg::Relation<ygg::Schema>>& out)
{
    m_graph.clear();
    m_targets.clear();
    if (sources.empty() || targets.empty())
        return;
    m_graph.outgoing.reserve(edges.size());
    for (size_t i = 0; i < edges.size(); ++i)
    {
        const auto [from, to] = m_graph.insert_endpoints(edges.row(i).get_values());
        m_graph.outgoing.insert({ from, to });
    }
    for (size_t i = 0; i < sources.size(); ++i)
        m_graph.insert_vertex(sources.row(i).get_values());
    m_targets.reserve(targets.size());
    for (size_t i = 0; i < targets.size(); ++i)
        m_targets.push_back(m_graph.insert_vertex(targets.row(i).get_values()));

    for (size_t i = 0; i < sources.size(); ++i)
    {
        const auto from = m_graph.find_vertex(sources.row(i).get_values());
        URD_ASSUME(!from.is_none());  // every source was inserted above
        detail::breadth_first_search(m_graph, from, m_distances, m_queue);
        for (const auto to : m_targets)
            if (m_distances[to] != detail::distance_infinity)
                out.insert(detail::make_distance_row(m_graph, from, to, m_distances[to], m_row));
    }
}

template<RelationViewConcept S, RelationViewConcept E, RelationViewConcept T>
void distance(const S& sources,
              const E& edges,
              const T& targets,
              const DistancePlan& plan,
              urd::builder<ygg::Relation<ygg::Schema>>& out,
              DistanceWorkspace& workspace)
{
    assert(std::ranges::equal(sources.columns(), plan.source_columns()));
    assert(std::ranges::equal(edges.columns(), plan.edge_columns()));
    assert(std::ranges::equal(targets.columns(), plan.target_columns()));
    assert(workspace.matches(plan) && std::ranges::equal(out.columns(), plan.output_columns()));
    detail::prepare_output(out, sources, edges, targets);
    workspace.append_distances(sources, edges, targets, plan, out);
}

template<RelationViewConcept S, RelationViewConcept E, RelationViewConcept T>
ygg::uint_t min_distance(const S& sources,
                         const E& edges,
                         const T& targets,
                         const DistancePlan& plan,
                         urd::builder<ygg::Relation<ygg::Schema>>& result,
                         DistanceWorkspace& workspace)
{
    distance(sources, edges, targets, plan, result, workspace);
    const auto position = result.columns().size() - 1;  // The distance column follows the edge columns.
    auto best = std::numeric_limits<ygg::uint_t>::max();
    for (size_t i = 0; i < result.size(); ++i)
        best = std::min(best, ygg::decode_value<ygg::uint_t>(result.storage().at(ygg::RowPool::handle_type(ygg::to_uint_t(i))).get(position)));
    return best;
}

}  // namespace tyr::datalog::database

#endif
