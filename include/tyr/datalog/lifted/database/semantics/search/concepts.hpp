/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_SEARCH_CONCEPTS_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_SEARCH_CONCEPTS_HPP_

#include "yggdrasil/core/config.hpp"
#include "yggdrasil/formalism/relation_builder.hpp"

#include <concepts>
#include <cstddef>
#include <limits>
#include <span>
#include <urd/repr/pools/pools.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::datalog::database
{

/// The distance of a vertex from which no target is reachable.
inline constexpr ygg::uint_t infinity = std::numeric_limits<ygg::uint_t>::max();
/// No level (BackwardFrontier).
inline constexpr ygg::uint_t none = std::numeric_limits<ygg::uint_t>::max();

/// Vertices of a search as rows of values V of one size, interned in a urd pool (rows stored flat, never moved by
/// inserts): a vertex is its handle (handle_type; none, its default, is no vertex), and what is kept per vertex is a
/// urd::sequence addressed by it. V is ygg::Value (canonical values: the distances of relations) or ygg::uint_t (the
/// rows of the Datalog evaluator); the search classes are templates on it.
template<typename V>
using BasicVertexSet = urd::Store<urd::pool<urd::Store<urd::sequence<V>, urd::frozen>>>;
using VertexSet = BasicVertexSet<ygg::Value>;

/// An estimate of a vertex's distance to the targets; infinity means no target is reachable.
///
/// Contract, relied on for exactness: the estimate is consistent (h(u) <= 1 + h(v) for every edge (u, v), and 0 on
/// targets) in every graph the inputs can form while the targets stay unchanged, i.e. it depends on the targets and on
/// static inputs only. Learned distances are repaired against it after edge additions, which is only sound if it does
/// not drop with them; Generalized Adaptive A* assumes the same of its user heuristic (Sun, Koenig & Yeoh 2008).
template<typename H, typename V = ygg::Value>
concept DistanceHeuristic = requires(const H& h, std::span<const V> vertex) {
    { h.estimate(vertex) } -> std::same_as<ygg::uint_t>;
};

/// A heuristic may also prove, for the current inputs only, that no target is reachable from a vertex: unreachable(v),
/// at a cost of about proof_cost() (e.g. an abstraction of the current graph). A search tries it once it has expanded
/// that many vertices, so that it at most doubles the search's cost.
template<typename H, typename V = ygg::Value>
concept ProvesUnreachable = requires(H& h, std::span<const V> vertex) {
    { h.unreachable(vertex) } -> std::same_as<bool>;
    { h.proof_cost() } -> std::convertible_to<size_t>;
};

/// The zero estimate: breadth-first order.
struct BlindDistanceHeuristic
{
    template<typename V>
    [[nodiscard]] ygg::uint_t estimate(std::span<const V>) const noexcept
    { return 0; }
};

/// A vertex as its values: a row of values as it is, a stored row's values read into `buffer`, which the result
/// borrows.
inline std::span<const ygg::Value> vertex_values(std::span<const ygg::Value> vertex, urd::Store<urd::sequence<ygg::Value>>& /*buffer*/) noexcept
{ return vertex; }

inline std::span<const ygg::uint_t> vertex_values(std::span<const ygg::uint_t> vertex, urd::Store<urd::sequence<ygg::uint_t>>& /*buffer*/) noexcept
{ return vertex; }

inline std::span<const ygg::Value> vertex_values(const urd::packed_row<false>& row, urd::Store<urd::sequence<ygg::Value>>& buffer)
{
    buffer.clear();
    for (size_t i = 0; i < row.size(); ++i)
        buffer.push_back(row.get(i));
    return buffer;
}

/// A row's values, the row as it is held: a row view, or a builder's row child resolved (a variant of row views).
template<typename R>
    requires urd::View<R> || urd::variant_value<R>
std::span<const ygg::Value> vertex_values(const R& row, urd::Store<urd::sequence<ygg::Value>>& buffer)
{
    return ygg::with_values(row, [&buffer](const auto& values) { return vertex_values(values, buffer); });
}

/// The number of vertices of a range: its rows' (a relation view) or its size.
template<typename R>
size_t vertex_count(const R& range)
{
    if constexpr (requires { range.get_rows().size(); })
        return range.get_rows().size();
    else
        return range.size();
}

/// A random-access sequence of vertices: vertex_count() of them, row(i) the i-th, its values or a stored row (e.g. a
/// relation view).
template<typename R, typename V = ygg::Value>
concept VertexRange = requires(const R& range, size_t i, urd::Store<urd::sequence<V>>& buffer) {
    { vertex_count(range) } -> std::convertible_to<size_t>;
    { vertex_values(range.row(i), buffer) } -> std::same_as<std::span<const V>>;
};

/// Calls on_vertex(w) for each edge (v, w); stops when it returns false.
template<typename O, typename V = ygg::Value>
concept SuccessorOracle = requires(O& oracle, std::span<const V> vertex) { oracle.successors(vertex, [](std::span<const V>) { return true; }); };

/// Calls on_vertex(u) for each edge (u, v); stops when it returns false.
template<typename O, typename V = ygg::Value>
concept PredecessorOracle = requires(O& oracle, std::span<const V> vertex) { oracle.predecessors(vertex, [](std::span<const V>) { return true; }); };

/// Whether the edge (u, v) exists.
template<typename O, typename V = ygg::Value>
concept EdgeCheck = requires(O& oracle, std::span<const V> from, std::span<const V> to) {
    { oracle.edge_exists(from, to) } -> std::same_as<bool>;
};

}  // namespace tyr::datalog::database

#endif
