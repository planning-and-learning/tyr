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

#include "module.hpp"

#include "../planning.hpp"
#include "astar_eager/module.hpp"
#include "brfs/module.hpp"
#include "gbfs_lazy/module.hpp"
#include "iw/module.hpp"
#include "siw/module.hpp"

#include <nanobind/stl/pair.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/vector.h>

namespace tyr::planning
{

void bind_lifted_module_definitions(nb::module_& m)
{
    auto astar_eager_module = m.def_submodule("astar_eager");
    astar_eager::bind_lifted_module_definitions(astar_eager_module);

    auto brfs_module = m.def_submodule("brfs");
    brfs::bind_lifted_module_definitions(brfs_module);

    auto gbfs_lazy_module = m.def_submodule("gbfs_lazy");
    gbfs_lazy::bind_lifted_module_definitions(gbfs_lazy_module);

    auto iw_module = m.def_submodule("iw");
    iw::bind_lifted_module_definitions(iw_module);

    auto siw_module = m.def_submodule("siw");
    siw::bind_lifted_module_definitions(siw_module);

    nb::enum_<GroundTaskInstantiationStatus>(m, "GroundTaskInstantiationStatus")
        .value("SUCCESS", GroundTaskInstantiationStatus::SUCCESS)
        .value("PROVEN_UNSOLVABLE", GroundTaskInstantiationStatus::PROVEN_UNSOLVABLE);

    nb::class_<GroundTaskInstantiationResult>(m, "GroundTaskInstantiationResult")
        .def(nb::init<>())
        .def(nb::init<TaskPtr<GroundTag>, GroundTaskInstantiationStatus>(), "task"_a, "status"_a)
        .def_rw("task", &GroundTaskInstantiationResult::task)
        .def_rw("status", &GroundTaskInstantiationResult::status);

    nb::class_<GroundTaskInstantiationOptions>(m, "GroundTaskInstantiationOptions")
        .def(nb::init<>())
        .def(nb::init<bool>(), "disable_invariant_synthesis"_a = true)
        .def_rw("disable_invariant_synthesis", &GroundTaskInstantiationOptions::disable_invariant_synthesis);

    nb::class_<Task<LiftedTag>>(m, "Task")  //
        .def(nb::new_([](formalism::planning::PlanningTask&& task) { return std::make_shared<Task<LiftedTag>>(std::move(task)); }),
             "formalism_task"_a,
             R"doc(
Create a planning task from a formalism task.

Parameters
----------
formalism_task : formalism.planning.Task
    The formalism-level task used to construct the planning task.

Notes
-----
The `formalism_task` is **moved** into the planning task. After calling this
constructor, the original formalism task should be considered consumed and
should not be used further.
        )doc")
        .def_static(
            "create",
            [](formalism::planning::PlanningTask&& task) { return Task<LiftedTag>::create(std::move(task)); },
            "formalism_task"_a)
        .def("get_formalism_task", &Task<LiftedTag>::get_formalism_task, nb::rv_policy::reference_internal)
        .def("get_repository", &Task<LiftedTag>::get_repository)
        .def("get_task", &Task<LiftedTag>::get_task, nb::keep_alive<0, 1>())
        .def("get_fdr_context", nb::overload_cast<>(&Task<LiftedTag>::get_fdr_context, nb::const_), nb::rv_policy::reference_internal)
        .def("instantiate_ground_task", &Task<LiftedTag>::instantiate_ground_task, "execution_context"_a, "options"_a = GroundTaskInstantiationOptions());

    using ApplicableProgram = ApplicableActionProgram<LiftedTag>;
    nb::class_<ApplicableProgram>(m, "ApplicableActionProgram")
        .def(nb::init<formalism::planning::TaskView>(), "task"_a, nb::keep_alive<1, 2>())
        .def("get_datalog_program", nb::overload_cast<>(&ApplicableProgram::get_datalog_program), nb::rv_policy::reference_internal)
        .def("get_const_program_workspace", &ApplicableProgram::get_const_program_workspace, nb::rv_policy::reference_internal)
        .def(
            "get_predicate_to_action_mapping",
            [](const ApplicableProgram& self)
            {
                using Entry = std::pair<const formalism::datalog::PredicateView<formalism::FluentTag>*, const formalism::planning::ActionView<LiftedTag>*>;
                auto result = std::vector<Entry> {};
                for (const auto& [predicate, action] : self.get_predicate_to_action_mapping())
                    result.emplace_back(&predicate, &action);
                return result;
            },
            nb::rv_policy::reference_internal);

    using AxiomProgram = AxiomEvaluatorProgram<LiftedTag>;
    nb::class_<AxiomProgram>(m, "AxiomEvaluatorProgram")
        .def(nb::init<formalism::planning::TaskView>(), "task"_a, nb::keep_alive<1, 2>())
        .def("get_datalog_program", nb::overload_cast<>(&AxiomProgram::get_datalog_program), nb::rv_policy::reference_internal)
        .def("get_const_program_workspace", &AxiomProgram::get_const_program_workspace, nb::rv_policy::reference_internal);

    using RelaxedProgram = RPGProgram<LiftedTag>;
    nb::class_<RelaxedProgram>(m, "RPGProgram")
        .def(nb::init<formalism::planning::TaskView, CostMode>(), "task"_a, "cost_mode"_a = CostMode::GENERAL, nb::keep_alive<1, 2>())
        .def("get_datalog_program", nb::overload_cast<>(&RelaxedProgram::get_datalog_program), nb::rv_policy::reference_internal)
        .def("get_goal", &RelaxedProgram::get_goal, nb::keep_alive<0, 1>())
        .def(
            "get_rule_to_action_mapping",
            [](const RelaxedProgram& self)
            {
                using Entry =
                    std::pair<const formalism::datalog::RuleView<LiftedTag, formalism::PredicateTag>*, const formalism::planning::ActionView<LiftedTag>*>;
                auto result = std::vector<Entry> {};
                for (const auto& [rule, action] : self.get_rule_to_action_mapping<formalism::PredicateTag>())
                    result.emplace_back(&rule, &action);
                return result;
            },
            nb::rv_policy::reference_internal);

    nb::class_<GroundTaskProgram>(m, "GroundTaskProgram")
        .def(nb::init<formalism::planning::TaskView>(), "task"_a, nb::keep_alive<1, 2>())
        .def("get_datalog_program", nb::overload_cast<>(&GroundTaskProgram::get_datalog_program), nb::rv_policy::reference_internal)
        .def("get_const_program_workspace", &GroundTaskProgram::get_const_program_workspace, nb::rv_policy::reference_internal)
        .def(
            "get_predicate_to_action_mapping",
            [](const GroundTaskProgram& self)
            {
                using Entry = std::pair<const formalism::datalog::PredicateView<formalism::FluentTag>*, const formalism::planning::ActionView<LiftedTag>*>;
                auto result = std::vector<Entry> {};
                for (const auto& [predicate, action] : self.get_predicate_to_action_mapping())
                    result.emplace_back(&predicate, &action);
                return result;
            },
            nb::rv_policy::reference_internal)
        .def(
            "get_predicate_to_axiom_mapping",
            [](const GroundTaskProgram& self)
            {
                using Axioms = std::vector<const formalism::planning::AxiomView<LiftedTag>*>;
                using Entry = std::pair<const formalism::datalog::PredicateView<formalism::FluentTag>*, Axioms>;
                auto result = std::vector<Entry> {};
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

    bind_index<ygg::Index<State<LiftedTag>>>(m, "StateIndex");
    bind_state<LiftedTag>(m, "State");
    bind_state_builder<LiftedTag>(m, "StateBuilder");
    bind_node<LiftedTag>(m, "Node");
    bind_labeled_node<LiftedTag>(m, "LabeledNode");
    bind_plan<LiftedTag>(m, "Plan");
    bind_axiom_evaluator<LiftedTag>(m, "AxiomEvaluator");
    bind_axiom_evaluator_factory<LiftedTag>(m, "AxiomEvaluatorFactory");
    bind_state_repository<LiftedTag>(m, "StateRepository");
    bind_state_repository_factory<LiftedTag>(m, "StateRepositoryFactory");
    bind_successor_generator<LiftedTag>(m, "SuccessorGenerator");
    bind_successor_generator_factory<LiftedTag>(m, "SuccessorGeneratorFactory");
    bind_search_result<LiftedTag>(m, "SearchResult");
    bind_goal_strategy<LiftedTag>(m, "GoalStrategy");
    bind_conjunctive_goal_strategy<LiftedTag>(m, "ConjunctiveGoalStrategy");
    bind_exhaustive_goal_strategy<LiftedTag>(m, "ExhaustiveGoalStrategy");
    bind_pruning_strategy<LiftedTag>(m, "PruningStrategy");
    bind_heuristic<LiftedTag>(m, "Heuristic");
    bind_blind_heuristic<LiftedTag>(m, "BlindHeuristic");
    bind_rpg_max_heuristic<LiftedTag>(m, "MaxRPGHeuristic");
    bind_rpg_add_heuristic<LiftedTag>(m, "AddRPGHeuristic");
    bind_rpg_ff_heuristic<LiftedTag>(m, "FFRPGHeuristic");
    bind_lmcut_heuristic<LiftedTag>(m, "LMCutHeuristic");
    bind_goal_count_heuristic<LiftedTag>(m, "GoalCountHeuristic");
}

}  // namespace tyr::planning
