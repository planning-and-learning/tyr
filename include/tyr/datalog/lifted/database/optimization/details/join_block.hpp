/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_OPTIMIZATION_DETAILS_JOIN_BLOCK_HPP_
#define TYR_DATALOG_DATABASE_OPTIMIZATION_DETAILS_JOIN_BLOCK_HPP_

#include "tyr/datalog/lifted/database/syntax/operator_builder.hpp"
#include "tyr/datalog/lifted/database/syntax/query.hpp"
#include "yggdrasil/semantics/canonicalization.hpp"

#include <algorithm>
#include <concepts>
#include <span>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>

namespace tyr::datalog::database::detail
{
/// A natural join of atoms whose output keeps the required variables: atom_1 ⋈ … ⋈ atom_n.
/// Atoms are planned operands; columns with equal labels are join variables.
struct JoinBlock
{
    urd::Store<urd::sequence<QueryView<>>> atoms;
    /// The output variables as a sorted set.
    urd::Store<urd::sequence<urd::index<ygg::Column>>> required;
};

urd::Store<urd::sequence<urd::index<ygg::Column>>> variables(QueryView<> query);

/// R ⋉ S, written R ⋈ π_common(S) with the existing operators, as Yannakakis' semijoin programs use it.
QueryView<> semijoin(OperatorBuilder& build, QueryView<> lhs, QueryView<> rhs);

/// The atoms' variables ordered by label.
urd::Store<urd::sequence<urd::index<ygg::Column>>> variable_order(std::span<const QueryView<>> atoms);
}  // namespace tyr::datalog::database::detail

#endif
