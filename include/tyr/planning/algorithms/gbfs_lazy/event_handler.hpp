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

#ifndef TYR_PLANNING_ALGORITHMS_GBFS_LAZY_EVENT_HANDLER_HPP_
#define TYR_PLANNING_ALGORITHMS_GBFS_LAZY_EVENT_HANDLER_HPP_

#include "tyr/planning/algorithms/statistics.hpp"
#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/worker_index.hpp"

#include <cstddef>
#include <memory>

namespace tyr::planning::gbfs_lazy
{

/// @brief Worker-local events emitted synchronously by lazy GBFS.
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

/// @brief Search-wide events shared by all lazy GBFS workers.
/// on_new_best_h_value is serialized but runs on the worker that found the value.
template<TaskKind Kind>
class EventHandler
{
public:
    using StatisticsType = tyr::planning::Statistics;

    virtual ~EventHandler() = default;

    virtual void on_start_search(const Node<Kind>& node, ygg::float_t h_value) = 0;
    virtual void on_new_best_h_value(ygg::float_t h_value) = 0;
    virtual void on_end_search(SearchStatus status, const tyr::planning::Statistics& statistics) = 0;
    virtual void on_solved(const Plan<Kind>& plan) = 0;
    virtual WorkerEventHandlerPtr<Kind> make_worker(ygg::Index<Worker>) { return nullptr; }
};

template<TaskKind Kind>
class DefaultEventHandler : public EventHandler<Kind>
{
public:
    explicit DefaultEventHandler(size_t verbosity = 0);

    void on_start_search(const Node<Kind>& node, ygg::float_t h_value) override;
    void on_new_best_h_value(ygg::float_t h_value) override;
    void on_end_search(SearchStatus status, const tyr::planning::Statistics& statistics) override;
    void on_solved(const Plan<Kind>& plan) override;
    WorkerEventHandlerPtr<Kind> make_worker(ygg::Index<Worker> index) override;

    static DefaultEventHandlerPtr<Kind> create(size_t verbosity = 0);

private:
    size_t m_verbosity;
};

}

#endif
