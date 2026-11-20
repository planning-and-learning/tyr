/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_INCREMENTAL_PROJECTION_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_INCREMENTAL_PROJECTION_HPP_

#include "tyr/datalog/lifted/database/semantics/incremental/delta.hpp"
#include "tyr/datalog/lifted/database/semantics/operations.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <limits>
#include <span>
#include <urd/repr/maps/index_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace tyr::datalog::database::incremental
{

/// Maintains set projection and the number of input rows supporting each output.
/// Support counting follows Gupta, Mumick and Subrahmanian, "Maintaining Views
/// Incrementally" (SIGMOD 1993), Sections 4-5: https://doi.org/10.1145/170035.170066.
/// Inputs are borrowed only for each call and must not alias this evaluator's
/// result/delta or the sequential caller workspace. Results and deltas are borrowed
/// until the next mutation. Rejected schema/overlap checks preserve the previous
/// evaluation. After a failure during mutation, results and deltas are unavailable
/// until initialize().
class ProjectionEvaluator
{
    ProjectionPlan m_plan;
    urd::handle<ygg::FreeScratchRelation> m_result;
    using Slot = ygg::FreeScratchRelation::Slot;
    /// The supporting input rows of each result row, by the slot of its record in the result's records (a dense index,
    /// stable while the result holds the row: its position shifts); its memory kept, as the evaluator is pooled.
    urd::Store<urd::map<Slot, size_t>, urd::dense, urd::retains> m_supports;
    Delta m_delta;
    urd::Store<urd::sequence<Slot>> m_pending_removals;
    bool m_initialized = false;

    /// The projection of an input row's values, in the workspace's row (valid until its next use).
    template<ygg::RowValues Values>
    std::span<const ygg::Value> project_row(const Values& input, Workspace& workspace) const
    {
        auto& row = workspace.row;
        row.clear();
        database::detail::copy_fields(row, input, m_plan.positions());
        return row;
    }

    bool add(std::span<const ygg::Value> row);
    void remove(std::span<const ygg::Value> row);
    /// Applies a change that is known to be valid.
    template<RelationChange C>
    void apply(const C& change, Workspace& workspace);

    friend struct detail::NodeRules<QueryProjectTag>;

public:
    /// Its relations are drawn from `scratch`, which must outlive it.
    ProjectionEvaluator(ygg::FreeScratchRelations& scratch, ProjectionPlan plan);

    /// Replace the baseline, retaining buffers, and clear the last output delta.
    template<ygg::RelationViewConcept<ygg::Schema> V>
    void initialize(const V& input, Workspace& workspace);

    /// The change must describe an actual change to the initialized input set.
    /// Schema, overlap, and aliasing errors are rejected before changing the result.
    template<RelationChange C>
    void update(const C& change, Workspace& workspace);

    [[nodiscard]] ygg::FreeRelationBuilderView get_result() const&;
    [[nodiscard]] ygg::FreeRelationBuilderView get_result() const&& = delete;
    [[nodiscard]] const Delta& get_delta() const&;
    [[nodiscard]] const Delta& get_delta() const&& = delete;

    /// Retained working storage; excludes the immutable plan and caller workspace.
    [[nodiscard]] size_t memory_usage() const;
};

template<ygg::RelationViewConcept<ygg::Schema> V>
void ProjectionEvaluator::initialize(const V& input, Workspace& workspace)
{
    assert(std::ranges::equal(input.columns(), m_plan.input_columns()));
    detail::require_unaliased(input, *m_result, m_delta);
    m_initialized = false;
    m_result->clear();
    m_supports.clear();
    m_delta.clear();
    m_pending_removals.clear();
    for (const auto& held : input.get_rows())
        ygg::with_values(held, [&](const auto& values) { add(project_row(values, workspace)); });
    m_initialized = true;
}

template<RelationChange C>
void ProjectionEvaluator::update(const C& change, Workspace& workspace)
{
    detail::require_initialized(m_initialized);
    detail::require_change(change, m_plan.input_columns(), *m_result, m_delta);
    apply(change, workspace);
}

template<RelationChange C>
void ProjectionEvaluator::apply(const C& change, Workspace& workspace)
{
    const auto& [added, removed] = change;
    m_initialized = false;
    m_delta.clear();
    m_pending_removals.clear();
    for (const auto& held : removed.get_rows())
        ygg::with_values(held, [&](const auto& values) { remove(project_row(values, workspace)); });
    for (const auto& held : added.get_rows())
        ygg::with_values(held,
                         [&](const auto& values)
                         {
                             const auto row = project_row(values, workspace);
                             if (add(row))
                                 m_delta.added->insert(row);
                         });
    // Inspect final support counts only (DBSP, Proposition 4.7). A replaced witness revives its existing row without an
    // erase/insert cycle. A row may be pending twice (its support fell to zero, rose, and fell again): once erased, its
    // support is gone.
    for (const auto slot : m_pending_removals)
        if (const auto found = m_supports.find(slot); found != m_supports.end() && found->second == 0)
        {
            m_supports.erase(slot);
            // Its record stays in its slot, values and all, once the row is erased (until a later insert takes the slot).
            const auto& values = m_result->at(slot).values;
            if (m_result->erase(values))
                m_delta.removed->insert(values);
        }
    m_initialized = true;
}

}  // namespace tyr::datalog::database::incremental

#endif
