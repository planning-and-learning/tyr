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
/// Inserts rows absent from `rows`, indexing their positions by the hash of `keys`.
template<RelationViewConcept V>
void insert_rows(const V& input, urd::builder<ygg::Relation<ygg::Schema>>& rows, std::span<const ColumnSlice> keys, urd::Store<urd::map<ygg::hash_t, ygg::uint_t>, urd::multi, urd::retains>& index)
{
    if (!keys.empty())
        index.reserve(rows.size() + input.size());
    for (size_t i = 0; i < input.size(); ++i)
    {
        const auto row = input.row(i).get_values();
        const auto previous_size = rows.size();
        const auto position = rows.insert(row);
        if (position != previous_size)
            throw std::invalid_argument("Incremental evaluation: added row is already present in the input.");
        if (!keys.empty())
            index.insert({ database::detail::join_key_hash(row, keys), position });
    }
}

/// Erases rows present in `rows` (read through `layer`), keeping the position index of insert_rows() current.
template<RelationViewConcept V>
void erase_rows(const ygg::FreeRelationRepository& layer,
                const V& input,
                urd::builder<ygg::Relation<ygg::Schema>>& rows,
                std::span<const ColumnSlice> keys,
                urd::Store<urd::map<ygg::hash_t, ygg::uint_t>, urd::multi, urd::retains>& index)
{
    const auto stored = layer.view(rows);
    for (size_t i = 0; i < input.size(); ++i)
    {
        const auto position = rows.find(input.row(i).get_values());
        if (!position)
            throw std::invalid_argument("Incremental evaluation: removed row is absent from the input.");
        const auto last = ygg::to_uint_t(rows.size() - 1);
        if (keys.empty())
        {
            rows.erase(*position);
            continue;
        }
        const auto removed_key = database::detail::join_key_hash(stored.row(*position).get_values(), keys);
        rows.erase(*position);
        const auto erase = [&](ygg::hash_t key, ygg::uint_t value)
        {
            const auto matches = index.equal_range(key);
            const auto found = std::find_if(matches.first, matches.second, [&](const auto& entry) { return entry.second == value; });
            assert(found != matches.second);
            index.erase(found);
        };
        erase(removed_key, *position);
        if (*position != last)
        {
            // The last row moved into the erased one's position.
            const auto moved_key = database::detail::join_key_hash(stored.row(*position).get_values(), keys);
            erase(moved_key, last);
            index.insert({ moved_key, *position });
        }
    }
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
    const ygg::FreeRelationRepository* m_layer;
    JoinPlan m_plan;
    urd::builder<ygg::Relation<ygg::Schema>> m_lhs;
    urd::builder<ygg::Relation<ygg::Schema>> m_rhs;
    urd::Store<urd::map<ygg::hash_t, ygg::uint_t>, urd::multi, urd::retains> m_lhs_index;
    urd::Store<urd::map<ygg::hash_t, ygg::uint_t>, urd::multi, urd::retains> m_rhs_index;
    urd::builder<ygg::Relation<ygg::Schema>> m_result;
    Delta m_delta;
    bool m_initialized = false;

    template<RelationViewConcept L, RelationViewConcept R>
    void join(const L& lhs, const R& rhs, bool build_left, urd::builder<ygg::Relation<ygg::Schema>>& output, Workspace& workspace) const;

    /// Applies changes that are known to be valid.
    template<RelationChange L, RelationChange R>
    void apply(const L& lhs, const R& rhs, Workspace& workspace);

    friend struct detail::NodeRules<QueryJoinTag>;

public:
    /// Its builders are read through `layer`, which must outlive it.
    JoinEvaluator(const ygg::FreeRelationRepository& layer, JoinPlan plan);

    /// Replace the baseline, retaining buffers, and clear the last output delta.
    template<RelationViewConcept L, RelationViewConcept R>
    void initialize(const L& lhs, const R& rhs, Workspace& workspace);

    /// Each change describes actual set changes in that input's original column
    /// order. Pass empty changes for an unchanged side; swap added/removed on
    /// both sides to undo. Reports the exact net change of the complete batch.
    template<RelationChange L, RelationChange R>
    void update(const L& lhs, const R& rhs, Workspace& workspace);

    [[nodiscard]] ygg::FreeBuilderRelationView get_result() const&;
    [[nodiscard]] ygg::FreeBuilderRelationView get_result() const&& = delete;
    [[nodiscard]] const Delta& get_delta() const&;
    [[nodiscard]] const Delta& get_delta() const&& = delete;

    /// Retained inputs, result, delta and indexes; excludes plan and caller workspace.
    [[nodiscard]] size_t memory_usage() const noexcept;
};

template<RelationViewConcept L, RelationViewConcept R>
void JoinEvaluator::join(const L& lhs, const R& rhs, bool build_left, urd::builder<ygg::Relation<ygg::Schema>>& output, Workspace& workspace) const
{
    database::detail::join_rows(lhs, rhs, m_plan, build_left, build_left ? m_lhs_index : m_rhs_index, output, workspace);
}

template<RelationViewConcept L, RelationViewConcept R>
void JoinEvaluator::initialize(const L& lhs, const R& rhs, Workspace& workspace)
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
    // Both indexes are ready, so probe the smaller input.
    join(m_layer->view(m_lhs), m_layer->view(m_rhs), m_lhs.size() >= m_rhs.size(), m_result, workspace);
    m_initialized = true;
}

template<RelationChange L, RelationChange R>
void JoinEvaluator::update(const L& lhs, const R& rhs, Workspace& workspace)
{
    detail::require_initialized(m_initialized);
    detail::require_change(lhs, m_plan.lhs_columns(), m_result, m_delta);
    detail::require_change(rhs, m_plan.rhs_columns(), m_result, m_delta);
    apply(lhs, rhs, workspace);
}

template<RelationChange L, RelationChange R>
void JoinEvaluator::apply(const L& lhs, const R& rhs, Workspace& workspace)
{
    const auto& [lhs_added, lhs_removed] = lhs;
    const auto& [rhs_added, rhs_removed] = rhs;
    m_initialized = false;
    m_delta.clear();

    // Remove from both sides before inserting. Each old/new pair is visited
    // once, and no temporary match between an added and removed row is emitted.
    const auto current_lhs = m_layer->view(m_lhs);
    const auto current_rhs = m_layer->view(m_rhs);
    join(lhs_removed, current_rhs, false, m_delta.removed, workspace);
    detail::erase_rows(*m_layer, lhs_removed, m_lhs, m_plan.lhs_keys(), m_lhs_index);
    join(current_lhs, rhs_removed, true, m_delta.removed, workspace);
    detail::erase_rows(*m_layer, rhs_removed, m_rhs, m_plan.rhs_keys(), m_rhs_index);
    join(lhs_added, current_rhs, false, m_delta.added, workspace);
    detail::insert_rows(lhs_added, m_lhs, m_plan.lhs_keys(), m_lhs_index);
    join(current_lhs, rhs_added, true, m_delta.added, workspace);
    detail::insert_rows(rhs_added, m_rhs, m_plan.rhs_keys(), m_rhs_index);

    detail::apply_delta(m_delta, m_result);
    m_initialized = true;
}

}  // namespace tyr::datalog::database::incremental

#endif
