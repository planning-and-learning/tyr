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

#include "tyr/planning/lifted/successor_generator.hpp"

#include "../metric.hpp"
#include "../outcome_distributions.hpp"
#include "../temporal_transitions.hpp"
#include "effect_atom_index.hpp"
#include "lifted_outcomes.hpp"
#include "tyr/datalog/formatter.hpp"
#include "tyr/datalog/lifted/contexts/program.hpp"
#include "tyr/datalog/lifted/kckp/static_extensions.hpp"
#include "tyr/datalog/solver.hpp"
#include "tyr/formalism/grounding_context.hpp"
#include "tyr/formalism/planning/grounder.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/planning/action_executor.hpp"
#include "tyr/planning/applicability_lifted.hpp"
#include "tyr/planning/axiom_evaluator.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/match_tree/match_tree.hpp"
#include "tyr/planning/lifted/programs/action.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/state_delta.hpp"
#include "tyr/planning/state_repository.hpp"
#include "tyr/planning/successor_generator.hpp"
#include "tyr/planning/task_utils.hpp"

#include <algorithm>
#include <cassert>
#include <fmt/ostream.h>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <urd/elements/flat.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/segmented.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/execution/onetbb.hpp>
#include <yggdrasil/formalism/builder_instantiations.hpp>

namespace tyr::planning
{
namespace
{
using Source = SourceNode<LiftedTag>;
using Builder = urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>;
using BuilderNode = Node<LiftedTag, StateBuilderView<LiftedTag>>;
using Bindings = urd::Store<urd::sequence<formalism::ActionRowView>>;
using BindingData = urd::builder<ygg::Row<formalism::Action<LiftedTag>>, formalism::Repository>;

/// The action a binding names, read through `repository`: interned (a binding's relation is an interned action schema,
/// as its rows' relation).
formalism::ActionView<LiftedTag> action_of(const BindingData& binding, const formalism::Repository& repository)
{ return repository.view(std::get<urd::interned_child<formalism::Action<LiftedTag>, formalism::Repository>>(binding.relation)); }

void validate_task(const TaskPtr<LiftedTag>& task, const StateRepository<LiftedTag>& state_repository)
{
    if (state_repository.get_task() != task)
        throw std::invalid_argument("SuccessorGenerator: state repository belongs to a different task.");
}

void validate_task(const TaskPtr<LiftedTag>& task, const AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    if (axiom_evaluator.get_task() != task)
        throw std::invalid_argument("SuccessorGenerator: axiom evaluator belongs to a different task.");
}

void validate_task(const TaskPtr<LiftedTag>& task, const Source& node)
{
    if (&node.get_task() != task.get())
        throw std::invalid_argument("SuccessorGenerator: state belongs to a different task.");
}

/// Successors go into a repository of the task, a builder other than the source's, or fresh builders of a list.
void validate_storage(const TaskPtr<LiftedTag>& task, const Source&, const StateRepository<LiftedTag>& storage) { validate_task(task, storage); }

void validate_storage(const TaskPtr<LiftedTag>&, const Source& node, const Builder& storage)
{
    if (&storage.numeric_storage.values == &node.state().numeric_storage().get_values())
        throw std::invalid_argument("SuccessorGenerator: source and output builder must not alias.");
}

void validate_storage(const TaskPtr<LiftedTag>&, const Source&, const SuccessorBuilders<LiftedTag>&) {}

void validate_storage(const TaskPtr<LiftedTag>&, const Source&, const OutcomeStorage<LiftedTag>&) {}

/// The storage of a list's next successor: the repository, or a builder appended to the list.
StateRepository<LiftedTag>& next_storage(StateRepository<LiftedTag>& storage) { return storage; }

Builder& next_storage(Builder& storage) { return storage; }

Builder& next_storage(SuccessorBuilders<LiftedTag>& storage) { return storage.emplace_back(); }

/// Sinks of successors: appending to a list, as they are or packed.
template<typename Out>
auto append(Out& out)
{
    return [&out](auto successor)
    {
        out.push_back(std::move(successor));
        return true;
    };
}

template<typename Out>
auto append_packed(Out& out)
{
    return [&out](const auto& successor)
    {
        out.push_back(successor.pack());
        return true;
    };
}
}

struct SuccessorGenerator<LiftedTag>::Impl
{
    using Program = ApplicableActionProgram;
    using ActionBindingMap = urd::Store<urd::map<formalism::ActionRowView, formalism::ActionView<GroundTag>>>;
    using StateKey = std::pair<const void*, ygg::uint_t>;

    struct SchemaEvaluator
    {
        formalism::ProgramView<LiftedTag> program;
        const StateInputs* inputs;
        urd::Store<urd::sequence<datalog::Scheduler<LiftedTag>>> schedulers;
    };

    struct Definition
    {
        explicit Definition(TaskPtr<LiftedTag> task);

        TaskPtr<LiftedTag> task;
        Program action_program;
    };

    struct Evaluator
    {
        Evaluator(const Definition& definition, ygg::ExecutionContextPtr execution_context);

        ygg::ExecutionContextPtr execution_context;
        formalism::Builder scratch_builder;
        urd::handle<BindingData> scratch_action_binding;
        ActionBindingMap action_binding_to_ground_action;
        BindingData checked_binding;
        /// The objects of scratch_action_binding and checked_binding, as the grounder binds parameters to them.
        urd::flat_vector<urd::index<formalism::Object>> scratch_objects;
        urd::flat_vector<urd::index<formalism::Object>> checked_objects;
        datalog::ProgramWorkspace<LiftedTag> workspace;
        urd::Store<urd::map<formalism::ActionView<LiftedTag>, SchemaEvaluator>> schema_evaluators;
        datalog::StaticExtensionWorkspace static_extension_workspace;
        /// The effect atoms of the facts, grouped by action binding.
        EffectAtomIndex effect_atom_index;
        ActionExecutor executor;
        StateDelta successor_delta;
        LiftedOutcomeDistributions outcome_distributions;
        LiftedOutcomeBuilder outcome_builder;
        urd::Store<urd::sequence<LiftedOutcomePick>> outcome_picks;
        urd::Store<urd::sequence<LiftedEffects>> lifted_effects;
        Happening happening;
        TemporalStepper stepper;
        bool scheduled = true;
        /// The objects of the action bindings a transition applies (ranges in it: spans are taken once it is complete).
        urd::Store<urd::sequence<urd::index<formalism::Object>>> transition_objects;
        urd::Store<urd::sequence<OutcomeRange>> transition_bindings;
        urd::Store<urd::sequence<urd::index<formalism::Object>>> static_objects;
        OutcomeStorage<LiftedTag> outcome_builders;

        /// The facts the workspace holds. A state builder's contents can change, so its facts hold only while enumerating.
        struct Facts
        {
            std::optional<StateKey> state;
            /// The action of the schema program that computed them, or none for the full program (all actions).
            std::optional<urd::index<formalism::Action<LiftedTag>>> action;
            bool enumerating = false;
        } facts;

        /// The effect atoms of each action in the facts, grouped at the action's first successor.
    };

    /// Computes the facts of the node's state with the action's effects unless the workspace holds them.
    void require_action_facts(const Source& node, formalism::ActionView<LiftedTag> action);

    /// The repository and index of a registered state, or the address of a state builder.
    static StateKey state_key(const Source& node)
    {
        if (const auto* state = node.registered())
            return { static_cast<const void*>(&state->context()), static_cast<ygg::uint_t>(state->pack().index().value) };
        return { static_cast<const void*>(&node.state().numeric_storage().get_values()), std::numeric_limits<ygg::uint_t>::max() };
    }

    /// Forgets the facts of a state builder after single-binding work, since the builder can change.
    void end_single_binding_work(const Source& node)
    {
        if (!node.registered() && !evaluator.facts.enumerating)
            evaluator.facts.state = std::nullopt;
    }

    Impl(TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context) :
        definition(std::make_shared<Definition>(std::move(task))),
        evaluator(*definition, std::move(execution_context))
    {
    }

    Impl(std::shared_ptr<const Definition> definition_, ygg::ExecutionContextPtr execution_context) :
        definition(std::move(definition_)),
        evaluator(*this->definition, std::move(execution_context))
    {
    }

    SchemaEvaluator& get_schema_evaluator(formalism::ActionView<LiftedTag> action)
    {
        const auto it = evaluator.schema_evaluators.find(action);
        // Repository indices can collide across independent factories.
        if (it == evaluator.schema_evaluators.end() || &action.context() != &it->first.context())
            throw std::invalid_argument("SuccessorGenerator: action schema does not belong to the task domain.");
        return it->second;
    }

    template<typename Objects>
    bool validate_binding_objects(formalism::ActionView<LiftedTag> action, Objects objects)
    {
        get_schema_evaluator(action);
        if (objects.size() != action.get_arity())
            throw std::invalid_argument("SuccessorGenerator: object count does not match action arity.");
        const auto indices = objects.base();
        const auto& target_repository = *definition->task->get_repository();
        if (!std::ranges::all_of(indices, [&](const auto& value) { return objects.context().contains(value); })
            || !std::ranges::all_of(indices, [&](const auto& value) { return target_repository.contains(value); }))
            return false;
        if (!std::ranges::all_of(objects, [&](const auto& value) { return target_repository.contains(value); }))
            throw std::invalid_argument("SuccessorGenerator: object does not belong to the task repository.");
        return true;
    }

    void validate_binding(formalism::ActionRowBuilderView binding)
    {
        const auto action = action_of(binding);
        if (!binding.context().contains(action.index()))
            throw std::invalid_argument("SuccessorGenerator: action schema does not belong to its source repository.");
        if (!validate_binding_objects(action, binding.template resolve<formalism::Object>()))
            throw std::invalid_argument("SuccessorGenerator: object does not belong to the task repository.");
    }

    template<typename Objects>
    ActionBindingStatus check_action_binding(const Source& node, formalism::ActionView<LiftedTag> action, Objects objects);

    /// Computes the facts of the node's state with the action's schema program, or with the full program for all actions.
    void compute_action_facts(const Source& node, std::optional<formalism::ActionView<LiftedTag>> action);

    /// Calls `callback` with each applicable binding (in scratch_action_binding) of the action, or of all actions.
    template<typename Callback>
    bool for_each_applicable_action_binding(const Source& node, std::optional<formalism::ActionView<LiftedTag>> action, Callback&& callback);

    template<typename Accept, typename Callback, typename Stop>
    bool
    for_each_applicable_action_binding(const Source& node, std::optional<formalism::ActionView<LiftedTag>> action, Accept accept, Callback callback, Stop stop);

    /// The core: writes the successor of `node` by binding data into `out_state`, without axiom closure.
    ygg::float_t
    generate(const Source& node, const BindingData& binding, urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>& out_state, StateDelta& out_delta);

    /// Like generate(), by a ground action.
    ygg::float_t generate(const Source& node, formalism::ActionView<GroundTag> action, Builder& out_state, StateDelta& out_delta)
    {
        const auto state_context = node.state();
        return evaluator.executor.apply_action_unregistered(state_context, action, out_state, out_delta);
    }

    /// Copies a binding (interned, or borrowed) into the single-binding scratch.
    template<typename Binding>
    const BindingData& stage(const Binding& binding)
    {
        auto& data = evaluator.checked_binding;
        urd::store(data.relation, action_of(binding));
        data.values.clear();
        for (const auto value : binding.get_values())
            data.values.push_back(value);
        return data;
    }

    template<typename Storage>
    void validate(const Source& node, const Storage& storage, const AxiomEvaluator<LiftedTag>& axiom_evaluator) const
    {
        validate_task(definition->task, node);
        validate_task(definition->task, axiom_evaluator);
        validate_storage(definition->task, node, storage);
    }

    /// The registering wrapper: a fresh builder of the repository, the core, then registration, which closes the state
    /// under the axioms only if it is new.
    template<typename Action>
    Node<LiftedTag> successor(SuccessorGenerator& self,
                              const Source& node,
                              const Action& action,
                              StateRepository<LiftedTag>& storage,
                              AxiomEvaluator<LiftedTag>& axiom_evaluator)
    {
        validate(node, storage, axiom_evaluator);
        auto successor_state = storage.get_state_builder();
        auto& delta = evaluator.successor_delta;
        const auto auxiliary_value = generate(node, action, *successor_state, delta);
        if (const auto* parent = node.registered())
            return self.finalize_successor_state(storage, axiom_evaluator, std::move(successor_state), auxiliary_value, *parent, delta);
        return self.finalize_successor_state(storage, axiom_evaluator, std::move(successor_state), auxiliary_value);
    }

    /// The core into a caller's builder, closed under the axioms.
    template<typename Action>
    BuilderNode successor(SuccessorGenerator&, const Source& node, const Action& action, Builder& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator)
    {
        validate(node, storage, axiom_evaluator);
        storage.clear();
        const auto auxiliary_value = generate(node, action, storage, evaluator.successor_delta);
        const auto metric = evaluate_successor_metric(node.get_state_repository(), storage, auxiliary_value);
        axiom_evaluator.compute_extended_state(storage, node.get_state_repository());
        return BuilderNode(urd::resolve(storage, node.get_state_repository()), metric);
    }

    /// Passes the successor of each applicable binding of the action (or of all actions), generated into `storage`, to `sink`.
    template<typename Storage, typename Sink>
    bool for_each_successor(SuccessorGenerator& self,
                            const Source& node,
                            std::optional<formalism::ActionView<LiftedTag>> action,
                            Storage& storage,
                            AxiomEvaluator<LiftedTag>& axiom_evaluator,
                            const Sink& sink)
    {
        return for_each_applicable_action_binding(node,
                                                  action,
                                                  [&](auto& binding)
                                                  { return sink(self.get_successor_node(node, binding, next_storage(storage), axiom_evaluator)); });
    }

    /// Like for_each_successor(), with each successor labeled by its binding, interned before the successor is generated.
    template<typename Storage, typename Sink>
    bool for_each_labeled_successor(SuccessorGenerator& self,
                                    const Source& node,
                                    std::optional<formalism::ActionView<LiftedTag>> action,
                                    Storage& storage,
                                    AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                    const Sink& sink)
    {
        return for_each_applicable_action_binding(
            node,
            action,
            [&](auto& binding)
            {
                const auto label = definition->task->get_repository()->insert(binding, evaluator.workspace.planning_builder).first;
                auto successor = self.get_successor_node(node, binding, next_storage(storage), axiom_evaluator);
                return sink(LabeledNode<LiftedTag, std::remove_cvref_t<decltype(successor.get_state())>> { label, std::move(successor) });
            });
    }

    std::shared_ptr<const Definition> definition;
    Evaluator evaluator;
};

SuccessorGenerator<LiftedTag>::Impl::Definition::Definition(TaskPtr<LiftedTag> task_) : task(std::move(task_)), action_program(task->get_formalism_task()) {}

SuccessorGenerator<LiftedTag>::Impl::Evaluator::Evaluator(const Definition& definition, ygg::ExecutionContextPtr execution_context_) :
    execution_context(std::move(execution_context_)),
    scratch_builder(),
    scratch_action_binding(formalism::checkout<ygg::Row<formalism::Action<LiftedTag>>>(scratch_builder)),
    action_binding_to_ground_action(),
    workspace(definition.action_program.get_datalog_program()),
    static_extension_workspace(),
    effect_atom_index(definition.action_program.get_predicate_to_action_mapping(),
                      definition.action_program.get_conditional_effect_to_predicate_mapping(),
                      workspace.facts.fact_sets.get<formalism::DerivedTag>(),
                      *workspace.workspace_repository),
    executor()
{
    assert(execution_context);
    for (const auto& [action, schema] : definition.action_program.get_schema_programs())
        schema_evaluators.emplace(action,
                                  SchemaEvaluator { schema.program,
                                                    &schema.inputs,
                                                    datalog::create_schedulers(schema.strata, schema.listeners, schema.program.context(), schema.program) });
}

void SuccessorGenerator<LiftedTag>::Impl::compute_action_facts(const Source& node, std::optional<formalism::ActionView<LiftedTag>> action)
{
    auto* schema = action ? &get_schema_evaluator(*action) : nullptr;
    evaluator.workspace.reset_evaluation();
    evaluator.facts.state = state_key(node);
    evaluator.facts.action = action ? std::optional(action->index()) : std::nullopt;
    evaluator.effect_atom_index.clear();

    if (schema)
        insert_extended_state(node.state(), *definition->task->get_repository(), *schema->inputs, evaluator.workspace);
    else
        insert_extended_state(node.state(), *definition->task->get_repository(), AllStateInputs {}, evaluator.workspace);

    auto ctx =
        datalog::ProgramExecutionContext<LiftedTag>::from_reset_workspace(evaluator.workspace, schema ? schema->schedulers : evaluator.workspace.schedulers);
    datalog::execute_model(ctx, *evaluator.execution_context);
}

template<typename Callback>
bool SuccessorGenerator<LiftedTag>::Impl::for_each_applicable_action_binding(const Source& node,
                                                                             std::optional<formalism::ActionView<LiftedTag>> action,
                                                                             Callback&& callback)
{
    return for_each_applicable_action_binding(node, action, [](const auto&) { return true; }, std::forward<Callback>(callback), [] { return false; });
}

template<typename Accept, typename Callback, typename Stop>
bool SuccessorGenerator<LiftedTag>::Impl::for_each_applicable_action_binding(const Source& node,
                                                                             std::optional<formalism::ActionView<LiftedTag>> action,
                                                                             Accept accept,
                                                                             Callback callback,
                                                                             Stop stop)
{
    const auto program = action ? get_schema_evaluator(*action).program : definition->action_program.get_datalog_program().get_program();
    if (stop())
        return false;
    if (evaluator.facts.enumerating)
        throw std::logic_error("SuccessorGenerator: applicable action bindings are enumerated already.");
    compute_action_facts(node, action);
    evaluator.facts.enumerating = true;

    struct EndEnumeration
    {
        EndEnumeration(Evaluator::Facts& facts_, bool registered_) : facts(&facts_), registered(registered_) {}

        EndEnumeration(const EndEnumeration&) = delete;
        EndEnumeration& operator=(const EndEnumeration&) = delete;
        EndEnumeration(EndEnumeration&&) = delete;
        EndEnumeration& operator=(EndEnumeration&&) = delete;

        ~EndEnumeration()
        {
            facts->enumerating = false;
            if (!registered)
                facts->state = std::nullopt;
        }

        Evaluator::Facts* facts;
        bool registered;
    } end_enumeration { evaluator.facts, node.registered() != nullptr };

    auto& scratch_binding = *evaluator.scratch_action_binding;

    const auto state_context = node.state();
    auto grounder_context =
        formalism::GroundingContext { evaluator.workspace.planning_builder, *definition->task->get_repository(), evaluator.scratch_objects };
    const auto& mapping = definition->action_program.get_predicate_to_action_mapping();

    for (const auto rule : program.get_rules())
    {
        const auto predicate = rule.get_derived_heads().front().get_predicate();
        const auto it = mapping.find(predicate);
        if (it == mapping.end())
            continue;  // A conditional effect's rule.
        const auto schema = it->second;
        for (const auto& binding : datalog::rows(evaluator.workspace.facts.fact_sets.get<formalism::DerivedTag>(), predicate.index().value))
        {
            if (stop())
                return false;
            urd::store(scratch_binding.relation, schema);
            scratch_binding.values.clear();
            evaluator.scratch_objects.clear();
            for (const auto object : binding.template get<urd::index<formalism::Object>>())
            {
                scratch_binding.values.push_back(object.value);
                evaluator.scratch_objects.push_back(object);
            }
            if (!accept(scratch_binding))
                continue;

            assert(is_applicable(schema.get_condition(), ApplicabilityContext { state_context, grounder_context, *definition->task->get_fdr_context() })
                   && "ApplicableActionProgram emitted an action binding whose condition is not satisfied.");

            // Datalog certifies the action condition, not whether its grounded numeric effects are valid and mutually compatible.
            if (!evaluator.executor.is_applicable_if_fires(
                    schema,
                    state_context,
                    grounder_context,
                    *definition->task->get_fdr_context(),
                    [&](auto effect, auto apply) { evaluator.effect_atom_index.for_each_effect_binding(schema, effect, grounder_context.binding(), apply); }))
                continue;
            if (!callback(scratch_binding))
                return false;
        }
    }
    return true;
}

ygg::float_t SuccessorGenerator<LiftedTag>::Impl::generate(const Source& node,
                                                           const BindingData& binding,
                                                           urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>& out_state,
                                                           StateDelta& out_delta)
{
    evaluator.workspace.binding.clear();
    for (const auto value : binding.values)
        evaluator.workspace.binding.push_back(ygg::decode_value<urd::index<formalism::Object>>(value));

    auto grounder_context =
        formalism::GroundingContext { evaluator.workspace.planning_builder, *definition->task->get_repository(), evaluator.workspace.binding };
    const auto state_context = node.state();
    const auto action = action_of(binding, *definition->task->get_repository());
    if (action.get_temporal_part().has_value())
        throw std::invalid_argument(
            "SuccessorGenerator: a durative action happens at its start and its end, within a temporal network: use get_successor or for_each_successor.");
    if (!action.get_choices().empty())
        throw std::invalid_argument(
            "SuccessorGenerator: an action with oneof or probabilistic effects has several outcomes: use get_successor or for_each_successor.");

    require_action_facts(node, action);
    const auto auxiliary_value = evaluator.executor.apply_action_unregistered(
        state_context,
        action,
        grounder_context,
        *definition->task->get_fdr_context(),
        [&](auto effect, auto apply) { evaluator.effect_atom_index.for_each_effect_binding(action, effect, grounder_context.binding(), apply); },
        out_state,
        out_delta);
    end_single_binding_work(node);
    return auxiliary_value;
}

void SuccessorGenerator<LiftedTag>::Impl::require_action_facts(const Source& node, formalism::ActionView<LiftedTag> action)
{
    // The facts of another state, or without the action's effects, come from the action's schema program.
    if (evaluator.facts.state == state_key(node) && (!evaluator.facts.action || *evaluator.facts.action == action.index()))
        return;
    if (evaluator.facts.enumerating)
        throw std::logic_error("SuccessorGenerator: the state differs from the one whose applicable action bindings are enumerated.");
    compute_action_facts(node, action);
}

SuccessorGenerator<LiftedTag>::SuccessorGenerator(TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context) :
    m_impl(std::make_unique<Impl>(std::move(task), std::move(execution_context)))
{
}

SuccessorGenerator<LiftedTag>::SuccessorGenerator(std::unique_ptr<Impl> impl) noexcept : m_impl(std::move(impl)) {}

SuccessorGenerator<LiftedTag>::~SuccessorGenerator() = default;
SuccessorGenerator<LiftedTag>::SuccessorGenerator(SuccessorGenerator&&) noexcept = default;
SuccessorGenerator<LiftedTag>& SuccessorGenerator<LiftedTag>::operator=(SuccessorGenerator&&) noexcept = default;

SuccessorGeneratorPtr<LiftedTag> SuccessorGenerator<LiftedTag>::make_worker(ygg::ExecutionContextPtr execution_context) const
{ return SuccessorGeneratorPtr<LiftedTag>(new SuccessorGenerator<LiftedTag>(std::make_unique<Impl>(m_impl->definition, std::move(execution_context)))); }

Node<LiftedTag> SuccessorGenerator<LiftedTag>::get_initial_node(StateRepository<LiftedTag>& state_repository, AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    validate_task(m_impl->definition->task, state_repository);
    validate_task(m_impl->definition->task, axiom_evaluator);
    auto initial_state = state_repository.get_initial_state(axiom_evaluator);
    const auto state_context = StateContext<LiftedTag>(*m_impl->definition->task, initial_state, 0);
    const auto state_metric =
        evaluate_metric(m_impl->definition->task->get_task().get_metric(), m_impl->definition->task->get_task().get_auxiliary_fterm_value(), state_context);
    return Node<LiftedTag>(std::move(initial_state), state_metric);
}

NodeList<LiftedTag>
SuccessorGenerator<LiftedTag>::get_successor_nodes(const Source& node, StateRepository<LiftedTag>& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = NodeList<LiftedTag> {};
    get_successor_nodes(node, storage, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_successor_nodes(const Source& node,
                                                        StateRepository<LiftedTag>& storage,
                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                        NodeList<LiftedTag>& out_nodes)
{
    m_impl->validate(node, storage, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_successor(*this, node, std::nullopt, storage, axiom_evaluator, append(out_nodes));
}

NodeList<LiftedTag> SuccessorGenerator<LiftedTag>::get_successor_nodes(const Source& node,
                                                                       formalism::ActionView<LiftedTag> action,
                                                                       StateRepository<LiftedTag>& storage,
                                                                       AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = NodeList<LiftedTag> {};
    get_successor_nodes(node, action, storage, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_successor_nodes(const Source& node,
                                                        formalism::ActionView<LiftedTag> action,
                                                        StateRepository<LiftedTag>& storage,
                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                        NodeList<LiftedTag>& out_nodes)
{
    m_impl->validate(node, storage, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_successor(*this, node, action, storage, axiom_evaluator, append(out_nodes));
}

NodeList<LiftedTag, StateBuilderView<LiftedTag>>
SuccessorGenerator<LiftedTag>::get_successor_nodes(const Source& node, SuccessorBuilders<LiftedTag>& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = NodeList<LiftedTag, StateBuilderView<LiftedTag>> {};
    get_successor_nodes(node, storage, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_successor_nodes(const Source& node,
                                                        SuccessorBuilders<LiftedTag>& storage,
                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                        NodeList<LiftedTag, StateBuilderView<LiftedTag>>& out_nodes)
{
    m_impl->validate(node, storage, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_successor(*this, node, std::nullopt, storage, axiom_evaluator, append(out_nodes));
}

NodeList<LiftedTag, StateBuilderView<LiftedTag>> SuccessorGenerator<LiftedTag>::get_successor_nodes(const Source& node,
                                                                                                    formalism::ActionView<LiftedTag> action,
                                                                                                    SuccessorBuilders<LiftedTag>& storage,
                                                                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = NodeList<LiftedTag, StateBuilderView<LiftedTag>> {};
    get_successor_nodes(node, action, storage, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_successor_nodes(const Source& node,
                                                        formalism::ActionView<LiftedTag> action,
                                                        SuccessorBuilders<LiftedTag>& storage,
                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                        NodeList<LiftedTag, StateBuilderView<LiftedTag>>& out_nodes)
{
    m_impl->validate(node, storage, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_successor(*this, node, action, storage, axiom_evaluator, append(out_nodes));
}

LabeledNodeList<LiftedTag>
SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes(const Source& node, StateRepository<LiftedTag>& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = LabeledNodeList<LiftedTag> {};
    get_labeled_successor_nodes(node, storage, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes(const Source& node,
                                                                StateRepository<LiftedTag>& storage,
                                                                AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                LabeledNodeList<LiftedTag>& out_nodes)
{
    m_impl->validate(node, storage, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_labeled_successor(*this, node, std::nullopt, storage, axiom_evaluator, append(out_nodes));
}

LabeledNodeList<LiftedTag> SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes(const Source& node,
                                                                                      formalism::ActionView<LiftedTag> action,
                                                                                      StateRepository<LiftedTag>& storage,
                                                                                      AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = LabeledNodeList<LiftedTag> {};
    get_labeled_successor_nodes(node, action, storage, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes(const Source& node,
                                                                formalism::ActionView<LiftedTag> action,
                                                                StateRepository<LiftedTag>& storage,
                                                                AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                LabeledNodeList<LiftedTag>& out_nodes)
{
    m_impl->validate(node, storage, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_labeled_successor(*this, node, action, storage, axiom_evaluator, append(out_nodes));
}

LabeledNodeList<LiftedTag, StateBuilderView<LiftedTag>> SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes(const Source& node,
                                                                                                                   SuccessorBuilders<LiftedTag>& storage,
                                                                                                                   AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = LabeledNodeList<LiftedTag, StateBuilderView<LiftedTag>> {};
    get_labeled_successor_nodes(node, storage, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes(const Source& node,
                                                                SuccessorBuilders<LiftedTag>& storage,
                                                                AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                LabeledNodeList<LiftedTag, StateBuilderView<LiftedTag>>& out_nodes)
{
    m_impl->validate(node, storage, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_labeled_successor(*this, node, std::nullopt, storage, axiom_evaluator, append(out_nodes));
}

LabeledNodeList<LiftedTag, StateBuilderView<LiftedTag>> SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes(const Source& node,
                                                                                                                   formalism::ActionView<LiftedTag> action,
                                                                                                                   SuccessorBuilders<LiftedTag>& storage,
                                                                                                                   AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = LabeledNodeList<LiftedTag, StateBuilderView<LiftedTag>> {};
    get_labeled_successor_nodes(node, action, storage, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes(const Source& node,
                                                                formalism::ActionView<LiftedTag> action,
                                                                SuccessorBuilders<LiftedTag>& storage,
                                                                AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                LabeledNodeList<LiftedTag, StateBuilderView<LiftedTag>>& out_nodes)
{
    m_impl->validate(node, storage, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_labeled_successor(*this, node, action, storage, axiom_evaluator, append(out_nodes));
}

bool SuccessorGenerator<LiftedTag>::for_each_successor_node(const Source& node,
                                                            StateRepository<LiftedTag>& storage,
                                                            AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                            const std::function<bool(Node<LiftedTag>)>& callback)
{
    m_impl->validate(node, storage, axiom_evaluator);
    return m_impl->for_each_successor(*this, node, std::nullopt, storage, axiom_evaluator, callback);
}

bool SuccessorGenerator<LiftedTag>::for_each_successor_node(const Source& node,
                                                            formalism::ActionView<LiftedTag> action,
                                                            StateRepository<LiftedTag>& storage,
                                                            AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                            const std::function<bool(Node<LiftedTag>)>& callback)
{
    m_impl->validate(node, storage, axiom_evaluator);
    return m_impl->for_each_successor(*this, node, action, storage, axiom_evaluator, callback);
}

bool SuccessorGenerator<LiftedTag>::for_each_successor_node(const Source& node,
                                                            Builder& storage,
                                                            AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                            const std::function<bool(BuilderNode)>& callback)
{
    m_impl->validate(node, storage, axiom_evaluator);
    return m_impl->for_each_successor(*this, node, std::nullopt, storage, axiom_evaluator, callback);
}

bool SuccessorGenerator<LiftedTag>::for_each_successor_node(const Source& node,
                                                            formalism::ActionView<LiftedTag> action,
                                                            Builder& storage,
                                                            AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                            const std::function<bool(BuilderNode)>& callback)
{
    m_impl->validate(node, storage, axiom_evaluator);
    return m_impl->for_each_successor(*this, node, action, storage, axiom_evaluator, callback);
}

bool SuccessorGenerator<LiftedTag>::for_each_labeled_successor_node(const Source& node,
                                                                    StateRepository<LiftedTag>& storage,
                                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                    const std::function<bool(LabeledNode<LiftedTag>)>& callback)
{
    m_impl->validate(node, storage, axiom_evaluator);
    return m_impl->for_each_labeled_successor(*this, node, std::nullopt, storage, axiom_evaluator, callback);
}

bool SuccessorGenerator<LiftedTag>::for_each_labeled_successor_node(const Source& node,
                                                                    formalism::ActionView<LiftedTag> action,
                                                                    StateRepository<LiftedTag>& storage,
                                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                    const std::function<bool(LabeledNode<LiftedTag>)>& callback)
{
    m_impl->validate(node, storage, axiom_evaluator);
    return m_impl->for_each_labeled_successor(*this, node, action, storage, axiom_evaluator, callback);
}

bool SuccessorGenerator<LiftedTag>::for_each_labeled_successor_node(const Source& node,
                                                                    Builder& storage,
                                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                    const std::function<bool(LabeledNode<LiftedTag, StateBuilderView<LiftedTag>>)>& callback)
{
    m_impl->validate(node, storage, axiom_evaluator);
    return m_impl->for_each_labeled_successor(*this, node, std::nullopt, storage, axiom_evaluator, callback);
}

bool SuccessorGenerator<LiftedTag>::for_each_labeled_successor_node(const Source& node,
                                                                    formalism::ActionView<LiftedTag> action,
                                                                    Builder& storage,
                                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                    const std::function<bool(LabeledNode<LiftedTag, StateBuilderView<LiftedTag>>)>& callback)
{
    m_impl->validate(node, storage, axiom_evaluator);
    return m_impl->for_each_labeled_successor(*this, node, action, storage, axiom_evaluator, callback);
}

PackedNodeList<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_nodes(const Source& node,
                                                                                    StateRepository<LiftedTag>& state_repository,
                                                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = PackedNodeList<LiftedTag> {};
    get_packed_successor_nodes(node, state_repository, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_packed_successor_nodes(const Source& node,
                                                               StateRepository<LiftedTag>& state_repository,
                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                               PackedNodeList<LiftedTag>& out_nodes)
{
    m_impl->validate(node, state_repository, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_successor(*this, node, std::nullopt, state_repository, axiom_evaluator, append_packed(out_nodes));
}

PackedNodeList<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_nodes(const Source& node,
                                                                                    formalism::ActionView<LiftedTag> action,
                                                                                    StateRepository<LiftedTag>& state_repository,
                                                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = PackedNodeList<LiftedTag> {};
    get_packed_successor_nodes(node, action, state_repository, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_packed_successor_nodes(const Source& node,
                                                               formalism::ActionView<LiftedTag> action,
                                                               StateRepository<LiftedTag>& state_repository,
                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                               PackedNodeList<LiftedTag>& out_nodes)
{
    m_impl->validate(node, state_repository, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_successor(*this, node, action, state_repository, axiom_evaluator, append_packed(out_nodes));
}

PackedLabeledNodeList<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes(const Source& node,
                                                                                                   StateRepository<LiftedTag>& state_repository,
                                                                                                   AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = PackedLabeledNodeList<LiftedTag> {};
    get_packed_labeled_successor_nodes(node, state_repository, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes(const Source& node,
                                                                       StateRepository<LiftedTag>& state_repository,
                                                                       AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                       PackedLabeledNodeList<LiftedTag>& out_nodes)
{
    m_impl->validate(node, state_repository, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_labeled_successor(*this, node, std::nullopt, state_repository, axiom_evaluator, append_packed(out_nodes));
}

PackedLabeledNodeList<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes(const Source& node,
                                                                                                   formalism::ActionView<LiftedTag> action,
                                                                                                   StateRepository<LiftedTag>& state_repository,
                                                                                                   AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = PackedLabeledNodeList<LiftedTag> {};
    get_packed_labeled_successor_nodes(node, action, state_repository, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes(const Source& node,
                                                                       formalism::ActionView<LiftedTag> action,
                                                                       StateRepository<LiftedTag>& state_repository,
                                                                       AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                       PackedLabeledNodeList<LiftedTag>& out_nodes)
{
    m_impl->validate(node, state_repository, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_labeled_successor(*this, node, action, state_repository, axiom_evaluator, append_packed(out_nodes));
}

Node<LiftedTag> SuccessorGenerator<LiftedTag>::get_successor_node(const Source& node,
                                                                  formalism::ActionView<GroundTag> action,
                                                                  StateRepository<LiftedTag>& storage,
                                                                  AxiomEvaluator<LiftedTag>& axiom_evaluator)
{ return m_impl->successor(*this, node, action, storage, axiom_evaluator); }

BuilderNode SuccessorGenerator<LiftedTag>::get_successor_node(const Source& node,
                                                              formalism::ActionView<GroundTag> action,
                                                              Builder& storage,
                                                              AxiomEvaluator<LiftedTag>& axiom_evaluator)
{ return m_impl->successor(*this, node, action, storage, axiom_evaluator); }

Node<LiftedTag> SuccessorGenerator<LiftedTag>::get_successor_node(const Source& node,
                                                                  formalism::ActionRowView binding,
                                                                  StateRepository<LiftedTag>& storage,
                                                                  AxiomEvaluator<LiftedTag>& axiom_evaluator)
{ return get_successor_node(node, m_impl->stage(binding), storage, axiom_evaluator); }

BuilderNode SuccessorGenerator<LiftedTag>::get_successor_node(const Source& node,
                                                              formalism::ActionRowView binding,
                                                              Builder& storage,
                                                              AxiomEvaluator<LiftedTag>& axiom_evaluator)
{ return get_successor_node(node, m_impl->stage(binding), storage, axiom_evaluator); }

Node<LiftedTag> SuccessorGenerator<LiftedTag>::get_successor_node(const Source& node,
                                                                  formalism::ActionRowBuilderView binding,
                                                                  StateRepository<LiftedTag>& storage,
                                                                  AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    m_impl->validate_binding(binding);
    return get_successor_node(node, m_impl->stage(binding), storage, axiom_evaluator);
}

BuilderNode SuccessorGenerator<LiftedTag>::get_successor_node(const Source& node,
                                                              formalism::ActionRowBuilderView binding,
                                                              Builder& storage,
                                                              AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    m_impl->validate_binding(binding);
    return get_successor_node(node, m_impl->stage(binding), storage, axiom_evaluator);
}

Node<LiftedTag> SuccessorGenerator<LiftedTag>::get_successor_node(const Source& node,
                                                                  const BindingData& binding,
                                                                  StateRepository<LiftedTag>& storage,
                                                                  AxiomEvaluator<LiftedTag>& axiom_evaluator)
{ return m_impl->successor(*this, node, binding, storage, axiom_evaluator); }

BuilderNode
SuccessorGenerator<LiftedTag>::get_successor_node(const Source& node, const BindingData& binding, Builder& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator)
{ return m_impl->successor(*this, node, binding, storage, axiom_evaluator); }

PackedNode<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_node(const Source& node,
                                                                               formalism::ActionRowView binding,
                                                                               StateRepository<LiftedTag>& state_repository,
                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator)
{ return get_successor_node(node, binding, state_repository, axiom_evaluator).pack(); }

PackedNode<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_node(const Source& node,
                                                                               formalism::ActionView<GroundTag> action,
                                                                               StateRepository<LiftedTag>& state_repository,
                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator)
{ return get_successor_node(node, action, state_repository, axiom_evaluator).pack(); }

PackedNode<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_node(const Source& node,
                                                                               const BindingData& binding,
                                                                               StateRepository<LiftedTag>& state_repository,
                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator)
{ return get_successor_node(node, binding, state_repository, axiom_evaluator).pack(); }

template<typename Objects>
ActionBindingStatus SuccessorGenerator<LiftedTag>::Impl::check_action_binding(const Source& node, formalism::ActionView<LiftedTag> action, Objects objects)
{
    const auto& task = definition->task;
    validate_task(task, node);
    if (!validate_binding_objects(action, objects))
        return ActionBindingStatus::OUTSIDE_PARAMETER_DOMAIN;
    const auto indices = objects.base();

    const auto& domains = task->get_formalism_task().get_variable_domains().action_domains.at(action.index()).payload.precondition_domain.payload;
    for (size_t i = 0; i < objects.size(); ++i)
        if (i >= domains.size() || !std::binary_search(domains[i].objects.begin(), domains[i].objects.end(), indices[i]))
            return ActionBindingStatus::OUTSIDE_PARAMETER_DOMAIN;

    // Single-binding work has separate scratch from the tuple held by an outer enumeration callback.
    auto& workspace = evaluator.workspace;
    auto& binding = evaluator.checked_binding;
    urd::store(binding.relation, action);
    binding.values.clear();
    evaluator.checked_objects.clear();
    for (const auto object : indices)
    {
        binding.values.push_back(object.value);
        evaluator.checked_objects.push_back(object);
    }
    auto grounder = formalism::GroundingContext { workspace.planning_builder, *task->get_repository(), evaluator.checked_objects };
    const auto state = node.state();
    if (!is_applicable(action.get_condition(), ApplicabilityContext { state, grounder, *task->get_fdr_context() }))
        return ActionBindingStatus::INAPPLICABLE;
    require_action_facts(node, action);
    const auto applicable = evaluator.executor.is_applicable_if_fires(
        action,
        state,
        grounder,
        *task->get_fdr_context(),
        [&](auto effect, auto apply) { evaluator.effect_atom_index.for_each_effect_binding(action, effect, grounder.binding(), apply); });
    end_single_binding_work(node);
    return applicable ? ActionBindingStatus::APPLICABLE : ActionBindingStatus::INAPPLICABLE;
}

template<ygg::RowViewConcept<formalism::Action<LiftedTag>> Binding>
ActionBindingStatus SuccessorGenerator<LiftedTag>::check_action_binding(const Source& node, Binding binding)
{
    // A binding builder's row reads its values untyped (ygg::Value): each is an object's index.
    return m_impl->check_action_binding(node, action_of(binding), binding.template resolve<formalism::Object>());
}

formalism::ActionRowView SuccessorGenerator<LiftedTag>::materialize_action_binding(ActionBindingView<LiftedTag> binding)
{
    m_impl->validate_binding(binding);
    // Relation bindings preserve object order and are already canonical.
    auto row = urd::make<ygg::Row<formalism::Action<LiftedTag>>>(action_of(binding), binding.get_values());
    return m_impl->definition->task->get_repository()->insert(row).first;
}

template<ygg::RowViewConcept<formalism::Action<LiftedTag>> Binding>
ActionBindingResult SuccessorGenerator<LiftedTag>::try_get_applicable_action_binding(const Source& node, Binding binding)
{
    const auto status = check_action_binding(node, binding);
    if (status != ActionBindingStatus::APPLICABLE)
        return { status, std::nullopt };
    return { status,
             m_impl->definition->task->get_repository()->insert(m_impl->evaluator.checked_binding, m_impl->evaluator.workspace.planning_builder).first };
}

Bindings SuccessorGenerator<LiftedTag>::get_applicable_action_bindings(const Source& node)
{
    auto result = Bindings {};
    get_applicable_action_bindings(node, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_applicable_action_bindings(const Source& node, Bindings& out_bindings)
{
    validate_task(m_impl->definition->task, node);
    out_bindings.clear();
    for_each_applicable_action_binding(node, append(out_bindings));
}

Bindings SuccessorGenerator<LiftedTag>::get_applicable_action_bindings(const Source& node, formalism::ActionView<LiftedTag> action)
{
    auto result = Bindings {};
    get_applicable_action_bindings(node, action, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_applicable_action_bindings(const Source& node, formalism::ActionView<LiftedTag> action, Bindings& out_bindings)
{
    validate_task(m_impl->definition->task, node);
    m_impl->get_schema_evaluator(action);
    out_bindings.clear();
    for_each_applicable_action_binding(node, action, append(out_bindings));
}

ygg::float_t
SuccessorGenerator<LiftedTag>::generate_successor_state(const Source& node, formalism::ActionRowView binding, Builder& out_state, StateDelta& out_delta)
{
    validate_task(m_impl->definition->task, node);
    return m_impl->generate(node, m_impl->stage(binding), out_state, out_delta);
}

bool SuccessorGenerator<LiftedTag>::for_each_applicable_action_binding(const Source& node, const std::function<bool(formalism::ActionRowView)>& callback)
{
    validate_task(m_impl->definition->task, node);
    return m_impl->for_each_applicable_action_binding(
        node,
        std::nullopt,
        [&](auto& binding)
        { return callback(m_impl->definition->task->get_repository()->insert(binding, m_impl->evaluator.workspace.planning_builder).first); });
}

bool SuccessorGenerator<LiftedTag>::for_each_applicable_action_binding(const Source& node,
                                                                       formalism::ActionView<LiftedTag> action,
                                                                       const std::function<bool(formalism::ActionRowView)>& callback)
{
    validate_task(m_impl->definition->task, node);
    return m_impl->for_each_applicable_action_binding(
        node,
        action,
        [&](auto& binding)
        { return callback(m_impl->definition->task->get_repository()->insert(binding, m_impl->evaluator.workspace.planning_builder).first); });
}

bool SuccessorGenerator<LiftedTag>::for_each_borrowed_applicable_action_binding(const Source& node,
                                                                                const std::function<bool(ActionBindingView<LiftedTag>)>& callback)
{
    validate_task(m_impl->definition->task, node);
    return m_impl->for_each_applicable_action_binding(node,
                                                      std::nullopt,
                                                      [&](auto& binding)
                                                      { return callback(urd::resolve(binding, *m_impl->definition->task->get_repository())); });
}

bool SuccessorGenerator<LiftedTag>::for_each_borrowed_applicable_action_binding(const Source& node,
                                                                                formalism::ActionView<LiftedTag> action,
                                                                                const std::function<bool(ActionBindingView<LiftedTag>)>& callback)
{
    validate_task(m_impl->definition->task, node);
    return m_impl->for_each_applicable_action_binding(node,
                                                      action,
                                                      [&](auto& binding)
                                                      { return callback(urd::resolve(binding, *m_impl->definition->task->get_repository())); });
}

bool SuccessorGenerator<LiftedTag>::for_each_borrowed_applicable_action_binding(const Source& node,
                                                                                formalism::ActionView<LiftedTag> action,
                                                                                const std::function<bool(ActionBindingView<LiftedTag>)>& accept,
                                                                                const std::function<bool(ActionBindingView<LiftedTag>)>& callback,
                                                                                const std::function<bool()>& stop)
{
    validate_task(m_impl->definition->task, node);
    const auto& repository = *m_impl->definition->task->get_repository();
    return m_impl->for_each_applicable_action_binding(
        node,
        action,
        [&](auto& binding) { return accept(urd::resolve(binding, repository)); },
        [&](auto& binding) { return callback(urd::resolve(binding, repository)); },
        [&] { return stop(); });
}

PackedNode<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_initial_node(StateRepository<LiftedTag>& state_repository,
                                                                             AxiomEvaluator<LiftedTag>& axiom_evaluator)
{ return get_initial_node(state_repository, axiom_evaluator).pack(); }

PackedNode<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_node(StateRepository<LiftedTag>& state_repository, urd::index<State<LiftedTag>> state_index)
{ return get_node(state_repository, state_index).pack(); }

formalism::ActionView<GroundTag> SuccessorGenerator<LiftedTag>::ground_action(formalism::ActionRowView binding)
{
    if (const auto it = m_impl->evaluator.action_binding_to_ground_action.find(binding); it != m_impl->evaluator.action_binding_to_ground_action.end())
        return it->second;

    m_impl->evaluator.workspace.binding.clear();
    for (const auto object : binding.get<urd::index<formalism::Object>>())
        m_impl->evaluator.workspace.binding.push_back(object);

    auto grounder_context = formalism::GroundingContext { m_impl->evaluator.workspace.planning_builder,
                                                          *m_impl->definition->task->get_repository(),
                                                          m_impl->evaluator.workspace.binding };
    // Without a state, an effect has every completed binding that the static consistency graph of its rule allows.
    const auto action = binding.get_relation();
    const auto objects =
        urd::flat_vector<urd::index<formalism::Object>>(m_impl->evaluator.workspace.binding.begin(), m_impl->evaluator.workspace.binding.end());
    const auto& mapping = m_impl->definition->action_program.get_conditional_effect_to_predicate_mapping();
    const auto& rules = m_impl->definition->action_program.get_datalog_program().get_const_program_workspace().engine.rules;
    const auto schema_rules = m_impl->get_schema_evaluator(action).program.get_rules();
    const auto ground_action =
        formalism::ground(
            action,
            grounder_context,
            *m_impl->definition->task->get_fdr_context(),
            [&](auto effect, auto&& apply)
            {
                const auto it = mapping.find(std::pair(action, effect));
                if (it == mapping.end())
                {
                    apply();
                    return;
                }
                const auto rule =
                    *std::ranges::find_if(schema_rules,
                                          [&](auto schema_rule) { return schema_rule.get_derived_heads().front().get_predicate() == it->second; });
                datalog::for_each_static_extension(*rules[static_cast<ygg::uint_t>(rule.index().value)],
                                                   std::span<const urd::index<formalism::Object>>(objects.data(), objects.size()),
                                                   m_impl->evaluator.static_extension_workspace,
                                                   [&](auto extension)
                                                   {
                                                       grounder_context.binding().resize(objects.size());
                                                       grounder_context.binding().insert(grounder_context.binding().end(), extension.begin(), extension.end());
                                                       apply();
                                                   });
                grounder_context.binding().resize(objects.size());
            })
            .first;
    m_impl->evaluator.action_binding_to_ground_action.emplace(binding, ground_action);
    return ground_action;
}

Node<LiftedTag> SuccessorGenerator<LiftedTag>::finalize_successor_state(StateRepository<LiftedTag>& state_repository,
                                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                        std::shared_ptr<urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>> state,
                                                                        ygg::float_t auxiliary_value)
{
    validate_task(m_impl->definition->task, state_repository);
    validate_task(m_impl->definition->task, axiom_evaluator);
    const auto metric = evaluate_successor_metric(state_repository, *state, auxiliary_value);
    return Node<LiftedTag>(state_repository.register_state(axiom_evaluator, std::move(state)), metric);
}

Node<LiftedTag> SuccessorGenerator<LiftedTag>::finalize_successor_state(StateRepository<LiftedTag>& state_repository,
                                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                        std::shared_ptr<urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>> state,
                                                                        ygg::float_t auxiliary_value,
                                                                        const StateView<LiftedTag>& parent,
                                                                        const StateDelta& delta)
{
    validate_task(m_impl->definition->task, state_repository);
    validate_task(m_impl->definition->task, axiom_evaluator);
    const auto metric = evaluate_successor_metric(state_repository, *state, auxiliary_value);
    return Node<LiftedTag>(state_repository.register_state(axiom_evaluator, std::move(state), parent, delta), metric);
}

// Lookup
Node<LiftedTag> SuccessorGenerator<LiftedTag>::get_node(StateRepository<LiftedTag>& state_repository, urd::index<State<LiftedTag>> state_index)
{
    validate_task(m_impl->definition->task, state_repository);
    auto state = state_repository.get_registered_state(state_index);
    const auto state_context = StateContext<LiftedTag>(*m_impl->definition->task, state, 0);
    const auto state_metric =
        evaluate_metric(m_impl->definition->task->get_task().get_metric(), m_impl->definition->task->get_task().get_auxiliary_fterm_value(), state_context);
    return Node<LiftedTag>(std::move(state), state_metric);
}

const ApplicableActionProgram& SuccessorGenerator<LiftedTag>::get_action_program() const noexcept { return m_impl->definition->action_program; }

const TaskPtr<LiftedTag>& SuccessorGenerator<LiftedTag>::get_task() const noexcept { return m_impl->definition->task; }

// Diagnostics
void SuccessorGenerator<LiftedTag>::print_summary(size_t verbosity) const
{
    if (verbosity < 1)
        return;

    std::cout << "[Successor generator] Summary" << '\n';
    fmt::print(std::cout, "{}\n", m_impl->evaluator.workspace.statistics);
    datalog::print_rule_statistics(m_impl->evaluator.workspace.engine.rules);
}

template ActionBindingStatus SuccessorGenerator<LiftedTag>::check_action_binding(const Source& node, formalism::ActionRowView binding);
template ActionBindingStatus SuccessorGenerator<LiftedTag>::check_action_binding(const Source& node, formalism::ActionRowBuilderView binding);
template ActionBindingResult SuccessorGenerator<LiftedTag>::try_get_applicable_action_binding(const Source& node, formalism::ActionRowView binding);
template ActionBindingResult SuccessorGenerator<LiftedTag>::try_get_applicable_action_binding(const Source& node, formalism::ActionRowBuilderView binding);

}

namespace tyr::planning
{
namespace
{
using LiftedStateBuilder = urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>;

/// Puts the objects of an action binding into the context's binding.
inline void set_binding(urd::flat_vector<urd::index<formalism::Object>>& binding, formalism::ActionRowView row)
{
    binding.clear();
    for (const auto object : row.get<urd::index<formalism::Object>>())
        binding.push_back(object);
}

/// The FDR variables and fluent function terms a lifted condition reads in the context's binding (its atoms grounded; a
/// derived literal: every variable).
template<typename Vars, typename Terms>
void add_lifted_reads(formalism::ConjunctiveConditionView<LiftedTag> condition,
                      formalism::GroundingContext<formalism::Builder, formalism::Repository>& grounder,
                      formalism::FDRContext& fdr,
                      Vars& variables,
                      Terms& terms)
{
    for (const auto literal : condition.template get_literals<formalism::FluentTag>())
        variables.push_back(static_cast<std::uint32_t>(fdr.get_fact(formalism::ground(literal.get_atom(), grounder).first).get_variable().index().value));
    if (!condition.template get_literals<formalism::DerivedTag>().empty())
        variables.push_back(every_variable);
    for (const auto constraint : condition.get_numeric_constraints())
    {
        add_fluent_terms(formalism::ground(constraint.get_lhs(), grounder), terms);
        add_fluent_terms(formalism::ground(constraint.get_rhs(), grounder), terms);
    }
}

/// The lifted transitions' shared tools: the binding, the FDR context, and the bindings of an action's effect for which
/// its static condition holds (its static extensions, without the state: a running action's start condition need not
/// hold any more, so the state's effect atoms do not report its end effects).
template<typename Impl>
struct LiftedTools
{
    Impl* impl;
    formalism::GroundingContext<formalism::Builder, formalism::Repository> grounder;
    formalism::FDRContext* fdr;

    explicit LiftedTools(Impl& impl_) :
        impl(&impl_),
        grounder(impl_.evaluator.workspace.planning_builder, *impl_.definition->task->get_repository(), impl_.evaluator.workspace.binding),
        fdr(impl_.definition->task->get_fdr_context().get())
    {
    }

    /// Calls `apply` with each static extension of the effect in the grounder's binding (it holds the action binding before
    /// and after); an effect without variables and condition fires once.
    template<typename Apply>
    void for_each_static_binding(formalism::ActionView<LiftedTag> action, formalism::ConditionalEffectView<LiftedTag> effect, const Apply& apply)
    {
        const auto& mapping = impl->definition->action_program.get_conditional_effect_to_predicate_mapping();
        const auto it = mapping.find(std::pair(action, effect));
        if (it == mapping.end())
        {
            apply();
            return;
        }
        const auto& rules = impl->definition->action_program.get_datalog_program().get_const_program_workspace().engine.rules;
        const auto schema_rules = impl->get_schema_evaluator(action).program.get_rules();
        const auto rule =
            *std::ranges::find_if(schema_rules, [&](auto schema_rule) { return schema_rule.get_derived_heads().front().get_predicate() == it->second; });
        auto& binding = grounder.binding();
        const auto action_arity = binding.size();
        auto& evaluator = impl->evaluator;
        evaluator.static_objects.clear();
        for (const auto object : binding)
            evaluator.static_objects.push_back(object);
        const auto& rule_workspace = rules[static_cast<ygg::uint_t>(rule.index().value)];
        if (!rule_workspace)
            throw std::logic_error("LiftedTools::for_each_static_binding: the effect's rule has no workspace.");
        datalog::for_each_static_extension(*rule_workspace,
                                           std::span<const urd::index<formalism::Object>>(evaluator.static_objects),
                                           evaluator.static_extension_workspace,
                                           [&](auto extension)
                                           {
                                               binding.resize(action_arity);
                                               binding.insert(binding.end(), extension.begin(), extension.end());
                                               apply();
                                           });
        binding.resize(action_arity);
    }

    /// The reads of effects (their conditions, for each binding of their variables: evaluated, whether they fire or not).
    template<typename Effects>
    void add_effect_reads(formalism::ActionView<LiftedTag> action, Effects effects, Happening& happening)
    {
        for (const auto effect : effects)
            for_each_static_binding(action,
                                    effect,
                                    [&] { add_lifted_reads(effect.get_condition(), grounder, *fdr, happening.reads, happening.numeric_reads); });
    }

    /// The reads of choices: their outcomes' effects' conditions (which include the choices'), nested ones too.
    template<typename Choices>
    void add_choice_reads(formalism::ActionView<LiftedTag> action, Choices choices, Happening& happening)
    {
        for (const auto choice : choices)
            for (const auto outcome : choice.get_outcomes())
            {
                add_effect_reads(action, outcome.get_effects(), happening);
                add_choice_reads(action, outcome.get_choices(), happening);
            }
    }
};

/// Whether every open durative action's over-all condition holds in the outcome's state (each read lifted, under its
/// binding).
template<typename Impl, StateViewConcept<LiftedTag> State>
bool lifted_over_all_holds(Impl& impl, const Node<LiftedTag, State>& outcome)
{
    const auto& task = *outcome.get_state().context().get_task();
    const auto context = StateContext<LiftedTag>(task, outcome.get_state(), ygg::float_t { 0 });
    auto grounder = formalism::GroundingContext { impl.evaluator.workspace.planning_builder, *task.get_repository(), impl.evaluator.workspace.binding };
    const auto applicability_context = ApplicabilityContext { context, grounder, *task.get_fdr_context() };
    return std::ranges::all_of(context.temporal_storage().get_parts().open,
                               [&](const OpenAction& open)
                               {
                                   const auto row = urd::resolve(open.action, *task.get_repository());
                                   set_binding(grounder.binding(), row);
                                   return is_applicable(running_temporal_part(row.get_relation()).get_over_all(), applicability_context);
                               });
}

/// The successor of `node` by the happening labeled `label`: each joint outcome of `set` applies `effects` (each in its
/// action binding) and its picks, then adds the happening to the temporal parts (`happening`'s reads given, its writes
/// the effects fired), its outcome filled by `make(apply, outcome, valid)`; failed where the temporal network becomes inconsistent or an
/// open over-all condition does not hold. Effect bindings are those `for_each_effect_binding` reports. Under (minimize
/// (total-time)), the metric is the makespan.
template<typename Impl, typename ForEachEffectBinding, typename Make>
SuccessorHandle<LiftedTag> lifted_transition(Impl& impl,
                                             const SourceNode<LiftedTag>& node,
                                             const urd::data<Transition>& label,
                                             std::span<const LiftedEffects> effects,
                                             OutcomeRange set,
                                             ForEachEffectBinding& for_each_effect_binding,
                                              const TemporalStepper::Kind& temporal,
                                              Make make)
{
    auto& evaluator = impl.evaluator;
    auto& fdr = *impl.definition->task->get_fdr_context();
    auto grounder = formalism::GroundingContext { evaluator.workspace.planning_builder, *impl.definition->task->get_repository(), evaluator.workspace.binding };
    const auto time_metric = minimizes_total_time(node.get_task());
    const auto views = [&](std::span<const LiftedPick> picks)
    {
        evaluator.outcome_picks.clear();
        for (const auto& pick : picks)
            evaluator.outcome_picks.push_back(LiftedOutcomePick { pick.action, pick.outcome, evaluator.outcome_builder.binding(pick.binding) });
        return std::span<const LiftedOutcomePick>(evaluator.outcome_picks);
    };
    return generate_successor<LiftedTag>(
        evaluator.outcome_builders,
        label,
        evaluator.outcome_distributions,
        set,
        [&](std::span<const LiftedPick> picks)
        { return evaluator.executor.is_applicable_if_fires(effects, views(picks), node.state(), grounder, fdr, for_each_effect_binding); },
        [&](std::span<const LiftedPick> picks, auto& outcome)
        {
            const auto picked = views(picks);
            return make(
                [&](LiftedStateBuilder& state)
                {
                    const auto auxiliary_value = evaluator.executor.apply_effects_unregistered(node.state(),
                                                                                               effects,
                                                                                               picked,
                                                                                               grounder,
                                                                                               fdr,
                                                                                               for_each_effect_binding,
                                                                                               state,
                                                                                               evaluator.successor_delta);
                    auto& happening = evaluator.happening;
                    happening.writes.clear();
                    happening.numeric_writes.clear();
                    evaluator.executor.fired_writes(happening.writes, happening.numeric_writes);
                    auto makespan = time_metric ? node.get_metric() : ygg::float_t { 0 };
                    evaluator.scheduled = evaluator.stepper.add(state.temporal_storage.parts, happening, temporal, makespan);
                    return time_metric ? std::max(auxiliary_value, makespan) : auxiliary_value;
                },
                outcome,
                [&](const auto& successor) { return evaluator.scheduled && lifted_over_all_holds(impl, successor); });
        });
}

/// The successor of applying the instantaneous action binding, or of starting the durative one (none if its duration is
/// not a nonnegative number), without grounding the action: its effect bindings and its choices' instances are those the
/// state's effect atoms report.
template<typename Impl, typename Make>
SuccessorHandle<LiftedTag> lifted_action_transition(Impl& impl, const SourceNode<LiftedTag>& node, formalism::ActionRowView binding, Make make)
{
    auto& evaluator = impl.evaluator;
    const auto action = binding.get_relation();
    auto tools = LiftedTools<Impl>(impl);
    auto& grounder = tools.grounder;
    set_binding(grounder.binding(), binding);
    impl.require_action_facts(node, action);

    evaluator.transition_objects.clear();
    for (const auto object : grounder.binding())
        evaluator.transition_objects.push_back(object);
    const auto objects = std::span<const urd::index<formalism::Object>>(evaluator.transition_objects);

    auto for_each_effect_binding = [&](formalism::ActionView<LiftedTag> effect_action, formalism::ConditionalEffectView<LiftedTag> effect, auto apply)
    { evaluator.effect_atom_index.for_each_effect_binding(effect_action, effect, grounder.binding(), apply); };
    auto for_each_own_effect_binding = [&](formalism::ConditionalEffectView<LiftedTag> effect, auto apply) { for_each_effect_binding(action, effect, apply); };

    auto& happening = evaluator.happening;
    happening.clear();
    add_lifted_reads(action.get_condition(), grounder, *tools.fdr, happening.reads, happening.numeric_reads);
    tools.add_effect_reads(action, action.get_effects(), happening);
    tools.add_choice_reads(action, action.get_choices(), happening);

    auto label = urd::data<Transition>(TransitionKind::APPLY, urd::flat_optional<ActionRowKey>(binding.index()), 0);
    auto temporal = TemporalStepper::Kind {};
    auto applicable = true;
    if (const auto temporal_part = action.get_temporal_part(); temporal_part.has_value())
    {
        label.kind = TransitionKind::START;
        const auto [min_duration, max_duration] = duration_bounds(temporal_part->get_min_durations(),
                                                                  temporal_part->get_max_durations(),
                                                                  ApplicabilityContext { node.state(), grounder, *tools.fdr });
        applicable = min_duration <= max_duration;
        temporal = TemporalStepper::Kind { TransitionKind::START, 0, binding.index(), min_duration, max_duration, 0 };
        add_lifted_reads(temporal_part->get_over_all(), grounder, *tools.fdr, happening.over_all, happening.numeric_over_all);
    }

    auto result = SuccessorHandle<LiftedTag> {};
    if (!applicable)
        result = empty_successor(evaluator.outcome_builders, label);
    else
    {
        evaluator.outcome_distributions.clear();
        evaluator.outcome_builder.clear();
        const auto set =
            evaluator.outcome_builder.build(evaluator.outcome_distributions, action, action.get_choices(), grounder.binding(), for_each_own_effect_binding);
        const auto effects = std::array { LiftedEffects { action, action.get_effects(), objects } };
        result = lifted_transition(impl, node, label, std::span<const LiftedEffects>(effects), set, for_each_effect_binding, temporal, make);
    }
    impl.end_single_binding_work(node);
    return result;
}

/// The successor of ending the open durative action at position `open` (none if its end condition does not hold): its end
/// effects and choices apply for the bindings of their variables whose conditions hold, and it reads its over-all
/// condition up to here.
template<typename Impl, typename Make>
SuccessorHandle<LiftedTag> lifted_end_transition(Impl& impl, const SourceNode<LiftedTag>& node, std::uint32_t open, Make make)
{
    auto& evaluator = impl.evaluator;
    auto tools = LiftedTools<Impl>(impl);
    auto& grounder = tools.grounder;
    const auto row = urd::resolve(node.state().temporal_storage().get_parts().open[open].action, *impl.definition->task->get_repository());
    const auto action = row.get_relation();
    const auto label = urd::data<Transition>(TransitionKind::END, urd::flat_optional<ActionRowKey>(row.index()), open);
    set_binding(grounder.binding(), row);
    const auto applicability_context = ApplicabilityContext { node.state(), grounder, *tools.fdr };
    const auto temporal_part = running_temporal_part(action);
    if (!is_applicable(temporal_part.get_end_condition(), applicability_context))
        return empty_successor(evaluator.outcome_builders, label);

    evaluator.transition_objects.clear();
    for (const auto object : grounder.binding())
        evaluator.transition_objects.push_back(object);
    const auto objects = std::span<const urd::index<formalism::Object>>(evaluator.transition_objects);

    auto for_each_effect_binding = [&](formalism::ActionView<LiftedTag> effect_action, formalism::ConditionalEffectView<LiftedTag> effect, auto apply)
    {
        tools.for_each_static_binding(effect_action,
                                      effect,
                                      [&]
                                      {
                                          if (is_applicable(effect.get_condition(), applicability_context))
                                              apply();
                                      });
    };
    auto for_each_own_effect_binding = [&](formalism::ConditionalEffectView<LiftedTag> effect, auto apply) { for_each_effect_binding(action, effect, apply); };

    auto& happening = evaluator.happening;
    happening.clear();
    add_lifted_reads(temporal_part.get_end_condition(), grounder, *tools.fdr, happening.reads, happening.numeric_reads);
    add_lifted_reads(temporal_part.get_over_all(), grounder, *tools.fdr, happening.reads, happening.numeric_reads);
    tools.add_effect_reads(action, temporal_part.get_end_effects(), happening);
    tools.add_choice_reads(action, temporal_part.get_end_choices(), happening);

    evaluator.outcome_distributions.clear();
    evaluator.outcome_builder.clear();
    const auto set = evaluator.outcome_builder.build(evaluator.outcome_distributions,
                                                     action,
                                                     temporal_part.get_end_choices(),
                                                     grounder.binding(),
                                                     for_each_own_effect_binding);
    const auto effects = std::array { LiftedEffects { action, temporal_part.get_end_effects(), objects } };
    const auto [end_min, end_max] = duration_bounds(temporal_part.get_end_min_durations(), temporal_part.get_end_max_durations(), applicability_context);
    if (!(end_min <= end_max))
        return empty_successor(evaluator.outcome_builders, label);
    const auto temporal = TemporalStepper::Kind { TransitionKind::END, open, {}, 0, 0, 0, 0, end_min, end_max };
    return lifted_transition(impl, node, label, std::span<const LiftedEffects>(effects), set, for_each_effect_binding, temporal, make);
}

/// Fills an outcome by registering the applied state (as a delta to `node`'s state where it is registered).
template<typename Generator, typename Impl>
auto registering(Generator& generator,
                 Impl& impl,
                 const SourceNode<LiftedTag>& node,
                 StateRepository<LiftedTag>& storage,
                 AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    return [&generator, &impl, &node, &storage, &axiom_evaluator](auto apply, auto& outcome, auto valid)
    {
        auto& delta = impl.evaluator.successor_delta;
        return register_outcome(storage,
                                outcome,
                                apply,
                                [&](std::shared_ptr<LiftedStateBuilder> state, ygg::float_t auxiliary_value)
                                {
                                    if (const auto* parent = node.registered())
                                        return generator.finalize_successor_state(storage, axiom_evaluator, std::move(state), auxiliary_value, *parent, delta);
                                    return generator.finalize_successor_state(storage, axiom_evaluator, std::move(state), auxiliary_value);
                                },
                                valid);
    };
}

/// Fills an outcome by writing the applied state into a state builder checked out of `builders`, closed under the axioms.
inline auto building(const SourceNode<LiftedTag>& node, AxiomEvaluator<LiftedTag>& axiom_evaluator, OutcomeStorage<LiftedTag>& builders)
{
    return [&node, &axiom_evaluator, &builders](auto apply, auto& outcome, auto valid)
    { return build_outcome(node, axiom_evaluator, builders, outcome, apply, valid); };
}
}

SuccessorHandle<LiftedTag> SuccessorGenerator<LiftedTag>::get_successor(const Source& node,
                                                                  formalism::ActionRowView binding,
                                                                  StateRepository<LiftedTag>& storage,
                                                                  AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    m_impl->validate(node, storage, axiom_evaluator);
    return lifted_action_transition(*m_impl, node, binding, registering(*this, *m_impl, node, storage, axiom_evaluator));
}

SuccessorHandle<LiftedTag> SuccessorGenerator<LiftedTag>::get_end_successor(const Source& node,
                                                                      std::uint32_t open,
                                                                      StateRepository<LiftedTag>& storage,
                                                                      AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    m_impl->validate(node, storage, axiom_evaluator);
    return lifted_end_transition(*m_impl, node, open, registering(*this, *m_impl, node, storage, axiom_evaluator));
}

bool SuccessorGenerator<LiftedTag>::for_each_successor(const Source& node,
                                                       StateRepository<LiftedTag>& storage,
                                                       AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                       const std::function<bool(const SuccessorView<LiftedTag>&)>& callback)
{
    const auto exhausted = for_each_applicable_action_binding(node,
                                                              [&](formalism::ActionRowView binding)
                                                              {
                                                                  const auto successor = get_successor(node, binding, storage, axiom_evaluator);
                                                                  return successor->distributions.empty() || callback(urd::resolve(*successor, storage));
                                                              });
    if (!exhausted)
        return false;
    const auto open_count = static_cast<std::uint32_t>(node.state().temporal_storage().get_parts().open.size());
    for (std::uint32_t open = 0; open < open_count; ++open)
    {
        const auto ending = get_end_successor(node, open, storage, axiom_evaluator);
        if (!ending->distributions.empty() && !callback(urd::resolve(*ending, storage)))
            return false;
    }
    const auto timed = get_timed_literal_successor(node, storage, axiom_evaluator);
    return timed->distributions.empty() || callback(urd::resolve(*timed, storage));
}

SuccessorHandle<LiftedTag>
SuccessorGenerator<LiftedTag>::get_successor(const Source& node, formalism::ActionRowView binding, AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto& builders = m_impl->evaluator.outcome_builders;
    m_impl->validate(node, builders, axiom_evaluator);
    return lifted_action_transition(*m_impl, node, binding, building(node, axiom_evaluator, builders));
}

SuccessorHandle<LiftedTag>
SuccessorGenerator<LiftedTag>::get_end_successor(const Source& node, std::uint32_t open, AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto& builders = m_impl->evaluator.outcome_builders;
    m_impl->validate(node, builders, axiom_evaluator);
    return lifted_end_transition(*m_impl, node, open, building(node, axiom_evaluator, builders));
}

bool SuccessorGenerator<LiftedTag>::for_each_successor(const Source& node,
                                                       AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                       const std::function<bool(const SuccessorView<LiftedTag>&)>& callback)
{
    const auto exhausted = for_each_applicable_action_binding(node,
                                                              [&](formalism::ActionRowView binding)
                                                              {
                                                                  const auto successor = get_successor(node, binding, axiom_evaluator);
                                                                  return successor->distributions.empty() || callback(urd::resolve(*successor, node.get_state_repository()));
                                                              });
    if (!exhausted)
        return false;
    const auto open_count = static_cast<std::uint32_t>(node.state().temporal_storage().get_parts().open.size());
    for (std::uint32_t open = 0; open < open_count; ++open)
    {
        const auto ending = get_end_successor(node, open, axiom_evaluator);
        if (!ending->distributions.empty() && !callback(urd::resolve(*ending, node.get_state_repository())))
            return false;
    }
    const auto timed = get_timed_literal_successor(node, axiom_evaluator);
    return timed->distributions.empty() || callback(urd::resolve(*timed, node.get_state_repository()));
}

SuccessorHandle<LiftedTag>
SuccessorGenerator<LiftedTag>::get_timed_literal_successor(const Source& node, StateRepository<LiftedTag>& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    m_impl->validate(node, storage, axiom_evaluator);
    return timed_literal_transition(m_impl->evaluator,
                                    node,
                                    registering(*this, *m_impl, node, storage, axiom_evaluator),
                                    [&](const auto& outcome) { return lifted_over_all_holds(*m_impl, outcome); });
}

SuccessorHandle<LiftedTag> SuccessorGenerator<LiftedTag>::get_timed_literal_successor(const Source& node,
                                                                                                             AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto& builders = m_impl->evaluator.outcome_builders;
    m_impl->validate(node, builders, axiom_evaluator);
    return timed_literal_transition(m_impl->evaluator,
                                    node,
                                    building(node, axiom_evaluator, builders),
                                    [&](const auto& outcome) { return lifted_over_all_holds(*m_impl, outcome); });
}
}

// The pools successors are handed out of (yggdrasil/formalism/builder_instantiations.hpp).
YGG_FORMALISM_BUILDER(, tyr::planning::OutcomeStorage<tyr::LiftedTag>, 2)
