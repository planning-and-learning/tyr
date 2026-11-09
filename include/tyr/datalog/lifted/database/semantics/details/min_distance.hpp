/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_DETAILS_MIN_DISTANCE_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_DETAILS_MIN_DISTANCE_HPP_

#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/pools/pools.hpp>
#include <urd/store.hpp>
#include "tyr/datalog/lifted/database/semantics/distance.hpp"
#include "tyr/datalog/lifted/database/semantics/relation_view.hpp"
#include "tyr/datalog/lifted/database/semantics/search/concepts.hpp"
#include "tyr/datalog/lifted/database/semantics/search/search_tree.hpp"
#include "tyr/datalog/lifted/database/syntax/query.hpp"

#include <algorithm>
#include <cstddef>
#include <ranges>
#include <span>
#include <vector>

namespace tyr::datalog::database::detail
{

urd::Store<urd::sequence<size_t>> column_range(size_t first, size_t last);

QueryView<> checked_edges(QueryView<> edges, const DistancePlan& plan);

/// An evaluator of a query of one input, slot 0, with the plan's edge columns. It compiles the query when constructed,
/// so the repository may end afterwards.
template<typename Evaluator>
Evaluator relation_edges(const ygg::FreeRelationRepository& layer, const DistancePlan& plan, bool static_edges)
{
    QueryRepository repository;
    auto builder = QueryBuilder {};
    auto input = checkout<Query<QueryInputTag>>(builder);
    input->input_slot = 0;
    const auto columns = plan.edge_columns();
    input->columns.set(columns.begin(), columns.end());
    return Evaluator(layer, insert_query(repository, builder, *input), { static_edges });
}

/// An edge query bound for search: `Evaluator` (a bound::QueryEvaluator or incremental::bound::QueryEvaluator) over
/// edge columns k·u·u' (key columns k, possibly none, then two vertices of `arity` columns), with the bindings of the
/// key and a source vertex (successors), the key and a target vertex (predecessors), and all of them (edge checks).
/// The key is an argument of each call; with_key fixes it for a search, as a model of the search oracle concepts.
template<typename Evaluator>
class EdgeQuery
{
public:
    EdgeQuery(Evaluator edges, std::span<const ygg::ColumnLayout> /*edge_columns*/, size_t keys, size_t arity) :
        m_key_size(keys),
        m_target_offset(keys + arity),
        m_vertex_size(arity),
        m_edges(std::move(edges)),
        m_forward(m_edges.prepare(column_range(0, keys + arity))),
        m_backward(m_edges.prepare(key_and_target_positions(keys, arity))),
        m_check(m_edges.prepare(column_range(0, keys + 2 * arity))),
        m_bound(m_key_size + m_vertex_size),
        m_edge(m_key_size + 2 * m_vertex_size)
    {
    }

    Evaluator& evaluator() noexcept { return m_edges; }
    [[nodiscard]] const Evaluator& evaluator() const noexcept { return m_edges; }
    [[nodiscard]] size_t vertex_size() const noexcept { return m_vertex_size; }
    /// Where an edge row's target vertex starts (a column position).
    [[nodiscard]] size_t target_offset() const noexcept { return m_target_offset; }

    template<RowPredicate OnVertex>
    void successors(std::span<const ygg::Value> key, std::span<const ygg::Value> vertex, const OnVertex& on_vertex)
    {
        m_edges.evaluate(m_forward,
                         bound(key, vertex),
                         [&](std::span<const ygg::Value> edge) { return on_vertex(edge.subspan(m_target_offset, m_vertex_size)); });
    }
    template<RowPredicate OnVertex>
    void predecessors(std::span<const ygg::Value> key, std::span<const ygg::Value> vertex, const OnVertex& on_vertex)
    {
        m_edges.evaluate(m_backward, bound(key, vertex), [&](std::span<const ygg::Value> edge) { return on_vertex(edge.subspan(m_key_size, m_vertex_size)); });
    }
    bool edge_exists(std::span<const ygg::Value> key, std::span<const ygg::Value> from, std::span<const ygg::Value> to)
    {
        std::ranges::copy(key, m_edge.begin());
        std::ranges::copy(from, m_edge.begin() + static_cast<std::ptrdiff_t>(m_key_size));
        std::ranges::copy(to, m_edge.begin() + static_cast<std::ptrdiff_t>(m_key_size + m_vertex_size));
        auto exists = false;
        m_edges.evaluate(m_check,
                         m_edge,
                         [&](std::span<const ygg::Value>)
                         {
                             exists = true;
                             return false;
                         });
        return exists;
    }

    /// The edges of one key as a search oracle (SuccessorOracle, PredecessorOracle, EdgeCheck).
    class Keyed
    {
    public:
        Keyed(EdgeQuery& edges, std::span<const ygg::Value> key) : m_edges(&edges), m_key(key) {}

        template<RowPredicate OnVertex>
        void successors(std::span<const ygg::Value> vertex, const OnVertex& on_vertex)
        { m_edges->successors(m_key, vertex, on_vertex); }
        template<RowPredicate OnVertex>
        void predecessors(std::span<const ygg::Value> vertex, const OnVertex& on_vertex)
        { m_edges->predecessors(m_key, vertex, on_vertex); }
        bool edge_exists(std::span<const ygg::Value> from, std::span<const ygg::Value> to) { return m_edges->edge_exists(m_key, from, to); }

    private:
        EdgeQuery* m_edges;
        std::span<const ygg::Value> m_key;
    };

    Keyed with_key(std::span<const ygg::Value> key) { return Keyed(*this, key); }

private:
    /// The key columns and the target vertex of an edge, the bound columns of a predecessor query.
    static urd::Store<urd::sequence<size_t>> key_and_target_positions(size_t keys, size_t arity)
    {
        auto positions = column_range(0, keys);
        for (const auto position : column_range(keys + arity, keys + 2 * arity))
            positions.push_back(position);
        return positions;
    }

    /// The key and a vertex as one row of bound columns; without a key, the vertex itself.
    std::span<const ygg::Value> bound(std::span<const ygg::Value> key, std::span<const ygg::Value> vertex)
    {
        if (key.empty())
            return vertex;
        std::ranges::copy(key, m_bound.begin());
        std::ranges::copy(vertex, m_bound.begin() + static_cast<std::ptrdiff_t>(m_key_size));
        return m_bound;
    }

    size_t m_key_size;
    size_t m_target_offset;
    size_t m_vertex_size;
    Evaluator m_edges;
    size_t m_forward;
    size_t m_backward;
    size_t m_check;
    urd::Store<urd::sequence<ygg::Value>> m_bound;  ///< The key and a vertex, kept to avoid allocations.
    urd::Store<urd::sequence<ygg::Value>> m_edge;   ///< The key and the vertices of the checked edge, kept to avoid allocations.
};

/// The targets of a search, as a vertex set.
class TargetSet
{
public:
    TargetSet() : m_vertices() {}

    template<VertexRange Targets>
    [[nodiscard]] bool same(const Targets& targets) const
    {
        return targets.size() == m_vertices.size()
               && std::ranges::all_of(std::views::iota(static_cast<size_t>(0), targets.size()),
                                      [&](size_t i) { return m_vertices.contains(vertex_values(targets.row(i), m_buffer)); });
    }
    template<VertexRange Targets>
    void assign(const Targets& targets)
    {
        m_vertices.clear();
        m_handles.clear();
        for (size_t i = 0; i < targets.size(); ++i)
            if (const auto [handle, added] = m_vertices.insert(vertex_values(targets.row(i), m_buffer)); added)
                m_handles.push_back(handle);
    }
    [[nodiscard]] const VertexSet& vertices() const noexcept { return m_vertices; }

    /// The targets' vertices in the tree, stored with their estimates; valid until the next call.
    template<DistanceHeuristic H>
    std::span<const VertexSet::handle_type> ensure_in(SearchTree& tree, const H& heuristic)
    {
        m_ids.clear();
        for (const auto handle : m_handles)
            m_ids.push_back(tree.ensure(m_vertices.view(handle), heuristic));
        return m_ids;
    }

private:
    VertexSet m_vertices;
    mutable urd::Store<urd::sequence<ygg::Value>> m_buffer;  ///< A target's values, read from its row.
    urd::Store<urd::sequence<VertexSet::handle_type>> m_handles;  ///< Of m_vertices, as inserted.
    urd::Store<urd::sequence<VertexSet::handle_type>> m_ids;
};

}  // namespace tyr::datalog::database::detail

#endif
