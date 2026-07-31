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

#include "tyr/planning/heuristics/goal_count.hpp"

#include "tyr/planning/applicability.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/task.hpp"

namespace tyr::planning
{

template<TaskKind Kind>
GoalCountHeuristic<Kind>::GoalCountHeuristic(std::shared_ptr<const Task<Kind>> task) : m_task(std::move(task)), m_goal(m_task->get_task().get_goal())
{
}

template<TaskKind Kind>
std::shared_ptr<GoalCountHeuristic<Kind>> GoalCountHeuristic<Kind>::create(std::shared_ptr<const Task<Kind>> task)
{
    return std::make_shared<GoalCountHeuristic<Kind>>(std::move(task));
}

template<TaskKind Kind>
void GoalCountHeuristic<Kind>::set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal)
{
    m_goal = goal;
}

template<TaskKind Kind>
ygg::float_t GoalCountHeuristic<Kind>::evaluate(const StateView<Kind>& state)
{
    auto unsat_counter = ygg::float_t { 0 };

    auto state_context = StateContext<Kind> { *m_task, state.get_state_builder(), ygg::float_t { 0 } };

    for (const auto fact : m_goal.template get_facts<::tyr::formalism::PositiveTag>())
    {
        if (!is_applicable<::tyr::formalism::PositiveTag>(fact, state_context))
            ++unsat_counter;
    }

    for (const auto fact : m_goal.template get_facts<::tyr::formalism::NegativeTag>())
    {
        if (!is_applicable<::tyr::formalism::NegativeTag>(fact, state_context))
            ++unsat_counter;
    }

    for (const auto fact : m_goal.template get_literals<::tyr::formalism::DerivedTag>())
    {
        if (!is_applicable(fact, state_context))
            ++unsat_counter;
    }

    for (const auto numeric_constraint : m_goal.get_numeric_constraints())
    {
        if (!is_applicable(numeric_constraint, state_context))
            ++unsat_counter;
    }

    return unsat_counter;
}

template class GoalCountHeuristic<LiftedTag>;
template class GoalCountHeuristic<GroundTag>;

}
