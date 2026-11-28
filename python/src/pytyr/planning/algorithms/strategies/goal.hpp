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

#ifndef TYR_PYTHON_PLANNING_ALGORITHMS_STRATEGIES_GOAL_HPP_
#define TYR_PYTHON_PLANNING_ALGORITHMS_STRATEGIES_GOAL_HPP_

#include "pytyr/planning/module.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/trampoline.h>
#include <string>
#include <tyr/planning/algorithms/strategies/goal.hpp>
#include <tyr/planning/declarations.hpp>
#include <tyr/planning/states/state_view.hpp>
#include <tyr/planning/tasks/task.hpp>
#include <urd/python/bindings.hpp>

namespace tyr::planning
{

template<TaskKind Kind>
class PyGoalStrategy : public GoalStrategy<Kind>
{
public:
    using Base = GoalStrategy<Kind>;
    using Base::is_dynamic_goal_satisfied;

    NB_TRAMPOLINE(Base);

    [[nodiscard]] GoalStrategyPtr<Kind> make_worker(urd::index<Worker> index) const override { NB_OVERRIDE(make_worker, index); }

    bool is_static_goal_satisfied(const Task<Kind>& task) override { NB_OVERRIDE_PURE(is_static_goal_satisfied, task); }

    bool is_dynamic_goal_satisfied(const StateBuilderView<Kind>& seed_state, const StateBuilderView<Kind>& state) override
    { NB_OVERRIDE_PURE(is_dynamic_goal_satisfied, seed_state, state); }
};

template<TaskKind Kind>
void bind_goal_strategy(nb::module_& m, const std::string& name)
{
    using namespace nb::literals;
    using T = GoalStrategy<Kind>;

    nb::class_<T, PyGoalStrategy<Kind>>(m, name.c_str())  //
        .def(nb::init<>())
        .def("make_worker", &T::make_worker, "worker_index"_a)
        .def("is_static_goal_satisfied", &T::is_static_goal_satisfied, "task"_a)
        .def("is_dynamic_goal_satisfied",
             &T::is_dynamic_goal_satisfied,
             "seed_state"_a,
             "state"_a,
             "A search calls it with views it owns, valid during the call: keep one by copying it (a registered state: state.pack(); a state: "
             "StateBuilder.assign).");
}

template<TaskKind Kind>
void bind_conjunctive_goal_strategy(nb::module_& m, const std::string& name)
{
    using namespace nb::literals;
    using T = ConjunctiveGoalStrategy<Kind>;

    nb::class_<T, GoalStrategy<Kind>>(m, name.c_str())  //
        .def(nb::init<const Task<Kind>&>(), "task"_a)
        .def(nb::init<formalism::ConjunctiveConditionView<GroundTag>>(), "goal"_a)
        .def("set_goal", &T::set_goal, "goal"_a);
}

template<TaskKind Kind>
void bind_exhaustive_goal_strategy(nb::module_& m, const std::string& name)
{
    using T = ExhaustiveGoalStrategy<Kind>;

    nb::class_<T, GoalStrategy<Kind>>(m, name.c_str())  //
        .def(nb::init<>());
}

}  // namespace tyr::planning

#endif
