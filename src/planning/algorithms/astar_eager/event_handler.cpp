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

#include "tyr/planning/algorithms/astar_eager/event_handler.hpp"

#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/planning/formatter.hpp"
#include "tyr/planning/ground/state_view.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/state_view.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/plan.hpp"

#include <fmt/ostream.h>

namespace tyr::planning::astar_eager
{
namespace
{

template<TaskKind Kind>
class DefaultWorkerEventHandler final : public WorkerEventHandler<Kind>
{
public:
    DefaultWorkerEventHandler(ygg::Index<Worker> index, bool trace_nodes) : m_index(index), m_trace_nodes(trace_nodes) {}

    void on_expand_node(const Node<Kind>& node) override
    {
        if (!m_trace_nodes)
            return;
        fmt::print("[ASTAR][Worker {}] ----------------------------------------\n[ASTAR][Worker {}] Expanding node: {}\n\n",
                   ygg::uint_t(m_index),
                   ygg::uint_t(m_index),
                   node);
    }

    void on_generate_transition(const Node<Kind>&, const LabeledNode<Kind>& labeled_succ_node, TransitionOutcome outcome) override
    {
        if (!m_trace_nodes
            || (outcome != TransitionOutcome::OPENED && outcome != TransitionOutcome::RELAXED && outcome != TransitionOutcome::DEAD_END
                && outcome != TransitionOutcome::GOAL))
            return;
        fmt::print("[ASTAR][Worker {}] Action: {}\n[ASTAR][Worker {}] Successor node: {}\n\n",
                   ygg::uint_t(m_index),
                   labeled_succ_node.label,
                   ygg::uint_t(m_index),
                   labeled_succ_node.node);
    }

    void on_finish_f_layer(ygg::float_t f_value) override { fmt::print("[ASTAR][Worker {}] Finished f-layer: {}\n", ygg::uint_t(m_index), f_value); }

private:
    ygg::Index<Worker> m_index;
    bool m_trace_nodes;
};

}

template<TaskKind Kind>
DefaultEventHandler<Kind>::DefaultEventHandler(size_t verbosity) : m_verbosity(verbosity)
{
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_start_search(const Node<Kind>&, ygg::float_t f_value)
{
    if (m_verbosity < 1)
        return;
    fmt::print("[ASTAR] Search started.\n[ASTAR] Start node f_value: {}\n", f_value);
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_end_search(SearchStatus, const tyr::planning::Statistics& statistics)
{
    if (m_verbosity < 1)
        return;
    fmt::print("[ASTAR] Search ended.\n{}\n", statistics);
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_solved(const Plan<Kind>& plan)
{
    if (m_verbosity < 1)
        return;
    fmt::print("[ASTAR] Plan found.\n[ASTAR] Plan cost: {}\n[ASTAR] Plan length: {}\n{}\n", plan.get_cost(), plan.get_length(), plan);
}

template<TaskKind Kind>
WorkerEventHandlerPtr<Kind> DefaultEventHandler<Kind>::make_worker(ygg::Index<Worker> index)
{
    if (m_verbosity < 1)
        return nullptr;
    return std::make_unique<DefaultWorkerEventHandler<Kind>>(index, m_verbosity >= 2);
}

template<TaskKind Kind>
DefaultEventHandlerPtr<Kind> DefaultEventHandler<Kind>::create(size_t verbosity)
{
    return std::make_shared<DefaultEventHandler<Kind>>(verbosity);
}

template class DefaultEventHandler<LiftedTag>;
template class DefaultEventHandler<GroundTag>;

}
