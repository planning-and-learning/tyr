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

#include "tyr/planning/algorithms/utils.hpp"

#include "tyr/planning/ground/state_repository.hpp"
#include "tyr/planning/ground/successor_generator.hpp"
#include "tyr/planning/lifted/state_repository.hpp"
#include "tyr/planning/lifted/successor_generator.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace tyr::planning
{

template<TaskKind Kind>
Node<Kind> normalize_start_node(Task<Kind>& task, SuccessorGenerator<Kind>& successor_generator, std::optional<Node<Kind>> start_node)
{
    auto& target_repository = *successor_generator.get_state_repository();
    if (target_repository.get_task().get() != &task)
        throw std::invalid_argument("normalize_start_node(...): successor generator belongs to a different task.");

    auto node = start_node ? std::move(*start_node) : successor_generator.get_initial_node();
    if (node.get_state().get_state_repository()->get_task().get() != &task)
        throw std::invalid_argument("normalize_start_node(...): start node belongs to a different task.");
    if (!std::isfinite(node.get_metric()))
        throw std::runtime_error("normalize_start_node(...): start node metric value is not finite.");

    return Node<Kind>(materialize_state(node.get_state(), target_repository), node.get_metric());
}

template Node<GroundTag>
normalize_start_node(Task<GroundTag>& task, SuccessorGenerator<GroundTag>& successor_generator, std::optional<Node<GroundTag>> start_node);
template Node<LiftedTag>
normalize_start_node(Task<LiftedTag>& task, SuccessorGenerator<LiftedTag>& successor_generator, std::optional<Node<LiftedTag>> start_node);

}
