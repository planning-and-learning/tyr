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
#include "tyr/planning/axioms/axiom_evaluator.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"
#include "tyr/planning/states/state_builder.hpp"
#include "tyr/planning/states/state_repository.hpp"
#include "tyr/planning/successors/successors.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace tyr::planning
{

template<TaskKind Kind>
NodeBuilder<Kind> normalize_start_node(Task<Kind>& task,
                                       StateRepository<Kind>& state_repository,
                                       AxiomEvaluator<Kind>& axiom_evaluator,
                                       const std::optional<OwnedNodeRecordView<Kind>>& start_node)
{
    if (state_repository.get_task().get() != &task)
        throw std::invalid_argument("normalize_start_node(...): state repository belongs to a different task.");
    if (axiom_evaluator.get_task().get() != &task)
        throw std::invalid_argument("normalize_start_node(...): axiom evaluator belongs to a different task.");
    if (!start_node)
        return get_initial_node(state_repository, axiom_evaluator).release();

    const auto& source = start_node->context();
    if (source.get_task().get() != &task)
        throw std::invalid_argument("normalize_start_node(...): start node belongs to a different task.");
    const auto metric = start_node->get_metric();
    if (!std::isfinite(metric))
        throw std::runtime_error("normalize_start_node(...): start node metric value is not finite.");

    const auto source_state = source.get_registered_state(start_node->get_state().index());
    auto node = NodeBuilder<Kind>();
    node.state = materialize_state(urd::resolve(*source_state, source), state_repository, axiom_evaluator);
    node.metric = metric;
    return node;
}

template NodeBuilder<GroundTag> normalize_start_node(Task<GroundTag>& task,
                                                     StateRepository<GroundTag>& state_repository,
                                                     AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                     const std::optional<OwnedNodeRecordView<GroundTag>>& start_node);
template NodeBuilder<LiftedTag> normalize_start_node(Task<LiftedTag>& task,
                                                     StateRepository<LiftedTag>& state_repository,
                                                     AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                     const std::optional<OwnedNodeRecordView<LiftedTag>>& start_node);
}
