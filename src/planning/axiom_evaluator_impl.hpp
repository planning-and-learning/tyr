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

#ifndef TYR_SRC_PLANNING_AXIOM_EVALUATOR_IMPL_HPP_
#define TYR_SRC_PLANNING_AXIOM_EVALUATOR_IMPL_HPP_

#include "tyr/planning/axiom_evaluator.hpp"

#include <atomic>
#include <memory>
#include <utility>

/// The parts of AxiomEvaluator<Kind> that do not depend on the kind. Each kind's source file defines its
/// AxiomDefinition (shared by all workers), its AxiomWorkspace (one per worker) and compute_extended_state.
namespace tyr::planning
{
namespace detail
{
template<TaskKind Kind>
struct AxiomDefinition;

template<TaskKind Kind>
struct AxiomWorkspace;
}

template<TaskKind Kind>
struct AxiomEvaluator<Kind>::Impl
{
    Impl(ygg::uint_t index_,
         std::shared_ptr<const detail::AxiomDefinition<Kind>> definition_,
         ygg::ExecutionContextPtr execution_context,
         std::shared_ptr<std::atomic<ygg::uint_t>> next_index_) :
        index(index_),
        next_index(std::move(next_index_)),
        definition(std::move(definition_)),
        evaluator(*definition, std::move(execution_context))
    {
    }

    ygg::uint_t index;
    std::shared_ptr<std::atomic<ygg::uint_t>> next_index;
    std::shared_ptr<const detail::AxiomDefinition<Kind>> definition;
    detail::AxiomWorkspace<Kind> evaluator;
};

template<TaskKind Kind>
AxiomEvaluator<Kind>::~AxiomEvaluator() = default;

template<TaskKind Kind>
AxiomEvaluator<Kind>::AxiomEvaluator(ygg::uint_t index,
                                     TaskPtr<Kind> task,
                                     ygg::ExecutionContextPtr execution_context,
                                     std::shared_ptr<std::atomic<ygg::uint_t>> next_index) :
    m_impl(std::make_unique<Impl>(index, std::make_shared<const detail::AxiomDefinition<Kind>>(std::move(task)), std::move(execution_context), std::move(next_index)))
{
}

template<TaskKind Kind>
AxiomEvaluator<Kind>::AxiomEvaluator(std::unique_ptr<Impl> impl) : m_impl(std::move(impl))
{
}

template<TaskKind Kind>
AxiomEvaluatorPtr<Kind> AxiomEvaluator<Kind>::make_worker(ygg::ExecutionContextPtr execution_context) const
{
    return AxiomEvaluatorPtr<Kind>(new AxiomEvaluator<Kind>(std::make_unique<Impl>(m_impl->next_index->fetch_add(1, std::memory_order_relaxed),
                                                                                   m_impl->definition,
                                                                                   std::move(execution_context),
                                                                                   m_impl->next_index)));
}

template<TaskKind Kind>
const TaskPtr<Kind>& AxiomEvaluator<Kind>::get_task() const noexcept
{ return m_impl->definition->task; }

template<TaskKind Kind>
const ygg::ExecutionContextPtr& AxiomEvaluator<Kind>::get_execution_context() const noexcept
{ return m_impl->evaluator.execution_context; }

template<TaskKind Kind>
ygg::uint_t AxiomEvaluator<Kind>::get_index() const noexcept
{ return m_impl->index; }

}

#endif
