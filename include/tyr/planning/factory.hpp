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

#ifndef TYR_PLANNING_FACTORY_HPP_
#define TYR_PLANNING_FACTORY_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/axiom_evaluator.hpp"
#include "tyr/planning/ground/state_repository.hpp"
#include "tyr/planning/ground/successor_generator.hpp"
#include "tyr/planning/lifted/axiom_evaluator.hpp"
#include "tyr/planning/lifted/state_repository.hpp"
#include "tyr/planning/lifted/successor_generator.hpp"

#include <atomic>
#include <memory>
#include <utility>

namespace tyr::planning
{

template<TaskKind Kind>
class StateRepositoryFactory
{
public:
    StateRepositoryFactory() : m_next_index(std::make_shared<std::atomic<ygg::uint_t>>(0)) {}

    StateRepositoryPtr<Kind> create(TaskPtr<Kind> task, AxiomEvaluatorPtr<Kind> axiom_evaluator)
    {
        return StateRepositoryPtr<Kind>(
            new StateRepository<Kind>(m_next_index->fetch_add(1, std::memory_order_relaxed), std::move(task), std::move(axiom_evaluator), m_next_index));
    }

private:
    std::shared_ptr<std::atomic<ygg::uint_t>> m_next_index;
};

template<TaskKind Kind>
class AxiomEvaluatorFactory
{
public:
    AxiomEvaluatorFactory() : m_next_index(std::make_shared<std::atomic<ygg::uint_t>>(0)) {}

    AxiomEvaluatorPtr<Kind> create(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context)
    {
        if (!task->has_axioms())
            return nullptr;

        return AxiomEvaluatorPtr<Kind>(
            new AxiomEvaluator<Kind>(m_next_index->fetch_add(1, std::memory_order_relaxed), std::move(task), std::move(execution_context), m_next_index));
    }

private:
    std::shared_ptr<std::atomic<ygg::uint_t>> m_next_index;
};

template<TaskKind Kind>
class SuccessorGeneratorFactory
{
public:
    SuccessorGeneratorFactory() : m_next_index(std::make_shared<std::atomic<ygg::uint_t>>(0)) {}

    SuccessorGeneratorPtr<Kind> create(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context, StateRepositoryPtr<Kind> state_repository)
    {
        return SuccessorGeneratorPtr<Kind>(new SuccessorGenerator<Kind>(m_next_index->fetch_add(1, std::memory_order_relaxed),
                                                                        std::move(task),
                                                                        std::move(execution_context),
                                                                        std::move(state_repository),
                                                                        m_next_index));
    }

private:
    std::shared_ptr<std::atomic<ygg::uint_t>> m_next_index;
};

}

#endif
