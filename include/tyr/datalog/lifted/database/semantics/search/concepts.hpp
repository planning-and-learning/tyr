/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_SEARCH_CONCEPTS_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_SEARCH_CONCEPTS_HPP_

#include "yggdrasil/core/config.hpp"

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

/// Vertices of a search as rows of uint_t of one size (the rows of the Datalog evaluator), interned in a urd pool (rows
/// stored flat, never moved by inserts): a vertex is its handle (VertexSet::handle_type; none, its default, is no
/// vertex), and what is kept per vertex is a urd::sequence addressed by it.
using VertexSet = urd::Store<urd::pool<urd::Store<urd::sequence<ygg::uint_t>, urd::frozen>>>;

/// An estimate of a vertex's distance to the targets; infinity means no target is reachable.
///
/// Contract, relied on for exactness: the estimate is consistent (h(u) <= 1 + h(v) for every edge (u, v), and 0 on
/// targets) in every graph the inputs can form while the targets stay unchanged, i.e. it depends on the targets and on
/// static inputs only. Learned distances are repaired against it after edge additions, which is only sound if it does
/// not drop with them; Generalized Adaptive A* assumes the same of its user heuristic (Sun, Koenig & Yeoh 2008).
template<typename H>
concept DistanceHeuristic = requires(const H& h, std::span<const ygg::uint_t> vertex) {
    { h.estimate(vertex) } -> std::same_as<ygg::uint_t>;
};

/// A heuristic may also prove, for the current inputs only, that no target is reachable from a vertex: unreachable(v),
/// at a cost of about proof_cost() (e.g. an abstraction of the current graph). A search tries it once it has expanded
/// that many vertices, so that it at most doubles the search's cost.
template<typename H>
concept ProvesUnreachable = requires(H& h, std::span<const ygg::uint_t> vertex) {
    { h.unreachable(vertex) } -> std::same_as<bool>;
    { h.proof_cost() } -> std::convertible_to<size_t>;
};

/// The zero estimate: breadth-first order.
struct BlindDistanceHeuristic
{
    [[nodiscard]] ygg::uint_t estimate(std::span<const ygg::uint_t>) const noexcept { return 0; }
};

/// A random-access sequence of vertices: size() of them, row(i) the i-th.
template<typename R>
concept VertexRange = requires(const R& range, size_t i) {
    { range.size() } -> std::convertible_to<size_t>;
    { range.row(i) } -> std::convertible_to<std::span<const ygg::uint_t>>;
};

/// Calls on_vertex(w) for each edge (v, w); stops when it returns false.
template<typename O>
concept SuccessorOracle =
    requires(O& oracle, std::span<const ygg::uint_t> vertex) { oracle.successors(vertex, [](std::span<const ygg::uint_t>) { return true; }); };

/// Calls on_vertex(u) for each edge (u, v); stops when it returns false.
template<typename O>
concept PredecessorOracle =
    requires(O& oracle, std::span<const ygg::uint_t> vertex) { oracle.predecessors(vertex, [](std::span<const ygg::uint_t>) { return true; }); };

/// Whether the edge (u, v) exists.
template<typename O>
concept EdgeCheck = requires(O& oracle, std::span<const ygg::uint_t> from, std::span<const ygg::uint_t> to) {
    { oracle.edge_exists(from, to) } -> std::same_as<bool>;
};

}  // namespace tyr::datalog::database

#endif
