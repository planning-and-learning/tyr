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

#include "tyr/planning/heuristics/rpg_add.hpp"

#include "../ground/heuristics/rpg.hpp"
#include "../lifted/heuristics/rpg.hpp"
#include "rpg.hpp"
#include "tyr/datalog/ground/policies/annotation.hpp"
#include "tyr/datalog/lifted/policies/annotation.hpp"
#include "tyr/datalog/policies/cost.hpp"
#include "tyr/datalog/policies/termination.hpp"

#include <utility>

namespace tyr::planning
{

template<TaskKind Kind>
struct AddRPGHeuristic<Kind>::Impl :
    detail::RPGEvaluator<Impl, Kind, datalog::MinCostAnnotationPolicy<Kind, datalog::SumAggregation>, datalog::TerminationPolicy<Kind, datalog::SumAggregation>>
{
    using Base = detail::
        RPGEvaluator<Impl, Kind, datalog::MinCostAnnotationPolicy<Kind, datalog::SumAggregation>, datalog::TerminationPolicy<Kind, datalog::SumAggregation>>;

    using Base::Base;
};

template<TaskKind Kind>
AddRPGHeuristic<Kind>::AddRPGHeuristic(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) :
    m_impl(std::make_unique<Impl>(std::move(task), std::move(execution_context), cost_mode))
{
}

template<TaskKind Kind>
AddRPGHeuristic<Kind>::AddRPGHeuristic(std::unique_ptr<Impl> impl) : m_impl(std::move(impl))
{
}

template<TaskKind Kind>
AddRPGHeuristic<Kind>::~AddRPGHeuristic() = default;

template<TaskKind Kind>
AddRPGHeuristic<Kind>::AddRPGHeuristic(AddRPGHeuristic&&) noexcept = default;

template<TaskKind Kind>
AddRPGHeuristic<Kind>& AddRPGHeuristic<Kind>::operator=(AddRPGHeuristic&&) noexcept = default;

template<TaskKind Kind>
AddRPGHeuristicPtr<Kind> AddRPGHeuristic<Kind>::create(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode)
{
    return std::make_shared<AddRPGHeuristic<Kind>>(std::move(task), std::move(execution_context), cost_mode);
}

template<TaskKind Kind>
void AddRPGHeuristic<Kind>::set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal)
{
    m_impl->set_goal(goal);
}

template<TaskKind Kind>
ygg::float_t AddRPGHeuristic<Kind>::evaluate(const ygg::Builder<State<Kind>>& state)
{
    return m_impl->evaluate(state);
}

template<TaskKind Kind>
HeuristicPtr<Kind> AddRPGHeuristic<Kind>::make_worker(ygg::ExecutionContextPtr execution_context) const
{
    return HeuristicPtr<Kind>(new AddRPGHeuristic(std::make_unique<Impl>(*m_impl, std::move(execution_context))));
}

template<TaskKind Kind>
void AddRPGHeuristic<Kind>::print_summary(size_t verbosity) const
{
    m_impl->print_summary(verbosity);
}

template class AddRPGHeuristic<GroundTag>;
template class AddRPGHeuristic<LiftedTag>;

}
