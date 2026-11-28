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

#include "tyr/planning/successors/applicability.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"
#include "tyr/planning/states/state_view.hpp"

namespace tyr::planning
{

template<TaskKind Kind, typename V>
GoalCountHeuristic<Kind, V>::GoalCountHeuristic(std::shared_ptr<const Task<Kind>> task) : m_task(std::move(task)), m_goal(m_task->get_task().goal())
{
}

template<TaskKind Kind, typename V>
std::shared_ptr<GoalCountHeuristic<Kind, V>> GoalCountHeuristic<Kind, V>::create(std::shared_ptr<const Task<Kind>> task)
{ return std::make_shared<GoalCountHeuristic<Kind, V>>(std::move(task)); }

template<TaskKind Kind, typename V>
void GoalCountHeuristic<Kind, V>::set_goal(formalism::ConjunctiveConditionView<GroundTag> goal)
{ m_goal = goal; }

template<TaskKind Kind, typename V>
ygg::float_t GoalCountHeuristic<Kind, V>::evaluate(const V& state)
{
    auto unsat_counter = ygg::float_t { 0 };

    auto state_context = StateContext<Kind> { *m_task, state, ygg::float_t { 0 } };

    for (const auto fact : m_goal.template facts<formalism::PositiveTag>())
    {
        if (!is_applicable<formalism::PositiveTag>(fact, state_context))
            ++unsat_counter;
    }

    for (const auto fact : m_goal.template facts<formalism::NegativeTag>())
    {
        if (!is_applicable<formalism::NegativeTag>(fact, state_context))
            ++unsat_counter;
    }

    for (const auto fact : m_goal.template literals<formalism::DerivedTag>())
    {
        if (!is_applicable(fact, state_context))
            ++unsat_counter;
    }

    for (const auto numeric_constraint : m_goal.numeric_constraints())
    {
        if (!is_applicable(numeric_constraint, state_context))
            ++unsat_counter;
    }

    return unsat_counter;
}

template<TaskKind Kind, typename V>
HeuristicPtr<Kind, V> GoalCountHeuristic<Kind, V>::make_worker([[maybe_unused]] ygg::ExecutionContextPtr execution_context) const
{
    auto worker = create(m_task);
    worker->m_goal = m_goal;
    return worker;
}

template class GoalCountHeuristic<LiftedTag>;
template class GoalCountHeuristic<GroundTag>;

}
