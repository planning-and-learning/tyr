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

#include "tyr/planning/lifted/heuristics/rpg_add.hpp"

#include "rpg.hpp"
#include "tyr/datalog/lifted/policies/annotation.hpp"
#include "tyr/datalog/policies/termination.hpp"

namespace tyr::planning
{

struct AddRPGHeuristic<LiftedTag>::Impl :
    detail::LiftedRPGBase<Impl,
                          datalog::OrAnnotationPolicy<LiftedTag>,
                          datalog::AndAnnotationPolicy<LiftedTag, datalog::SumAggregation>,
                          datalog::TerminationPolicy<LiftedTag, datalog::SumAggregation>>
{
    using Base = detail::LiftedRPGBase<Impl,
                                       datalog::OrAnnotationPolicy<LiftedTag>,
                                       datalog::AndAnnotationPolicy<LiftedTag, datalog::SumAggregation>,
                                       datalog::TerminationPolicy<LiftedTag, datalog::SumAggregation>>;
    using Base::Base;
};

AddRPGHeuristic<LiftedTag>::AddRPGHeuristic(TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) :
    m_impl(std::make_unique<Impl>(std::move(task),
                                  std::move(execution_context),
                                  datalog::OrAnnotationPolicy<LiftedTag> {},
                                  datalog::AndAnnotationPolicy<LiftedTag, datalog::SumAggregation> {},
                                  cost_mode))
{
}

AddRPGHeuristic<LiftedTag>::~AddRPGHeuristic() = default;
AddRPGHeuristic<LiftedTag>::AddRPGHeuristic(AddRPGHeuristic&&) noexcept = default;
AddRPGHeuristic<LiftedTag>& AddRPGHeuristic<LiftedTag>::operator=(AddRPGHeuristic&&) noexcept = default;

AddRPGHeuristicPtr<LiftedTag> AddRPGHeuristic<LiftedTag>::create(TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode)
{
    return std::make_shared<AddRPGHeuristic<LiftedTag>>(std::move(task), std::move(execution_context), cost_mode);
}

void AddRPGHeuristic<LiftedTag>::set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal) { m_impl->set_goal(goal); }
ygg::float_t AddRPGHeuristic<LiftedTag>::evaluate(const StateView<LiftedTag>& state) { return m_impl->evaluate(state); }
void AddRPGHeuristic<LiftedTag>::print_summary(size_t verbosity) const { m_impl->print_summary(verbosity); }

}
