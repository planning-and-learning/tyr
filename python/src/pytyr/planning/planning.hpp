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

#include <nanobind/stl/chrono.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <ranges>
#include <type_traits>
#include <tyr/planning/planning.hpp>
#include <yggdrasil/python/bindings.hpp>
#include <yggdrasil/python/type_casters.hpp>

namespace tyr::planning
{
using ygg::bind_index;

namespace fp = tyr::formalism::planning;

template<typename Range, typename Value>
class OwningIterator
{
public:
    explicit OwningIterator(Range range) : m_range(std::move(range)), m_it(std::ranges::begin(m_range)), m_end(std::ranges::end(m_range)), m_exhausted(false) {}

    OwningIterator(OwningIterator&& other) noexcept :
        m_range(std::move(other.m_range)),
        m_it(std::ranges::begin(m_range)),
        m_end(std::ranges::end(m_range)),
        m_exhausted(false)
    {
    }

    OwningIterator(const OwningIterator&) = delete;
    OwningIterator& operator=(const OwningIterator&) = delete;
    OwningIterator& operator=(OwningIterator&&) = delete;

    Value next()
    {
        if (m_exhausted || m_it == m_end)
        {
            m_exhausted = true;
            throw nb::stop_iteration();
        }

        auto value = *m_it;
        ++m_it;
        return value;
    }

private:
    Range m_range;
    std::ranges::iterator_t<Range> m_it;
    std::ranges::sentinel_t<Range> m_end;
    bool m_exhausted;
};

template<typename Value, typename Range>
nb::typed<nb::iterator, Value> make_owning_iterator(nb::handle scope, const char* name, Range range)
{
    using Iterator = OwningIterator<Range, Value>;

    if (!nb::type<Iterator>().is_valid())
    {
        nb::class_<Iterator>(scope, name)  //
            .def("__iter__", [](nb::handle h) { return h; })
            .def("__next__", &Iterator::next, nb::rv_policy::copy);
    }

    return nb::borrow<nb::typed<nb::iterator, Value>>(nb::cast(Iterator(std::move(range))));
}

template<TaskKind Kind>
void bind_state(nb::module_& m, const std::string& name)
{
    using T = StateView<Kind>;

    auto cls = nb::class_<T>(m, name.c_str())  //
                   .def("get_index", &T::get_index, nb::rv_policy::copy)
                   .def("get_repository", &T::get_repository, nb::rv_policy::copy)
                   .def("get_state_repository", &T::get_state_repository, nb::rv_policy::copy)
                   // AccessibleStateConcept
                   .def("test",
                        nb::overload_cast<::tyr::formalism::planning::GroundAtomView<::tyr::formalism::StaticTag>>(&T::test, nb::const_),
                        nb::rv_policy::copy,
                        "static_atom"_a)
                   .def("test",
                        nb::overload_cast<::tyr::formalism::planning::GroundAtomView<::tyr::formalism::DerivedTag>>(&T::test, nb::const_),
                        nb::rv_policy::copy,
                        "derived_atom"_a)
                   .def("get",
                        nb::overload_cast<::tyr::formalism::planning::GroundFunctionTermView<::tyr::formalism::StaticTag>>(&T::get, nb::const_),
                        nb::rv_policy::copy,
                        "static_fterm"_a)
                   .def("get",
                        nb::overload_cast<::tyr::formalism::planning::FDRVariableView<::tyr::formalism::FluentTag>>(&T::get, nb::const_),
                        nb::rv_policy::copy,
                        "fluent_variable"_a)
                   .def("get",
                        nb::overload_cast<::tyr::formalism::planning::GroundFunctionTermView<::tyr::formalism::FluentTag>>(&T::get, nb::const_),
                        nb::rv_policy::copy,
                        "fluent_fterm"_a)
                   // IterableStateConcept
                   .def(
                       "static_atoms",
                       [](const T& s) {
                           return make_owning_iterator<fp::GroundAtomView<::tyr::formalism::StaticTag>>(nb::type<T>(),
                                                                                                        "static atom iterator",
                                                                                                        s.get_static_atoms_view());
                       },
                       nb::keep_alive<0, 1>())
                   .def(
                       "fluent_facts",
                       [](const T& s) {
                           return make_owning_iterator<fp::FDRFactView<::tyr::formalism::FluentTag>>(nb::type<T>(),
                                                                                                     "fluent facts iterator",
                                                                                                     s.get_fluent_facts_view());
                       },
                       nb::keep_alive<0, 1>())
                   .def(
                       "derived_atoms",
                       [](const T& s) {
                           return make_owning_iterator<fp::GroundAtomView<::tyr::formalism::DerivedTag>>(nb::type<T>(),
                                                                                                         "derived atom iterator",
                                                                                                         s.get_derived_atoms_view());
                       },
                       nb::keep_alive<0, 1>())
                   .def(
                       "static_fterm_values",
                       [](const T& s)
                       {
                           return make_owning_iterator<fp::GroundFunctionTermViewValuePair<::tyr::formalism::StaticTag>>(nb::type<T>(),
                                                                                                                         "static function term value iterator",
                                                                                                                         s.get_static_fterm_values_view());
                       },
                       nb::keep_alive<0, 1>())
                   .def(
                       "fluent_fterm_values",
                       [](const T& s)
                       {
                           return make_owning_iterator<fp::GroundFunctionTermViewValuePair<::tyr::formalism::FluentTag>>(nb::type<T>(),
                                                                                                                         "fluent function term value iterator",
                                                                                                                         s.get_fluent_fterm_values_view());
                       },
                       nb::keep_alive<0, 1>());
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<TaskKind Kind>
void bind_state_builder(nb::module_& m, const std::string& name)
{
    using T = ygg::Builder<State<Kind>>;

    nb::class_<T>(m, name.c_str(), "Owning read-only state snapshot that remains valid after the callback returns.")
        .def("get", nb::overload_cast<fp::FDRVariableView<::tyr::formalism::FluentTag>>(&T::get, nb::const_), "fluent_variable"_a)
        .def(
            "get",
            [](const T& state, fp::GroundFunctionTermView<::tyr::formalism::FluentTag> fterm) { return state.get(fterm.get_index()); },
            "fluent_fterm"_a)
        .def("test", nb::overload_cast<fp::GroundAtomView<::tyr::formalism::DerivedTag>>(&T::test, nb::const_), "derived_atom"_a);
}

template<TaskKind Kind>
void bind_node(nb::module_& m, const std::string& name)
{
    using T = Node<Kind>;

    auto cls = nb::class_<T>(m, name.c_str())
                   .def(nb::init<StateView<Kind>, ygg::float_t>(), "state"_a, "metric_value"_a)
                   .def("get_state", &T::get_state, nb::rv_policy::reference_internal)
                   .def("get_metric", &T::get_metric, nb::rv_policy::copy);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<TaskKind Kind>
void bind_labeled_node(nb::module_& m, const std::string& name)
{
    using T = LabeledNode<Kind>;

    auto cls = nb::class_<T>(m, name.c_str())  //
                   .def(nb::init<fp::ActionBindingView, Node<Kind>>(), "label"_a, "node"_a)
                   .def_ro("label", &T::label, nb::rv_policy::copy)
                   .def_ro("node", &T::node, nb::rv_policy::copy);
    ygg::add_print(cls);
}

template<TaskKind Kind>
void bind_plan(nb::module_& m, const std::string& name)
{
    using T = Plan<Kind>;

    auto cls = nb::class_<T>(m, name.c_str())  //
                   .def(nb::init<Node<Kind>>(), "start_node"_a)
                   .def(nb::init<Node<Kind>, LabeledNodeList<Kind>>(), "start_node"_a, "labeled_succ_nodes"_a)
                   .def("get_start_node", &T::get_start_node, nb::rv_policy::copy)
                   .def("get_labeled_succ_nodes", &T::get_labeled_succ_nodes, nb::rv_policy::copy)
                   .def("get_cost", &T::get_cost)
                   .def("get_length", &T::get_length)
                   .def("empty", &T::empty);
    ygg::add_print(cls);
}

template<TaskKind Kind>
void bind_axiom_evaluator(nb::module_& m, const std::string& name)
{
    using T = AxiomEvaluator<Kind>;

    nb::class_<T>(m, name.c_str()).def("get_index", &T::get_index);
}

template<TaskKind Kind>
void bind_state_repository(nb::module_& m, const std::string& name)
{
    using T = StateRepository<Kind>;

    nb::class_<T>(m, name.c_str())  //
        .def("get_index", &T::get_index)
        .def("get_initial_state", &T::get_initial_state, nb::rv_policy::move, "axiom_evaluator"_a)
        .def("get_registered_state", &T::get_registered_state, nb::rv_policy::move, "state_index"_a)
        .def("is_concurrent", &T::is_concurrent)
        .def("num_states", &T::num_states)
        .def("memory_usage", &T::memory_usage)
        .def("create_state",
             nb::overload_cast<
                 AxiomEvaluator<Kind>&,
                 const std::vector<ygg::Data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>>&,
                 const std::vector<std::pair<ygg::Index<::tyr::formalism::planning::GroundFunctionTerm<::tyr::formalism::FluentTag>>, ygg::float_t>>&>(
                 &T::create_state),
             nb::rv_policy::move,
             "axiom_evaluator"_a,
             "fluent_facts"_a,
             "fterm_values"_a)
        .def("create_state",
             nb::overload_cast<AxiomEvaluator<Kind>&,
                               const std::vector<::tyr::formalism::planning::FDRFactView<::tyr::formalism::FluentTag>>&,
                               const std::vector<::tyr::formalism::planning::GroundFunctionTermViewValuePair<::tyr::formalism::FluentTag>>&>(&T::create_state),
             nb::rv_policy::move,
             "axiom_evaluator"_a,
             "fluent_facts"_a,
             "fterm_values"_a);
}

template<TaskKind Kind>
void bind_successor_generator(nb::module_& m, const std::string& name)
{
    using T = SuccessorGenerator<Kind>;

    auto cls = nb::class_<T>(m, name.c_str())
                   .def("get_index", &T::get_index)
                   .def("get_initial_node", &T::get_initial_node, nb::rv_policy::move, "state_repository"_a, "axiom_evaluator"_a)
                   .def("get_labeled_successor_nodes",
                        nb::overload_cast<const Node<Kind>&, StateRepository<Kind>&, AxiomEvaluator<Kind>&>(&T::get_labeled_successor_nodes),
                        nb::rv_policy::move,
                        "node"_a,
                        "state_repository"_a,
                        "axiom_evaluator"_a,
                        nb::call_guard<nb::gil_scoped_release>())
                   .def("ground_action", &T::ground_action, "binding"_a)
                   .def("get_node", &T::get_node, nb::rv_policy::move, "state_repository"_a, "state_index"_a);

    cls.def("get_successor_node",
            nb::overload_cast<const Node<Kind>&, fp::ActionBindingView, StateRepository<Kind>&, AxiomEvaluator<Kind>&>(&T::get_successor_node),
            "node"_a,
            "binding"_a,
            "state_repository"_a,
            "axiom_evaluator"_a)
        .def("get_successor_node",
             nb::overload_cast<const Node<Kind>&, fp::GroundActionView, StateRepository<Kind>&, AxiomEvaluator<Kind>&>(&T::get_successor_node),
             "node"_a,
             "action"_a,
             "state_repository"_a,
             "axiom_evaluator"_a)
        .def("get_applicable_action_bindings",
             nb::overload_cast<const Node<Kind>&>(&T::get_applicable_action_bindings),
             nb::rv_policy::move,
             "node"_a,
             nb::call_guard<nb::gil_scoped_release>());

    if constexpr (std::is_same_v<Kind, LiftedTag>)
        cls.def("get_action_program", &T::get_action_program, nb::rv_policy::reference_internal);
}

template<TaskKind Kind>
void bind_axiom_evaluator_factory(nb::module_& m, const std::string& name)
{
    using T = AxiomEvaluatorFactory<Kind>;

    nb::class_<T>(m, name.c_str())  //
        .def(nb::init<>())
        .def("create", &T::create, "task"_a, "execution_context"_a);
}

template<TaskKind Kind>
void bind_state_repository_factory(nb::module_& m, const std::string& name)
{
    using T = StateRepositoryFactory<Kind>;

    nb::class_<T>(m, name.c_str())  //
        .def(nb::init<>())
        .def("create", &T::create, "task"_a)
        .def("create_concurrent", &T::create_concurrent, "task"_a);
}

template<TaskKind Kind>
void bind_successor_generator_factory(nb::module_& m, const std::string& name)
{
    using T = SuccessorGeneratorFactory<Kind>;

    nb::class_<T>(m, name.c_str())  //
        .def(nb::init<>())
        .def("create", &T::create, "task"_a, "execution_context"_a);
}

template<TaskKind Kind>
void bind_search_result(nb::module_& m, const std::string& name)
{
    using T = SearchResult<Kind>;

    nb::class_<T>(m, name.c_str())
        .def(nb::init<>())
        .def_rw("status", &T::status)
        .def_rw("plan", &T::plan)
        .def_rw("goal_node", &T::goal_node)
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

    NB_TRAMPOLINE(Base, 3);

    GoalStrategyPtr<Kind> make_worker(ygg::Index<Worker> index) const override { NB_OVERRIDE(make_worker, index); }

    bool is_static_goal_satisfied(const Task<Kind>& task) override { NB_OVERRIDE_PURE(is_static_goal_satisfied, task); }

    bool is_dynamic_goal_satisfied(const StateView<Kind>& seed_state, const ygg::Builder<State<Kind>>& state) override
    {
        NB_OVERRIDE_PURE(is_dynamic_goal_satisfied, seed_state, state);
    }
};

template<TaskKind Kind>
void bind_goal_strategy(nb::module_& m, const std::string& name)
{
    using T = GoalStrategy<Kind>;

    nb::class_<T, PyGoalStrategy<Kind>>(m, name.c_str())  //
        .def(nb::init<>())
        .def("make_worker", &T::make_worker, "worker_index"_a)
        .def("is_static_goal_satisfied", &T::is_static_goal_satisfied, "task"_a)
        .def("is_dynamic_goal_satisfied",
             nb::overload_cast<const StateView<Kind>&, const StateView<Kind>&>(&T::is_dynamic_goal_satisfied),
             "seed_state"_a,
             "state"_a);
}

template<TaskKind Kind>
void bind_conjunctive_goal_strategy(nb::module_& m, const std::string& name)
{
    using T = ConjunctiveGoalStrategy<Kind>;

    nb::class_<T, GoalStrategy<Kind>>(m, name.c_str())  //
        .def(nb::init<const Task<Kind>&>(), "task"_a)
        .def(nb::init<fp::GroundConjunctiveConditionView>(), "goal"_a)
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

    NB_TRAMPOLINE(Base, 3);

    PruningStrategyPtr<Kind> make_worker(ygg::Index<Worker> index) const override { NB_OVERRIDE(make_worker, index); }

    bool should_prune_state(const StateView<Kind>& state) override { NB_OVERRIDE(should_prune_state, state); }

    bool should_prune_successor_state(const StateView<Kind>& state, const StateView<Kind>& succ_state, bool is_new_succ_state) override
    {
        NB_OVERRIDE(should_prune_successor_state, state, succ_state, is_new_succ_state);
    }
};

template<TaskKind Kind>
void bind_pruning_strategy(nb::module_& m, const std::string& name)
{
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

    NB_TRAMPOLINE(Base, 4);

    void set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal) override { NB_OVERRIDE_PURE(set_goal, goal); }

    ygg::float_t evaluate(const ygg::Builder<State<Kind>>& state) override { NB_OVERRIDE_PURE(evaluate, state); }

    HeuristicPtr<Kind> make_worker(ygg::ExecutionContextPtr execution_context) const override { NB_OVERRIDE_PURE(make_worker, execution_context); }

    const ygg::UnorderedSet<fp::ActionBindingView>& get_preferred_actions() override { return Base::get_preferred_actions(); }
};

template<TaskKind Kind>
void bind_heuristic(nb::module_& m, const std::string& name)
{
    using T = Heuristic<Kind>;

    nb::class_<T, PyHeuristic<Kind>>(m, name.c_str())  //
        .def(nb::init<>())
        .def("set_goal", &T::set_goal, "goal"_a)
        .def(
            "evaluate",
            [](T& self, const StateView<Kind>& state) { return self.evaluate(state); },
            "state"_a,
            nb::call_guard<nb::gil_scoped_release>())
        .def("get_preferred_actions", &T::get_preferred_actions);
}

template<TaskKind Kind>
void bind_blind_heuristic(nb::module_& m, const std::string& name)
{
    using T = BlindHeuristic<Kind>;

    nb::class_<T, Heuristic<Kind>>(m, name.c_str())  //
        .def(nb::new_([]() { return std::make_shared<T>(); }))
        .def_static("create", &T::create);
}

template<TaskKind Kind>
void bind_goal_count_heuristic(nb::module_& m, const std::string& name)
{
    using T = GoalCountHeuristic<Kind>;

    nb::class_<T, Heuristic<Kind>>(m, name.c_str())  //
        .def(nb::new_([](std::shared_ptr<const Task<Kind>> task) { return std::make_shared<T>(std::move(task)); }))
        .def_static("create", &T::create, "task"_a);
}

template<TaskKind Kind>
void bind_rpg_max_heuristic(nb::module_& m, const std::string& name)
{
    using T = MaxRPGHeuristic<Kind>;

    nb::class_<T, Heuristic<Kind>>(m, name.c_str())  //
        .def(nb::new_([](TaskPtr<Kind> task, std::shared_ptr<ygg::ExecutionContext> execution_context, CostMode cost_mode)
                      { return std::make_shared<T>(std::move(task), std::move(execution_context), cost_mode); }),
             "task"_a,
             "execution_context"_a,
             "cost_mode"_a = CostMode::GENERAL)
        .def_static("create", &T::create, "task"_a, "execution_context"_a, "cost_mode"_a = CostMode::GENERAL);
}

template<TaskKind Kind>
void bind_rpg_add_heuristic(nb::module_& m, const std::string& name)
{
    using T = AddRPGHeuristic<Kind>;

    nb::class_<T, Heuristic<Kind>>(m, name.c_str())  //
        .def(nb::new_([](TaskPtr<Kind> task, std::shared_ptr<ygg::ExecutionContext> execution_context, CostMode cost_mode)
                      { return std::make_shared<T>(std::move(task), std::move(execution_context), cost_mode); }),
             "task"_a,
             "execution_context"_a,
             "cost_mode"_a = CostMode::GENERAL)
        .def_static("create", &T::create, "task"_a, "execution_context"_a, "cost_mode"_a = CostMode::GENERAL);
}

template<TaskKind Kind>
void bind_rpg_ff_heuristic(nb::module_& m, const std::string& name)
{
    using T = FFRPGHeuristic<Kind>;

    nb::class_<T, Heuristic<Kind>>(m, name.c_str())  //
        .def(nb::new_([](TaskPtr<Kind> task, std::shared_ptr<ygg::ExecutionContext> execution_context, CostMode cost_mode)
                      { return std::make_shared<T>(std::move(task), std::move(execution_context), cost_mode); }),
             "task"_a,
             "execution_context"_a,
             "cost_mode"_a = CostMode::GENERAL)
        .def_static("create", &T::create, "task"_a, "execution_context"_a, "cost_mode"_a = CostMode::GENERAL);
}

template<TaskKind Kind>
void bind_lmcut_heuristic(nb::module_& m, const std::string& name)
{
    using T = LMCutHeuristic<Kind>;

    nb::class_<T, Heuristic<Kind>>(m, name.c_str())  //
        .def(nb::new_([](TaskPtr<Kind> task, std::shared_ptr<ygg::ExecutionContext> execution_context, CostMode cost_mode)
                      { return std::make_shared<T>(std::move(task), std::move(execution_context), cost_mode); }),
             "task"_a,
             "execution_context"_a,
             "cost_mode"_a = CostMode::GENERAL)
        .def_static("create", &T::create, "task"_a, "execution_context"_a, "cost_mode"_a = CostMode::GENERAL);
}

}  // namespace tyr::planning

#endif
