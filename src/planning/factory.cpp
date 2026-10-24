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

#include "tyr/planning/factory.hpp"

#include "tyr/planning/ground/axiom_evaluator.hpp"
#include "tyr/planning/ground/state_repository.hpp"
#include "tyr/planning/ground/successor_generator.hpp"
#include "tyr/planning/lifted/axiom_evaluator.hpp"
#include "tyr/planning/lifted/state_repository.hpp"
#include "tyr/planning/lifted/successor_generator.hpp"

#include <utility>

namespace tyr::planning
{

template<TaskKind Kind>
StateRepositoryPtr<Kind> StateRepositoryFactory<Kind>::create(TaskPtr<Kind> task)
{ return StateRepositoryPtr<Kind>(new StateRepository<Kind>(m_next_index->fetch_add(1, std::memory_order_relaxed), std::move(task), false, m_next_index)); }

template<TaskKind Kind>
StateRepositoryPtr<Kind> StateRepositoryFactory<Kind>::create_concurrent(TaskPtr<Kind> task)
{ return StateRepositoryPtr<Kind>(new StateRepository<Kind>(m_next_index->fetch_add(1, std::memory_order_relaxed), std::move(task), true, m_next_index)); }

template<TaskKind Kind>
AxiomEvaluatorPtr<Kind> AxiomEvaluatorFactory<Kind>::create(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context)
{
    return AxiomEvaluatorPtr<Kind>(
        new AxiomEvaluator<Kind>(m_next_index->fetch_add(1, std::memory_order_relaxed), std::move(task), std::move(execution_context), m_next_index));
}

template<TaskKind Kind>
SuccessorGeneratorPtr<Kind> SuccessorGeneratorFactory<Kind>::create(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context)
{
    return SuccessorGeneratorPtr<Kind>(
        new SuccessorGenerator<Kind>(m_next_index->fetch_add(1, std::memory_order_relaxed), std::move(task), std::move(execution_context), m_next_index));
}

template class StateRepositoryFactory<GroundTag>;
template class StateRepositoryFactory<LiftedTag>;
template class AxiomEvaluatorFactory<GroundTag>;
template class AxiomEvaluatorFactory<LiftedTag>;
template class SuccessorGeneratorFactory<GroundTag>;
template class SuccessorGeneratorFactory<LiftedTag>;

}
