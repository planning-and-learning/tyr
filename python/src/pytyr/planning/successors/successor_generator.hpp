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

#ifndef TYR_PYTHON_PLANNING_SUCCESSORS_SUCCESSOR_GENERATOR_HPP_
#define TYR_PYTHON_PLANNING_SUCCESSORS_SUCCESSOR_GENERATOR_HPP_

#include "pytyr/planning/module.hpp"
#include "pytyr/planning/plans/plans.hpp"
#include "pytyr/planning/successors/successors.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <nanobind/nanobind.h>
#include <nanobind/stl/function.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/vector.h>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <tyr/planning/axioms/axiom_evaluator.hpp>
#include <tyr/planning/declarations.hpp>
#include <tyr/planning/nodes/node_view.hpp>
#include <tyr/planning/states/state_repository.hpp>
#include <tyr/planning/successors/ground/successor_generator.hpp>
#include <tyr/planning/successors/lifted/successor_generator.hpp>
#include <tyr/planning/successors/successor_view.hpp>
#include <tyr/planning/successors/successors.hpp>
#include <urd/python/bindings.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::planning
{

/// The deterministic successor state of `node` by `binding`, interned or borrowed, in a builder taken from the node's
/// repository: neither closed under the axioms nor registered (find_successor, extend_successor, register_successor).
template<TaskKind Kind, formalism::ActionRowViewConcept Binding>
SuccessorState<Kind> successor_state_of(SuccessorGenerator<Kind>& generator, const NodeBuilderView<Kind>& node, Binding binding)
{
    auto result = SuccessorState<Kind> { ::tyr::planning::get_state_builder(node.context()), 0, StateDelta {} };
    result.auxiliary_value = generator.generate_successor_state(node, binding, *result.state, result.delta);
    return result;
}

/// `successor_state`, unless making a node consumed it already (ValueError).
template<TaskKind Kind>
SuccessorState<Kind>& unconsumed(SuccessorState<Kind>& successor_state)
{
    if (successor_state.state == nullptr)
        throw std::invalid_argument("the successor state was consumed: register_successor or successor_node made its node.");
    return successor_state;
}

/// The options of an enumeration from Python's keywords (SuccessorOptions): the action to keep to, a test each candidate
/// binding passes first, a callable that arranges the transitions once all are known (given their labels, it returns the
/// positions to generate, in order), and a poll that ends it. The callables are borrowed for the call.
template<TaskKind Kind>
SuccessorOptions<Kind> options_of(const std::optional<formalism::ActionView<LiftedTag>>& action,
                                  const std::optional<nb::typed<nb::callable, bool(ActionBindingView<Kind>)>>& accept,
                                  const std::optional<nb::typed<nb::callable, std::vector<std::size_t>(std::vector<TransitionLabel<Kind>>)>>& arrange,
                                  const std::optional<nb::typed<nb::callable, bool()>>& stop)
{
    auto options = SuccessorOptions<Kind> {};
    options.action = action;
    if (accept)
        options.accept = [&accept](ActionBindingView<Kind> binding) { return nb::cast<bool>((*accept)(binding)); };
    if (arrange)
        options.arrange = [&arrange](std::span<const TransitionLabel<Kind>> labels, std::vector<std::size_t>& order)
        { order = nb::cast<std::vector<std::size_t>>((*arrange)(std::vector<TransitionLabel<Kind>>(labels.begin(), labels.end()))); };
    if (stop)
        options.stop = [&stop] { return nb::cast<bool>((*stop)()); };
    return options;
}

/// The deterministic environment (the submodule `deterministic` of a task kind's module): its plans, and one successor
/// state per transition, neither closed under the axioms nor registered (the kind's find_successor, extend_successor and
/// register_successor do that, each its own step).
template<TaskKind Kind>
void bind_deterministic_environment(nb::module_& m)
{
    using namespace nb::literals;
    using T = SuccessorGenerator<Kind>;

    // What solves a task in this environment: a plan, a start node and the labeled nodes that follow it.
    bind_plans<Kind>(m);

    m.def(
        "require",
        [](const T& generator) { tyr::planning::require_environment<DeterministicTag>(generator); },
        "generator"_a,
        "Raises ValueError unless the generator's task has neither oneof nor probabilistic effects.");
    // Every step at once: labeled nodes, their states closed under the axioms; registered and their labels interned unless
    // `intern` is False.
    m.def(
        "get_successors",
        [](T& generator, StateRepository<Kind>& repository, AxiomEvaluator<Kind>& axiom_evaluator, const NodeBuilderView<Kind>& node, bool intern)
        { return tyr::planning::get_successors<DeterministicTag>(generator, repository, axiom_evaluator, node, intern); },
        "generator"_a,
        "repository"_a,
        "axiom_evaluator"_a,
        "node"_a,
        "intern"_a = true);
    // The successor state is the enumeration's, reused across the transitions: valid during the callback, which may make
    // its node (register_successor, successor_node: its state is then taken).
    m.def(
        "for_each_successor",
        [](T& generator,
           const NodeBuilderView<Kind>& node,
           const nb::typed<nb::callable, bool(TransitionLabel<Kind>, SuccessorState<Kind>&)>& callback,
           std::optional<formalism::ActionView<LiftedTag>> action,
           const std::optional<nb::typed<nb::callable, bool(ActionBindingView<Kind>)>>& accept,
           const std::optional<nb::typed<nb::callable, std::vector<std::size_t>(std::vector<TransitionLabel<Kind>>)>>& arrange,
           const std::optional<nb::typed<nb::callable, bool()>>& stop)
        {
            return tyr::planning::for_each_successor<DeterministicTag>(
                generator,
                node,
                [&](const TransitionLabel<Kind>& transition, SuccessorState<Kind>& successor)
                { return nb::cast<bool>(callback(transition, nb::cast(&successor, nb::rv_policy::reference))); },
                options_of<Kind>(action, accept, arrange, stop));
        },
        "generator"_a,
        "node"_a,
        "callback"_a,
        "action"_a = nb::none(),
        "accept"_a = nb::none(),
        "arrange"_a = nb::none(),
        "stop"_a = nb::none());
    // One known transition, without enumerating the node: by its label, or by an action binding of either form, interned
    // or borrowed (a label's is the form its kind enumerates).
    m.def(
        "generate_successor",
        [](T& generator, const NodeBuilderView<Kind>& node, const TransitionLabel<Kind>& transition)
        {
            auto result = SuccessorState<Kind> { ::tyr::planning::get_state_builder(node.context()), 0, StateDelta {} };
            tyr::planning::generate_successor<DeterministicTag>(generator, node, transition, result);
            return result;
        },
        "generator"_a,
        "node"_a,
        "transition"_a);
    m.def(
        "generate_successor",
        [](T& generator, const NodeBuilderView<Kind>& node, formalism::ActionRowView binding) { return successor_state_of(generator, node, binding); },
        "generator"_a,
        "node"_a,
        "binding"_a);
    m.def(
        "generate_successor",
        [](T& generator, const NodeBuilderView<Kind>& node, formalism::ActionRowBuilderView binding) { return successor_state_of(generator, node, binding); },
        "generator"_a,
        "node"_a,
        "binding"_a);
}

/// An environment in which nature acts (the submodules `nondeterministic`, `probabilistic` and `general` of a task
/// kind's module), its functions and records: a successor is its label and its outcomes (in the general one its
/// distributions of them), their states neither closed under the axioms nor registered (register_outcomes is its axiom
/// step).
template<NatureEnvironment E, TaskKind Kind>
void bind_nature_environment(nb::module_& m)
{
    using namespace nb::literals;
    using T = SuccessorGenerator<Kind>;

    bind_environment_successors<E, Kind>(m);

    // A successor: owned (its outcome states builders of the node's repository), or borrowed during for_each_successor's
    // callback (valid only there).
    const auto owned = [](const NodeBuilderView<Kind>& node, SuccessorBuilder<E, Kind> successor)
    { return OwnedSuccessorBuilderView<E, Kind>(std::move(successor), &node.context()); };

    m.def(
        "require",
        [](const T& generator) { tyr::planning::require_environment<E>(generator); },
        "generator"_a,
        "Raises ValueError unless this environment gives the generator's task its exact successors.");
    m.def(
        "get_successors",
        [](T& generator, StateRepository<Kind>& repository, AxiomEvaluator<Kind>& axiom_evaluator, const NodeBuilderView<Kind>& node, bool intern)
        { return tyr::planning::get_successors<E>(generator, repository, axiom_evaluator, node, intern); },
        "generator"_a,
        "repository"_a,
        "axiom_evaluator"_a,
        "node"_a,
        "intern"_a = true);
    m.def(
        "for_each_successor",
        [](T& generator,
           const NodeBuilderView<Kind>& node,
           const nb::typed<nb::callable, bool(TransitionLabel<Kind>, SuccessorBuilderView<E, Kind>)>& callback,
           std::optional<formalism::ActionView<LiftedTag>> action,
           const std::optional<nb::typed<nb::callable, bool(ActionBindingView<Kind>)>>& accept,
           const std::optional<nb::typed<nb::callable, std::vector<std::size_t>(std::vector<TransitionLabel<Kind>>)>>& arrange,
           const std::optional<nb::typed<nb::callable, bool()>>& stop)
        {
            return tyr::planning::for_each_successor<E>(
                generator,
                node,
                [&](const TransitionLabel<Kind>& transition, SuccessorBuilder<E, Kind>& successor)
                { return nb::cast<bool>(callback(transition, urd::resolve(std::as_const(successor), node.context()))); },
                options_of<Kind>(action, accept, arrange, stop));
        },
        "generator"_a,
        "node"_a,
        "callback"_a,
        "action"_a = nb::none(),
        "accept"_a = nb::none(),
        "arrange"_a = nb::none(),
        "stop"_a = nb::none());
    // One known transition, without enumerating the node. Each gives the successor whether or not the transition can
    // happen: one that cannot has nothing to pick from.
    m.def(
        "generate_successor",
        [owned](T& generator, const NodeBuilderView<Kind>& node, const TransitionLabel<Kind>& transition)
        {
            auto successor = SuccessorBuilder<E, Kind>();
            tyr::planning::generate_successor<E>(generator, node, transition, successor);
            return owned(node, std::move(successor));
        },
        "generator"_a,
        "node"_a,
        "transition"_a);
    m.def(
        "generate_successor",
        [owned](T& generator, const NodeBuilderView<Kind>& node, formalism::ActionRowView binding)
        {
            auto successor = SuccessorBuilder<E, Kind>();
            generator.template generate_successor<E>(node, binding, successor);
            return owned(node, std::move(successor));
        },
        "generator"_a,
        "node"_a,
        "binding"_a);
    m.def(
        "generate_successor",
        [owned](T& generator, const NodeBuilderView<Kind>& node, formalism::ActionRowBuilderView binding)
        {
            auto successor = SuccessorBuilder<E, Kind>();
            generator.template generate_successor<E>(node, binding, successor);
            return owned(node, std::move(successor));
        },
        "generator"_a,
        "node"_a,
        "binding"_a);
    m.def(
        "generate_end_successor",
        [owned](T& generator, const NodeBuilderView<Kind>& node, std::uint32_t open)
        {
            auto successor = SuccessorBuilder<E, Kind>();
            generator.template generate_end_successor<E>(node, open, successor);
            return owned(node, std::move(successor));
        },
        "generator"_a,
        "node"_a,
        "open"_a);
    m.def(
        "generate_timed_literal_successor",
        [owned](T& generator, const NodeBuilderView<Kind>& node)
        {
            auto successor = SuccessorBuilder<E, Kind>();
            generator.template generate_timed_literal_successor<E>(node, successor);
            return owned(node, std::move(successor));
        },
        "generator"_a,
        "node"_a);
    // The three steps. A successor from Python is read through a view, so each step gives a copy: the lookup's has the index
    // and the derived atoms of each outcome state that is registered already; the axiom step's has the others closed under
    // the axioms (None where every outcome failed: the transition is not applicable); registering's has them registered.
    m.def(
        "find_outcomes",
        [](StateRepository<Kind>& repository, const SuccessorBuilderView<E, Kind>& successor)
        {
            auto found = urd::clone(successor).release();
            tyr::planning::find_outcomes(repository, found);
            return OwnedSuccessorBuilderView<E, Kind>(std::move(found), &repository);
        },
        "repository"_a,
        "successor"_a);
    m.def(
        "extend_outcomes",
        [](T& generator,
           AxiomEvaluator<Kind>& axiom_evaluator,
           const SuccessorBuilderView<E, Kind>& successor) -> std::optional<OwnedSuccessorBuilderView<E, Kind>>
        {
            auto extended = urd::clone(successor).release();
            if (!tyr::planning::extend_outcomes(generator, successor.context(), axiom_evaluator, extended))
                return std::nullopt;
            return OwnedSuccessorBuilderView<E, Kind>(std::move(extended), &successor.context());
        },
        "generator"_a,
        "axiom_evaluator"_a,
        "successor"_a);
    m.def(
        "register_outcomes",
        [](StateRepository<Kind>& repository, const SuccessorBuilderView<E, Kind>& successor)
        {
            auto registered = urd::clone(successor).release();
            tyr::planning::register_outcomes(repository, registered);
            return OwnedSuccessorBuilderView<E, Kind>(std::move(registered), &repository);
        },
        "repository"_a,
        "successor"_a);
    m.def(
        "intern_label",
        [](StateRepository<Kind>& repository, const SuccessorBuilderView<E, Kind>& successor)
        {
            auto interned = urd::clone(successor).release();
            auto builder = formalism::Builder {};
            tyr::planning::intern_label(repository, builder, interned);
            return OwnedSuccessorBuilderView<E, Kind>(std::move(interned), &repository);
        },
        "repository"_a,
        "successor"_a);
}

/// The successor generator, what is alike in every environment (a transition's label, a successor state and its
/// registration, interning a binding, nodes), and a submodule per environment with that environment's successor
/// functions (tyr/planning/successors/successors.hpp): `deterministic`, `nondeterministic`, `probabilistic` and
/// `general`. A lifted binding (a row being built) is borrowed, valid only during the callback that receives it
/// (intern_action_binding keeps one).
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
        .def("features", &T::features, "What its task asks of an environment: oneof, probabilistic effects, temporal.")
        .def(
            "check_action_binding",
            [](T& self, const NodeBuilderView<Kind>& node, formalism::ActionRowView binding) { return self.check_action_binding(node, binding); },
            "node"_a,
            "binding"_a)
        .def(
            "check_action_binding",
            [](T& self, const NodeBuilderView<Kind>& node, formalism::ActionRowBuilderView binding) { return self.check_action_binding(node, binding); },
            "node"_a,
            "binding"_a)
        .def("validate_action_binding", [](T& self, ActionBindingView<Kind> binding) { self.validate_action_binding(binding); }, "binding"_a);

    cls.def("ground_action", &T::ground_action, "binding"_a);

    if constexpr (std::is_same_v<Kind, LiftedTag>)
        cls.def("get_action_program", &T::get_action_program, nb::rv_policy::reference_internal);

    // A transition's label: its binding borrowed (a lifted one valid only during the callback that receives it).
    nb::class_<TransitionLabel<Kind>>(m, "TransitionLabel")
        .def(
            "__init__",
            [](TransitionLabel<Kind>* self, TransitionKind kind, std::optional<ActionBindingView<Kind>> binding, std::uint32_t open)
            { new (self) TransitionLabel<Kind> { kind, binding, open }; },
            "kind"_a,
            "binding"_a = nb::none(),
            "open"_a = 0)
        .def_prop_ro("kind", [](const TransitionLabel<Kind>& self) { return self.kind; })
        .def_prop_ro("binding", [](const TransitionLabel<Kind>& self) { return self.binding; })
        .def_prop_ro("open", [](const TransitionLabel<Kind>& self) { return self.open; });
    nb::class_<SuccessorState<Kind>>(m, "SuccessorState")
        .def_ro("auxiliary_value", &SuccessorState<Kind>::auxiliary_value)
        .def_ro("delta", &SuccessorState<Kind>::delta);

    // The free functions that are alike in every environment (tyr/planning/successors/successors.hpp), as C++ names them.
    // The deterministic steps, on the successor state in place: the lookup (whether its state is registered already; it
    // then has that state's index and derived atoms), the axiom step (unless the lookup found it), and its node, with
    // interning (register_successor) or without (successor_node). Making the node consumes the successor state.
    m.def(
        "find_successor",
        [](StateRepository<Kind>& repository, const NodeBuilderView<Kind>& node, SuccessorState<Kind>& successor_state)
        { return tyr::planning::find_successor(repository, node, unconsumed(successor_state)); },
        "repository"_a,
        "node"_a,
        "successor_state"_a);
    m.def(
        "extend_successor",
        [](AxiomEvaluator<Kind>& axiom_evaluator, const NodeBuilderView<Kind>& node, SuccessorState<Kind>& successor_state)
        { tyr::planning::extend_successor(axiom_evaluator, node, unconsumed(successor_state)); },
        "axiom_evaluator"_a,
        "node"_a,
        "successor_state"_a);
    m.def(
        "register_successor",
        [](StateRepository<Kind>& repository, const NodeBuilderView<Kind>& node, SuccessorState<Kind>& successor_state)
        { return OwnedNodeBuilderView<Kind>(tyr::planning::register_successor(repository, node, unconsumed(successor_state)), &repository); },
        "repository"_a,
        "node"_a,
        "successor_state"_a);
    m.def(
        "successor_node",
        [](const NodeBuilderView<Kind>& node, SuccessorState<Kind>& successor_state)
        { return OwnedNodeBuilderView<Kind>(tyr::planning::successor_node(node, unconsumed(successor_state)), &node.context()); },
        "node"_a,
        "successor_state"_a);
    m.def(
        "intern_action_binding",
        [](T& generator, ActionBindingView<Kind> binding)
        {
            auto builder = formalism::Builder {};
            return tyr::planning::intern_action_binding(generator, builder, binding);
        },
        "generator"_a,
        "binding"_a);
    m.def(
        "get_initial_node",
        [](StateRepository<Kind>& repository, AxiomEvaluator<Kind>& axiom_evaluator) { return tyr::planning::get_initial_node(repository, axiom_evaluator); },
        "repository"_a,
        "axiom_evaluator"_a);
    m.def(
        "get_node",
        [](const StateRepository<Kind>& repository, urd::index<State<Kind>> state_index)
        { return OwnedNodeBuilderView<Kind>(tyr::planning::get_node(repository, state_index), &repository); },
        "repository"_a,
        "state_index"_a);
    m.def(
        "get_node",
        [](const StateRepository<Kind>& repository, const urd::data<Node<Kind>>& node)
        { return OwnedNodeBuilderView<Kind>(tyr::planning::get_node(repository, node), &repository); },
        "repository"_a,
        "node"_a);
    // A node's record through its repository, kept apart from the node: what a search takes as its start node.
    m.def("packed_node", [](const NodeBuilderView<Kind>& node) { return OwnedNodeRecordView<Kind>(node.pack(), &node.context()); }, "node"_a);

    // An environment each: what nature does once a transition is chosen.
    auto deterministic = m.def_submodule("deterministic", "One successor state per transition.");
    bind_deterministic_environment<Kind>(deterministic);
    auto nondeterministic = m.def_submodule("nondeterministic", "An adversary picks one of a transition's outcomes (oneof).");
    bind_nature_environment<NondeterministicTag, Kind>(nondeterministic);
    auto probabilistic = m.def_submodule("probabilistic", "Chance draws one of a transition's outcomes (probabilistic).");
    bind_nature_environment<ProbabilisticTag, Kind>(probabilistic);
    auto general = m.def_submodule("general", "An adversary picks a distribution, chance draws an outcome from it.");
    bind_nature_environment<GeneralTag, Kind>(general);
}

}  // namespace tyr::planning

#endif
