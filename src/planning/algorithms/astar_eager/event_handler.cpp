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
#include <iostream>
#include <yggdrasil/core/chrono.hpp>

namespace tyr::planning::astar_eager
{
template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_expand_node_impl(const Node<Kind>& node) const
{
    fmt::print(std::cout, "[ASTAR] ----------------------------------------\n[ASTAR] Expanding node: {}\n\n", node);
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_expand_goal_node_impl(const Node<Kind>& node) const
{
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_generate_node_impl(const LabeledNode<Kind>& labeled_succ_node) const
{
    fmt::print(std::cout, "[ASTAR] Action: {}\n", labeled_succ_node.label);
    fmt::print(std::cout, "[ASTAR] Successor node: {}\n\n", labeled_succ_node.node);
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_generate_node_relaxed_impl(const LabeledNode<Kind>& labeled_succ_node) const
{
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_generate_node_not_relaxed_impl(const LabeledNode<Kind>& labeled_succ_node) const
{
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_close_node_impl(const Node<Kind>& node) const
{
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_prune_node_impl(const Node<Kind>& node) const
{
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_start_search_impl(const Node<Kind>& node, ygg::float_t f_value) const
{
    std::cout << "[ASTAR] Search started.\n"
              << "[ASTAR] Start node f_value: " << f_value << std::endl;
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_finish_f_layer_impl(ygg::float_t f_value, uint64_t num_expanded_states, uint64_t num_generated_states) const
{
    std::cout << "[ASTAR] Finished f-layer: " << f_value << " with num expanded states " << num_expanded_states << " and num generated states "
              << num_generated_states << " (" << ygg::to_ms(this->get_statistics().get_current_search_time()) << " ms)" << std::endl;
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_end_search_impl(tyr::planning::SearchStatus status) const
{
    static_cast<void>(status);
    fmt::print(std::cout, "[ASTAR] Search ended.\n{}\n{}\n", this->get_statistics(), this->get_progress_statistics());
}

template<TaskKind Kind>
void DefaultEventHandler<Kind>::on_solved_impl(const Plan<Kind>& plan) const
{
    std::cout << "[ASTAR] Plan found.\n"
              << "[ASTAR] Plan cost: " << plan.get_cost() << "\n"
              << "[ASTAR] Plan length: " << plan.get_length() << std::endl;

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
