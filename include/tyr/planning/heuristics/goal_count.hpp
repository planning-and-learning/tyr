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

#ifndef TYR_PLANNING_HEURISTICS_GOAL_COUNT_HPP_
#define TYR_PLANNING_HEURISTICS_GOAL_COUNT_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/heuristics/heuristic.hpp"

#include <memory>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/execution/declarations.hpp>

namespace tyr::planning
{

template<TaskKind Kind, typename V>
class GoalCountHeuristic : public Heuristic<Kind, V>
{
public:
    explicit GoalCountHeuristic(std::shared_ptr<const Task<Kind>> task);

    static std::shared_ptr<GoalCountHeuristic<Kind, V>> create(std::shared_ptr<const Task<Kind>> task);

    using Heuristic<Kind, V>::evaluate;

    void set_goal(formalism::ConjunctiveConditionView<GroundTag> goal) override;

    ygg::float_t evaluate(const V& state) override;

    [[nodiscard]] HeuristicPtr<Kind, V> make_worker(ygg::ExecutionContextPtr execution_context) const override;

private:
    std::shared_ptr<const Task<Kind>> m_task;

    formalism::ConjunctiveConditionView<GroundTag> m_goal;
};

}

#endif
