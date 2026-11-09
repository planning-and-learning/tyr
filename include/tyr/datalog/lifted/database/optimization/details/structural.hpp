/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_OPTIMIZATION_DETAILS_STRUCTURAL_HPP_
#define TYR_DATALOG_DATABASE_OPTIMIZATION_DETAILS_STRUCTURAL_HPP_

#include "tyr/datalog/lifted/database/optimization/details/join_block.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

#include <algorithm>
#include <optional>
#include <vector>

/// Data-independent planning of a join block from its hypergraph alone.
namespace tyr::datalog::database::detail
{
/// GYO reduction (M. H. Graham, "On the Universal Relation", 1979; C. T. Yu and
/// M. Z. Özsoyoğlu, "An Algorithm for Tree-Query Membership of a Distributed Query",
/// COMPSAC 1979): repeatedly drop variables occurring in a single edge and edges
/// contained in another edge. The hypergraph is acyclic iff one edge remains; each
/// dropped edge's container is its parent in a join tree.
std::optional<urd::Store<urd::sequence<std::optional<size_t>>>> join_tree(urd::Store<urd::sequence<urd::Store<urd::sequence<urd::index<ygg::Column>>>>> edges);

/// Yannakakis (M. Yannakakis, "Algorithms for Acyclic Database Schemes", VLDB 1981):
/// a bottom-up and a top-down semijoin pass remove dangling tuples, then joins along
/// the join tree keep only output variables and variables shared with the parent.
/// The result has the block's output variables in schema order.
QueryView<> yannakakis(const JoinBlock& block, std::span<const std::optional<size_t>> parent, OperatorBuilder& build);

/// Plans a block from structure: Yannakakis if acyclic, otherwise one Generic Join
/// (H. Q. Ngo, C. Ré, A. Rudra, "Skew Strikes Back", SIGMOD Record 2013), whose
/// running time is bounded by the AGM bound of the full join (A. Atserias, M. Grohe,
/// D. Marx, FOCS 2008) for every variable order; variables are ordered by label.
QueryView<> structural_plan(const JoinBlock& block, OperatorBuilder& build);
}  // namespace tyr::datalog::database::detail

#endif
