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

#include "tyr/planning/algorithms/gbfs_lazy/event_handler.hpp"

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

namespace tyr::planning::gbfs_lazy
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
                   "[GBFS][Worker {}] ----------------------------------------\n[GBFS][Worker {}] Expanding node: {}\n\n",
                   ygg::uint_t(m_index),
                   ygg::uint_t(m_index),
                   node);
    }

    void on_generate_node(const Node<Kind>&, const LabeledNode<Kind>& labeled_succ_node) override
    {
        auto out = std::osyncstream(std::cout);
        fmt::print(out,
                   "[GBFS][Worker {}] Action: {}\n[GBFS][Worker {}] Successor node: {}\n\n",
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
void DefaultEventHandler<Kind>::on_start_search(const Node<Kind>&, ygg::float_t h_value)
{
    if (m_verbosity < 1)
        return;
    auto out = std::osyncstream(std::cout);
    fmt::print(out, "[GBFS] Search started.\n[GBFS] Start node h_value: {}\n", h_value);
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_new_best_h_value(ygg::float_t h_value)
{
    if (m_verbosity < 1)
        return;
    auto out = std::osyncstream(std::cout);
    fmt::print(out, "[GBFS] New best h_value: {}\n", h_value);
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_end_search(SearchStatus, const tyr::planning::Statistics& statistics)
{
    if (m_verbosity < 1)
        return;
    auto out = std::osyncstream(std::cout);
    fmt::print(out, "[GBFS] Search ended.\n{}\n", statistics);
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_solved(const Plan<Kind>& plan)
{
    if (m_verbosity < 1)
        return;
    auto out = std::osyncstream(std::cout);
    fmt::print(out, "[GBFS] Plan found.\n[GBFS] Plan cost: {}\n[GBFS] Plan length: {}\n{}\n", plan.get_cost(), plan.get_length(), plan);
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
