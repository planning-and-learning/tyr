/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_BOUND_UNION_NORMAL_FORM_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_BOUND_UNION_NORMAL_FORM_HPP_

#include "tyr/datalog/lifted/database/syntax/query.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace tyr::datalog::database::bound
{

/// One input occurrence of a conjunctive branch: the variable bound by each of its columns.
struct ConjunctiveAtom
{
    size_t slot = 0;
    urd::Store<urd::sequence<uint32_t>> vars;
};

struct ConjunctiveBranch;

/// Rows whose `outer` variables match some branch are rejected (a difference's right side).
struct ConjunctiveNegation
{
    urd::Store<urd::sequence<uint32_t>> outer;
    std::vector<ConjunctiveBranch> branches;  // std::vector: ConjunctiveBranch is incomplete here (it holds negations); a Store needs its element type complete
};

/// A conjunctive branch: one conjunctive query (a rule body) of the union, with its negated subqueries.
struct ConjunctiveBranch
{
    urd::Store<urd::sequence<ConjunctiveAtom>> atoms;
    urd::Store<urd::sequence<std::pair<uint32_t, uint32_t>>> equal;
    urd::Store<urd::sequence<std::pair<uint32_t, ygg::Value>>> value;
    urd::Store<urd::sequence<ConjunctiveNegation>> negations;
};

struct QueryVariable
{
    ygg::TypeId type;  ///< set where it is made
};

/// A query as a union of conjunctive queries with nested negation (UCQ¬; Abiteboul, Hull & Vianu, "Foundations of
/// Databases", 1995, ch. 5), its branches over shared variables. Differences and antijoins stay nested as negated
/// branches. Every variable of a branch is bound by one of its atoms, so a negation
/// only filters rows once its outer variables are bound (relational algebra is safe-range).
struct UnionNormalForm
{
    urd::Store<urd::sequence<QueryVariable>> variables;  ///< The variables, one value each in a frame.
    urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::ColumnLayout>>>> input_columns;  ///< The schema of each input slot.
    urd::Store<urd::sequence<uint32_t>> output;                          ///< The variable of each of the query's columns.
    std::vector<ConjunctiveBranch> branches;  // std::vector: as ConjunctiveNegation::branches, so that lists of branches are one type
};

namespace detail
{

/// Flattens a query node into branches; `fresh` allocates the variables of projected-away columns.
class UnionNormalFormBuilder
{
    UnionNormalForm m_result;  ///< Variables and input schemas so far.

    static std::vector<ConjunctiveBranch> product(const std::vector<ConjunctiveBranch>& lhs, const std::vector<ConjunctiveBranch>& rhs);  // std::vector: as ConjunctiveNegation::branches

    /// The child's variables: its columns found in `parent` take the parent's variable, others a fresh one.
    urd::Store<urd::sequence<uint32_t>> by_label(std::span<const ygg::ColumnLayout> child, std::span<const ygg::ColumnLayout> parent, std::span<const uint32_t> mapping)
    {
        urd::Store<urd::sequence<uint32_t>> result;
        for (const auto& column : child)
        {
            const auto found = std::ranges::find(parent, column.label, &ygg::ColumnLayout::label);
            result.push_back(found != parent.end() ? mapping[static_cast<size_t>(found - parent.begin())] : fresh(column));
        }
        return result;
    }

public:
    /// The union normal form of a query.
    UnionNormalForm build(QueryView<> query) &&
    {
        for (const auto& column : query.columns())
            m_result.output.push_back(fresh(column));
        m_result.branches = flatten(query, m_result.output);
        return std::move(m_result);
    }

    uint32_t fresh(const ygg::ColumnLayout& column)
    {
        m_result.variables.push_back(QueryVariable { column.type });
        return static_cast<uint32_t>(m_result.variables.size() - 1);
    }

    /// `mapping` holds the variable of each of the query's columns, by position.
    std::vector<ConjunctiveBranch> flatten(QueryView<> query, std::span<const uint32_t> mapping);  // std::vector: as ConjunctiveNegation::branches
};

}  // namespace detail

UnionNormalForm union_normal_form(QueryView<> query);

}  // namespace tyr::datalog::database::bound

#endif
