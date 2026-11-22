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

#ifndef TYR_PYTHON_PLANNING_PLANNING_HPP_
#define TYR_PYTHON_PLANNING_PLANNING_HPP_

#include "module.hpp"
#include "pytyr/module.hpp"

#include <nanobind/stl/chrono.h>
#include <nanobind/stl/function.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <ranges>
#include <type_traits>
#include <tyr/planning/algorithms/astar_eager.hpp>
#include <tyr/planning/algorithms/astar_eager/event_handler.hpp>
#include <tyr/planning/algorithms/brfs.hpp>
#include <tyr/planning/algorithms/brfs/event_handler.hpp>
#include <tyr/planning/algorithms/gbfs_lazy.hpp>
#include <tyr/planning/algorithms/gbfs_lazy/event_handler.hpp>
#include <tyr/planning/algorithms/iw.hpp>
#include <tyr/planning/algorithms/iw/novelty_table.hpp>
#include <tyr/planning/algorithms/iw/pruning_strategy.hpp>
#include <tyr/planning/algorithms/serialized.hpp>
#include <tyr/planning/algorithms/serialized/event_handler.hpp>
#include <tyr/planning/algorithms/serialized/statistics.hpp>
#include <tyr/planning/algorithms/siw.hpp>
#include <tyr/planning/algorithms/siw/event_handler.hpp>
#include <tyr/planning/algorithms/siw/statistics.hpp>
#include <tyr/planning/algorithms/statistics.hpp>
#include <tyr/planning/algorithms/strategies/goal.hpp>
#include <tyr/planning/algorithms/strategies/pruning.hpp>
#include <tyr/planning/axiom_evaluator.hpp>
#include <tyr/planning/declarations.hpp>
#include <tyr/planning/formatter.hpp>
#include <tyr/planning/state_storage/policy.hpp>
#include <tyr/planning/ground/successor_generator.hpp>
#include <tyr/planning/ground/task.hpp>
#include <tyr/planning/heuristics/blind.hpp>
#include <tyr/planning/heuristics/goal_count.hpp>
#include <tyr/planning/heuristics/lmcut.hpp>
#include <tyr/planning/heuristics/rpg_add.hpp>
#include <tyr/planning/heuristics/rpg_ff.hpp>
#include <tyr/planning/heuristics/rpg_max.hpp>
#include <tyr/planning/lifted/programs/action.hpp>
#include <tyr/planning/lifted/programs/axiom.hpp>
#include <tyr/planning/lifted/programs/ground.hpp>
#include <tyr/planning/lifted/successor_generator.hpp>
#include <tyr/planning/lifted/task.hpp>
#include <tyr/planning/node.hpp>
#include <tyr/planning/plan.hpp>
#include <tyr/planning/programs/rpg.hpp>
#include <tyr/planning/search_space/search_node.hpp>
#include <tyr/planning/state_builder.hpp>
#include <tyr/planning/state_data.hpp>
#include <tyr/planning/state_repository.hpp>
#include <tyr/planning/state_routing/dist_hash.hpp>
#include <tyr/planning/task.hpp>
#include <tyr/planning/worker_state_index.hpp>
#include <urd/python/bindings.hpp>
#include <urd/python/sequences/sequence.hpp>
#include <urd/python/values/dense_set.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <yggdrasil/execution/onetbb.hpp>
#include <yggdrasil/python/bindings.hpp>

namespace tyr::planning
{
template<TaskKind Kind>
void bind_node(nb::module_& m, const std::string& name)
{
    using namespace nb::literals;
    using T = Node<Kind>;

    auto cls = nb::class_<T>(m, name.c_str())
                   .def(nb::init<StateView<Kind>, ygg::float_t>(), "state"_a, "metric_value"_a, nb::keep_alive<1, 2>())
                   .def("get_state", &T::get_state, nb::rv_policy::reference_internal)
                   .def("get_metric", &T::get_metric, nb::rv_policy::copy)
                   .def("pack", &T::pack);
    ygg::add_print(cls);
    urd::python::add_comparison(cls);
    urd::python::add_hash(cls);
}

template<TaskKind Kind>
void bind_packed_node(nb::module_& m, const std::string& name)
{
    using namespace nb::literals;
    using T = PackedNode<Kind>;

    auto cls = nb::class_<T>(m, name.c_str())
                   .def(nb::init<PackedStateView<Kind>, ygg::float_t>(), "state"_a, "metric_value"_a)
                   .def("get_state", &T::get_state, nb::rv_policy::reference_internal)
                   .def("get_metric", &T::get_metric, nb::rv_policy::copy)
                   .def("unpack", &T::unpack);
    ygg::add_print(cls);
    urd::python::add_comparison(cls);
    urd::python::add_hash(cls);
}

template<TaskKind Kind>
void bind_labeled_node(nb::module_& m, const std::string& name)
{
    using namespace nb::literals;
    using T = LabeledNode<Kind>;

    auto cls = nb::class_<T>(m, name.c_str())  //
                   .def(nb::init<formalism::ActionRowView, Node<Kind>>(), "label"_a, "node"_a, nb::keep_alive<1, 3>())
                   .def_ro("label", &T::label, nb::rv_policy::reference_internal)
                   .def_ro("node", &T::node, nb::rv_policy::copy, nb::keep_alive<0, 1>())
                   .def("pack", &T::pack, nb::keep_alive<0, 1>());
    ygg::add_print(cls);
}

template<TaskKind Kind>
void bind_packed_labeled_node(nb::module_& m, const std::string& name)
{
    using namespace nb::literals;
    using T = PackedLabeledNode<Kind>;

    auto cls = nb::class_<T>(m, name.c_str())  //
                   .def(nb::init<formalism::ActionRowView, PackedNode<Kind>>(), "label"_a, "node"_a, nb::keep_alive<1, 3>())
                   .def_ro("label", &T::label, nb::rv_policy::reference_internal)
                   .def_ro("node", &T::node, nb::rv_policy::copy, nb::keep_alive<0, 1>())
                   .def("unpack", &T::unpack, nb::keep_alive<0, 1>());
    ygg::add_print(cls);
}

template<TaskKind Kind>
void bind_plan(nb::module_& m, const std::string& name)
{
    using namespace nb::literals;
    using T = Plan<Kind>;

    auto cls = nb::class_<T>(m, name.c_str())  //
                   .def(nb::init<Node<Kind>>(), "start_node"_a, nb::keep_alive<1, 2>())
                   .def(nb::init<Node<Kind>, LabeledNodeList<Kind>>(), "start_node"_a, "labeled_succ_nodes"_a, nb::keep_alive<1, 2>())
                   .def("get_start_node", &T::get_start_node, nb::rv_policy::copy, nb::keep_alive<0, 1>())
                   .def("get_labeled_succ_nodes", &T::get_labeled_succ_nodes, nb::rv_policy::copy)
                   .def("get_cost", &T::get_cost)
                   .def("get_length", &T::get_length)
                   .def("empty", &T::empty)
                   .def("pack", &T::pack, nb::keep_alive<0, 1>());
    ygg::add_print(cls);
}

template<TaskKind Kind>
void bind_packed_plan(nb::module_& m, const std::string& name)
{
    using namespace nb::literals;
    using T = PackedPlan<Kind>;

    auto cls = nb::class_<T>(m, name.c_str())  //
                   .def(nb::init<PackedNode<Kind>>(), "start_node"_a, nb::keep_alive<1, 2>())
                   .def(nb::init<PackedNode<Kind>, PackedLabeledNodeList<Kind>>(), "start_node"_a, "labeled_succ_nodes"_a, nb::keep_alive<1, 2>())
                   .def("get_start_node", &T::get_start_node, nb::rv_policy::copy, nb::keep_alive<0, 1>())
                   .def("get_labeled_succ_nodes", &T::get_labeled_succ_nodes, nb::rv_policy::copy)
                   .def("get_cost", &T::get_cost)
                   .def("get_length", &T::get_length)
                   .def("empty", &T::empty)
                   .def("unpack", &T::unpack, nb::keep_alive<0, 1>());
    ygg::add_print(cls);
}

template<TaskKind Kind>
void bind_axiom_evaluator(nb::module_& m, const std::string& name)
{
    using namespace nb::literals;
    using T = AxiomEvaluator<Kind>;

    nb::class_<T>(m, name.c_str())
        .def(nb::new_([](TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context)
                      { return std::make_shared<T>(std::move(task), std::move(execution_context)); }),
             "task"_a,
             "execution_context"_a);
}

template<TaskKind Kind>
void bind_state_repository(nb::module_& m, const std::string& name)
{
    using namespace nb::literals;
    using T = StateRepository<Kind>;

    nb::class_<T>(m, name.c_str())  //
        .def(nb::new_([](TaskPtr<Kind> task, bool concurrent) { return std::make_shared<T>(std::move(task), concurrent); }), "task"_a, "concurrent"_a = false)
        .def("get_worker_index", &T::get_worker_index)
        .def("layer", &T::layer)
        .def("get_initial_state", &T::get_initial_state, nb::rv_policy::move, "axiom_evaluator"_a)
        .def("get_registered_state", &T::get_registered_state, nb::rv_policy::move, "state_index"_a)
        .def("is_concurrent", &T::is_concurrent)
        .def("num_states", &T::num_states)
        .def("memory_usage", &T::memory_usage)
        .def("create_state",
             static_cast<StateView<Kind> (T::*)(
                 AxiomEvaluator<Kind>&,
                 const urd::Store<urd::sequence<urd::data<formalism::FDRFact<formalism::FluentTag>>>>&,
                 const urd::Store<urd::sequence<std::pair<urd::index<formalism::FunctionTerm<GroundTag, formalism::FluentTag>>, ygg::float_t>>>&)>(
                 &T::create_state),
             nb::rv_policy::move,
             "axiom_evaluator"_a,
             "fluent_facts"_a,
             "fterm_values"_a)
        .def("create_state",
             &T::template create_state<formalism::FDRFactView<formalism::FluentTag>>,
             nb::rv_policy::move,
             "axiom_evaluator"_a,
             "fluent_facts"_a,
             "fterm_values"_a)
        .def("create_state",
             &T::template create_state<urd::owning_view<formalism::FDRFact<formalism::FluentTag>, formalism::Repository>>,
             nb::rv_policy::move,
             "axiom_evaluator"_a,
             "fluent_facts"_a,
             "fterm_values"_a);
}

/// Runs visit(repository, emit) without the GIL, `emit` handing each value to `callback`; the values borrow the
/// repository, so each keeps the Python state repository alive.
template<TaskKind Kind, typename Value, typename Visit>
auto for_each_retaining(nb::handle state_repository, const nb::callable& callback, Visit visit)
{
    auto& repository = nb::cast<StateRepository<Kind>&>(state_repository);
    const auto emit = std::function<bool(Value)>(
        [&](Value value)
        {
            nb::gil_scoped_acquire gil;
            return nb::cast<bool>(callback(nb::cast(std::move(value), nb::rv_policy::move, state_repository)));
        });
    nb::gil_scoped_release release;
    return visit(repository, emit);
}

template<TaskKind Kind>
void bind_successor_generator(nb::module_& m, const std::string& name)
{
    using namespace nb::literals;
    using T = SuccessorGenerator<Kind>;

    auto cls = nb::class_<T>(m, name.c_str());

    // NOLINTNEXTLINE(performance-unnecessary-value-param): moved into the lifted generator's constructor, which takes it by value
    cls.def(nb::new_([](TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context)
                     { return std::make_shared<T>(std::move(task), std::move(execution_context)); }),
            "task"_a,
            "execution_context"_a)
        .def("get_initial_node", &T::get_initial_node, nb::rv_policy::move, "state_repository"_a, "axiom_evaluator"_a, nb::keep_alive<0, 2>())
        .def("get_node", &T::get_node, nb::rv_policy::move, "state_repository"_a, "state_index"_a, nb::keep_alive<0, 2>())
        .def("get_packed_initial_node", &T::get_packed_initial_node, nb::rv_policy::move, "state_repository"_a, "axiom_evaluator"_a)
        .def("get_packed_node", &T::get_packed_node, nb::rv_policy::move, "state_repository"_a, "state_index"_a);

    cls.def(
           "get_applicable_action_bindings",
           [](T& self, const Node<Kind>& node) { return self.get_applicable_action_bindings(node); },
           nb::rv_policy::move,
           "node"_a,
           nb::call_guard<nb::gil_scoped_release>())
        .def(
            "get_applicable_action_bindings",
            [](T& self, const Node<Kind>& node, formalism::ActionView<LiftedTag> action) { return self.get_applicable_action_bindings(node, action); },
            nb::rv_policy::move,
            "node"_a,
            "action"_a,
            nb::call_guard<nb::gil_scoped_release>());

    cls.def("ground_action", &T::ground_action, "binding"_a);

    cls.def(
           "get_successor_node",
           [](T& self, const Node<Kind>& node, formalism::ActionRowView binding, StateRepository<Kind>& state_repository, AxiomEvaluator<Kind>& axiom_evaluator)
           { return self.get_successor_node(node, binding, state_repository, axiom_evaluator); },
           "node"_a,
           "binding"_a,
           "state_repository"_a,
           "axiom_evaluator"_a,
           nb::keep_alive<0, 4>())
        .def(
            "get_successor_node",
            [](T& self,
               const Node<Kind>& node,
               formalism::ActionView<GroundTag> action,
               StateRepository<Kind>& state_repository,
               AxiomEvaluator<Kind>& axiom_evaluator) { return self.get_successor_node(node, action, state_repository, axiom_evaluator); },
            "node"_a,
            "action"_a,
            "state_repository"_a,
            "axiom_evaluator"_a,
            nb::keep_alive<0, 4>());

    cls.def(
           "get_successor_nodes",
           [](T& self, const Node<Kind>& node, StateRepository<Kind>& state_repository, AxiomEvaluator<Kind>& axiom_evaluator)
           { return self.get_successor_nodes(node, state_repository, axiom_evaluator); },
           nb::rv_policy::move,
           "node"_a,
           "state_repository"_a,
           "axiom_evaluator"_a,
           nb::call_guard<nb::gil_scoped_release>())
        .def(
            "get_successor_nodes",
            [](T& self,
               const Node<Kind>& node,
               formalism::ActionView<LiftedTag> action,
               StateRepository<Kind>& state_repository,
               AxiomEvaluator<Kind>& axiom_evaluator) { return self.get_successor_nodes(node, action, state_repository, axiom_evaluator); },
            nb::rv_policy::move,
            "node"_a,
            "action"_a,
            "state_repository"_a,
            "axiom_evaluator"_a,
            nb::call_guard<nb::gil_scoped_release>());

    cls.def(
           "get_labeled_successor_nodes",
           [](T& self, const Node<Kind>& node, StateRepository<Kind>& state_repository, AxiomEvaluator<Kind>& axiom_evaluator)
           { return self.get_labeled_successor_nodes(node, state_repository, axiom_evaluator); },
           nb::rv_policy::move,
           "node"_a,
           "state_repository"_a,
           "axiom_evaluator"_a,
           nb::call_guard<nb::gil_scoped_release>())
        .def(
            "get_labeled_successor_nodes",
            [](T& self,
               const Node<Kind>& node,
               formalism::ActionView<LiftedTag> action,
               StateRepository<Kind>& state_repository,
               AxiomEvaluator<Kind>& axiom_evaluator) { return self.get_labeled_successor_nodes(node, action, state_repository, axiom_evaluator); },
            nb::rv_policy::move,
            "node"_a,
            "action"_a,
            "state_repository"_a,
            "axiom_evaluator"_a,
            nb::call_guard<nb::gil_scoped_release>());

    cls.def(
           "get_packed_successor_node",
           [](T& self, const Node<Kind>& node, formalism::ActionRowView binding, StateRepository<Kind>& state_repository, AxiomEvaluator<Kind>& axiom_evaluator)
           { return self.get_packed_successor_node(node, binding, state_repository, axiom_evaluator); },
           "node"_a,
           "binding"_a,
           "state_repository"_a,
           "axiom_evaluator"_a)
        .def(
            "get_packed_successor_node",
            [](T& self,
               const Node<Kind>& node,
               formalism::ActionView<GroundTag> action,
               StateRepository<Kind>& state_repository,
               AxiomEvaluator<Kind>& axiom_evaluator) { return self.get_packed_successor_node(node, action, state_repository, axiom_evaluator); },
            "node"_a,
            "action"_a,
            "state_repository"_a,
            "axiom_evaluator"_a);

    cls.def(
           "get_packed_successor_nodes",
           [](T& self, const Node<Kind>& node, StateRepository<Kind>& state_repository, AxiomEvaluator<Kind>& axiom_evaluator)
           { return self.get_packed_successor_nodes(node, state_repository, axiom_evaluator); },
           nb::rv_policy::move,
           "node"_a,
           "state_repository"_a,
           "axiom_evaluator"_a,
           nb::call_guard<nb::gil_scoped_release>())
        .def(
            "get_packed_successor_nodes",
            [](T& self,
               const Node<Kind>& node,
               formalism::ActionView<LiftedTag> action,
               StateRepository<Kind>& state_repository,
               AxiomEvaluator<Kind>& axiom_evaluator) { return self.get_packed_successor_nodes(node, action, state_repository, axiom_evaluator); },
            nb::rv_policy::move,
            "node"_a,
            "action"_a,
            "state_repository"_a,
            "axiom_evaluator"_a,
            nb::call_guard<nb::gil_scoped_release>());

    cls.def(
           "get_packed_labeled_successor_nodes",
           [](T& self, const Node<Kind>& node, StateRepository<Kind>& state_repository, AxiomEvaluator<Kind>& axiom_evaluator)
           { return self.get_packed_labeled_successor_nodes(node, state_repository, axiom_evaluator); },
           nb::rv_policy::move,
           "node"_a,
           "state_repository"_a,
           "axiom_evaluator"_a,
           nb::call_guard<nb::gil_scoped_release>())
        .def(
            "get_packed_labeled_successor_nodes",
            [](T& self,
               const Node<Kind>& node,
               formalism::ActionView<LiftedTag> action,
               StateRepository<Kind>& state_repository,
               AxiomEvaluator<Kind>& axiom_evaluator) { return self.get_packed_labeled_successor_nodes(node, action, state_repository, axiom_evaluator); },
            nb::rv_policy::move,
            "node"_a,
            "action"_a,
            "state_repository"_a,
            "axiom_evaluator"_a,
            nb::call_guard<nb::gil_scoped_release>());

    cls.def(
           "for_each_successor_node",
           [](T& self,
              const Node<Kind>& node,
              nb::typed<nb::handle, StateRepository<Kind>> state_repository,
              AxiomEvaluator<Kind>& axiom_evaluator,
              const nb::typed<nb::callable, bool(Node<Kind>)>& callback)
           {
               return for_each_retaining<Kind, Node<Kind>>(state_repository,
                                                           callback,
                                                           [&](auto& repository, const auto& emit)
                                                           { return self.for_each_successor_node(node, repository, axiom_evaluator, emit); });
           },
           "node"_a,
           "state_repository"_a,
           "axiom_evaluator"_a,
           "callback"_a)
        .def(
            "for_each_successor_node",
            [](T& self,
               const Node<Kind>& node,
               formalism::ActionView<LiftedTag> action,
               nb::typed<nb::handle, StateRepository<Kind>> state_repository,
               AxiomEvaluator<Kind>& axiom_evaluator,
               const nb::typed<nb::callable, bool(Node<Kind>)>& callback)
            {
                return for_each_retaining<Kind, Node<Kind>>(state_repository,
                                                            callback,
                                                            [&](auto& repository, const auto& emit)
                                                            { return self.for_each_successor_node(node, action, repository, axiom_evaluator, emit); });
            },
            "node"_a,
            "action"_a,
            "state_repository"_a,
            "axiom_evaluator"_a,
            "callback"_a);

    cls.def(
           "for_each_labeled_successor_node",
           [](T& self,
              const Node<Kind>& node,
              nb::typed<nb::handle, StateRepository<Kind>> state_repository,
              AxiomEvaluator<Kind>& axiom_evaluator,
              const nb::typed<nb::callable, bool(LabeledNode<Kind>)>& callback)
           {
               return for_each_retaining<Kind, LabeledNode<Kind>>(state_repository,
                                                                  callback,
                                                                  [&](auto& repository, const auto& emit)
                                                                  { return self.for_each_labeled_successor_node(node, repository, axiom_evaluator, emit); });
           },
           "node"_a,
           "state_repository"_a,
           "axiom_evaluator"_a,
           "callback"_a)
        .def(
            "for_each_labeled_successor_node",
            [](T& self,
               const Node<Kind>& node,
               formalism::ActionView<LiftedTag> action,
               nb::typed<nb::handle, StateRepository<Kind>> state_repository,
               AxiomEvaluator<Kind>& axiom_evaluator,
               const nb::typed<nb::callable, bool(LabeledNode<Kind>)>& callback)
            {
                return for_each_retaining<Kind, LabeledNode<Kind>>(
                    state_repository,
                    callback,
                    [&](auto& repository, const auto& emit) { return self.for_each_labeled_successor_node(node, action, repository, axiom_evaluator, emit); });
            },
            "node"_a,
            "action"_a,
            "state_repository"_a,
            "axiom_evaluator"_a,
            "callback"_a);

    if constexpr (std::is_same_v<Kind, LiftedTag>)
        cls.def("get_action_program", &T::get_action_program, nb::rv_policy::reference_internal);
}

template<TaskKind Kind>
void bind_search_result(nb::module_& m, const std::string& name)
{
    using T = SearchResult<Kind>;

    nb::class_<T>(m, name.c_str())
        .def(nb::init<>())
        .def_rw("status", &T::status)
        .def_rw("plan", &T::plan, nb::for_setter(nb::keep_alive<1, 2>()))
        .def_rw("goal_node", &T::goal_node, nb::for_setter(nb::keep_alive<1, 2>()))
        .def_rw("cycle_range", &T::cycle_range)
        .def_rw("statistics", &T::statistics)
        .def_rw("worker_statistics", &T::worker_statistics)
        .def_prop_ro("worker_utilization", &T::get_worker_utilization);
}

template<TaskKind Kind>
class PyGoalStrategy : public GoalStrategy<Kind>
{
public:
    using Base = GoalStrategy<Kind>;
    using Base::is_dynamic_goal_satisfied;

    NB_TRAMPOLINE(Base);

    [[nodiscard]] GoalStrategyPtr<Kind> make_worker(urd::index<Worker> index) const override { NB_OVERRIDE(make_worker, index); }

    bool is_static_goal_satisfied(const Task<Kind>& task) override { NB_OVERRIDE_PURE(is_static_goal_satisfied, task); }

    bool is_dynamic_goal_satisfied(const StateView<Kind>& seed_state, const StateView<Kind>& state) override
    { NB_OVERRIDE_PURE(is_dynamic_goal_satisfied, seed_state, state); }

    bool is_dynamic_goal_satisfied(const StateView<Kind>& seed_state, const StateBuilderView<Kind>& state) override
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
             nb::overload_cast<const StateView<Kind>&, const StateView<Kind>&>(&T::is_dynamic_goal_satisfied),
             "seed_state"_a,
             "state"_a)
        .def("is_dynamic_goal_satisfied",
             nb::overload_cast<const StateView<Kind>&, const StateBuilderView<Kind>&>(&T::is_dynamic_goal_satisfied),
             "seed_state"_a,
             "state"_a);
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

template<TaskKind Kind>
class PyPruningStrategy : public PruningStrategy<Kind>
{
public:
    using Base = PruningStrategy<Kind>;

    NB_TRAMPOLINE(Base);

    [[nodiscard]] PruningStrategyPtr<Kind> make_worker(urd::index<Worker> index) const override { NB_OVERRIDE(make_worker, index); }

    bool should_prune_state(const StateView<Kind>& state) override { NB_OVERRIDE(should_prune_state, state); }

    bool should_prune_successor_state(const StateView<Kind>& state, const StateView<Kind>& succ_state, bool is_new_succ_state) override
    { NB_OVERRIDE(should_prune_successor_state, state, succ_state, is_new_succ_state); }
};

template<TaskKind Kind>
void bind_pruning_strategy(nb::module_& m, const std::string& name)
{
    using namespace nb::literals;
    using T = PruningStrategy<Kind>;

    nb::class_<T, PyPruningStrategy<Kind>>(m, name.c_str())  //
        .def(nb::init<>())
        .def("make_worker", &T::make_worker, "worker_index"_a)
        .def("should_prune_state", nb::overload_cast<const StateView<Kind>&>(&T::should_prune_state), "state"_a)
        .def("should_prune_successor_state",
             nb::overload_cast<const StateView<Kind>&, const StateView<Kind>&, bool>(&T::should_prune_successor_state),
             "state"_a,
             "succ_state"_a,
             "is_new_succ_state"_a);
}

template<TaskKind Kind>
class PyHeuristic : public Heuristic<Kind>
{
public:
    using Base = Heuristic<Kind>;
    using Base::evaluate;

    void set_goal(formalism::ConjunctiveConditionView<GroundTag> goal) override { NB_OVERRIDE_PURE(set_goal, goal); }

    ygg::float_t evaluate(const StateView<Kind>& state) override { NB_OVERRIDE_PURE(evaluate, state); }

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
            [](T& self, const StateView<Kind>& state) { return self.evaluate(state); },
            "state"_a,
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

/// The classes bound identically for both task kinds.
template<TaskKind Kind>
void bind_kind_definitions(nb::module_& m)
{
    urd::python::bind_index<FactStorage<Kind>>(m);
    urd::python::bind_index<AtomStorage<Kind>>(m);
    urd::python::bind_index<NumericStorage<Kind>>(m);
    urd::python::bind_data<FactStorage<Kind>>(m);
    urd::python::bind_data<AtomStorage<Kind>>(m);
    urd::python::bind_data<NumericStorage<Kind>>(m);
    urd::python::bind_builder<FactStorage<Kind>, StateRepository<Kind>>(m);
    urd::python::bind_builder<AtomStorage<Kind>, StateRepository<Kind>>(m);
    urd::python::bind_builder<NumericStorage<Kind>, StateRepository<Kind>>(m);
    urd::python::bind_view<FactStorageView<Kind>>(m);
    urd::python::bind_view<AtomStorageView<Kind>>(m);
    urd::python::bind_view<NumericStorageView<Kind>>(m);
    urd::python::bind_index<State<Kind>>(m);
    urd::python::bind_data<State<Kind>>(m);
    urd::python::bind_builder<State<Kind>, StateRepository<Kind>>(m);
    urd::python::bind_view<PackedStateView<Kind>>(m);
    urd::python::bind_view<StateView<Kind>>(m);
    urd::python::bind_view<StateBuilderView<Kind>>(m);
    bind_node<Kind>(m, "Node");
    bind_packed_node<Kind>(m, "PackedNode");
    bind_labeled_node<Kind>(m, "LabeledNode");
    bind_packed_labeled_node<Kind>(m, "PackedLabeledNode");
    bind_plan<Kind>(m, "Plan");
    bind_packed_plan<Kind>(m, "PackedPlan");
    bind_axiom_evaluator<Kind>(m, "AxiomEvaluator");
    bind_state_repository<Kind>(m, "StateRepository");
    bind_successor_generator<Kind>(m, "SuccessorGenerator");
    bind_search_result<Kind>(m, "SearchResult");
    bind_goal_strategy<Kind>(m, "GoalStrategy");
    bind_conjunctive_goal_strategy<Kind>(m, "ConjunctiveGoalStrategy");
    bind_exhaustive_goal_strategy<Kind>(m, "ExhaustiveGoalStrategy");
    bind_pruning_strategy<Kind>(m, "PruningStrategy");
    bind_heuristic<Kind>(m, "Heuristic");
    bind_blind_heuristic<Kind>(m, "BlindHeuristic");
    bind_task_heuristic<Kind, MaxRPGHeuristic<Kind>>(m, "MaxRPGHeuristic");
    bind_task_heuristic<Kind, AddRPGHeuristic<Kind>>(m, "AddRPGHeuristic");
    bind_task_heuristic<Kind, FFRPGHeuristic<Kind>>(m, "FFRPGHeuristic");
    bind_task_heuristic<Kind, LMCutHeuristic<Kind>>(m, "LMCutHeuristic");
    bind_goal_count_heuristic<Kind>(m, "GoalCountHeuristic");
}

}  // namespace tyr::planning

#endif
