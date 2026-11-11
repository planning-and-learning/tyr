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

/// Vertices of a search as rows of canonical values of one size, interned in a urd pool (rows stored flat, never moved by inserts): a
/// vertex is its handle (VertexSet::handle_type; none, its default, is no vertex), and what is kept per vertex is a
/// urd::sequence addressed by it.
using VertexSet = urd::Store<urd::pool<urd::Store<urd::sequence<ygg::Value>, urd::frozen>>>;

/// An estimate of a vertex's distance to the targets; infinity means no target is reachable.
///
/// Contract, relied on for exactness: the estimate is consistent (h(u) <= 1 + h(v) for every edge (u, v), and 0 on
/// targets) in every graph the inputs can form while the targets stay unchanged, i.e. it depends on the targets and on
/// static inputs only. Learned distances are repaired against it after edge additions, which is only sound if it does
/// not drop with them; Generalized Adaptive A* assumes the same of its user heuristic (Sun, Koenig & Yeoh 2008).
template<typename H>
concept DistanceHeuristic = requires(const H& h, std::span<const ygg::Value> vertex) {
    { h.estimate(vertex) } -> std::same_as<ygg::uint_t>;
};

/// The zero estimate: breadth-first order.
struct BlindDistanceHeuristic
{
    [[nodiscard]] ygg::uint_t estimate(std::span<const ygg::Value>) const noexcept { return 0; }
};

/// A vertex as its values: a row of values as it is, a stored row's values read into `buffer`, which the result
/// borrows.
inline std::span<const ygg::Value> vertex_values(std::span<const ygg::Value> vertex, urd::Store<urd::sequence<ygg::Value>>& /*buffer*/) noexcept
{ return vertex; }
inline std::span<const ygg::Value> vertex_values(const ygg::RowPool::row_type& row, urd::Store<urd::sequence<ygg::Value>>& buffer)
{
    buffer.clear();
    for (size_t i = 0; i < row.size(); ++i)
        buffer.push_back(row.get(i));
    return buffer;
}
/// A row view's values (its stored row's).
template<ygg::RowViewConcept<ygg::Schema> R>
std::span<const ygg::Value> vertex_values(const R& row, urd::Store<urd::sequence<ygg::Value>>& buffer)
{ return vertex_values(row.get_values(), buffer); }

/// A random-access sequence of vertices: size() of them, row(i) the i-th, its values or a stored row (e.g. a relation
/// view).
template<typename R>
concept VertexRange = requires(const R& range, size_t i, urd::Store<urd::sequence<ygg::Value>>& buffer) {
    { range.size() } -> std::convertible_to<size_t>;
    { vertex_values(range.row(i), buffer) } -> std::same_as<std::span<const ygg::Value>>;
};

/// Calls on_vertex(w) for each edge (v, w); stops when it returns false.
template<typename O>
concept SuccessorOracle =
    requires(O& oracle, std::span<const ygg::Value> vertex) { oracle.successors(vertex, [](std::span<const ygg::Value>) { return true; }); };

/// Calls on_vertex(u) for each edge (u, v); stops when it returns false.
template<typename O>
concept PredecessorOracle =
    requires(O& oracle, std::span<const ygg::Value> vertex) { oracle.predecessors(vertex, [](std::span<const ygg::Value>) { return true; }); };

/// Whether the edge (u, v) exists.
template<typename O>
concept EdgeCheck = requires(O& oracle, std::span<const ygg::Value> from, std::span<const ygg::Value> to) {
    { oracle.edge_exists(from, to) } -> std::same_as<bool>;
};

}  // namespace tyr::datalog::database

#endif
