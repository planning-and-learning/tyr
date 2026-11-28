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

// The bindings of the lifted Datalog programs, apart from the rest of the lifted module (module.cpp), so that no one
// source compiles both (brokkr's template rule T5).

#include "pytyr/planning/programs/lifted/programs.hpp"

#include "../../lifted/module.hpp"
#include "pytyr/module.hpp"

#include <nanobind/stl/pair.h>
#include <nanobind/stl/vector.h>
#include <tyr/formalism/datalog/rule_view.hpp>
#include <tyr/formalism/planning/action_view.hpp>
#include <tyr/formalism/planning/axiom_view.hpp>
#include <tyr/formalism/planning/conditional_effect_view.hpp>
#include <tyr/formalism/planning/planning_task.hpp>
#include <tyr/formalism/predicate_view.hpp>
#include <tyr/formalism/repository.hpp>
#include <tyr/planning/programs/axiom.hpp>
#include <tyr/planning/programs/lifted/action.hpp>
#include <tyr/planning/programs/lifted/ground.hpp>
#include <tyr/planning/programs/rpg.hpp>
#include <urd/python/sequences/sequence.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>

namespace nb = nanobind;

namespace tyr::planning
{

void bind_lifted_programs(nb::module_& m)
{
    using namespace nb::literals;
    using ApplicableProgram = ApplicableActionProgram;
    nb::class_<ApplicableProgram>(m, "ApplicableActionProgram")
        .def(nb::init<const formalism::PlanningTask<LiftedTag>&>(), "task"_a, nb::keep_alive<1, 2>())
        .def("get_datalog_program", nb::overload_cast<>(&ApplicableProgram::get_datalog_program), nb::rv_policy::reference_internal)
        .def("get_const_program_workspace", &ApplicableProgram::get_const_program_workspace, nb::rv_policy::reference_internal)
        .def(
            "get_conditional_effect_to_predicate_mapping",
            [](const ApplicableProgram& self)
            {
                using Key = std::pair<const formalism::ActionView<LiftedTag>*, const formalism::ConditionalEffectView<LiftedTag>*>;
                using Entry = std::pair<Key, const formalism::PredicateView<formalism::DerivedTag>*>;
                auto result = urd::Store<urd::sequence<Entry>> {};
                for (const auto& [key, predicate] : self.get_conditional_effect_to_predicate_mapping())
                    result.emplace_back(Key { &key.first, &key.second }, &predicate);
                return result;
            },
            nb::rv_policy::reference_internal)
        .def(
            "get_predicate_to_action_mapping",
            [](const ApplicableProgram& self)
            {
                using Entry = std::pair<const formalism::PredicateView<formalism::DerivedTag>*, const formalism::ActionView<LiftedTag>*>;
                auto result = urd::Store<urd::sequence<Entry>> {};
                for (const auto& [predicate, action] : self.get_predicate_to_action_mapping())
                    result.emplace_back(&predicate, &action);
                return result;
            },
            nb::rv_policy::reference_internal);

    using AxiomProgram = AxiomEvaluatorProgram<LiftedTag>;
    nb::class_<AxiomProgram>(m, "AxiomEvaluatorProgram")
        .def(nb::init<const formalism::PlanningTask<LiftedTag>&>(), "task"_a, nb::keep_alive<1, 2>())
        .def("get_datalog_program", nb::overload_cast<>(&AxiomProgram::get_datalog_program), nb::rv_policy::reference_internal)
        .def("get_const_program_workspace", &AxiomProgram::get_const_program_workspace, nb::rv_policy::reference_internal);

    using RelaxedProgram = RPGProgram<LiftedTag>;
    nb::class_<RelaxedProgram>(m, "RPGProgram")
        .def(nb::init<const formalism::PlanningTask<LiftedTag>&, CostMode>(), "task"_a, "cost_mode"_a = CostMode::UNIT, nb::keep_alive<1, 2>())
        .def("get_datalog_program", nb::overload_cast<>(&RelaxedProgram::get_datalog_program), nb::rv_policy::reference_internal)
        .def("get_goal", &RelaxedProgram::get_goal)
        .def("get_rule_to_action_mapping",
             [](const RelaxedProgram& self)
             {
                 const auto& repository = self.get_datalog_program().get_program_repository();
                 using Entry = std::pair<formalism::RuleView<LiftedTag>, formalism::ActionView<LiftedTag>>;
                 auto result = urd::Store<urd::sequence<Entry>> {};
                 for (const auto& [rule, actions] : self.get_rule_to_action_mapping())
                     for (const auto action : actions)
                         result.emplace_back(urd::resolve(rule, repository), action);
                 return result;
             });

    nb::class_<GroundTaskProgram>(m, "GroundTaskProgram")
        .def(nb::init<const formalism::PlanningTask<LiftedTag>&>(), "task"_a, nb::keep_alive<1, 2>())
        .def("get_datalog_program", nb::overload_cast<>(&GroundTaskProgram::get_datalog_program), nb::rv_policy::reference_internal)
        .def("get_const_program_workspace", &GroundTaskProgram::get_const_program_workspace, nb::rv_policy::reference_internal)
        .def(
            "get_conditional_effect_to_predicate_mapping",
            [](const GroundTaskProgram& self)
            {
                using Key = std::pair<const formalism::ActionView<LiftedTag>*, const formalism::ConditionalEffectView<LiftedTag>*>;
                using Entry = std::pair<Key, const formalism::PredicateView<formalism::DerivedTag>*>;
                auto result = urd::Store<urd::sequence<Entry>> {};
                for (const auto& [key, predicate] : self.get_conditional_effect_to_predicate_mapping())
                    result.emplace_back(Key { &key.first, &key.second }, &predicate);
                return result;
            },
            nb::rv_policy::reference_internal)
        .def(
            "get_predicate_to_action_mapping",
            [](const GroundTaskProgram& self)
            {
                using Entry = std::pair<const formalism::PredicateView<formalism::DerivedTag>*, const formalism::ActionView<LiftedTag>*>;
                auto result = urd::Store<urd::sequence<Entry>> {};
                for (const auto& [predicate, action] : self.get_predicate_to_action_mapping())
                    result.emplace_back(&predicate, &action);
                return result;
            },
            nb::rv_policy::reference_internal)
        .def(
            "get_predicate_to_axiom_mapping",
            [](const GroundTaskProgram& self)
            {
                using Axioms = urd::Store<urd::sequence<const formalism::AxiomView<LiftedTag>*>>;
                using Entry = std::pair<const formalism::PredicateView<formalism::DerivedTag>*, Axioms>;
                auto result = urd::Store<urd::sequence<Entry>> {};
                for (const auto& [predicate, axioms] : self.get_predicate_to_axiom_mapping())
                {
                    auto views = Axioms {};
                    for (const auto& axiom : axioms)
                        views.push_back(&axiom);
                    result.emplace_back(&predicate, std::move(views));
                }
                return result;
            },
            nb::rv_policy::reference_internal);
}

}  // namespace tyr::planning
