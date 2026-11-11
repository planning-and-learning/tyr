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
    urd::builder<ygg::Relation<ygg::Schema>> m_lhs;
    urd::builder<ygg::Relation<ygg::Schema>> m_rhs;
    urd::Store<urd::map<ygg::hash_t, ygg::uint_t>, urd::multi, urd::retains> m_lhs_index;
    urd::Store<urd::map<ygg::hash_t, ygg::uint_t>, urd::multi, urd::retains> m_rhs_index;
    urd::builder<ygg::Relation<ygg::Schema>> m_result;
    Delta m_delta;
    bool m_initialized = false;

    [[nodiscard]] bool matched(const ygg::RowPool::row_type& lhs_row) const;
    [[nodiscard]] bool kept(const ygg::RowPool::row_type& lhs_row) const { return matched(lhs_row) != m_anti; }

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
    void reconcile_matches(const V& rhs_rows)
    {
        const auto lhs_keys = m_plan.lhs_keys();
        const auto rhs_keys = m_plan.rhs_keys();
        if (rhs_rows.empty())
            return;
        const auto current = m_layer->view(m_lhs);
        if (lhs_keys.empty())
        {
            for (size_t i = 0; i < current.size(); ++i)
                reconcile(current.row(i).get_values());
            return;
        }
        for (size_t i = 0; i < rhs_rows.size(); ++i)
        {
            const auto rhs_row = rhs_rows.row(i).get_values();
            for (const auto [first, last] = m_lhs_index.equal_range(database::detail::join_key_hash(rhs_row, rhs_keys));
                 const auto position : std::ranges::subrange(first, last) | std::views::values)
                if (database::detail::join_keys_equal(current.row(position).get_values(), lhs_keys, rhs_row, rhs_keys))
                    reconcile(current.row(position).get_values());
        }
    }

    template<RelationChange L, RelationChange R>
    void apply(const L& lhs, const R& rhs)
    {
        const auto& [lhs_added, lhs_removed] = lhs;
        const auto& [rhs_added, rhs_removed] = rhs;
        m_initialized = false;
        m_delta.clear();
        for (size_t i = 0; i < lhs_removed.size(); ++i)
            if (const auto row = lhs_removed.row(i).get_values(); m_result.contains(row))
                m_delta.removed.insert(row);
        detail::erase_rows(*m_layer, lhs_removed, m_lhs, m_plan.lhs_keys(), m_lhs_index);
        detail::erase_rows(*m_layer, rhs_removed, m_rhs, m_plan.rhs_keys(), m_rhs_index);
        detail::insert_rows(rhs_added, m_rhs, m_plan.rhs_keys(), m_rhs_index);
        detail::insert_rows(lhs_added, m_lhs, m_plan.lhs_keys(), m_lhs_index);
        for (size_t i = 0; i < lhs_added.size(); ++i)
            reconcile(lhs_added.row(i).get_values());
        if (m_plan.lhs_keys().empty())
            reconcile_matches(rhs_added.empty() ? rhs_removed : rhs_added);
        else
        {
            reconcile_matches(rhs_removed);
            reconcile_matches(rhs_added);
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
        m_lhs(m_plan.lhs_columns()),
        m_rhs(m_plan.rhs_columns()),
        m_result(m_plan.lhs_columns()),
        m_delta(layer, m_plan.lhs_columns())
    {
    }

    /// Replace the baseline, retaining buffers, and clear the last output delta.
    template<RelationViewConcept L, RelationViewConcept R>
    void initialize(const L& lhs, const R& rhs)
    {
        assert(std::ranges::equal(lhs.columns(), m_plan.lhs_columns()));
        assert(std::ranges::equal(rhs.columns(), m_plan.rhs_columns()));
        detail::require_unaliased(lhs, m_result, m_delta);
        detail::require_unaliased(rhs, m_result, m_delta);
        m_initialized = false;
        m_lhs.clear();
        m_rhs.clear();
        m_lhs_index.clear();
        m_rhs_index.clear();
        m_result.clear();
        m_delta.clear();
        detail::insert_rows(lhs, m_lhs, m_plan.lhs_keys(), m_lhs_index);
        detail::insert_rows(rhs, m_rhs, m_plan.rhs_keys(), m_rhs_index);
        const auto current = m_layer->view(m_lhs);
        for (size_t i = 0; i < current.size(); ++i)
            if (kept(current.row(i).get_values()))
                m_result.insert(current.row(i).get_values());
        m_initialized = true;
    }

    /// Each change describes actual set changes of that input. Reports the exact net change of the batch.
    template<RelationChange L, RelationChange R>
    void update(const L& lhs, const R& rhs)
    {
        detail::require_initialized(m_initialized);
        detail::require_change(lhs, m_plan.lhs_columns(), m_result, m_delta);
        detail::require_change(rhs, m_plan.rhs_columns(), m_result, m_delta);
        apply(lhs, rhs);
    }

    [[nodiscard]] ygg::FreeBuilderRelationView get_result() const&
    {
        detail::require_initialized(m_initialized);
        return m_layer->view(m_result);
    }
    [[nodiscard]] ygg::FreeBuilderRelationView get_result() const&& = delete;
    [[nodiscard]] const Delta& get_delta() const&
    {
        detail::require_initialized(m_initialized);
        return m_delta;
    }
    [[nodiscard]] const Delta& get_delta() const&& = delete;

    [[nodiscard]] size_t memory_usage() const noexcept
    {
        return m_lhs.memory_usage() + m_rhs.memory_usage() + m_result.memory_usage() + m_delta.memory_usage() + urd::memory_usage(m_lhs_index)
               + urd::memory_usage(m_rhs_index);
    }
};

}  // namespace tyr::datalog::database::incremental

#endif
