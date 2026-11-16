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
    const ygg::FreeRelationRepository* m_layer;
    JoinPlan m_plan;
    bool m_anti;
    urd::builder<ygg::Relation<ygg::Schema>> m_left_operand;
    urd::builder<ygg::Relation<ygg::Schema>> m_right_operand;
    urd::Store<urd::map<ygg::hash_t, ygg::uint_t>, urd::multi, urd::retains> m_left_index;
    urd::Store<urd::map<ygg::hash_t, ygg::uint_t>, urd::multi, urd::retains> m_right_index;
    urd::builder<ygg::Relation<ygg::Schema>> m_result;
    Delta m_delta;
    bool m_initialized = false;

    [[nodiscard]] bool matched(const ygg::RowPool::row_type& left_row) const;
    [[nodiscard]] bool kept(const ygg::RowPool::row_type& left_row) const { return matched(left_row) != m_anti; }

    /// Records the change of a row of the current L against the result before this update.
    void reconcile(const ygg::RowPool::row_type& row)
    {
        const auto was = m_result.contains(row);
        const auto is = kept(row);
        if (is && !was)
            m_delta.added.insert(row);
        else if (!is && was)
            m_delta.removed.insert(row);
    }
    /// Reconciles the rows of the current L that share the key of a changed row of R.
    template<RelationViewConcept V>
    void reconcile_matches(const V& right_rows)
    {
        const auto left_keys = m_plan.left_keys();
        const auto right_keys = m_plan.right_keys();
        if (right_rows.empty())
            return;
        const auto current = m_layer->view(m_left_operand);
        if (left_keys.empty())
        {
            for (size_t i = 0; i < current.size(); ++i)
                reconcile(current.row(i).get_values());
            return;
        }
        for (size_t i = 0; i < right_rows.size(); ++i)
        {
            const auto right_row = right_rows.row(i).get_values();
            for (const auto [first, last] = m_left_index.equal_range(database::detail::join_key_hash(right_row, right_keys));
                 const auto position : std::ranges::subrange(first, last) | std::views::values)
                if (database::detail::join_keys_equal(current.row(position).get_values(), left_keys, right_row, right_keys))
                    reconcile(current.row(position).get_values());
        }
    }

    template<RelationChange L, RelationChange R>
    void apply(const L& left_operand, const R& right_operand)
    {
        const auto& [left_added, left_removed] = left_operand;
        const auto& [right_added, right_removed] = right_operand;
        m_initialized = false;
        m_delta.clear();
        for (size_t i = 0; i < left_removed.size(); ++i)
            if (const auto row = left_removed.row(i).get_values(); m_result.contains(row))
                m_delta.removed.insert(row);
        detail::erase_rows(*m_layer, left_removed, m_left_operand, m_plan.left_keys(), m_left_index);
        detail::erase_rows(*m_layer, right_removed, m_right_operand, m_plan.right_keys(), m_right_index);
        detail::insert_rows(right_added, m_right_operand, m_plan.right_keys(), m_right_index);
        detail::insert_rows(left_added, m_left_operand, m_plan.left_keys(), m_left_index);
        for (size_t i = 0; i < left_added.size(); ++i)
            reconcile(left_added.row(i).get_values());
        if (m_plan.left_keys().empty())
            reconcile_matches(right_added.empty() ? right_removed : right_added);
        else
        {
            reconcile_matches(right_removed);
            reconcile_matches(right_added);
        }
        detail::apply_delta(m_delta, m_result);
        m_initialized = true;
    }

    template<typename>
    friend struct detail::NodeRules;

public:
    /// Its builders are read through `layer`, which must outlive it.
    SemijoinEvaluator(const ygg::FreeRelationRepository& layer, JoinPlan plan, bool anti) :
        m_layer(&layer),
        m_plan(std::move(plan)),
        m_anti(anti),
        m_left_operand(m_plan.left_columns()),
        m_right_operand(m_plan.right_columns()),
        m_result(m_plan.left_columns()),
        m_delta(layer, m_plan.left_columns())
    {
    }

    /// Replace the baseline, retaining buffers, and clear the last output delta.
    template<RelationViewConcept L, RelationViewConcept R>
    void initialize(const L& left_operand, const R& right_operand)
    {
        assert(std::ranges::equal(left_operand.columns(), m_plan.left_columns()));
        assert(std::ranges::equal(right_operand.columns(), m_plan.right_columns()));
        detail::require_unaliased(left_operand, m_result, m_delta);
        detail::require_unaliased(right_operand, m_result, m_delta);
        m_initialized = false;
        m_left_operand.clear();
        m_right_operand.clear();
        m_left_index.clear();
        m_right_index.clear();
        m_result.clear();
        m_delta.clear();
        detail::insert_rows(left_operand, m_left_operand, m_plan.left_keys(), m_left_index);
        detail::insert_rows(right_operand, m_right_operand, m_plan.right_keys(), m_right_index);
        const auto current = m_layer->view(m_left_operand);
        for (size_t i = 0; i < current.size(); ++i)
            if (kept(current.row(i).get_values()))
                m_result.insert(current.row(i).get_values());
        m_initialized = true;
    }

    /// Each change describes actual set changes of that input. Reports the exact net change of the batch.
    template<RelationChange L, RelationChange R>
    void update(const L& left_operand, const R& right_operand)
    {
        detail::require_initialized(m_initialized);
        detail::require_change(left_operand, m_plan.left_columns(), m_result, m_delta);
        detail::require_change(right_operand, m_plan.right_columns(), m_result, m_delta);
        apply(left_operand, right_operand);
    }

    [[nodiscard]] ygg::FreeRelationBuilderView get_result() const&
    {
        detail::require_initialized(m_initialized);
        return m_layer->view(m_result);
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
        return m_left_operand.memory_usage() + m_right_operand.memory_usage() + m_result.memory_usage() + m_delta.memory_usage()
               + urd::memory_usage(m_left_index) + urd::memory_usage(m_right_index);
    }
};

}  // namespace tyr::datalog::database::incremental

#endif
