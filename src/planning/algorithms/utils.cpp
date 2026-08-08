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

#include "../metric.hpp"
#include "tyr/planning/ground/axiom_evaluator.hpp"
#include "tyr/planning/ground/state_repository.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/axiom_evaluator.hpp"
#include "tyr/planning/lifted/state_repository.hpp"
#include "tyr/planning/lifted/task.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace tyr::planning
{

template<TaskKind Kind>
Node<Kind>
normalize_start_node(Task<Kind>& task, StateRepository<Kind>& state_repository, AxiomEvaluator<Kind>& axiom_evaluator, std::optional<Node<Kind>> start_node)
{
    if (state_repository.get_task().get() != &task)
        throw std::invalid_argument("normalize_start_node(...): state repository belongs to a different task.");
    if (axiom_evaluator.get_task().get() != &task)
        throw std::invalid_argument("normalize_start_node(...): axiom evaluator belongs to a different task.");

    auto node = [&]
    {
        if (start_node)
            return std::move(*start_node);

        auto state = state_repository.get_initial_state(axiom_evaluator);
        const auto state_context = StateContext<Kind>(task, state.get_state_builder(), 0);
        const auto metric = evaluate_metric(task.get_task().get_metric(), task.get_task().get_auxiliary_fterm_value(), state_context);
        return Node<Kind>(std::move(state), metric);
    }();
    if (node.get_state().get_state_repository()->get_task().get() != &task)
        throw std::invalid_argument("normalize_start_node(...): start node belongs to a different task.");
    if (!std::isfinite(node.get_metric()))
        throw std::runtime_error("normalize_start_node(...): start node metric value is not finite.");

    return Node<Kind>(materialize_state(node.get_state(), state_repository, axiom_evaluator), node.get_metric());
}

template Node<GroundTag> normalize_start_node(Task<GroundTag>& task,
                                              StateRepository<GroundTag>& state_repository,
                                              AxiomEvaluator<GroundTag>& axiom_evaluator,
                                              std::optional<Node<GroundTag>> start_node);
template Node<LiftedTag> normalize_start_node(Task<LiftedTag>& task,
                                              StateRepository<LiftedTag>& state_repository,
                                              AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                              std::optional<Node<LiftedTag>> start_node);

}
