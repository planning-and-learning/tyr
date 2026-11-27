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
#include <memory>
#include <nanobind/nanobind.h>
#include <nanobind/stl/function.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/shared_ptr.h>
#include <cstdint>
#include <optional>
#include <nanobind/stl/vector.h>
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
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::planning
{

/// The successor generator and what completes its results (tyr/planning/successors.hpp): it enumerates applicable
/// bindings borrowed, a lifted one (a row being built) valid only during the callback (intern_action_binding keeps one),
/// and writes a successor state into a builder of a state repository (get_state_builder), which register_successor
/// registers.
template<TaskKind Kind>
void bind_successor_generator(nb::module_& m, const std::string& name)
{
    using namespace nb::literals;
    using T = SuccessorGenerator<Kind>;
    using Builder = urd::builder<State<Kind>, StateRepository<Kind>>;

    auto cls = nb::class_<T>(m, name.c_str());

    // NOLINTNEXTLINE(performance-unnecessary-value-param): moved into the lifted generator's constructor, which takes it by value
    cls.def(nb::new_([](TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context)
                     { return std::make_shared<T>(std::move(task), std::move(execution_context)); }),
            "task"_a,
            "execution_context"_a)
        .def(
            "for_each_applicable_action_binding",
            [](T& self,
               const NodeBuilderView<Kind>& node,
               const nb::typed<nb::callable, bool(ActionBindingView<Kind>)>& callback,
               std::optional<formalism::ActionView<LiftedTag>> action,
               std::optional<nb::typed<nb::callable, bool(ActionBindingView<Kind>)>> accept,
               std::optional<nb::typed<nb::callable, bool()>> stop)
            {
                auto filter = ActionBindingFilter<Kind> { action, {}, {} };
                if (accept)
                    filter.accept = [&](ActionBindingView<Kind> binding) { return nb::cast<bool>((*accept)(binding)); };
                if (stop)
                    filter.stop = [&] { return nb::cast<bool>((*stop)()); };
                return self.for_each_applicable_action_binding(node,
                                                               filter,
                                                               [&](ActionBindingView<Kind> binding) { return nb::cast<bool>(callback(binding)); });
            },
            "node"_a,
            "callback"_a,
            "action"_a = nb::none(),
            "accept"_a = nb::none(),
            "stop"_a = nb::none())
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
        .def(
            "validate_action_binding",
            [](T& self, ActionBindingView<Kind> binding) { self.validate_action_binding(binding); },
            "binding"_a)
        .def(
            "generate_successor_state",
            [](T& self, const NodeBuilderView<Kind>& node, formalism::ActionRowView binding, Builder& out_state, StateDelta& out_delta)
            { return self.generate_successor_state(node, binding, out_state, out_delta); },
            "node"_a,
            "binding"_a,
            "out_state"_a,
            "out_delta"_a)
        .def(
            "generate_successor_state",
            [](T& self, const NodeBuilderView<Kind>& node, formalism::ActionRowBuilderView binding, Builder& out_state, StateDelta& out_delta)
            { return self.generate_successor_state(node, binding, out_state, out_delta); },
            "node"_a,
            "binding"_a,
            "out_state"_a,
            "out_delta"_a);

    // A successor with outcomes: owned (its outcome states builders of the node's repository, not registered), or borrowed
    // during for_each_successor's callback (valid only there).
    const auto owned = [](const NodeBuilderView<Kind>& node, SuccessorBuilder<Kind> successor)
    { return OwnedSuccessorBuilderView<Kind>(std::move(successor), &node.context()); };
    cls.def(
           "generate_successor",
           [owned](T& self, const NodeBuilderView<Kind>& node, formalism::ActionRowView binding, AxiomEvaluator<Kind>& axiom_evaluator)
           {
               auto successor = SuccessorBuilder<Kind>();
               self.generate_successor(node, binding, axiom_evaluator, successor);
               return owned(node, std::move(successor));
           },
           "node"_a,
           "binding"_a,
           "axiom_evaluator"_a)
        .def(
            "generate_successor",
            [owned](T& self, const NodeBuilderView<Kind>& node, formalism::ActionRowBuilderView binding, AxiomEvaluator<Kind>& axiom_evaluator)
            {
                auto successor = SuccessorBuilder<Kind>();
                self.generate_successor(node, binding, axiom_evaluator, successor);
                return owned(node, std::move(successor));
            },
            "node"_a,
            "binding"_a,
            "axiom_evaluator"_a)
        .def(
            "generate_end_successor",
            [owned](T& self, const NodeBuilderView<Kind>& node, std::uint32_t open, AxiomEvaluator<Kind>& axiom_evaluator)
            {
                auto successor = SuccessorBuilder<Kind>();
                self.generate_end_successor(node, open, axiom_evaluator, successor);
                return owned(node, std::move(successor));
            },
            "node"_a,
            "open"_a,
            "axiom_evaluator"_a)
        .def(
            "generate_timed_literal_successor",
            [owned](T& self, const NodeBuilderView<Kind>& node, AxiomEvaluator<Kind>& axiom_evaluator)
            {
                auto successor = SuccessorBuilder<Kind>();
                self.generate_timed_literal_successor(node, axiom_evaluator, successor);
                return owned(node, std::move(successor));
            },
            "node"_a,
            "axiom_evaluator"_a)
        .def(
            "for_each_successor",
            [](T& self,
               const NodeBuilderView<Kind>& node,
               AxiomEvaluator<Kind>& axiom_evaluator,
               const nb::typed<nb::callable, bool(SuccessorBuilderView<Kind>)>& callback)
            {
                return self.for_each_successor(node,
                                               axiom_evaluator,
                                               [&](SuccessorBuilder<Kind>& successor)
                                               { return nb::cast<bool>(callback(urd::resolve(std::as_const(successor), node.context()))); });
            },
            "node"_a,
            "axiom_evaluator"_a,
            "callback"_a);

    cls.def("ground_action", &T::ground_action, "binding"_a);

    if constexpr (std::is_same_v<Kind, LiftedTag>)
        cls.def("get_action_program", &T::get_action_program, nb::rv_policy::reference_internal);

    // The free functions (tyr/planning/successors/successors.hpp), as C++ names them.
    nb::class_<SuccessorState<Kind>>(m, "SuccessorState")
        .def_ro("auxiliary_value", &SuccessorState<Kind>::auxiliary_value)
        .def_ro("delta", &SuccessorState<Kind>::delta);
    m.def(
        "get_successors",
        [](T& generator, StateRepository<Kind>& repository, AxiomEvaluator<Kind>& axiom_evaluator, const NodeBuilderView<Kind>& node)
        { return tyr::planning::get_successors(generator, repository, axiom_evaluator, node); },
        "generator"_a,
        "repository"_a,
        "axiom_evaluator"_a,
        "node"_a);
    m.def(
        "for_each_successor",
        [](T& generator,
           const NodeBuilderView<Kind>& node,
           AxiomEvaluator<Kind>& axiom_evaluator,
           const nb::typed<nb::callable, bool(SuccessorBuilderView<Kind>)>& callback)
        {
            return tyr::planning::for_each_successor(generator,
                                                     node,
                                                     axiom_evaluator,
                                                     [&](SuccessorBuilder<Kind>& successor)
                                                     { return nb::cast<bool>(callback(urd::resolve(std::as_const(successor), node.context()))); });
        },
        "generator"_a,
        "node"_a,
        "axiom_evaluator"_a,
        "callback"_a);
    // A successor from Python is read through a view: the one with its label interned is a copy.
    m.def(
        "intern_label",
        [](StateRepository<Kind>& repository, const SuccessorBuilderView<Kind>& successor)
        {
            auto interned = urd::clone(successor).release();
            auto builder = formalism::Builder {};
            tyr::planning::intern_label(repository, builder, interned);
            return OwnedSuccessorBuilderView<Kind>(std::move(interned), &repository);
        },
        "repository"_a,
        "successor"_a);
    m.def(
        "for_each_applicable_action_binding",
        [](T& generator,
           const NodeBuilderView<Kind>& node,
           const nb::typed<nb::callable, bool(ActionBindingView<Kind>)>& callback,
           std::optional<formalism::ActionView<LiftedTag>> action,
           std::optional<nb::typed<nb::callable, bool(ActionBindingView<Kind>)>> accept,
           std::optional<nb::typed<nb::callable, bool()>> stop)
        {
            auto filter = ActionBindingFilter<Kind> { action, {}, {} };
            if (accept)
                filter.accept = [&](ActionBindingView<Kind> binding) { return nb::cast<bool>((*accept)(binding)); };
            if (stop)
                filter.stop = [&] { return nb::cast<bool>((*stop)()); };
            return tyr::planning::for_each_applicable_action_binding(generator,
                                                                     node,
                                                                     [&](ActionBindingView<Kind> binding) { return nb::cast<bool>(callback(binding)); },
                                                                     filter);
        },
        "generator"_a,
        "node"_a,
        "callback"_a,
        "action"_a = nb::none(),
        "accept"_a = nb::none(),
        "stop"_a = nb::none());
    m.def(
        "generate_successor_state",
        [](T& generator, const StateRepository<Kind>& repository, const NodeBuilderView<Kind>& node, formalism::ActionRowView binding)
        { return tyr::planning::generate_successor_state(generator, repository, node, binding); },
        "generator"_a,
        "repository"_a,
        "node"_a,
        "binding"_a);
    m.def(
        "generate_successor_state",
        [](T& generator, const StateRepository<Kind>& repository, const NodeBuilderView<Kind>& node, formalism::ActionRowBuilderView binding)
        { return tyr::planning::generate_successor_state(generator, repository, node, binding); },
        "generator"_a,
        "repository"_a,
        "node"_a,
        "binding"_a);
    // The successor state is consumed: its state becomes the node's.
    m.def(
        "register_successor",
        [](StateRepository<Kind>& repository, AxiomEvaluator<Kind>& axiom_evaluator, const NodeBuilderView<Kind>& node, SuccessorState<Kind>& successor_state)
        {
            return OwnedNodeBuilderView<Kind>(tyr::planning::register_successor(repository, axiom_evaluator, node, std::move(successor_state)), &repository);
        },
        "repository"_a,
        "axiom_evaluator"_a,
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
    // A state builder from Python is its own (StateRepository.get_state_builder): the successor holds a copy.
    m.def(
        "register_successor",
        [](StateRepository<Kind>& repository,
           AxiomEvaluator<Kind>& axiom_evaluator,
           const Builder& state,
           ygg::float_t auxiliary_value,
           const NodeBuilderView<Kind>& node,
           const StateDelta& delta)
        {
            auto node_builder =
                tyr::planning::register_successor(repository, axiom_evaluator, urd::make_owned<Builder>(urd::clone(state)), auxiliary_value, node, delta);
            return OwnedNodeBuilderView<Kind>(std::move(node_builder), &repository);
        },
        "repository"_a,
        "axiom_evaluator"_a,
        "state"_a,
        "auxiliary_value"_a,
        "node"_a,
        "delta"_a);
    // A successor from Python is read through a view: the registered successor is a copy.
    m.def(
        "register_outcomes",
        [](StateRepository<Kind>& repository, const SuccessorBuilderView<Kind>& successor)
        {
            auto registered = urd::clone(successor).release();
            tyr::planning::register_outcomes(repository, registered);
            return OwnedSuccessorBuilderView<Kind>(std::move(registered), &repository);
        },
        "repository"_a,
        "successor"_a);
    m.def(
        "get_initial_node",
        [](StateRepository<Kind>& repository, AxiomEvaluator<Kind>& axiom_evaluator)
        { return tyr::planning::get_initial_node(repository, axiom_evaluator); },
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
}

}  // namespace tyr::planning

#endif
