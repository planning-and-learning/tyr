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

#ifndef TYR_PYTHON_PLANNING_HEURISTICS_HEURISTICS_HPP_
#define TYR_PYTHON_PLANNING_HEURISTICS_HEURISTICS_HPP_

#include "pytyr/planning/module.hpp"

#include <memory>
#include <nanobind/nanobind.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/trampoline.h>
#include <optional>
#include <string>
#include <tyr/planning/declarations.hpp>
#include <tyr/planning/heuristics/blind.hpp>
#include <tyr/planning/heuristics/goal_count.hpp>
#include <tyr/planning/heuristics/heuristic.hpp>
#include <tyr/planning/heuristics/lmcut.hpp>
#include <tyr/planning/heuristics/rpg_add.hpp>
#include <tyr/planning/heuristics/rpg_ff.hpp>
#include <tyr/planning/heuristics/rpg_max.hpp>
#include <tyr/planning/states/state_view.hpp>
#include <urd/python/bindings.hpp>
#include <urd/python/values/dense_set.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <utility>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::planning
{

template<TaskKind Kind>
class PyHeuristic : public Heuristic<Kind>
{
public:
    using Base = Heuristic<Kind>;
    using Base::evaluate;

    void set_goal(formalism::ConjunctiveConditionView<GroundTag> goal) override { NB_OVERRIDE_PURE(set_goal, goal); }

    ygg::float_t evaluate(const StateBuilderView<Kind>& state) override { NB_OVERRIDE_PURE(evaluate, state); }

    HeuristicPtr<Kind> make_worker(ygg::ExecutionContextPtr execution_context) const override { NB_OVERRIDE_PURE(make_worker, execution_context); }

    /// A Python override returns None or a set; the set is kept here so the returned pointer stays valid until the next call.
    const urd::Store<urd::set<formalism::ActionRowView>>* get_relaxed_plan() const override
    {
        constexpr uint64_t nb_hash = nanobind::detail::str_hash("get_relaxed_plan");
        nanobind::detail::ticket nb_ticket(nb_trampoline, "get_relaxed_plan", nb_hash, false);
        if (!nb_ticket.key.is_valid())
            return Base::get_relaxed_plan();
        m_relaxed_plan = nb::cast<std::optional<urd::Store<urd::set<formalism::ActionRowView>>>>(nb_trampoline.base().attr(nb_ticket.key)());
        return m_relaxed_plan ? &*m_relaxed_plan : nullptr;
    }

private:
    // The inherited constructors keep the access of the base constructors.
    NB_TRAMPOLINE(Base);

    mutable std::optional<urd::Store<urd::set<formalism::ActionRowView>>> m_relaxed_plan;
};

template<TaskKind Kind>
void bind_heuristic(nb::module_& m, const std::string& name)
{
    using namespace nb::literals;
    using T = Heuristic<Kind>;

    nb::class_<T, PyHeuristic<Kind>>(m, name.c_str())  //
        .def(nb::init<>())
        .def("set_goal", &T::set_goal, "goal"_a)
        .def(
            "evaluate",
            [](T& self, const StateBuilderView<Kind>& state) { return self.evaluate(state); },
            "state"_a,
            "A search calls it with views it owns, valid during the call: keep one by copying it (a registered state: state.pack(); a state: "
            "StateBuilder.assign).",
            nb::call_guard<nb::gil_scoped_release>())
        .def("get_relaxed_plan",
             [](const T& self) -> std::optional<urd::Store<urd::set<formalism::ActionRowView>>>
             {
                 const auto* plan = self.get_relaxed_plan();
                 return plan ? std::optional(*plan) : std::nullopt;
             });
}

template<TaskKind Kind>
void bind_blind_heuristic(nb::module_& m, const std::string& name)
{
    using T = BlindHeuristic<Kind>;

    nb::class_<T, Heuristic<Kind>>(m, name.c_str())  //
        .def(nb::new_([]() { return std::make_shared<T>(); }));
}

template<TaskKind Kind>
void bind_goal_count_heuristic(nb::module_& m, const std::string& name)
{
    using T = GoalCountHeuristic<Kind>;

    nb::class_<T, Heuristic<Kind>>(m, name.c_str())  //
        .def(nb::new_([](std::shared_ptr<const Task<Kind>> task) { return std::make_shared<T>(std::move(task)); }));
}

/// The RPG and LM-cut heuristics share one constructor signature.
template<TaskKind Kind, typename T>
void bind_task_heuristic(nb::module_& m, const std::string& name)
{
    using namespace nb::literals;

    nb::class_<T, Heuristic<Kind>>(m, name.c_str())  //
        .def(nb::new_([](TaskPtr<Kind> task, std::shared_ptr<ygg::ExecutionContext> execution_context, CostMode cost_mode)
                      { return std::make_shared<T>(std::move(task), std::move(execution_context), cost_mode); }),
             "task"_a,
             "execution_context"_a,
             "cost_mode"_a = CostMode::UNIT);
}

}  // namespace tyr::planning

#endif
