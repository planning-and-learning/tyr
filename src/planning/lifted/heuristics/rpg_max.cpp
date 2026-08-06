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

#include "tyr/planning/lifted/heuristics/rpg_max.hpp"

#include "rpg.hpp"
#include "tyr/datalog/lifted/policies/annotation.hpp"
#include "tyr/datalog/policies/termination.hpp"

namespace tyr::planning
{

struct MaxRPGHeuristic<LiftedTag>::Impl :
    detail::LiftedRPGBase<Impl,
                          datalog::OrAnnotationPolicy<LiftedTag>,
                          datalog::AndAnnotationPolicy<LiftedTag, datalog::MaxAggregation>,
                          datalog::TerminationPolicy<LiftedTag, datalog::MaxAggregation>>
{
    using Base = detail::LiftedRPGBase<Impl,
                                       datalog::OrAnnotationPolicy<LiftedTag>,
                                       datalog::AndAnnotationPolicy<LiftedTag, datalog::MaxAggregation>,
                                       datalog::TerminationPolicy<LiftedTag, datalog::MaxAggregation>>;
    using Base::Base;

    Impl(const Impl& source, ygg::ExecutionContextPtr execution_context) :
        Base(source.m_definition,
             source.m_task,
             std::move(execution_context),
             datalog::OrAnnotationPolicy<LiftedTag> {},
             datalog::AndAnnotationPolicy<LiftedTag, datalog::MaxAggregation> {},
             source.m_source_goal)
    {
    }
};

MaxRPGHeuristic<LiftedTag>::MaxRPGHeuristic(TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) :
    m_impl(std::make_unique<Impl>(std::move(task),
                                  std::move(execution_context),
                                  datalog::OrAnnotationPolicy<LiftedTag> {},
                                  datalog::AndAnnotationPolicy<LiftedTag, datalog::MaxAggregation> {},
                                  cost_mode))
{
}

MaxRPGHeuristic<LiftedTag>::MaxRPGHeuristic(std::unique_ptr<Impl> impl) : m_impl(std::move(impl)) {}

MaxRPGHeuristic<LiftedTag>::~MaxRPGHeuristic() = default;
MaxRPGHeuristic<LiftedTag>::MaxRPGHeuristic(MaxRPGHeuristic&&) noexcept = default;
MaxRPGHeuristic<LiftedTag>& MaxRPGHeuristic<LiftedTag>::operator=(MaxRPGHeuristic&&) noexcept = default;

MaxRPGHeuristicPtr<LiftedTag> MaxRPGHeuristic<LiftedTag>::create(TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode)
{
    return std::make_shared<MaxRPGHeuristic<LiftedTag>>(std::move(task), std::move(execution_context), cost_mode);
}

void MaxRPGHeuristic<LiftedTag>::set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal) { m_impl->set_goal(goal); }
ygg::float_t MaxRPGHeuristic<LiftedTag>::evaluate(const ygg::Builder<State<LiftedTag>>& state) { return m_impl->evaluate(state); }
HeuristicPtr<LiftedTag> MaxRPGHeuristic<LiftedTag>::make_worker(ygg::ExecutionContextPtr execution_context) const
{
    return HeuristicPtr<LiftedTag>(new MaxRPGHeuristic(std::make_unique<Impl>(*m_impl, std::move(execution_context))));
}
void MaxRPGHeuristic<LiftedTag>::print_summary(size_t verbosity) const { m_impl->print_summary(verbosity); }

}
