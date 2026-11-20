/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_INCREMENTAL_JOIN_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_INCREMENTAL_JOIN_HPP_

#include "tyr/datalog/lifted/database/semantics/incremental/delta.hpp"
#include "tyr/datalog/lifted/database/semantics/operations.hpp"

#include <algorithm>
#include <cassert>
#include <urd/memory.hpp>
#include <urd/repr/maps/postings.hpp>
#include <urd/store.hpp>
#include <utility>

namespace tyr::datalog::database::incremental
{
namespace detail
{
/// The rows of a scratch relation by the hash of their join keys: each entry a row's slot in its records, which stays
/// while the relation holds the row (its position does not: its rows are sorted).
using RowsByKey = urd::Store<urd::map<ygg::hash_t, ygg::FreeScratchRelation::Slot>, urd::multi, urd::retains>;

/// Inserts rows absent from `rows`, indexing their records by the hash of `keys`.
template<ygg::RelationViewConcept<ygg::Schema> V>
void insert_rows(const V& input, ygg::FreeScratchRelation& rows, std::span<const ColumnSlice> keys, RowsByKey& index)
{
    if (!keys.empty())
        index.reserve(rows.view().get_rows().size() + input.get_rows().size());
    for (const auto& held : input.get_rows())
        ygg::with_values(held,
                         [&](const auto& values)
                         {
                             if (!rows.insert(values).second)
                                 throw std::invalid_argument("Incremental evaluation: added row is already present in the input.");
                             if (!keys.empty())
                                 index.insert({ database::detail::join_key_hash(values, keys), rows.slot(values) });
                         });
}

/// Erases rows present in `rows`, and their entries of the index of insert_rows().
template<ygg::RelationViewConcept<ygg::Schema> V>
void erase_rows(const V& input, ygg::FreeScratchRelation& rows, std::span<const ColumnSlice> keys, RowsByKey& index)
{
    for (const auto& held : input.get_rows())
        ygg::with_values(held,
                         [&](const auto& values)
                         {
                             const auto slot = rows.slot(values);
                             if (!rows.erase(values))
                                 throw std::invalid_argument("Incremental evaluation: removed row is absent from the input.");
                             if (keys.empty())
                                 return;
                             const auto matches = index.equal_range(database::detail::join_key_hash(values, keys));
                             const auto found = std::find_if(matches.first, matches.second, [&](const auto& entry) { return entry.second == slot; });
                             assert(found != matches.second);
                             index.erase(found);
                         });
}

}  // namespace detail

/// First-order join delta rules as in Gupta et al. (SIGMOD 1993), Section 4:
/// https://doi.org/10.1145/170035.170066. See also Budiu et al., "DBSP"
/// (PVLDB 2023), Theorem 3.4: https://doi.org/10.14778/3587136.3587137.
/// Maintains natural join when either or both inputs change. Owns the current
/// input sets and their mutable indexes; supplied views are borrowed only during
/// calls. Inputs must not alias this evaluator's result/delta or the sequential
/// caller workspace. Results and deltas are borrowed until the next mutation.
/// Rejected schema/overlap checks preserve the previous evaluation. After a failure
/// during mutation, results and deltas are unavailable until initialize().
/// No repository publication is required.
class JoinEvaluator
{
    JoinPlan m_plan;
    urd::handle<ygg::FreeScratchRelation> m_left_operand;
    urd::handle<ygg::FreeScratchRelation> m_right_operand;
    detail::RowsByKey m_left_index;
    detail::RowsByKey m_right_index;
    urd::handle<ygg::FreeScratchRelation> m_result;
    Delta m_delta;
    bool m_initialized = false;

    template<ygg::RelationViewConcept<ygg::Schema> L, ygg::RelationViewConcept<ygg::Schema> R>
    void join(const L& left_operand, const R& right_operand, bool build_left, ygg::FreeScratchRelation& output, Workspace& workspace) const;

    /// Applies changes that are known to be valid.
    template<RelationChange L, RelationChange R>
    void apply(const L& left_operand, const R& right_operand, Workspace& workspace);

    friend struct detail::NodeRules<QueryJoinTag>;

public:
    /// Its relations are drawn from `scratch`, which must outlive it.
    JoinEvaluator(ygg::FreeScratchRelations& scratch, JoinPlan plan);

    /// Replace the baseline, retaining buffers, and clear the last output delta.
    template<ygg::RelationViewConcept<ygg::Schema> L, ygg::RelationViewConcept<ygg::Schema> R>
    void initialize(const L& left_operand, const R& right_operand, Workspace& workspace);

    /// Each change describes actual set changes in that input's original column
    /// order. Pass empty changes for an unchanged side; swap added/removed on
    /// both sides to undo. Reports the exact net change of the complete batch.
    template<RelationChange L, RelationChange R>
    void update(const L& left_operand, const R& right_operand, Workspace& workspace);

    [[nodiscard]] ygg::FreeRelationBuilderView get_result() const&;
    [[nodiscard]] ygg::FreeRelationBuilderView get_result() const&& = delete;
    [[nodiscard]] const Delta& get_delta() const&;
    [[nodiscard]] const Delta& get_delta() const&& = delete;

    /// Retained inputs, result, delta and indexes; excludes plan and caller workspace.
    [[nodiscard]] size_t memory_usage() const;
};

template<ygg::RelationViewConcept<ygg::Schema> L, ygg::RelationViewConcept<ygg::Schema> R>
void JoinEvaluator::join(const L& left_operand, const R& right_operand, bool build_left, ygg::FreeScratchRelation& output, Workspace& workspace) const
{
    const auto& build = build_left ? *m_left_operand : *m_right_operand;
    const auto row_at = [&build](const auto& /*operand*/, ygg::FreeScratchRelation::Slot slot) -> const auto& { return build.at(slot).values; };
    database::detail::join_rows(left_operand, right_operand, m_plan, build_left, build_left ? m_left_index : m_right_index, row_at, output, workspace);
}

template<ygg::RelationViewConcept<ygg::Schema> L, ygg::RelationViewConcept<ygg::Schema> R>
void JoinEvaluator::initialize(const L& left_operand, const R& right_operand, Workspace& workspace)
{
    assert(std::ranges::equal(left_operand.columns(), m_plan.left_columns()));
    assert(std::ranges::equal(right_operand.columns(), m_plan.right_columns()));
    detail::require_unaliased(left_operand, *m_result, m_delta);
    detail::require_unaliased(right_operand, *m_result, m_delta);
    m_initialized = false;
    m_left_operand->clear();
    m_right_operand->clear();
    m_left_index.clear();
    m_right_index.clear();
    m_result->clear();
    m_delta.clear();
    detail::insert_rows(left_operand, *m_left_operand, m_plan.left_keys(), m_left_index);
    detail::insert_rows(right_operand, *m_right_operand, m_plan.right_keys(), m_right_index);
    // Both indexes are ready, so probe the smaller input.
    const auto left = m_left_operand->view();
    const auto right = m_right_operand->view();
    join(left, right, left.get_rows().size() >= right.get_rows().size(), *m_result, workspace);
    m_initialized = true;
}

template<RelationChange L, RelationChange R>
void JoinEvaluator::update(const L& left_operand, const R& right_operand, Workspace& workspace)
{
    detail::require_initialized(m_initialized);
    detail::require_change(left_operand, m_plan.left_columns(), *m_result, m_delta);
    detail::require_change(right_operand, m_plan.right_columns(), *m_result, m_delta);
    apply(left_operand, right_operand, workspace);
}

template<RelationChange L, RelationChange R>
void JoinEvaluator::apply(const L& left_operand, const R& right_operand, Workspace& workspace)
{
    const auto& [left_added, left_removed] = left_operand;
    const auto& [right_added, right_removed] = right_operand;
    m_initialized = false;
    m_delta.clear();

    // Remove from both sides before inserting. Each old/new pair is visited
    // once, and no temporary match between an added and removed row is emitted.
    const auto current_left = m_left_operand->view();
    const auto current_right = m_right_operand->view();
    join(left_removed, current_right, false, *m_delta.removed, workspace);
    detail::erase_rows(left_removed, *m_left_operand, m_plan.left_keys(), m_left_index);
    join(current_left, right_removed, true, *m_delta.removed, workspace);
    detail::erase_rows(right_removed, *m_right_operand, m_plan.right_keys(), m_right_index);
    join(left_added, current_right, false, *m_delta.added, workspace);
    detail::insert_rows(left_added, *m_left_operand, m_plan.left_keys(), m_left_index);
    join(current_left, right_added, true, *m_delta.added, workspace);
    detail::insert_rows(right_added, *m_right_operand, m_plan.right_keys(), m_right_index);

    detail::apply_delta(m_delta, *m_result);
    m_initialized = true;
}

}  // namespace tyr::datalog::database::incremental

#endif
