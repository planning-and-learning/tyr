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

#include "tyr/planning/successors/successors.hpp"

#include "../metric.hpp"
#include "tyr/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/planning/successors/applicability_decl.hpp"
#include "tyr/planning/axioms/axiom_evaluator.hpp"
#include "tyr/planning/successors/ground/successor_generator.hpp"
#include "tyr/planning/nodes/labeled_node_view.hpp"
#include "tyr/planning/successors/lifted/successor_generator.hpp"
#include "tyr/planning/nodes/node_view.hpp"
#include "tyr/planning/states/state_delta.hpp"
#include "tyr/planning/states/state_repository.hpp"
#include "tyr/planning/successors/successor_generator_concepts.hpp"
#include "tyr/planning/tasks/task.hpp"

#include <functional>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <urd/elements/view.hpp>
#include <utility>
#include <vector>
#include <variant>

namespace tyr::planning
{

template<TaskKind Kind>
formalism::ActionRowView intern_action_binding(SuccessorGenerator<Kind>& generator, formalism::Builder& builder, ActionBindingView<Kind> binding)
{
    generator.validate_action_binding(binding);
    if constexpr (std::same_as<Kind, GroundTag>)
    {
        return binding;
    }
    else
    {
        auto row = builder.template scratch<ygg::Row<formalism::Action<LiftedTag>>>();
        row->relation = action_of(binding).index();
        row->values.clear();
        for (const auto value : binding.get_values())
            row->values.push_back(value);
        return generator.get_task()->get_repository()->insert(*row).first;
    }
}

namespace
{
/// A node of `state` and its metric.
template<TaskKind Kind>
NodeBuilder<Kind> node_of(StateBuilderHandle<Kind> state, ygg::float_t metric)
{
    auto node = NodeBuilder<Kind>();
    node.state = std::move(state);
    node.metric = metric;
    return node;
}

/// The metric of a registered state of `repository` (its auxiliary value from the task's initial one).
template<TaskKind Kind>
ygg::float_t state_metric(const StateRepository<Kind>& repository, const urd::builder<State<Kind>, StateRepository<Kind>>& state)
{
    const auto& task = *repository.get_task();
    const auto state_context = StateContext<Kind>(repository, state, 0);
    return evaluate_metric(task.get_task().get_metric(), task.get_task().get_auxiliary_fterm_value(), state_context);
}
}

template<TaskKind Kind>
NodeBuilder<Kind> register_successor(StateRepository<Kind>& repository,
                                     AxiomEvaluator<Kind>& axiom_evaluator,
                                     StateBuilderHandle<Kind> state,
                                     ygg::float_t auxiliary_value,
                                     const std::type_identity_t<NodeBuilderView<Kind>>& node,
                                     const StateDelta& delta)
{
    const auto metric = evaluate_successor_metric(repository, *state, auxiliary_value);
    if (is_registered(node))
        ::tyr::planning::register_state(repository, axiom_evaluator, *state, node.get_state(), delta);
    else
        ::tyr::planning::register_state(repository, axiom_evaluator, *state);
    return node_of<Kind>(std::move(state), metric);
}

template<TaskKind Kind>
NodeBuilder<Kind> extend_successor(StateBuilderHandle<Kind> state,
                                   ygg::float_t auxiliary_value,
                                   const std::type_identity_t<NodeBuilderView<Kind>>& node,
                                   AxiomEvaluator<Kind>& axiom_evaluator)
{
    const auto& repository = node.context();
    const auto metric = evaluate_successor_metric(repository, *state, auxiliary_value);
    axiom_evaluator.compute_extended_state(*state, repository);
    return node_of<Kind>(std::move(state), metric);
}

template<TaskKind Kind>
void register_outcomes(StateRepository<Kind>& repository, SuccessorBuilder<Kind>& successor)
{
    for (auto& distribution : successor.distributions)
        for (auto& outcome : distribution->outcomes)
            if (!outcome->failed)
                ::tyr::planning::register_extended_state(repository, *outcome->node.state);
}

template<TaskKind Kind>
OwnedNodeBuilderView<Kind> get_initial_node(StateRepository<Kind>& repository, AxiomEvaluator<Kind>& axiom_evaluator)
{
    auto state = ::tyr::planning::get_initial_state(repository, axiom_evaluator);
    const auto metric = state_metric(repository, *state);
    return OwnedNodeBuilderView<Kind>(node_of<Kind>(std::move(state), metric), &repository);
}

template<TaskKind Kind>
NodeBuilder<Kind> get_node(const StateRepository<Kind>& repository, urd::index<State<Kind>> state_index)
{
    auto state = ::tyr::planning::get_registered_state(repository, state_index);
    const auto metric = state_metric(repository, *state);
    return node_of<Kind>(std::move(state), metric);
}

template<TaskKind Kind>
NodeBuilder<Kind> get_node(const StateRepository<Kind>& repository, const urd::data<Node<Kind>>& node)
{ return node_of<Kind>(::tyr::planning::get_registered_state(repository, node.state), node.metric); }

template<TaskKind Kind>
LabeledNodeBuilder<Kind> get_labeled_node(const StateRepository<Kind>& repository, const urd::data<LabeledNode<Kind>>& node)
{
    auto labeled = LabeledNodeBuilder<Kind>();
    labeled.label.template emplace<0>(node.label);
    labeled.node = get_node(repository, node.node);
    return labeled;
}

template<TaskKind Kind>
PlanBuilder<Kind> replay_plan(const urd::data<Node<Kind>>& initial,
                              std::span<const formalism::ActionRowView> actions,
                              SuccessorGenerator<Kind>& generator,
                              StateRepository<Kind>& states,
                              AxiomEvaluator<Kind>& axioms)
{
    auto plan = PlanBuilder<Kind>();
    plan.start_node = get_node(states, initial);
    plan.labeled_succ_nodes.reserve(actions.size());
    auto delta = StateDelta {};
    for (const auto action : actions)
    {
        const auto& source = plan.labeled_succ_nodes.empty() ? plan.start_node : plan.labeled_succ_nodes.back().node;
        const auto source_view = urd::resolve(source, std::as_const(states));
        auto state = ::tyr::planning::get_state_builder(states);
        const auto auxiliary_value = generator.generate_successor_state(source_view, action, *state, delta);
        auto node = register_successor(states, axioms, std::move(state), auxiliary_value, source_view, delta);
        plan.labeled_succ_nodes.push_back(LabeledNodeBuilder<Kind> { action.index(), std::move(node) });
    }
    return plan;
}


namespace
{
/// Throws unless `node` reads through `repository`: a successor's states are builders of its node's repository.
template<TaskKind Kind>
void check_repository(const StateRepository<Kind>& repository, const NodeBuilderView<Kind>& node)
{
    if (&node.context() != &repository)
        throw std::invalid_argument("successors: the node belongs to a different state repository.");
}
}

template<TaskKind Kind>
std::vector<OwnedSuccessorBuilderView<Kind>> get_successors(SuccessorGenerator<Kind>& generator,
                                                            StateRepository<Kind>& repository,
                                                            AxiomEvaluator<Kind>& axiom_evaluator,
                                                            const std::type_identity_t<NodeBuilderView<Kind>>& node)
{
    check_repository(repository, node);
    auto result = std::vector<OwnedSuccessorBuilderView<Kind>> {};
    auto builder = formalism::Builder {};
    generator.for_each_successor(node,
                                 axiom_evaluator,
                                 [&](SuccessorBuilder<Kind>& successor)
                                 {
                                     register_outcomes(repository, successor);
                                     intern_label(repository, builder, successor);
                                     result.emplace_back(std::move(successor), &repository);
                                     return true;
                                 });
    return result;
}

template<TaskKind Kind>
bool for_each_successor(SuccessorGenerator<Kind>& generator,
                        const std::type_identity_t<NodeBuilderView<Kind>>& node,
                        AxiomEvaluator<Kind>& axiom_evaluator,
                        const std::type_identity_t<std::function<bool(SuccessorBuilder<Kind>&)>>& callback)
{ return generator.for_each_successor(node, axiom_evaluator, callback); }

template<TaskKind Kind>
void intern_label(StateRepository<Kind>& repository, formalism::Builder& builder, SuccessorBuilder<Kind>& successor)
{
    // A label with a binding of its own (one not interned yet).
    auto& label = successor.label;
    if (!label.binding.has_value() || label.binding->index() == 0)
        return;
    auto& binding = *label.binding;
    auto scratch = builder.template scratch<ygg::Row<formalism::Action<LiftedTag>>>();
    binding.template emplace<0>(repository.get_task()->get_repository()->insert(urd::child_record(binding, *scratch)).first.index());
}

template<TaskKind Kind>
bool for_each_applicable_action_binding(SuccessorGenerator<Kind>& generator,
                                        const std::type_identity_t<NodeBuilderView<Kind>>& node,
                                        const std::type_identity_t<std::function<bool(ActionBindingView<Kind>)>>& callback,
                                        const std::type_identity_t<ActionBindingFilter<Kind>>& filter)
{ return generator.for_each_applicable_action_binding(node, filter, callback); }

template<TaskKind Kind, formalism::ActionRowViewConcept Binding>
SuccessorState<Kind> generate_successor_state(SuccessorGenerator<Kind>& generator,
                                              const StateRepository<Kind>& repository,
                                              const std::type_identity_t<NodeBuilderView<Kind>>& node,
                                              Binding binding)
{
    check_repository(repository, node);
    auto result = SuccessorState<Kind> { ::tyr::planning::get_state_builder(repository), 0, StateDelta {} };
    result.auxiliary_value = generator.generate_successor_state(node, binding, *result.state, result.delta);
    return result;
}

template<TaskKind Kind>
NodeBuilder<Kind> register_successor(StateRepository<Kind>& repository,
                                     AxiomEvaluator<Kind>& axiom_evaluator,
                                     const std::type_identity_t<NodeBuilderView<Kind>>& node,
                                     SuccessorState<Kind> successor_state)
{
    check_repository(repository, node);
    return register_successor(repository, axiom_evaluator, std::move(successor_state.state), successor_state.auxiliary_value, node, successor_state.delta);
}

template std::vector<OwnedSuccessorBuilderView<GroundTag>>
get_successors(SuccessorGenerator<GroundTag>&, StateRepository<GroundTag>&, AxiomEvaluator<GroundTag>&, const NodeBuilderView<GroundTag>&);
template bool for_each_successor(SuccessorGenerator<GroundTag>&,
                                 const NodeBuilderView<GroundTag>&,
                                 AxiomEvaluator<GroundTag>&,
                                 const std::function<bool(SuccessorBuilder<GroundTag>&)>&);
template void intern_label(StateRepository<GroundTag>&, formalism::Builder&, SuccessorBuilder<GroundTag>&);
template bool for_each_applicable_action_binding(SuccessorGenerator<GroundTag>&,
                                                 const NodeBuilderView<GroundTag>&,
                                                 const std::function<bool(ActionBindingView<GroundTag>)>&,
                                                 const ActionBindingFilter<GroundTag>&);
template SuccessorState<GroundTag> generate_successor_state(SuccessorGenerator<GroundTag>&, const StateRepository<GroundTag>&, const NodeBuilderView<GroundTag>&, formalism::ActionRowView);
template SuccessorState<GroundTag>
generate_successor_state(SuccessorGenerator<GroundTag>&, const StateRepository<GroundTag>&, const NodeBuilderView<GroundTag>&, formalism::ActionRowBuilderView);
template NodeBuilder<GroundTag> register_successor(StateRepository<GroundTag>&, AxiomEvaluator<GroundTag>&, const NodeBuilderView<GroundTag>&, SuccessorState<GroundTag>);
template std::vector<OwnedSuccessorBuilderView<LiftedTag>>
get_successors(SuccessorGenerator<LiftedTag>&, StateRepository<LiftedTag>&, AxiomEvaluator<LiftedTag>&, const NodeBuilderView<LiftedTag>&);
template bool for_each_successor(SuccessorGenerator<LiftedTag>&,
                                 const NodeBuilderView<LiftedTag>&,
                                 AxiomEvaluator<LiftedTag>&,
                                 const std::function<bool(SuccessorBuilder<LiftedTag>&)>&);
template void intern_label(StateRepository<LiftedTag>&, formalism::Builder&, SuccessorBuilder<LiftedTag>&);
template bool for_each_applicable_action_binding(SuccessorGenerator<LiftedTag>&,
                                                 const NodeBuilderView<LiftedTag>&,
                                                 const std::function<bool(ActionBindingView<LiftedTag>)>&,
                                                 const ActionBindingFilter<LiftedTag>&);
template SuccessorState<LiftedTag> generate_successor_state(SuccessorGenerator<LiftedTag>&, const StateRepository<LiftedTag>&, const NodeBuilderView<LiftedTag>&, formalism::ActionRowView);
template SuccessorState<LiftedTag>
generate_successor_state(SuccessorGenerator<LiftedTag>&, const StateRepository<LiftedTag>&, const NodeBuilderView<LiftedTag>&, formalism::ActionRowBuilderView);
template NodeBuilder<LiftedTag> register_successor(StateRepository<LiftedTag>&, AxiomEvaluator<LiftedTag>&, const NodeBuilderView<LiftedTag>&, SuccessorState<LiftedTag>);

template formalism::ActionRowView intern_action_binding(SuccessorGenerator<GroundTag>&, formalism::Builder&, ActionBindingView<GroundTag>);
template NodeBuilder<GroundTag> register_successor(StateRepository<GroundTag>&,
                                                   AxiomEvaluator<GroundTag>&,
                                                   StateBuilderHandle<GroundTag>,
                                                   ygg::float_t,
                                                   const NodeBuilderView<GroundTag>&,
                                                   const StateDelta&);
template NodeBuilder<GroundTag> extend_successor(StateBuilderHandle<GroundTag>, ygg::float_t, const NodeBuilderView<GroundTag>&, AxiomEvaluator<GroundTag>&);
template void register_outcomes(StateRepository<GroundTag>&, SuccessorBuilder<GroundTag>&);
template OwnedNodeBuilderView<GroundTag> get_initial_node(StateRepository<GroundTag>&, AxiomEvaluator<GroundTag>&);
template NodeBuilder<GroundTag> get_node(const StateRepository<GroundTag>&, urd::index<State<GroundTag>>);
template NodeBuilder<GroundTag> get_node(const StateRepository<GroundTag>&, const urd::data<Node<GroundTag>>&);
template LabeledNodeBuilder<GroundTag> get_labeled_node(const StateRepository<GroundTag>&, const urd::data<LabeledNode<GroundTag>>&);
template PlanBuilder<GroundTag> replay_plan(const urd::data<Node<GroundTag>>&,
                                       std::span<const formalism::ActionRowView>,
                                       SuccessorGenerator<GroundTag>&,
                                       StateRepository<GroundTag>&,
                                       AxiomEvaluator<GroundTag>&);

template formalism::ActionRowView intern_action_binding(SuccessorGenerator<LiftedTag>&, formalism::Builder&, ActionBindingView<LiftedTag>);
template NodeBuilder<LiftedTag> register_successor(StateRepository<LiftedTag>&,
                                                   AxiomEvaluator<LiftedTag>&,
                                                   StateBuilderHandle<LiftedTag>,
                                                   ygg::float_t,
                                                   const NodeBuilderView<LiftedTag>&,
                                                   const StateDelta&);
template NodeBuilder<LiftedTag> extend_successor(StateBuilderHandle<LiftedTag>, ygg::float_t, const NodeBuilderView<LiftedTag>&, AxiomEvaluator<LiftedTag>&);
template void register_outcomes(StateRepository<LiftedTag>&, SuccessorBuilder<LiftedTag>&);
template OwnedNodeBuilderView<LiftedTag> get_initial_node(StateRepository<LiftedTag>&, AxiomEvaluator<LiftedTag>&);
template NodeBuilder<LiftedTag> get_node(const StateRepository<LiftedTag>&, urd::index<State<LiftedTag>>);
template NodeBuilder<LiftedTag> get_node(const StateRepository<LiftedTag>&, const urd::data<Node<LiftedTag>>&);
template LabeledNodeBuilder<LiftedTag> get_labeled_node(const StateRepository<LiftedTag>&, const urd::data<LabeledNode<LiftedTag>>&);
template PlanBuilder<LiftedTag> replay_plan(const urd::data<Node<LiftedTag>>&,
                                       std::span<const formalism::ActionRowView>,
                                       SuccessorGenerator<LiftedTag>&,
                                       StateRepository<LiftedTag>&,
                                       AxiomEvaluator<LiftedTag>&);

}
