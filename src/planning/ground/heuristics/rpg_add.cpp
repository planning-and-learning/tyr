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

#include "tyr/planning/ground/heuristics/rpg_add.hpp"

#include "rpg.hpp"
#include "tyr/datalog/ground/policies/annotation.hpp"
#include "tyr/datalog/ground/policies/cost.hpp"
#include "tyr/datalog/policies/termination.hpp"

#include <utility>

namespace tyr::planning
{

struct AddRPGHeuristic<GroundTag>::Impl :
    detail::GroundRPGEvaluator<Impl,
                               datalog::OrAnnotationPolicy<GroundTag>,
                               datalog::AndAnnotationPolicy<GroundTag, datalog::SumAggregation>,
                               datalog::TerminationPolicy<GroundTag, datalog::SumAggregation>,
                               datalog::RuleCostOverridePolicy<GroundTag>>
{
    using Base = detail::GroundRPGEvaluator<Impl,
                                            datalog::OrAnnotationPolicy<GroundTag>,
                                            datalog::AndAnnotationPolicy<GroundTag, datalog::SumAggregation>,
                                            datalog::TerminationPolicy<GroundTag, datalog::SumAggregation>,
                                            datalog::RuleCostOverridePolicy<GroundTag>>;

    Impl(TaskPtr<GroundTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) :
        Base(std::move(task),
             std::move(execution_context),
             datalog::OrAnnotationPolicy<GroundTag>(),
             datalog::AndAnnotationPolicy<GroundTag, datalog::SumAggregation>(),
             cost_mode)
    {
    }

    Impl(const Impl& source, ygg::ExecutionContextPtr execution_context) :
        Base(source.m_definition,
             std::move(execution_context),
             datalog::OrAnnotationPolicy<GroundTag>(),
             datalog::AndAnnotationPolicy<GroundTag, datalog::SumAggregation>())
    {
    }
};

AddRPGHeuristic<GroundTag>::AddRPGHeuristic(TaskPtr<GroundTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) :
    m_impl(std::make_unique<Impl>(std::move(task), std::move(execution_context), cost_mode))
{
}

AddRPGHeuristic<GroundTag>::AddRPGHeuristic(std::unique_ptr<Impl> impl) : m_impl(std::move(impl)) {}

AddRPGHeuristic<GroundTag>::~AddRPGHeuristic() = default;
AddRPGHeuristic<GroundTag>::AddRPGHeuristic(AddRPGHeuristic&&) noexcept = default;
AddRPGHeuristic<GroundTag>& AddRPGHeuristic<GroundTag>::operator=(AddRPGHeuristic&&) noexcept = default;

AddRPGHeuristicPtr<GroundTag> AddRPGHeuristic<GroundTag>::create(TaskPtr<GroundTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode)
{
    return std::make_shared<AddRPGHeuristic<GroundTag>>(std::move(task), std::move(execution_context), cost_mode);
}

void AddRPGHeuristic<GroundTag>::set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal) { m_impl->set_goal(goal); }

ygg::float_t AddRPGHeuristic<GroundTag>::evaluate(const StateView<GroundTag>& state) { return m_impl->evaluate(state); }

HeuristicPtr<GroundTag> AddRPGHeuristic<GroundTag>::make_worker(ygg::ExecutionContextPtr execution_context) const
{
    return HeuristicPtr<GroundTag>(new AddRPGHeuristic(std::make_unique<Impl>(*m_impl, std::move(execution_context))));
}

}
