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

#include "tyr/planning/algorithms/brfs/event_handler.hpp"

#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/planning/formatter.hpp"
#include "tyr/planning/ground/state_view.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/state_view.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/plan.hpp"

#include <fmt/ostream.h>
#include <iostream>
#include <syncstream>
#include <yggdrasil/core/chrono.hpp>

namespace tyr::planning::brfs
{
namespace
{

template<TaskKind Kind>
class DefaultWorkerEventHandler final : public WorkerEventHandler<Kind>
{
public:
    explicit DefaultWorkerEventHandler(ygg::Index<Worker> index) : m_index(index) {}

    void on_expand_node(const Node<Kind>& node) override
    {
        auto out = std::osyncstream(std::cout);
        fmt::print(out,
                   "[BRFS][Worker {}] ----------------------------------------\n[BRFS][Worker {}] Expanding node: {}\n\n",
                   ygg::uint_t(m_index),
                   ygg::uint_t(m_index),
                   node);
    }

    void on_generate_transition(const Node<Kind>&, const LabeledNode<Kind>& labeled_succ_node, TransitionOutcome outcome) override
    {
        if (outcome != TransitionOutcome::OPENED && outcome != TransitionOutcome::GOAL)
            return;

        auto out = std::osyncstream(std::cout);
        fmt::print(out,
                   "[BRFS][Worker {}] Action: {}\n[BRFS][Worker {}] Successor node: {}\n\n",
                   ygg::uint_t(m_index),
                   labeled_succ_node.label,
                   ygg::uint_t(m_index),
                   labeled_succ_node.node);
    }

private:
    ygg::Index<Worker> m_index;
};

}

template<TaskKind Kind>
DefaultEventHandler<Kind>::DefaultEventHandler(size_t verbosity) : m_verbosity(verbosity)
{
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_start_search(const Node<Kind>& node)
{
    m_progress_statistics.clear();
    if (m_verbosity < 1)
        return;
    auto out = std::osyncstream(std::cout);
    fmt::print(out, "[BRFS] Search started.\n[BRFS] Start node: {}\n", node);
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_finish_layer(ygg::uint_t layer, const tyr::planning::Statistics& statistics)
{
    m_progress_statistics.add_snapshot(statistics);
    if (m_verbosity < 1)
        return;
    auto out = std::osyncstream(std::cout);
    fmt::print(out,
               "[BRFS] Finished layer: {} with num expanded states {} and num generated states {} ({} ms)\n",
               layer,
               statistics.get_num_expanded(),
               statistics.get_num_generated(),
               ygg::to_ms(statistics.get_current_search_time()));
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_end_search(tyr::planning::SearchStatus, const tyr::planning::Statistics& statistics)
{
    if (m_verbosity < 1)
        return;
    auto out = std::osyncstream(std::cout);
    fmt::print(out, "[BRFS] Search ended.\n{}\n{}\n", statistics, m_progress_statistics);
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_solved(const Plan<Kind>& plan)
{
    if (m_verbosity < 1)
        return;
    auto out = std::osyncstream(std::cout);
    fmt::print(out, "[BRFS] Plan found.\n[BRFS] Plan cost: {}\n[BRFS] Plan length: {}\n{}\n", plan.get_cost(), plan.get_length(), plan);
}

template<TaskKind Kind>
WorkerEventHandlerPtr<Kind> DefaultEventHandler<Kind>::make_worker(ygg::Index<Worker> index)
{
    if (m_verbosity < 2)
        return nullptr;

    return std::make_unique<DefaultWorkerEventHandler<Kind>>(index);
}

template<TaskKind Kind>
DefaultEventHandlerPtr<Kind> DefaultEventHandler<Kind>::create(size_t verbosity)
{
    return std::make_shared<DefaultEventHandler<Kind>>(verbosity);
}

template class DefaultEventHandler<LiftedTag>;
template class DefaultEventHandler<GroundTag>;

}
