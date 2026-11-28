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
#include "tyr/planning/axioms/axiom_evaluator.hpp"
#include "tyr/planning/nodes/labeled_node_view.hpp"
#include "tyr/planning/nodes/node_view.hpp"
#include "tyr/planning/states/state_delta.hpp"
#include "tyr/planning/states/state_repository.hpp"
#include "tyr/planning/successors/applicability_decl.hpp"
#include "tyr/planning/successors/ground/successor_generator.hpp"
#include "tyr/planning/successors/lifted/successor_generator.hpp"
#include "tyr/planning/successors/successor_generator_concepts.hpp"
#include "tyr/planning/tasks/task.hpp"

#include <cstddef>
#include <cstdint>
#include <fmt/format.h>
#include <functional>
#include <memory>
#include <numeric>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <urd/elements/view.hpp>
#include <utility>
#include <variant>
#include <vector>

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
        // The borrowed binding's builder, interned as the repository interns a view of one.
        return generator.get_task()->get_repository()->insert(binding, builder).first;
    }
}

namespace
{
/// A node of `state` and its metric.
template<TaskKind Kind>
NodeBuilder<Kind> node_of(StateBuilderHandle<Kind> state, ygg::float_t metric)
{
    // Its state as given: a default-made node would make a state builder of its own first.
    return NodeBuilder<Kind> { std::move(state), metric };
}

/// The metric of a registered state of `repository` (its auxiliary value from the task's initial one).
template<TaskKind Kind>
ygg::float_t state_metric(const StateRepository<Kind>& repository, const urd::builder<State<Kind>, StateRepository<Kind>>& state)
{
    const auto& task = *repository.get_task();
    const auto state_context = StateContext<Kind>(repository, state, 0);
    return evaluate_metric(task.get_task().metric(), task.get_task().auxiliary_fterm_value(), state_context);
}

/// Throws unless `node` reads through `repository`: a successor's states are builders of its node's repository.
template<TaskKind Kind>
void check_repository(const StateRepository<Kind>& repository, const NodeBuilderView<Kind>& node)
{
    if (&node.context() != &repository)
        throw std::invalid_argument("successors: the node belongs to a different state repository.");
}
}

template<TaskKind Kind>
bool find_successor(StateRepository<Kind>& repository, const std::type_identity_t<NodeBuilderView<Kind>>& node, SuccessorState<Kind>& successor_state)
{ return ::tyr::planning::find_state(repository, *successor_state.state, node.state(), successor_state.delta); }

template<TaskKind Kind>
void extend_successor(AxiomEvaluator<Kind>& axiom_evaluator, const std::type_identity_t<NodeBuilderView<Kind>>& node, SuccessorState<Kind>& successor_state)
{
    // A state that has its index is registered: the lookup gave it its derived atoms.
    if (successor_state.state->index.is_none())
        axiom_evaluator.compute_extended_state(*successor_state.state, node.context());
}

template<TaskKind Kind>
NodeBuilder<Kind>
register_successor(StateRepository<Kind>& repository, const std::type_identity_t<NodeBuilderView<Kind>>& node, SuccessorState<Kind>& successor_state)
{
    auto& state = *successor_state.state;
    const auto metric = evaluate_successor_metric(repository, state, successor_state.auxiliary_value);
    if (state.index.is_none())
        ::tyr::planning::register_extended_state(repository, state, node.state(), successor_state.delta);
    return node_of<Kind>(std::move(successor_state.state), metric);
}

template<TaskKind Kind>
NodeBuilder<Kind> successor_node(const std::type_identity_t<NodeBuilderView<Kind>>& node, SuccessorState<Kind>& successor_state)
{
    const auto metric = evaluate_successor_metric(node.context(), *successor_state.state, successor_state.auxiliary_value);
    return node_of<Kind>(std::move(successor_state.state), metric);
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
    urd::store(labeled.label, node.label);
    labeled.node = get_node(repository, node.node);
    return labeled;
}

template<TaskKind Kind>
PlanBuilder<DeterministicTag, Kind> replay_plan(const urd::data<Node<Kind>>& initial,
                                                std::span<const formalism::ActionRowView> actions,
                                                SuccessorGenerator<Kind>& generator,
                                                StateRepository<Kind>& states,
                                                AxiomEvaluator<Kind>& axioms)
{
    auto plan = PlanBuilder<DeterministicTag, Kind>();
    plan.start_node = get_node(states, initial);
    plan.labeled_succ_nodes.reserve(actions.size());
    auto successor = SuccessorState<Kind> {};
    for (const auto action : actions)
    {
        const auto& source = plan.labeled_succ_nodes.empty() ? plan.start_node : plan.labeled_succ_nodes.back().node;
        const auto source_view = urd::resolve(source, std::as_const(states));
        successor.state = ::tyr::planning::get_state_builder(states);
        successor.auxiliary_value = generator.generate_successor_state(source_view, action, *successor.state, successor.delta);
        find_successor(states, source_view, successor);
        extend_successor(axioms, source_view, successor);
        auto node = register_successor(states, source_view, successor);
        plan.labeled_succ_nodes.push_back(LabeledNodeBuilder<Kind> { action.index(), std::move(node) });
    }
    return plan;
}

namespace
{
template<Environment E>
constexpr std::string_view environment_name() noexcept
{
    if constexpr (std::same_as<E, DeterministicTag>)
        return "deterministic";
    else if constexpr (std::same_as<E, NondeterministicTag>)
        return "nondeterministic";
    else if constexpr (std::same_as<E, ProbabilisticTag>)
        return "probabilistic";
    else
        return "general";
}
}

template<Environment E, TaskKind Kind>
void require_environment(const SuccessorGenerator<Kind>& generator)
{
    const auto features = generator.features();
    if (!supports<E>(features))
        throw std::invalid_argument(fmt::format("successors: the {} environment does not give this task its successors: it has {}{}{} effects.",
                                                environment_name<E>(),
                                                features.oneof ? "oneof" : "",
                                                features.oneof && features.probabilistic ? " and " : "",
                                                features.probabilistic ? "probabilistic" : ""));
}

namespace
{
/// Calls `visit(label)` with each transition `node` offers that `filter` lets through, labels only (nothing is generated;
/// false stops): its applicable action bindings (an instantaneous action applied, a durative one started), then, in a
/// temporal task and unless the filter names an action, the end of each open action and the next timed initial literals
/// (whether those can happen is decided when they are generated). Whether every transition was visited.
template<TaskKind Kind, typename Visit>
bool for_each_label(SuccessorGenerator<Kind>& generator, const NodeBuilderView<Kind>& node, const ActionBindingFilter<Kind>& filter, const Visit& visit)
{
    const auto temporal = generator.features().temporal;
    // Passed by reference (std::ref): a std::function of the lambda itself would allocate for its captures.
    const auto offer = [&](ActionBindingView<Kind> binding)
    {
        const auto starts = temporal && action_of(binding).temporal_part().has_value();
        return visit(TransitionLabel<Kind> { starts ? TransitionKind::START : TransitionKind::APPLY, binding, 0 });
    };
    if (!generator.for_each_applicable_action_binding(node, filter, std::ref(offer)))
        return false;
    if (!temporal || filter.action.has_value())
        return true;

    const auto open_count = static_cast<std::uint32_t>(node.state().temporal_storage().parts().open.size());
    for (std::uint32_t open = 0; open < open_count; ++open)
        if (!visit(TransitionLabel<Kind> { TransitionKind::END, std::nullopt, open }))
            return false;
    return visit(TransitionLabel<Kind> { TransitionKind::TIMED_LITERAL, std::nullopt, 0 });
}

/// The labels of an enumeration kept past it (an arranged one generates after every label is known): a ground binding is
/// its interned row; a lifted one, borrowed only while it is enumerated, is copied into a row of its own.
template<TaskKind Kind>
struct KeptLabels
{
    using Row = urd::builder<ygg::Row<formalism::Action<LiftedTag>>, formalism::Repository>;

    std::vector<std::unique_ptr<Row>> rows;
    std::vector<TransitionLabel<Kind>> labels;

    void keep(const TransitionLabel<Kind>& label, const formalism::Repository& repository)
    {
        if constexpr (std::same_as<Kind, GroundTag>)
        {
            labels.push_back(label);
        }
        else
        {
            if (!label.binding.has_value())
            {
                labels.push_back(label);
            }
            else
            {
                auto& row = *rows.emplace_back(std::make_unique<Row>());
                urd::store(row.relation, action_of(*label.binding));
                for (const auto value : label.binding->values())
                    row.values.push_back(value);
                labels.push_back(TransitionLabel<Kind> { label.kind, urd::resolve(row, repository), label.open });
            }
        }
    }
};

/// Calls `expand(label)` with each transition of `node` the options let through, in the order they are found, or, where
/// the options arrange them, in the order `arrange` leaves (those it keeps). False from `expand` stops; whether the
/// enumeration ran to its end.
template<TaskKind Kind, typename Expand>
bool enumerate(SuccessorGenerator<Kind>& generator, const NodeBuilderView<Kind>& node, const SuccessorOptions<Kind>& options, const Expand& expand)
{
    if (!options.arrange)
        return for_each_label(generator, node, options, expand);

    // ponytail: buffers of the call's own; pooled in the generator if an arranged enumeration (a shuffled search) matters.
    auto kept = KeptLabels<Kind> {};
    const auto& repository = *generator.get_task()->get_repository();
    const auto keep = [&](const TransitionLabel<Kind>& label)
    {
        kept.keep(label, repository);
        return true;
    };
    if (!for_each_label(generator, node, options, keep))
        return false;

    auto order = std::vector<std::size_t>(kept.labels.size());
    std::iota(order.begin(), order.end(), std::size_t { 0 });
    options.arrange(std::span<const TransitionLabel<Kind>>(kept.labels), order);
    for (const auto position : order)
    {
        if (options.stop && options.stop())
            return false;
        if (!expand(kept.labels.at(position)))
            return false;
    }
    return true;
}

/// Whether `outcome`, its state closed under the axioms, is valid: in a temporal task every open durative action's
/// over-all condition holds in it. Marks it failed otherwise.
template<TaskKind Kind, typename Outcome>
bool check_over_all(SuccessorGenerator<Kind>& generator, const StateRepository<Kind>& repository, Outcome& outcome)
{
    if (generator.features().temporal && !generator.over_all_holds(urd::resolve(*outcome.node.state, repository)))
    {
        outcome.failed = true;
        return false;
    }
    return true;
}

/// Calls `visit` with each outcome of `successor` (a general successor's are its distributions') that did not fail.
template<NatureEnvironment E, TaskKind Kind, typename Visit>
void for_each_outcome(SuccessorBuilder<E, Kind>& successor, Visit visit)
{
    if constexpr (std::same_as<E, GeneralTag>)
    {
        for (auto& distribution : successor.distributions)
            for (auto& outcome : distribution->outcomes)
                if (!outcome->failed)
                    visit(*outcome);
    }
    else
    {
        for (auto& outcome : successor.outcomes)
            if (!outcome->failed)
                visit(*outcome);
    }
}

/// The action binding `transition` applies or starts (a transition of another kind has none).
template<TaskKind Kind>
const ActionBindingView<Kind>& binding_of(const TransitionLabel<Kind>& transition)
{
    if (!transition.binding.has_value())
        throw std::invalid_argument("generate_successor: an application and a start have an action binding.");
    return *transition.binding;
}
}

template<Environment E, TaskKind Kind>
bool generate_successor(SuccessorGenerator<Kind>& generator,
                        const std::type_identity_t<NodeBuilderView<Kind>>& node,
                        const std::type_identity_t<TransitionLabel<Kind>>& transition,
                        std::type_identity_t<SuccessorOf<E, Kind>>& out)
{
    if constexpr (std::same_as<E, DeterministicTag>)
    {
        // ponytail: a deterministic successor carries no temporal network yet (a plan's labels are action bindings);
        // temporal transitions join when a labeled node's label is a transition.
        if (transition.kind != TransitionKind::APPLY)
            throw std::invalid_argument("generate_successor<DeterministicTag>: a temporal transition (a durative action's start or end, timed "
                                        "initial literals) has no deterministic successor yet.");
        out.auxiliary_value = generator.generate_successor_state(node, binding_of<Kind>(transition), *out.state, out.delta);
        return true;
    }
    else
    {
        if (transition.kind == TransitionKind::APPLY || transition.kind == TransitionKind::START)
            generator.template generate_successor<E>(node, binding_of<Kind>(transition), out);
        else if (transition.kind == TransitionKind::END)
            generator.template generate_end_successor<E>(node, transition.open, out);
        else
            generator.template generate_timed_literal_successor<E>(node, out);
        return has_outcomes(out);
    }
}

template<Environment E, TaskKind Kind>
bool for_each_successor(SuccessorGenerator<Kind>& generator,
                        const std::type_identity_t<NodeBuilderView<Kind>>& node,
                        const std::type_identity_t<std::function<bool(const TransitionLabel<Kind>&, SuccessorOf<E, Kind>&)>>& callback,
                        const std::type_identity_t<SuccessorOptions<Kind>>& options)
{
    const auto& repository = node.context();
    if constexpr (std::same_as<E, DeterministicTag>)
    {
        // One successor state reused across the transitions (a callback that took its builder gets a fresh one next), its
        // changes' buffers the generator's, reused across calls.
        auto successor = SuccessorState<Kind> {};
        std::swap(successor.delta, generator.scratch_delta());
        const auto expand = [&](const TransitionLabel<Kind>& transition)
        {
            if (successor.state == nullptr)
                successor.state = ::tyr::planning::get_state_builder(repository);
            if (!generate_successor<E>(generator, node, transition, successor))
                return true;
            return callback(transition, successor);
        };
        const auto exhausted = enumerate(generator, node, options, expand);
        std::swap(successor.delta, generator.scratch_delta());
        return exhausted;
    }
    else
    {
        // One of the node's repository's builders, reused across its successors: back to its pool when this returns.
        auto successor = ygg::formalism::checkout<Successor<E, Kind>>(repository.builders());
        const auto expand = [&](const TransitionLabel<Kind>& transition)
        {
            if (!generate_successor<E>(generator, node, transition, *successor))
                return true;
            return callback(transition, *successor);
        };
        return enumerate(generator, node, options, expand);
    }
}

template<NatureEnvironment E, TaskKind Kind>
void find_outcomes(StateRepository<Kind>& repository, SuccessorBuilder<E, Kind>& successor)
{
    for_each_outcome<E, Kind>(successor, [&](auto& outcome) { ::tyr::planning::find_state(repository, *outcome.node.state); });
}

template<NatureEnvironment E, TaskKind Kind>
bool extend_outcomes(SuccessorGenerator<Kind>& generator,
                     const StateRepository<Kind>& repository,
                     AxiomEvaluator<Kind>& axiom_evaluator,
                     SuccessorBuilder<E, Kind>& successor)
{
    auto any_valid = false;
    for_each_outcome<E, Kind>(successor,
                              [&](auto& outcome)
                              {
                                  // A state that has its index is registered: the lookup gave it its derived atoms.
                                  if (outcome.node.state->index.is_none())
                                      axiom_evaluator.compute_extended_state(*outcome.node.state, repository);
                                  if (check_over_all(generator, repository, outcome))
                                      any_valid = true;
                              });
    if (!any_valid)
        clear_outcomes(successor);
    return any_valid;
}

template<NatureEnvironment E, TaskKind Kind>
void register_outcomes(StateRepository<Kind>& repository, SuccessorBuilder<E, Kind>& successor)
{
    for_each_outcome<E, Kind>(successor,
                              [&](auto& outcome)
                              {
                                  if (outcome.node.state->index.is_none())
                                      ::tyr::planning::register_extended_state(repository, *outcome.node.state);
                              });
}

template<NatureEnvironment E, TaskKind Kind>
std::vector<OwnedSuccessorBuilderView<E, Kind>> get_successors(SuccessorGenerator<Kind>& generator,
                                                               StateRepository<Kind>& repository,
                                                               AxiomEvaluator<Kind>& axiom_evaluator,
                                                               const std::type_identity_t<NodeBuilderView<Kind>>& node,
                                                               bool intern)
{
    check_repository(repository, node);
    auto result = std::vector<OwnedSuccessorBuilderView<E, Kind>> {};
    auto builder = formalism::Builder {};
    for_each_successor<E>(generator,
                          node,
                          [&](const TransitionLabel<Kind>& /*transition*/, SuccessorBuilder<E, Kind>& successor)
                          {
                              if (intern)
                                  find_outcomes(repository, successor);
                              if (!extend_outcomes(generator, repository, axiom_evaluator, successor))
                                  return true;
                              if (intern)
                              {
                                  register_outcomes(repository, successor);
                                  intern_label(repository, builder, successor);
                              }
                              result.emplace_back(urd::take(successor), &repository);
                              return true;
                          });
    return result;
}

template<std::same_as<DeterministicTag> E, TaskKind Kind>
std::vector<OwnedLabeledNodeBuilderView<Kind>> get_successors(SuccessorGenerator<Kind>& generator,
                                                              StateRepository<Kind>& repository,
                                                              AxiomEvaluator<Kind>& axiom_evaluator,
                                                              const std::type_identity_t<NodeBuilderView<Kind>>& node,
                                                              bool intern)
{
    check_repository(repository, node);
    auto result = std::vector<OwnedLabeledNodeBuilderView<Kind>> {};
    auto builder = formalism::Builder {};
    for_each_successor<DeterministicTag>(generator,
                                         node,
                                         [&](const TransitionLabel<Kind>& transition, SuccessorState<Kind>& successor)
                                         {
                                             auto labeled = LabeledNodeBuilder<Kind> {};
                                             const auto binding = transition.binding.value();
                                             if (intern)
                                             {
                                                 find_successor(repository, node, successor);
                                                 extend_successor(axiom_evaluator, node, successor);
                                                 labeled.node = register_successor(repository, node, successor);
                                                 urd::store(labeled.label, intern_action_binding(generator, builder, binding));
                                             }
                                             else
                                             {
                                                 extend_successor(axiom_evaluator, node, successor);
                                                 labeled.node = successor_node(node, successor);
                                                 if constexpr (std::same_as<Kind, GroundTag>)
                                                 {
                                                     // A ground binding is its ground action's row, interned already.
                                                     urd::store(labeled.label, binding);
                                                 }
                                                 else
                                                 {
                                                     // A borrowed binding, kept as a row of the label's own.
                                                     using Row = ygg::Row<formalism::Action<LiftedTag>>;
                                                     auto& row = urd::builder_of(labeled.label,
                                                                                 [&repository]() -> urd::built_child<Row, StateRepository<Kind>>
                                                                                 { return ygg::formalism::checkout<Row>(repository.builders()); });
                                                     urd::store(row.relation, action_of(binding));
                                                     row.values.clear();
                                                     for (const auto value : binding.values())
                                                         row.values.push_back(value);
                                                 }
                                             }
                                             result.emplace_back(std::move(labeled), &repository);
                                             return true;
                                         });
    return result;
}

template<NatureEnvironment E, TaskKind Kind>
void intern_label(StateRepository<Kind>& repository, formalism::Builder& builder, SuccessorBuilder<E, Kind>& successor)
{
    // A label with a binding of its own (one not interned yet).
    auto& label = *successor.label;
    using Row = ygg::Row<formalism::Action<LiftedTag>>;
    if (!label.binding.has_value() || std::holds_alternative<urd::key_t<Row>>(*label.binding))
        return;
    auto& binding = *label.binding;
    auto scratch = builder.template scratch<Row>();
    urd::store(binding, repository.get_task()->get_repository()->insert(urd::child_record(binding, *scratch)).first);
}

template std::vector<OwnedLabeledNodeBuilderView<GroundTag>> get_successors<DeterministicTag, GroundTag>(SuccessorGenerator<GroundTag>&,
                                                                                                         StateRepository<GroundTag>&,
                                                                                                         AxiomEvaluator<GroundTag>&,
                                                                                                         const NodeBuilderView<GroundTag>&,
                                                                                                         bool);
template std::vector<OwnedLabeledNodeBuilderView<LiftedTag>> get_successors<DeterministicTag, LiftedTag>(SuccessorGenerator<LiftedTag>&,
                                                                                                         StateRepository<LiftedTag>&,
                                                                                                         AxiomEvaluator<LiftedTag>&,
                                                                                                         const NodeBuilderView<LiftedTag>&,
                                                                                                         bool);
template void require_environment<DeterministicTag, GroundTag>(const SuccessorGenerator<GroundTag>&);
template bool generate_successor<DeterministicTag, GroundTag>(SuccessorGenerator<GroundTag>&,
                                                              const NodeBuilderView<GroundTag>&,
                                                              const TransitionLabel<GroundTag>&,
                                                              SuccessorState<GroundTag>&);
template bool for_each_successor<DeterministicTag, GroundTag>(SuccessorGenerator<GroundTag>&,
                                                              const NodeBuilderView<GroundTag>&,
                                                              const std::function<bool(const TransitionLabel<GroundTag>&, SuccessorState<GroundTag>&)>&,
                                                              const SuccessorOptions<GroundTag>&);
template void require_environment<NondeterministicTag, GroundTag>(const SuccessorGenerator<GroundTag>&);
template bool generate_successor<NondeterministicTag, GroundTag>(SuccessorGenerator<GroundTag>&,
                                                                 const NodeBuilderView<GroundTag>&,
                                                                 const TransitionLabel<GroundTag>&,
                                                                 SuccessorBuilder<NondeterministicTag, GroundTag>&);
template bool for_each_successor<NondeterministicTag, GroundTag>(
    SuccessorGenerator<GroundTag>&,
    const NodeBuilderView<GroundTag>&,
    const std::function<bool(const TransitionLabel<GroundTag>&, SuccessorBuilder<NondeterministicTag, GroundTag>&)>&,
    const SuccessorOptions<GroundTag>&);
template void find_outcomes<NondeterministicTag, GroundTag>(StateRepository<GroundTag>&, SuccessorBuilder<NondeterministicTag, GroundTag>&);
template void register_outcomes<NondeterministicTag, GroundTag>(StateRepository<GroundTag>&, SuccessorBuilder<NondeterministicTag, GroundTag>&);
template bool extend_outcomes<NondeterministicTag, GroundTag>(SuccessorGenerator<GroundTag>&,
                                                              const StateRepository<GroundTag>&,
                                                              AxiomEvaluator<GroundTag>&,
                                                              SuccessorBuilder<NondeterministicTag, GroundTag>&);
template std::vector<OwnedSuccessorBuilderView<NondeterministicTag, GroundTag>>
get_successors<NondeterministicTag, GroundTag>(SuccessorGenerator<GroundTag>&,
                                               StateRepository<GroundTag>&,
                                               AxiomEvaluator<GroundTag>&,
                                               const NodeBuilderView<GroundTag>&,
                                               bool);
template void intern_label<NondeterministicTag, GroundTag>(StateRepository<GroundTag>&, formalism::Builder&, SuccessorBuilder<NondeterministicTag, GroundTag>&);
template void require_environment<ProbabilisticTag, GroundTag>(const SuccessorGenerator<GroundTag>&);
template bool generate_successor<ProbabilisticTag, GroundTag>(SuccessorGenerator<GroundTag>&,
                                                              const NodeBuilderView<GroundTag>&,
                                                              const TransitionLabel<GroundTag>&,
                                                              SuccessorBuilder<ProbabilisticTag, GroundTag>&);
template bool
for_each_successor<ProbabilisticTag, GroundTag>(SuccessorGenerator<GroundTag>&,
                                                const NodeBuilderView<GroundTag>&,
                                                const std::function<bool(const TransitionLabel<GroundTag>&, SuccessorBuilder<ProbabilisticTag, GroundTag>&)>&,
                                                const SuccessorOptions<GroundTag>&);
template void find_outcomes<ProbabilisticTag, GroundTag>(StateRepository<GroundTag>&, SuccessorBuilder<ProbabilisticTag, GroundTag>&);
template void register_outcomes<ProbabilisticTag, GroundTag>(StateRepository<GroundTag>&, SuccessorBuilder<ProbabilisticTag, GroundTag>&);
template bool extend_outcomes<ProbabilisticTag, GroundTag>(SuccessorGenerator<GroundTag>&,
                                                           const StateRepository<GroundTag>&,
                                                           AxiomEvaluator<GroundTag>&,
                                                           SuccessorBuilder<ProbabilisticTag, GroundTag>&);
template std::vector<OwnedSuccessorBuilderView<ProbabilisticTag, GroundTag>> get_successors<ProbabilisticTag, GroundTag>(SuccessorGenerator<GroundTag>&,
                                                                                                                         StateRepository<GroundTag>&,
                                                                                                                         AxiomEvaluator<GroundTag>&,
                                                                                                                         const NodeBuilderView<GroundTag>&,
                                                                                                                         bool);
template void intern_label<ProbabilisticTag, GroundTag>(StateRepository<GroundTag>&, formalism::Builder&, SuccessorBuilder<ProbabilisticTag, GroundTag>&);
template void require_environment<GeneralTag, GroundTag>(const SuccessorGenerator<GroundTag>&);
template bool generate_successor<GeneralTag, GroundTag>(SuccessorGenerator<GroundTag>&,
                                                        const NodeBuilderView<GroundTag>&,
                                                        const TransitionLabel<GroundTag>&,
                                                        SuccessorBuilder<GeneralTag, GroundTag>&);
template bool for_each_successor<GeneralTag, GroundTag>(SuccessorGenerator<GroundTag>&,
                                                        const NodeBuilderView<GroundTag>&,
                                                        const std::function<bool(const TransitionLabel<GroundTag>&, SuccessorBuilder<GeneralTag, GroundTag>&)>&,
                                                        const SuccessorOptions<GroundTag>&);
template void find_outcomes<GeneralTag, GroundTag>(StateRepository<GroundTag>&, SuccessorBuilder<GeneralTag, GroundTag>&);
template void register_outcomes<GeneralTag, GroundTag>(StateRepository<GroundTag>&, SuccessorBuilder<GeneralTag, GroundTag>&);
template bool extend_outcomes<GeneralTag, GroundTag>(SuccessorGenerator<GroundTag>&,
                                                     const StateRepository<GroundTag>&,
                                                     AxiomEvaluator<GroundTag>&,
                                                     SuccessorBuilder<GeneralTag, GroundTag>&);
template std::vector<OwnedSuccessorBuilderView<GeneralTag, GroundTag>> get_successors<GeneralTag, GroundTag>(SuccessorGenerator<GroundTag>&,
                                                                                                             StateRepository<GroundTag>&,
                                                                                                             AxiomEvaluator<GroundTag>&,
                                                                                                             const NodeBuilderView<GroundTag>&,
                                                                                                             bool);
template void intern_label<GeneralTag, GroundTag>(StateRepository<GroundTag>&, formalism::Builder&, SuccessorBuilder<GeneralTag, GroundTag>&);
template void require_environment<DeterministicTag, LiftedTag>(const SuccessorGenerator<LiftedTag>&);
template bool generate_successor<DeterministicTag, LiftedTag>(SuccessorGenerator<LiftedTag>&,
                                                              const NodeBuilderView<LiftedTag>&,
                                                              const TransitionLabel<LiftedTag>&,
                                                              SuccessorState<LiftedTag>&);
template bool for_each_successor<DeterministicTag, LiftedTag>(SuccessorGenerator<LiftedTag>&,
                                                              const NodeBuilderView<LiftedTag>&,
                                                              const std::function<bool(const TransitionLabel<LiftedTag>&, SuccessorState<LiftedTag>&)>&,
                                                              const SuccessorOptions<LiftedTag>&);
template void require_environment<NondeterministicTag, LiftedTag>(const SuccessorGenerator<LiftedTag>&);
template bool generate_successor<NondeterministicTag, LiftedTag>(SuccessorGenerator<LiftedTag>&,
                                                                 const NodeBuilderView<LiftedTag>&,
                                                                 const TransitionLabel<LiftedTag>&,
                                                                 SuccessorBuilder<NondeterministicTag, LiftedTag>&);
template bool for_each_successor<NondeterministicTag, LiftedTag>(
    SuccessorGenerator<LiftedTag>&,
    const NodeBuilderView<LiftedTag>&,
    const std::function<bool(const TransitionLabel<LiftedTag>&, SuccessorBuilder<NondeterministicTag, LiftedTag>&)>&,
    const SuccessorOptions<LiftedTag>&);
template void find_outcomes<NondeterministicTag, LiftedTag>(StateRepository<LiftedTag>&, SuccessorBuilder<NondeterministicTag, LiftedTag>&);
template void register_outcomes<NondeterministicTag, LiftedTag>(StateRepository<LiftedTag>&, SuccessorBuilder<NondeterministicTag, LiftedTag>&);
template bool extend_outcomes<NondeterministicTag, LiftedTag>(SuccessorGenerator<LiftedTag>&,
                                                              const StateRepository<LiftedTag>&,
                                                              AxiomEvaluator<LiftedTag>&,
                                                              SuccessorBuilder<NondeterministicTag, LiftedTag>&);
template std::vector<OwnedSuccessorBuilderView<NondeterministicTag, LiftedTag>>
get_successors<NondeterministicTag, LiftedTag>(SuccessorGenerator<LiftedTag>&,
                                               StateRepository<LiftedTag>&,
                                               AxiomEvaluator<LiftedTag>&,
                                               const NodeBuilderView<LiftedTag>&,
                                               bool);
template void intern_label<NondeterministicTag, LiftedTag>(StateRepository<LiftedTag>&, formalism::Builder&, SuccessorBuilder<NondeterministicTag, LiftedTag>&);
template void require_environment<ProbabilisticTag, LiftedTag>(const SuccessorGenerator<LiftedTag>&);
template bool generate_successor<ProbabilisticTag, LiftedTag>(SuccessorGenerator<LiftedTag>&,
                                                              const NodeBuilderView<LiftedTag>&,
                                                              const TransitionLabel<LiftedTag>&,
                                                              SuccessorBuilder<ProbabilisticTag, LiftedTag>&);
template bool
for_each_successor<ProbabilisticTag, LiftedTag>(SuccessorGenerator<LiftedTag>&,
                                                const NodeBuilderView<LiftedTag>&,
                                                const std::function<bool(const TransitionLabel<LiftedTag>&, SuccessorBuilder<ProbabilisticTag, LiftedTag>&)>&,
                                                const SuccessorOptions<LiftedTag>&);
template void find_outcomes<ProbabilisticTag, LiftedTag>(StateRepository<LiftedTag>&, SuccessorBuilder<ProbabilisticTag, LiftedTag>&);
template void register_outcomes<ProbabilisticTag, LiftedTag>(StateRepository<LiftedTag>&, SuccessorBuilder<ProbabilisticTag, LiftedTag>&);
template bool extend_outcomes<ProbabilisticTag, LiftedTag>(SuccessorGenerator<LiftedTag>&,
                                                           const StateRepository<LiftedTag>&,
                                                           AxiomEvaluator<LiftedTag>&,
                                                           SuccessorBuilder<ProbabilisticTag, LiftedTag>&);
template std::vector<OwnedSuccessorBuilderView<ProbabilisticTag, LiftedTag>> get_successors<ProbabilisticTag, LiftedTag>(SuccessorGenerator<LiftedTag>&,
                                                                                                                         StateRepository<LiftedTag>&,
                                                                                                                         AxiomEvaluator<LiftedTag>&,
                                                                                                                         const NodeBuilderView<LiftedTag>&,
                                                                                                                         bool);
template void intern_label<ProbabilisticTag, LiftedTag>(StateRepository<LiftedTag>&, formalism::Builder&, SuccessorBuilder<ProbabilisticTag, LiftedTag>&);
template void require_environment<GeneralTag, LiftedTag>(const SuccessorGenerator<LiftedTag>&);
template bool generate_successor<GeneralTag, LiftedTag>(SuccessorGenerator<LiftedTag>&,
                                                        const NodeBuilderView<LiftedTag>&,
                                                        const TransitionLabel<LiftedTag>&,
                                                        SuccessorBuilder<GeneralTag, LiftedTag>&);
template bool for_each_successor<GeneralTag, LiftedTag>(SuccessorGenerator<LiftedTag>&,
                                                        const NodeBuilderView<LiftedTag>&,
                                                        const std::function<bool(const TransitionLabel<LiftedTag>&, SuccessorBuilder<GeneralTag, LiftedTag>&)>&,
                                                        const SuccessorOptions<LiftedTag>&);
template void find_outcomes<GeneralTag, LiftedTag>(StateRepository<LiftedTag>&, SuccessorBuilder<GeneralTag, LiftedTag>&);
template void register_outcomes<GeneralTag, LiftedTag>(StateRepository<LiftedTag>&, SuccessorBuilder<GeneralTag, LiftedTag>&);
template bool extend_outcomes<GeneralTag, LiftedTag>(SuccessorGenerator<LiftedTag>&,
                                                     const StateRepository<LiftedTag>&,
                                                     AxiomEvaluator<LiftedTag>&,
                                                     SuccessorBuilder<GeneralTag, LiftedTag>&);
template std::vector<OwnedSuccessorBuilderView<GeneralTag, LiftedTag>> get_successors<GeneralTag, LiftedTag>(SuccessorGenerator<LiftedTag>&,
                                                                                                             StateRepository<LiftedTag>&,
                                                                                                             AxiomEvaluator<LiftedTag>&,
                                                                                                             const NodeBuilderView<LiftedTag>&,
                                                                                                             bool);
template void intern_label<GeneralTag, LiftedTag>(StateRepository<LiftedTag>&, formalism::Builder&, SuccessorBuilder<GeneralTag, LiftedTag>&);

template formalism::ActionRowView intern_action_binding(SuccessorGenerator<GroundTag>&, formalism::Builder&, ActionBindingView<GroundTag>);
template bool find_successor(StateRepository<GroundTag>&, const NodeBuilderView<GroundTag>&, SuccessorState<GroundTag>&);
template void extend_successor(AxiomEvaluator<GroundTag>&, const NodeBuilderView<GroundTag>&, SuccessorState<GroundTag>&);
template NodeBuilder<GroundTag> register_successor(StateRepository<GroundTag>&, const NodeBuilderView<GroundTag>&, SuccessorState<GroundTag>&);
template NodeBuilder<GroundTag> successor_node(const NodeBuilderView<GroundTag>&, SuccessorState<GroundTag>&);
template OwnedNodeBuilderView<GroundTag> get_initial_node(StateRepository<GroundTag>&, AxiomEvaluator<GroundTag>&);
template NodeBuilder<GroundTag> get_node(const StateRepository<GroundTag>&, urd::index<State<GroundTag>>);
template NodeBuilder<GroundTag> get_node(const StateRepository<GroundTag>&, const urd::data<Node<GroundTag>>&);
template LabeledNodeBuilder<GroundTag> get_labeled_node(const StateRepository<GroundTag>&, const urd::data<LabeledNode<GroundTag>>&);
template PlanBuilder<DeterministicTag, GroundTag> replay_plan(const urd::data<Node<GroundTag>>&,
                                                              std::span<const formalism::ActionRowView>,
                                                              SuccessorGenerator<GroundTag>&,
                                                              StateRepository<GroundTag>&,
                                                              AxiomEvaluator<GroundTag>&);

template formalism::ActionRowView intern_action_binding(SuccessorGenerator<LiftedTag>&, formalism::Builder&, ActionBindingView<LiftedTag>);
template bool find_successor(StateRepository<LiftedTag>&, const NodeBuilderView<LiftedTag>&, SuccessorState<LiftedTag>&);
template void extend_successor(AxiomEvaluator<LiftedTag>&, const NodeBuilderView<LiftedTag>&, SuccessorState<LiftedTag>&);
template NodeBuilder<LiftedTag> register_successor(StateRepository<LiftedTag>&, const NodeBuilderView<LiftedTag>&, SuccessorState<LiftedTag>&);
template NodeBuilder<LiftedTag> successor_node(const NodeBuilderView<LiftedTag>&, SuccessorState<LiftedTag>&);
template OwnedNodeBuilderView<LiftedTag> get_initial_node(StateRepository<LiftedTag>&, AxiomEvaluator<LiftedTag>&);
template NodeBuilder<LiftedTag> get_node(const StateRepository<LiftedTag>&, urd::index<State<LiftedTag>>);
template NodeBuilder<LiftedTag> get_node(const StateRepository<LiftedTag>&, const urd::data<Node<LiftedTag>>&);
template LabeledNodeBuilder<LiftedTag> get_labeled_node(const StateRepository<LiftedTag>&, const urd::data<LabeledNode<LiftedTag>>&);
template PlanBuilder<DeterministicTag, LiftedTag> replay_plan(const urd::data<Node<LiftedTag>>&,
                                                              std::span<const formalism::ActionRowView>,
                                                              SuccessorGenerator<LiftedTag>&,
                                                              StateRepository<LiftedTag>&,
                                                              AxiomEvaluator<LiftedTag>&);

}
