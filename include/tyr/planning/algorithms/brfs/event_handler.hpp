/*
 * Copyright (C) 2025-2026 Dominik Drexler
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef TYR_PLANNING_ALGORITHMS_BRFS_EVENT_HANDLER_HPP_
#define TYR_PLANNING_ALGORITHMS_BRFS_EVENT_HANDLER_HPP_

#include "tyr/planning/algorithms/statistics.hpp"
#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/worker_index.hpp"

#include <cstddef>
#include <memory>

namespace tyr::planning::brfs
{

/// @brief Worker-local events emitted synchronously by BrFS search.
/// Calls are serialized per logical worker but have no OS-thread affinity. For remote transitions, the source belongs to the sender repository and the
/// target to the receiver repository; retaining either node intentionally retains its repository. A callback must not re-enter the search or wait for work
/// from the same logical worker.
template<TaskKind Kind>
class WorkerEventHandler
{
public:
    virtual ~WorkerEventHandler() = default;

    virtual void on_expand_node(const Node<Kind>&) {}
    virtual void on_expand_goal_node(const Node<Kind>&) {}
    virtual void on_generate_transition(const Node<Kind>&, const LabeledNode<Kind>&, TransitionOutcome) {}
};

/// @brief Search-lifecycle events shared by all BrFS workers. on_finish_layer runs on the worker that completes the depth barrier.
/// on_start_search is emitted only after the root passes immediate terminal checks.
template<TaskKind Kind>
class EventHandler
{
public:
    using StatisticsType = tyr::planning::Statistics;

    virtual ~EventHandler() = default;

    virtual void on_start_search(const Node<Kind>& node) = 0;
    virtual void on_finish_layer(ygg::uint_t layer, const tyr::planning::Statistics& statistics) = 0;
    virtual void on_end_search(tyr::planning::SearchStatus status, const tyr::planning::Statistics& statistics) = 0;
    virtual void on_solved(const Plan<Kind>& plan) = 0;
    virtual WorkerEventHandlerPtr<Kind> make_worker(ygg::Index<Worker>) { return nullptr; }
};

template<TaskKind Kind>
class DefaultEventHandler : public EventHandler<Kind>
{
public:
    explicit DefaultEventHandler(size_t verbosity = 0);

    void on_start_search(const Node<Kind>& node) override;
    void on_finish_layer(ygg::uint_t layer, const tyr::planning::Statistics& statistics) override;
    void on_end_search(tyr::planning::SearchStatus status, const tyr::planning::Statistics& statistics) override;
    void on_solved(const Plan<Kind>& plan) override;
    WorkerEventHandlerPtr<Kind> make_worker(ygg::Index<Worker> index) override;

    const tyr::planning::ProgressStatistics& get_progress_statistics() const noexcept { return m_progress_statistics; }

    static DefaultEventHandlerPtr<Kind> create(size_t verbosity = 0);

private:
    tyr::planning::ProgressStatistics m_progress_statistics;
    size_t m_verbosity;
};

}

#endif
