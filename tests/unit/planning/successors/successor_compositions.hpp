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

#ifndef TYR_TESTS_UNIT_PLANNING_SUCCESSOR_COMPOSITIONS_HPP_
#define TYR_TESTS_UNIT_PLANNING_SUCCESSOR_COMPOSITIONS_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/nodes/labeled_node_view.hpp"
#include "tyr/planning/nodes/node_concepts.hpp"
#include "tyr/planning/nodes/node_view.hpp"
#include "tyr/planning/states/state_delta.hpp"
#include "tyr/planning/successors/successor_generator.hpp"
#include "tyr/planning/successors/successor_generator_concepts.hpp"
#include "tyr/planning/successors/successors.hpp"

#include <concepts>
#include <initializer_list>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <urd/elements/builder.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <variant>
#include <vector>

// The successors the tests take, composed from a generator and tyr/planning/successors.hpp: single successors, successors
// enumerated (labeled by their interned bindings, or not), as lists, packed, and interned bindings; registered in a state
// repository, or unregistered (closed under the axioms in builders of its pool).

namespace tyr::tests
{
/// The built state a node holds (urd::built_view): what a test reads of a node it built (a generator's successor, a
/// repository's node).
template<TaskKind Kind, typename C, typename R>
planning::StateBuilderView<Kind> built_state(const urd::borrowed_view<planning::Node<Kind>, C, R>& node)
{ return urd::built_view(node.get_state()); }

/// Successors closed under the axioms in builders of `repository`'s pool, not registered (extend_successor).
template<TaskKind Kind>
struct Unregistered
{
    planning::StateRepository<Kind>* repository;
};

/// The repository successors are written through: registering ones, or unregistered ones'.
template<TaskKind Kind>
planning::StateRepository<Kind>& repository_of(planning::StateRepository<Kind>& storage)
{ return storage; }

template<TaskKind Kind>
planning::StateRepository<Kind>& repository_of(const Unregistered<Kind>& storage)
{ return *storage.repository; }

template<TaskKind Kind>
using OwnedNodeBuilderViewList = std::vector<planning::OwnedNodeBuilderView<Kind>>;
template<TaskKind Kind>
using OwnedLabeledNodeBuilderViewList = std::vector<planning::OwnedLabeledNodeBuilderView<Kind>>;
template<TaskKind Kind>
using OwnedNodeRecordViewList = std::vector<planning::OwnedNodeRecordView<Kind>>;
template<TaskKind Kind>
using OwnedLabeledNodeRecordViewList = std::vector<planning::OwnedLabeledNodeRecordView<Kind>>;

/// A binding from outside the generator validated (a lifted row being built), as a caller of the generator does.
template<TaskKind Kind, typename Binding>
void validate_binding(planning::SuccessorGenerator<Kind>& generator, Binding binding)
{
    if constexpr (std::same_as<Kind, LiftedTag> && std::same_as<Binding, formalism::ActionRowBuilderView>)
        generator.validate_action_binding(binding);
}

template<TaskKind Kind, typename Storage>
void validate_storage(planning::SuccessorGenerator<Kind>& generator, Storage& storage, planning::AxiomEvaluator<Kind>& axiom_evaluator)
{
    const auto& repository = repository_of<Kind>(storage);
    if (repository.get_task() != generator.get_task() || axiom_evaluator.get_task() != generator.get_task())
        throw std::invalid_argument("tests: the successor generator, state repository and axiom evaluator must be of one task.");
}

/// The successor of `node` by `binding`, as a node builder: registered in a repository, or closed under the axioms and not
/// registered.
template<TaskKind Kind, typename Binding>
planning::NodeBuilder<Kind> successor_builder(planning::SuccessorGenerator<Kind>& generator,
                                              const planning::NodeBuilderView<Kind>& node,
                                              Binding binding,
                                              planning::StateRepository<Kind>& repository,
                                              planning::AxiomEvaluator<Kind>& axiom_evaluator,
                                              planning::StateDelta& delta)
{
    auto state = ::tyr::planning::get_state_builder(repository);
    const auto auxiliary_value = generator.generate_successor_state(node, binding, *state, delta);
    return planning::register_successor(repository, axiom_evaluator, std::move(state), auxiliary_value, node, delta);
}

template<TaskKind Kind, typename Binding>
planning::NodeBuilder<Kind> successor_builder(planning::SuccessorGenerator<Kind>& generator,
                                              const planning::NodeBuilderView<Kind>& node,
                                              Binding binding,
                                              const Unregistered<Kind>& storage,
                                              planning::AxiomEvaluator<Kind>& axiom_evaluator,
                                              planning::StateDelta& delta)
{
    auto state = ::tyr::planning::get_state_builder(*storage.repository);
    const auto auxiliary_value = generator.generate_successor_state(node, binding, *state, delta);
    return planning::extend_successor(std::move(state), auxiliary_value, node, axiom_evaluator);
}

template<TaskKind Kind, typename Binding, typename Storage>
planning::OwnedNodeBuilderView<Kind> get_successor_node(planning::SuccessorGenerator<Kind>& generator,
                                             const std::type_identity_t<planning::NodeBuilderView<Kind>>& node,
                                             Binding binding,
                                             Storage& storage,
                                             planning::AxiomEvaluator<Kind>& axiom_evaluator)
{
    validate_storage(generator, storage, axiom_evaluator);
    validate_binding(generator, binding);
    auto delta = planning::StateDelta {};
    return planning::OwnedNodeBuilderView<Kind>(successor_builder(generator, node, binding, storage, axiom_evaluator, delta), &repository_of<Kind>(storage));
}

/// Like get_successor_node, labeled by `binding`, interned.
template<TaskKind Kind, typename Binding, typename Storage>
planning::OwnedLabeledNodeBuilderView<Kind> get_labeled_successor_node(planning::SuccessorGenerator<Kind>& generator,
                                                            const std::type_identity_t<planning::NodeBuilderView<Kind>>& node,
                                                            Binding binding,
                                                            Storage& storage,
                                                            planning::AxiomEvaluator<Kind>& axiom_evaluator)
{
    validate_storage(generator, storage, axiom_evaluator);
    validate_binding(generator, binding);
    auto delta = planning::StateDelta {};
    auto builder = formalism::Builder {};
    const auto label = [&]
    {
        if constexpr (std::same_as<Binding, formalism::ActionRowView>)
            return binding;
        else
            return planning::intern_action_binding(generator, builder, binding);
    }();
    auto labeled = planning::LabeledNodeBuilder<Kind> { label.index(), successor_builder(generator, node, binding, storage, axiom_evaluator, delta) };
    return planning::OwnedLabeledNodeBuilderView<Kind>(std::move(labeled), &repository_of<Kind>(storage));
}

template<TaskKind Kind, typename Binding>
planning::OwnedNodeRecordView<Kind> get_packed_successor_node(planning::SuccessorGenerator<Kind>& generator,
                                                     const std::type_identity_t<planning::NodeBuilderView<Kind>>& node,
                                                     Binding binding,
                                                     planning::StateRepository<Kind>& repository,
                                                     planning::AxiomEvaluator<Kind>& axiom_evaluator)
{ return planning::OwnedNodeRecordView<Kind>(get_successor_node(generator, node, binding, repository, axiom_evaluator).pack(), &repository); }

/// Calls `callback` with the successor (an OwnedNodeBuilderView) of each applicable binding `filter` reports.
template<TaskKind Kind, typename Storage, typename Callback>
bool for_each_successor_node(planning::SuccessorGenerator<Kind>& generator,
                             const std::type_identity_t<planning::NodeBuilderView<Kind>>& node,
                             const planning::ActionBindingFilter<Kind>& filter,
                             Storage& storage,
                             planning::AxiomEvaluator<Kind>& axiom_evaluator,
                             const Callback& callback)
{
    validate_storage(generator, storage, axiom_evaluator);
    auto delta = planning::StateDelta {};
    auto builder = formalism::Builder {};
    return generator.for_each_applicable_action_binding(
        node,
        filter,
        [&](planning::ActionBindingView<Kind> binding)
        {
            return callback(
                planning::OwnedNodeBuilderView<Kind>(successor_builder(generator, node, binding, storage, axiom_evaluator, delta), &repository_of<Kind>(storage)));
        });
}

/// Like for_each_successor_node, each successor labeled by its binding, interned (an OwnedLabeledNodeBuilderView).
template<TaskKind Kind, typename Storage, typename Callback>
bool for_each_labeled_successor_node(planning::SuccessorGenerator<Kind>& generator,
                                     const std::type_identity_t<planning::NodeBuilderView<Kind>>& node,
                                     const planning::ActionBindingFilter<Kind>& filter,
                                     Storage& storage,
                                     planning::AxiomEvaluator<Kind>& axiom_evaluator,
                                     const Callback& callback)
{
    validate_storage(generator, storage, axiom_evaluator);
    auto delta = planning::StateDelta {};
    auto builder = formalism::Builder {};
    return generator.for_each_applicable_action_binding(
        node,
        filter,
        [&](planning::ActionBindingView<Kind> borrowed)
        {
            const auto binding = planning::intern_action_binding(generator, builder, borrowed);
            auto labeled = planning::LabeledNodeBuilder<Kind> { binding.index(), successor_builder(generator, node, binding, storage, axiom_evaluator, delta) };
            return callback(planning::OwnedLabeledNodeBuilderView<Kind>(std::move(labeled), &repository_of<Kind>(storage)));
        });
}

template<TaskKind Kind, typename Storage>
void get_successor_nodes(planning::SuccessorGenerator<Kind>& generator,
                         const std::type_identity_t<planning::NodeBuilderView<Kind>>& node,
                         const planning::ActionBindingFilter<Kind>& filter,
                         Storage& storage,
                         planning::AxiomEvaluator<Kind>& axiom_evaluator,
                         OwnedNodeBuilderViewList<Kind>& out_nodes)
{
    out_nodes.clear();
    for_each_successor_node(generator,
                            node,
                            filter,
                            storage,
                            axiom_evaluator,
                            [&](planning::OwnedNodeBuilderView<Kind> successor)
                            {
                                out_nodes.push_back(std::move(successor));
                                return true;
                            });
}

template<TaskKind Kind, typename Storage>
OwnedNodeBuilderViewList<Kind> get_successor_nodes(planning::SuccessorGenerator<Kind>& generator,
                                     const std::type_identity_t<planning::NodeBuilderView<Kind>>& node,
                                     const planning::ActionBindingFilter<Kind>& filter,
                                     Storage& storage,
                                     planning::AxiomEvaluator<Kind>& axiom_evaluator)
{
    auto result = OwnedNodeBuilderViewList<Kind> {};
    get_successor_nodes(generator, node, filter, storage, axiom_evaluator, result);
    return result;
}

template<TaskKind Kind, typename Storage>
void get_labeled_successor_nodes(planning::SuccessorGenerator<Kind>& generator,
                                 const std::type_identity_t<planning::NodeBuilderView<Kind>>& node,
                                 const planning::ActionBindingFilter<Kind>& filter,
                                 Storage& storage,
                                 planning::AxiomEvaluator<Kind>& axiom_evaluator,
                                 OwnedLabeledNodeBuilderViewList<Kind>& out_nodes)
{
    out_nodes.clear();
    for_each_labeled_successor_node(generator,
                                    node,
                                    filter,
                                    storage,
                                    axiom_evaluator,
                                    [&](planning::OwnedLabeledNodeBuilderView<Kind> successor)
                                    {
                                        out_nodes.push_back(std::move(successor));
                                        return true;
                                    });
}

template<TaskKind Kind, typename Storage>
OwnedLabeledNodeBuilderViewList<Kind> get_labeled_successor_nodes(planning::SuccessorGenerator<Kind>& generator,
                                                    const std::type_identity_t<planning::NodeBuilderView<Kind>>& node,
                                                    const planning::ActionBindingFilter<Kind>& filter,
                                                    Storage& storage,
                                                    planning::AxiomEvaluator<Kind>& axiom_evaluator)
{
    auto result = OwnedLabeledNodeBuilderViewList<Kind> {};
    get_labeled_successor_nodes(generator, node, filter, storage, axiom_evaluator, result);
    return result;
}

template<TaskKind Kind>
OwnedNodeRecordViewList<Kind> get_packed_successor_nodes(planning::SuccessorGenerator<Kind>& generator,
                                             const std::type_identity_t<planning::NodeBuilderView<Kind>>& node,
                                             const planning::ActionBindingFilter<Kind>& filter,
                                             planning::StateRepository<Kind>& repository,
                                             planning::AxiomEvaluator<Kind>& axiom_evaluator)
{
    auto result = OwnedNodeRecordViewList<Kind> {};
    for_each_successor_node(generator,
                            node,
                            filter,
                            repository,
                            axiom_evaluator,
                            [&](const planning::OwnedNodeBuilderView<Kind>& successor)
                            {
                                result.emplace_back(successor.pack(), &repository);
                                return true;
                            });
    return result;
}

template<TaskKind Kind>
OwnedLabeledNodeRecordViewList<Kind> get_packed_labeled_successor_nodes(planning::SuccessorGenerator<Kind>& generator,
                                                            const std::type_identity_t<planning::NodeBuilderView<Kind>>& node,
                                                            const planning::ActionBindingFilter<Kind>& filter,
                                                            planning::StateRepository<Kind>& repository,
                                                            planning::AxiomEvaluator<Kind>& axiom_evaluator)
{
    auto result = OwnedLabeledNodeRecordViewList<Kind> {};
    for_each_labeled_successor_node(generator,
                                    node,
                                    filter,
                                    repository,
                                    axiom_evaluator,
                                    [&](const planning::OwnedLabeledNodeBuilderView<Kind>& successor)
                                    {
                                        result.emplace_back(successor.pack(), &repository);
                                        return true;
                                    });
    return result;
}

/// The applicable bindings `filter` reports, interned.
using ActionBindings = urd::Store<urd::sequence<formalism::ActionRowView>>;

template<TaskKind Kind, typename Callback>
bool for_each_applicable_action_binding(planning::SuccessorGenerator<Kind>& generator,
                                        const std::type_identity_t<planning::NodeBuilderView<Kind>>& node,
                                        const planning::ActionBindingFilter<Kind>& filter,
                                        const Callback& callback)
{
    auto builder = formalism::Builder {};
    return generator.for_each_applicable_action_binding(node,
                                                        filter,
                                                        [&](planning::ActionBindingView<Kind> binding)
                                                        { return callback(planning::intern_action_binding(generator, builder, binding)); });
}

template<TaskKind Kind>
void get_applicable_action_bindings(planning::SuccessorGenerator<Kind>& generator,
                                    const std::type_identity_t<planning::NodeBuilderView<Kind>>& node,
                                    const planning::ActionBindingFilter<Kind>& filter,
                                    ActionBindings& out_bindings)
{
    out_bindings.clear();
    for_each_applicable_action_binding(generator,
                                       node,
                                       filter,
                                       [&](formalism::ActionRowView binding)
                                       {
                                           out_bindings.push_back(binding);
                                           return true;
                                       });
}

template<TaskKind Kind>
ActionBindings get_applicable_action_bindings(planning::SuccessorGenerator<Kind>& generator,
                                              const std::type_identity_t<planning::NodeBuilderView<Kind>>& node,
                                              const planning::ActionBindingFilter<Kind>& filter)
{
    auto result = ActionBindings {};
    get_applicable_action_bindings(generator, node, filter, result);
    return result;
}

/// The status of `binding` (check_action_binding), and the binding interned where it is applicable.
template<TaskKind Kind, typename Binding>
planning::ActionBindingResult
try_get_applicable_action_binding(planning::SuccessorGenerator<Kind>& generator, const std::type_identity_t<planning::NodeBuilderView<Kind>>& node, Binding binding)
{
    const auto status = generator.check_action_binding(node, binding);
    if (status != planning::ActionBindingStatus::APPLICABLE)
        return { status, std::nullopt };
    if constexpr (std::same_as<Binding, formalism::ActionRowView>)
    {
        return { status, binding };
    }
    else if constexpr (std::same_as<Kind, GroundTag>)
    {
        // An applicable ground binding names a ground action of the task: its row is interned already.
        auto row = urd::make<ygg::Row<formalism::Action<LiftedTag>>>(planning::action_of(binding), binding.get_values());
        return { status, generator.get_task()->get_repository()->find(row) };
    }
    else
    {
        auto builder = formalism::Builder {};
        return { status, planning::intern_action_binding(generator, builder, binding) };
    }
}

/// A node of `repository` owned on its own: the initial node, a registered state's node, a packed node unpacked.
template<TaskKind Kind>
planning::OwnedNodeBuilderView<Kind> owned_initial_node(planning::StateRepository<Kind>& repository, planning::AxiomEvaluator<Kind>& axiom_evaluator)
{ return planning::get_initial_node(repository, axiom_evaluator); }

template<TaskKind Kind, typename Key>
planning::OwnedNodeBuilderView<Kind> owned_node(const planning::StateRepository<Kind>& repository, const Key& key)
{ return planning::OwnedNodeBuilderView<Kind>(planning::get_node(repository, key), &repository); }

/// The initial node of `repository`, packed, with metric `metric` (a search's start node).
template<TaskKind Kind>
planning::OwnedNodeRecordView<Kind>
packed_initial_node(planning::StateRepository<Kind>& repository, planning::AxiomEvaluator<Kind>& axiom_evaluator, ygg::float_t metric)
{
    const auto initial = owned_initial_node(repository, axiom_evaluator);
    return planning::OwnedNodeRecordView<Kind>(urd::data<planning::Node<Kind>>(tyr::tests::built_state(initial).pack().index(), metric), &repository);
}

/// A node of `state` (any view of it: registered or not) with metric `metric`: the state copied into a builder of its
/// repository's pool, its index as it is.
template<TaskKind Kind>
planning::OwnedNodeBuilderView<Kind> node_of_state(const std::type_identity_t<planning::StateBuilderView<Kind>>& state, ygg::float_t metric)
{
    auto builder = ::tyr::planning::get_state_builder(state.context());
    builder->assign(state);
    return planning::OwnedNodeBuilderView<Kind>(planning::NodeBuilder<Kind> { std::move(builder), metric }, &state.context());
}

/// A packed node unpacked: its state unpacked from its repository, and its metric.
template<TaskKind Kind>
planning::OwnedNodeBuilderView<Kind> unpacked(const planning::OwnedNodeRecordView<Kind>& node)
{
    return planning::OwnedNodeBuilderView<Kind>(planning::get_node(node.context(), urd::data<planning::Node<Kind>>(node.get_state().index(), node.get_metric())),
                                     &node.context());
}

template<TaskKind Kind>
planning::OwnedLabeledNodeBuilderView<Kind> unpacked(const planning::OwnedLabeledNodeRecordView<Kind>& node)
{
    const auto packed_node = node.get_node();
    const auto record = urd::data<planning::LabeledNode<Kind>>(node.get_label().index(),
                                                               urd::data<planning::Node<Kind>>(packed_node.get_state().index(), packed_node.get_metric()));
    return planning::OwnedLabeledNodeBuilderView<Kind>(planning::get_labeled_node(node.context(), record), &node.context());
}

/// A node of a builder, packed and kept apart from it (a start or goal node a search takes or reports).
template<TaskKind Kind>
planning::OwnedNodeRecordView<Kind> packed(const std::type_identity_t<planning::NodeBuilderView<Kind>>& node)
{ return planning::OwnedNodeRecordView<Kind>(node.pack(), &node.context()); }

/// The plan from `start` through the labeled nodes `steps` (records), unpacked from `start`'s repository.
template<TaskKind Kind>
planning::OwnedPlanBuilderView<Kind> make_plan(const std::type_identity_t<planning::NodeBuilderView<Kind>>& start,
                                    std::initializer_list<urd::data<planning::LabeledNode<Kind>>> steps)
{
    const auto& repository = start.context();
    auto plan = planning::PlanBuilder<Kind>();
    plan.start_node = planning::get_node(repository, start.pack());
    for (const auto& step : steps)
        plan.labeled_succ_nodes.push_back(planning::get_labeled_node(repository, step));
    return planning::OwnedPlanBuilderView<Kind>(std::move(plan), &repository);
}

/// A labeled node's record from its label and its node.
template<TaskKind Kind>
urd::data<planning::LabeledNode<Kind>> labeled_record(formalism::ActionRowView label, const std::type_identity_t<planning::NodeBuilderView<Kind>>& node)
{ return urd::data<planning::LabeledNode<Kind>>(label.index(), node.pack()); }
}

#endif
