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
#include <iterator>
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

    /// A row's vertex, the row as it is held (a row view, a builder's row child resolved).
    template<typename R>
    VertexSet::handle_type insert_vertex(const R& row)
    { return insert_vertex(vertex_values(row, scratch)); }

    /// The stored vertex of a row, or none.
    template<typename R>
    [[nodiscard]] VertexSet::handle_type find_vertex(const R& row)
    { return vertices.find(vertex_values(row, scratch)); }

    /// Inserts both endpoint tuples of an edge's values; returns their handles.
    std::pair<VertexSet::handle_type, VertexSet::handle_type> insert_endpoints(std::span<const ygg::Value> edge);

    /// Inserts both endpoint tuples of an edge row, the row as it is held.
    template<typename R>
    std::pair<VertexSet::handle_type, VertexSet::handle_type> insert_endpoints(const R& edge)
    { return insert_endpoints(vertex_values(edge, scratch)); }

    [[nodiscard]] bool contains_edge(VertexSet::handle_type from, VertexSet::handle_type to) const;
    void clear();
    [[nodiscard]] size_t memory_usage() const;

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
    template<ygg::RelationViewConcept<ygg::Schema> S, ygg::RelationViewConcept<ygg::Schema> E, ygg::RelationViewConcept<ygg::Schema> T>
    void append_distances(const S& sources, const E& edges, const T& targets, const DistancePlan& plan, ygg::FreeScratchRelation& out);
    [[nodiscard]] size_t memory_usage() const;

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
template<ygg::RelationViewConcept<ygg::Schema> S, ygg::RelationViewConcept<ygg::Schema> E, ygg::RelationViewConcept<ygg::Schema> T>
void distance(const S& sources, const E& edges, const T& targets, const DistancePlan& plan, ygg::FreeScratchRelation& out, DistanceWorkspace& workspace);

/// The smallest distance between any source and target, from distance() into result.
/// uint_t's maximum if no target is reachable.
template<ygg::RelationViewConcept<ygg::Schema> S, ygg::RelationViewConcept<ygg::Schema> E, ygg::RelationViewConcept<ygg::Schema> T>
ygg::uint_t
min_distance(const S& sources, const E& edges, const T& targets, const DistancePlan& plan, ygg::FreeScratchRelation& result, DistanceWorkspace& workspace);

}  // namespace tyr::datalog::database

namespace tyr::datalog::database
{

namespace detail
{
inline constexpr ygg::uint_t distance_infinity = std::numeric_limits<ygg::uint_t>::max();

inline DistanceGraph::DistanceGraph(size_t tuple_size_) : tuple_size(tuple_size_) {}

inline size_t DistanceGraph::memory_usage() const { return urd::memory_usage(*this); }

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

template<ygg::RelationViewConcept<ygg::Schema> S, ygg::RelationViewConcept<ygg::Schema> E, ygg::RelationViewConcept<ygg::Schema> T>
void DistanceWorkspace::append_distances(const S& sources, const E& edges, const T& targets, const DistancePlan& /*plan*/, ygg::FreeScratchRelation& out)
{
    m_graph.clear();
    m_targets.clear();
    if (sources.get_rows().empty() || targets.get_rows().empty())
        return;
    m_graph.outgoing.reserve(edges.get_rows().size());
    for (size_t i = 0; i < edges.get_rows().size(); ++i)
    {
        const auto [from, to] = m_graph.insert_endpoints(edges.row(i));
        m_graph.outgoing.insert({ from, to });
    }
    for (size_t i = 0; i < sources.get_rows().size(); ++i)
        m_graph.insert_vertex(sources.row(i));
    m_targets.reserve(targets.get_rows().size());
    for (size_t i = 0; i < targets.get_rows().size(); ++i)
        m_targets.push_back(m_graph.insert_vertex(targets.row(i)));

    for (size_t i = 0; i < sources.get_rows().size(); ++i)
    {
        const auto from = m_graph.find_vertex(sources.row(i));
        URD_ASSUME(!from.is_none());  // every source was inserted above
        detail::breadth_first_search(m_graph, from, m_distances, m_queue);
        for (const auto to : m_targets)
            if (m_distances[to] != detail::distance_infinity)
                detail::insert(out, detail::make_distance_row(m_graph, from, to, m_distances[to], m_row));
    }
}

template<ygg::RelationViewConcept<ygg::Schema> S, ygg::RelationViewConcept<ygg::Schema> E, ygg::RelationViewConcept<ygg::Schema> T>
void distance(const S& sources, const E& edges, const T& targets, const DistancePlan& plan, ygg::FreeScratchRelation& out, DistanceWorkspace& workspace)
{
    detail::prepare_output(out, sources, edges, targets);
    assert(std::ranges::equal(sources.columns(), plan.source_columns()));
    assert(std::ranges::equal(edges.columns(), plan.edge_columns()));
    assert(std::ranges::equal(targets.columns(), plan.target_columns()));
    assert(workspace.matches(plan) && std::ranges::equal(out.view().columns(), plan.output_columns()));
    workspace.append_distances(sources, edges, targets, plan, out);
}

template<ygg::RelationViewConcept<ygg::Schema> S, ygg::RelationViewConcept<ygg::Schema> E, ygg::RelationViewConcept<ygg::Schema> T>
ygg::uint_t
min_distance(const S& sources, const E& edges, const T& targets, const DistancePlan& plan, ygg::FreeScratchRelation& result, DistanceWorkspace& workspace)
{
    distance(sources, edges, targets, plan, result, workspace);
    const auto view = result.view();
    const auto position = static_cast<std::ptrdiff_t>(view.arity() - 1);  // The distance column follows the edge columns.
    auto best = std::numeric_limits<ygg::uint_t>::max();
    for (const auto& held : view.get_rows())
        ygg::with_values(held,
                         [&](const auto& values)
                         { best = std::min(best, ygg::decode_value<ygg::uint_t>(*std::ranges::next(std::ranges::begin(values), position))); });
    return best;
}

}  // namespace tyr::datalog::database

#endif
