/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_INCREMENTAL_SEMIJOIN_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_INCREMENTAL_SEMIJOIN_HPP_

#include "tyr/datalog/lifted/database/semantics/incremental/delta.hpp"
#include "tyr/datalog/lifted/database/semantics/incremental/join.hpp"
#include "tyr/datalog/lifted/database/semantics/operations.hpp"

#include <cassert>
#include <ranges>
#include <span>
#include <urd/memory.hpp>
#include <urd/repr/maps/postings.hpp>
#include <urd/store.hpp>
#include <utility>

namespace tyr::datalog::database::incremental
{

/// Maintains the semijoin L ⋉ R, or the antijoin L ▷ R, when either or both inputs change. A row of L is in the
/// output iff it has a match in R on the shared columns (semijoin) or none (antijoin), so the rows whose membership
/// can change are the changed rows of L and the rows of L sharing a key with a changed row of R (the delta rules of
/// Gupta & Mumick, "Maintenance of Materialized Views", IEEE Data Eng. Bull. 1995, with an existence test in place
/// of the join). Owns the current inputs and their key indexes; supplied views are borrowed only during calls.
/// After a failure during mutation, results and deltas are unavailable until initialize().
class SemijoinEvaluator
{
    JoinPlan m_plan;
    bool m_anti;
    urd::handle<ygg::FreeScratchRelation> m_left_operand;
    urd::handle<ygg::FreeScratchRelation> m_right_operand;
    detail::RowsByKey m_left_index;
    detail::RowsByKey m_right_index;
    urd::handle<ygg::FreeScratchRelation> m_result;
    Delta m_delta;
    bool m_initialized = false;

    /// Whether a row of L, by its values, has a match in the current R on the plan's keys.
    template<ygg::RowValues Values>
    [[nodiscard]] bool matched(const Values& left_row) const
    {
        const auto left_keys = m_plan.left_keys();
        const auto right_keys = m_plan.right_keys();
        if (left_keys.empty())
            return !m_right_operand->view().empty();
        for (const auto [first, last] = m_right_index.equal_range(database::detail::join_key_hash(left_row, left_keys));
             const auto slot : std::ranges::subrange(first, last) | std::views::values)
            if (database::detail::join_keys_equal(m_right_operand->at(slot).values, right_keys, left_row, left_keys))
                return true;
        return false;
    }

    template<ygg::RowValues Values>
    [[nodiscard]] bool kept(const Values& left_row) const
    { return matched(left_row) != m_anti; }

    /// Records the change of a row of the current L against the result before this update.
    template<ygg::RowValues Values>
    void reconcile(const Values& row)
    {
        const auto was = m_result->view().contains(row);
        const auto is = kept(row);
        if (is && !was)
            m_delta.added->insert(row);
        else if (!is && was)
            m_delta.removed->insert(row);
    }

    /// Reconciles the rows of the current L that share the key of a changed row of R.
    template<ygg::RelationViewConcept<ygg::Schema> V>
    void reconcile_matches(const V& right_rows)
    {
        const auto left_keys = m_plan.left_keys();
        const auto right_keys = m_plan.right_keys();
        if (right_rows.get_rows().empty())
            return;
        if (left_keys.empty())
        {
            for (const auto& held : m_left_operand->view().get_rows())
                ygg::with_values(held, [&](const auto& values) { reconcile(values); });
            return;
        }
        for (const auto& right_held : right_rows.get_rows())
            ygg::with_values(right_held,
                             [&](const auto& right_row)
                             {
                                 for (const auto [first, last] = m_left_index.equal_range(database::detail::join_key_hash(right_row, right_keys));
                                      const auto slot : std::ranges::subrange(first, last) | std::views::values)
                                     if (const auto& left_row = m_left_operand->at(slot).values; database::detail::join_keys_equal(left_row, left_keys, right_row, right_keys))
                                         reconcile(left_row);
                             });
    }

    template<RelationChange L, RelationChange R>
    void apply(const L& left_operand, const R& right_operand)
    {
        const auto& [left_added, left_removed] = left_operand;
        const auto& [right_added, right_removed] = right_operand;
        m_initialized = false;
        m_delta.clear();
        for (const auto& held : left_removed.get_rows())
            ygg::with_values(held,
                             [&](const auto& values)
                             {
                                 if (m_result->view().contains(values))
                                     m_delta.removed->insert(values);
                             });
        detail::erase_rows(left_removed, *m_left_operand, m_plan.left_keys(), m_left_index);
        detail::erase_rows(right_removed, *m_right_operand, m_plan.right_keys(), m_right_index);
        detail::insert_rows(right_added, *m_right_operand, m_plan.right_keys(), m_right_index);
        detail::insert_rows(left_added, *m_left_operand, m_plan.left_keys(), m_left_index);
        for (const auto& held : left_added.get_rows())
            ygg::with_values(held, [&](const auto& values) { reconcile(values); });
        if (m_plan.left_keys().empty())
            reconcile_matches(right_added.get_rows().empty() ? right_removed : right_added);
        else
        {
            reconcile_matches(right_removed);
            reconcile_matches(right_added);
        }
        detail::apply_delta(m_delta, *m_result);
        m_initialized = true;
    }

    template<typename>
    friend struct detail::NodeRules;

public:
    /// Its relations are drawn from `scratch`, which must outlive it.
    SemijoinEvaluator(ygg::FreeScratchRelations& scratch, JoinPlan plan, bool anti) :
        m_plan(std::move(plan)),
        m_anti(anti),
        m_left_operand(scratch.draw(m_plan.left_columns())),
        m_right_operand(scratch.draw(m_plan.right_columns())),
        m_result(scratch.draw(m_plan.left_columns())),
        m_delta(scratch, m_plan.left_columns())
    {
    }

    /// Replace the baseline, retaining buffers, and clear the last output delta.
    template<ygg::RelationViewConcept<ygg::Schema> L, ygg::RelationViewConcept<ygg::Schema> R>
    void initialize(const L& left_operand, const R& right_operand)
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
        for (const auto& held : m_left_operand->view().get_rows())
            ygg::with_values(held,
                             [&](const auto& values)
                             {
                                 if (kept(values))
                                     m_result->insert(values);
                             });
        m_initialized = true;
    }

    /// Each change describes actual set changes of that input. Reports the exact net change of the batch.
    template<RelationChange L, RelationChange R>
    void update(const L& left_operand, const R& right_operand)
    {
        detail::require_initialized(m_initialized);
        detail::require_change(left_operand, m_plan.left_columns(), *m_result, m_delta);
        detail::require_change(right_operand, m_plan.right_columns(), *m_result, m_delta);
        apply(left_operand, right_operand);
    }

    [[nodiscard]] ygg::FreeRelationBuilderView get_result() const&
    {
        detail::require_initialized(m_initialized);
        return m_result->view();
    }

    [[nodiscard]] ygg::FreeRelationBuilderView get_result() const&& = delete;

    [[nodiscard]] const Delta& get_delta() const&
    {
        detail::require_initialized(m_initialized);
        return m_delta;
    }

    [[nodiscard]] const Delta& get_delta() const&& = delete;

    [[nodiscard]] size_t memory_usage() const noexcept
    {
        return urd::memory_usage(m_left_operand->relation) + urd::memory_usage(m_right_operand->relation) + urd::memory_usage(m_result->relation)
               + m_delta.memory_usage() + urd::memory_usage(m_left_index) + urd::memory_usage(m_right_index);
    }
};

}  // namespace tyr::datalog::database::incremental

#endif
