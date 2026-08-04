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
#include <yggdrasil/core/chrono.hpp>

namespace tyr::planning::brfs
{

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_expand_node_impl(const Node<Kind>& node) const
{
    fmt::print(std::cout, "[BRFS] ----------------------------------------\n[BRFS] Expanding node: {}\n\n", node);
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_expand_goal_node_impl(const Node<Kind>& node) const
{
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_generate_node_impl(const LabeledNode<Kind>& labeled_succ_node) const
{
    fmt::print(std::cout, "[BRFS] Action: {}\n", labeled_succ_node.label);
    fmt::print(std::cout, "[BRFS] Successor node: {}\n\n", labeled_succ_node.node);
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_prune_node_impl(const Node<Kind>& node) const
{
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_start_search_impl(const Node<Kind>& node) const
{
    fmt::print(std::cout, "[BRFS] Search started.\n[BRFS] Start node: {}\n", node);
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_finish_layer_impl(ygg::uint_t layer, const tyr::planning::Statistics& statistics) const
{
    std::cout << "[BRFS] Finished layer: " << layer << " with num expanded states " << statistics.get_num_expanded() << " and num generated states "
              << statistics.get_num_generated() << " (" << ygg::to_ms(statistics.get_current_search_time()) << " ms)" << std::endl;
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_end_search_impl(tyr::planning::SearchStatus status, const tyr::planning::Statistics& statistics) const
{
    static_cast<void>(status);
    fmt::print(std::cout, "[BRFS] Search ended.\n{}\n{}\n", statistics, this->get_progress_statistics());
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_solved_impl(const Plan<Kind>& plan) const
{
    std::cout << "[BRFS] Plan found.\n"
              << "[BRFS] Plan cost: " << plan.get_cost() << "\n"
              << "[BRFS] Plan length: " << plan.get_length() << std::endl;

    fmt::print(std::cout, "{}\n", plan);
}

template<TaskKind Kind>
DefaultEventHandler<Kind>::DefaultEventHandler(size_t verbosity) : EventHandlerBase<DefaultEventHandler<Kind>, Kind>(verbosity)
{
}

template<TaskKind Kind>
DefaultEventHandlerPtr<Kind> DefaultEventHandler<Kind>::create(size_t verbosity)
{
    return std::make_shared<DefaultEventHandler<Kind>>(verbosity);
}

template class DefaultEventHandler<LiftedTag>;
template class DefaultEventHandler<GroundTag>;

}
