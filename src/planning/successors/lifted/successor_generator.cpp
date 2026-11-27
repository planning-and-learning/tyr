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

#include "tyr/planning/successors/lifted/successor_generator.hpp"

#include "../../metric.hpp"
#include "../outcome_distributions.hpp"
#include "../temporal_transitions.hpp"
#include "effect_atom_index.hpp"
#include "lifted_outcomes.hpp"
#include "tyr/datalog/lifted/contexts/program.hpp"
#include "tyr/datalog/lifted/kckp/static_extensions.hpp"
#include "tyr/datalog/solver.hpp"
#include "tyr/formalism/grounding_context.hpp"
#include "tyr/formalism/planning/grounder.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/planning/successors/action_executor.hpp"
#include "tyr/planning/successors/applicability_lifted.hpp"
#include "tyr/planning/axioms/axiom_evaluator.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/successors/ground/match_tree/match_tree.hpp"
#include "tyr/planning/programs/lifted/action.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"
#include "tyr/planning/nodes/node_view.hpp"
#include "tyr/planning/states/state_builder.hpp"
#include "tyr/planning/states/state_delta.hpp"
#include "tyr/planning/states/state_repository.hpp"
#include "tyr/planning/successors/successor_generator.hpp"
#include "tyr/planning/tasks/task_utils.hpp"

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
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::planning
{
namespace
{
using Source = NodeBuilderView<LiftedTag>;
using Builder = urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>;
using Bindings = urd::Store<urd::sequence<formalism::ActionRowView>>;
using BindingData = urd::builder<ygg::Row<formalism::Action<LiftedTag>>, formalism::Repository>;

/// The action a binding names, read through `repository`: interned (a binding's relation is an interned action schema,
/// as its rows' relation).
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
    if (node.context().get_task() != task)
        throw std::invalid_argument("SuccessorGenerator: state belongs to a different task.");
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
        /// Whether transitions keep a temporal network (is_temporal).
        bool temporal;
        /// The fluent predicates some effect (of an action, an outcome or an end) or timed initial literal adds.
        urd::Store<urd::set<urd::index<formalism::Predicate<formalism::FluentTag>>>> added_predicates;
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
        /// The scratch of the static extensions of each effect rule (by its index), each kept prepared for its rule's
        /// static consistency graph: rules of different graphs sharing one would rebuild it on every switch.
        std::vector<datalog::StaticExtensionWorkspace> static_extension_workspaces;

        datalog::StaticExtensionWorkspace& static_extension_workspace(std::size_t rule)
        {
            if (rule >= static_extension_workspaces.size())
                static_extension_workspaces.resize(rule + 1);
            return static_extension_workspaces[rule];
        }
        /// The effect atoms of the facts, grouped by action binding.
        EffectAtomIndex effect_atom_index;
        ActionExecutor executor;
        StateDelta successor_delta;
        LiftedOutcomeDistributions outcome_distributions;
        LiftedOutcomeBuilder outcome_builder;
        urd::Store<urd::sequence<LiftedOutcomePick>> outcome_picks;
        Happening happening;
        TemporalStepper stepper;
        bool scheduled = true;
        /// The objects of the action binding a transition applies.
        urd::Store<urd::sequence<urd::index<formalism::Object>>> transition_objects;
        urd::Store<urd::sequence<urd::index<formalism::Object>>> static_objects;

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
        if (is_registered(node))
            return { static_cast<const void*>(&node.context()), static_cast<ygg::uint_t>(node.pack().state.value) };
        return { static_cast<const void*>(&state_context_of(node).numeric_storage().get_values()), std::numeric_limits<ygg::uint_t>::max() };
    }

    /// Forgets the facts of a state builder after single-binding work, since the builder can change.
    void end_single_binding_work(const Source& node)
    {
        if (!is_registered(node) && !evaluator.facts.enumerating)
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

    /// The core: writes the successor of `node` by `binding` (interned, or borrowed) into `out_state`, without axiom closure.
    template<formalism::ActionRowViewConcept Binding>
    ygg::float_t generate(const Source& node, Binding binding, Builder& out_state, StateDelta& out_delta);

    /// Like generate(), by a ground action.
    ygg::float_t generate(const Source& node, formalism::ActionView<GroundTag> action, Builder& out_state, StateDelta& out_delta)
    {
        const auto state_context = state_context_of(node);
        return evaluator.executor.apply_action_unregistered(state_context, action, out_state, out_delta);
    }

    template<typename Storage>
    void validate(const Source& node, const Storage& storage, const AxiomEvaluator<LiftedTag>& axiom_evaluator) const
    {
        validate_task(definition->task, node);
        validate_task(definition->task, axiom_evaluator);
        validate_storage(definition->task, node, storage);
    }

    std::shared_ptr<const Definition> definition;
    Evaluator evaluator;
};

namespace
{
/// Adds to `added` the fluent predicates the effects and choices add (nested choices too).
template<typename Effects, typename Choices, typename Added>
void add_added_predicates(Effects effects, Choices choices, Added& added)
{
    for (const auto effect : effects)
        for (const auto literal : effect.get_effect().get_literals())
            if (literal.get_polarity())
                added.insert(literal.get_atom().get_predicate().index());
    for (const auto choice : choices)
        for (const auto outcome : choice.get_outcomes())
            add_added_predicates(outcome.get_effects(), outcome.get_choices(), added);
}
}

SuccessorGenerator<LiftedTag>::Impl::Definition::Definition(TaskPtr<LiftedTag> task_) :
    task(std::move(task_)),
    action_program(task->get_formalism_task()),
    temporal(is_temporal(*task)),
    added_predicates()
{
    for (const auto action : task->get_task().get_domain().get_actions())
    {
        add_added_predicates(action.get_effects(), action.get_choices(), added_predicates);
        if (const auto temporal_part = action.get_temporal_part(); temporal_part.has_value())
            add_added_predicates(temporal_part->get_end_effects(), temporal_part->get_end_choices(), added_predicates);
    }
    for (const auto timed_literal : task->get_task().get_timed_literals())
        if (timed_literal.get_literal().get_polarity())
            added_predicates.insert(timed_literal.get_literal().get_atom().get_predicate().index());
}

SuccessorGenerator<LiftedTag>::Impl::Evaluator::Evaluator(const Definition& definition, ygg::ExecutionContextPtr execution_context_) :
    execution_context(std::move(execution_context_)),
    scratch_builder(),
    scratch_action_binding(formalism::checkout<ygg::Row<formalism::Action<LiftedTag>>>(scratch_builder)),
    action_binding_to_ground_action(),
    workspace(definition.action_program.get_datalog_program()),
    static_extension_workspaces(),
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
        insert_extended_state(state_context_of(node), *definition->task->get_repository(), *schema->inputs, evaluator.workspace);
    else
        insert_extended_state(state_context_of(node), *definition->task->get_repository(), AllStateInputs {}, evaluator.workspace);

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
    } end_enumeration { evaluator.facts, is_registered(node) };

    auto& scratch_binding = *evaluator.scratch_action_binding;

    const auto state_context = state_context_of(node);
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

template<formalism::ActionRowViewConcept Binding>
ygg::float_t SuccessorGenerator<LiftedTag>::Impl::generate(const Source& node, Binding binding, Builder& out_state, StateDelta& out_delta)
{
    evaluator.workspace.binding.clear();
    for (const auto value : binding.get_values())
        evaluator.workspace.binding.push_back(ygg::decode_value<urd::index<formalism::Object>>(value));

    auto grounder_context =
        formalism::GroundingContext { evaluator.workspace.planning_builder, *definition->task->get_repository(), evaluator.workspace.binding };
    const auto state_context = state_context_of(node);
    const auto action = action_of(binding);
    if (action.get_temporal_part().has_value())
        throw std::invalid_argument("SuccessorGenerator: a durative action happens at its start and its end, within a temporal network: use "
                                    "generate_successor or for_each_successor.");
    if (!action.get_choices().empty())
        throw std::invalid_argument(
            "SuccessorGenerator: an action with oneof or probabilistic effects has several outcomes: use generate_successor or for_each_successor.");

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
    const auto state = state_context_of(node);
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

template<formalism::ActionRowViewConcept Binding>
ActionBindingStatus SuccessorGenerator<LiftedTag>::check_action_binding(const Source& node, Binding binding)
{
    // A binding builder's row reads its values untyped (ygg::Value): each is an object's index.
    return m_impl->check_action_binding(node, action_of(binding), binding.template resolve<formalism::Object>());
}

void SuccessorGenerator<LiftedTag>::validate_action_binding(ActionBindingView<LiftedTag> binding) { m_impl->validate_binding(binding); }

template<formalism::ActionRowViewConcept Binding>
ygg::float_t SuccessorGenerator<LiftedTag>::generate_successor_state(const Source& node, Binding binding, Builder& out_state, StateDelta& out_delta)
{
    validate_task(m_impl->definition->task, node);
    return m_impl->generate(node, binding, out_state, out_delta);
}

bool SuccessorGenerator<LiftedTag>::for_each_applicable_action_binding(const Source& node,
                                                                       const ActionBindingFilter<LiftedTag>& filter,
                                                                       const std::function<bool(ActionBindingView<LiftedTag>)>& callback)
{
    validate_task(m_impl->definition->task, node);
    const auto& repository = *m_impl->definition->task->get_repository();
    return m_impl->for_each_applicable_action_binding(
        node,
        filter.action,
        [&](auto& binding) { return !filter.accept || filter.accept(urd::resolve(binding, repository)); },
        [&](auto& binding) { return callback(urd::resolve(binding, repository)); },
        [&] { return filter.stop && filter.stop(); });
}

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
                                                   m_impl->evaluator.static_extension_workspace(rule.index().value),
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

// Lookup
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

namespace
{
using LiftedStateBuilder = urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>;
using ActionRow = ygg::Row<formalism::Action<LiftedTag>>;

/// Puts the objects of an action binding (interned or borrowed) into `objects`.
template<formalism::ActionRowViewConcept Binding>
void set_binding(urd::flat_vector<urd::index<formalism::Object>>& objects, Binding binding)
{
    objects.clear();
    for (const auto value : binding.get_values())
        objects.push_back(ygg::decode_value<urd::index<formalism::Object>>(value));
}

/// The FDR variables and fluent function terms a lifted condition reads in the grounder's binding (its atoms grounded; a
/// derived literal: every variable).
template<typename Grounder, typename Vars, typename Terms>
void add_lifted_reads(formalism::ConjunctiveConditionView<LiftedTag> condition, Grounder& grounder, formalism::FDRContext& fdr, Vars& variables, Terms& terms)
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

/// The lifted transitions' shared tools: the grounder over the workspace's binding, the FDR context, and the bindings of
/// an action's effect for which its static condition holds (its static extensions, without the state: a running action's
/// start condition need not hold any more, so the state's effect atoms do not report its end effects).
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
                                           evaluator.static_extension_workspace(rule.index().value),
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
            for_each_static_binding(action, effect, [&] { add_lifted_reads(effect.get_condition(), grounder, *fdr, happening.reads, happening.numeric_reads); });
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

/// Whether every open durative action's over-all condition holds in `state` (an outcome's; each read lifted, under its
/// binding).
template<typename Impl>
bool lifted_over_all_holds(Impl& impl, const StateBuilderView<LiftedTag>& state)
{
    const auto& task = *state.context().get_task();
    const auto context = StateContext<LiftedTag>(task, state, ygg::float_t { 0 });
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

/// Writes into `out` the successor of `node` by the happening labeled `label`: each joint outcome of `set` applies
/// `effects` (each in its action binding) and its picks, then adds the happening to the temporal parts (`happening`'s
/// reads given, its writes the effects fired), its outcome filled by `make(apply, outcome, valid)`; failed where the
/// temporal network becomes inconsistent or an open over-all condition does not hold. Effect bindings are those
/// `for_each_effect_binding` reports. Under (minimize (total-time)), the metric is the makespan.
template<typename Impl, typename ForEachEffectBinding, typename Make>
void lifted_transition(Impl& impl,
                       const NodeBuilderView<LiftedTag>& node,
                       const urd::data<Transition>& label,
                       std::span<const LiftedEffects> effects,
                       OutcomeRange set,
                       ForEachEffectBinding& for_each_effect_binding,
                       const TemporalStepper::Kind& temporal,
                       Make make,
                       SuccessorBuilder<LiftedTag>& out)
{
    auto& evaluator = impl.evaluator;
    auto& fdr = *impl.definition->task->get_fdr_context();
    auto grounder =
        formalism::GroundingContext { evaluator.workspace.planning_builder, *impl.definition->task->get_repository(), evaluator.workspace.binding };
    const auto state_context = state_context_of(node);
    const auto time_metric = minimizes_total_time(*node.context().get_task());
    const auto views = [&](std::span<const LiftedPick> picks)
    {
        evaluator.outcome_picks.clear();
        for (const auto& pick : picks)
            evaluator.outcome_picks.push_back(LiftedOutcomePick { pick.action, pick.outcome, evaluator.outcome_builder.binding(pick.binding) });
        return std::span<const LiftedOutcomePick>(evaluator.outcome_picks);
    };
    generate_successor<LiftedTag>(
        out,
        node.context(),
        label,
        evaluator.outcome_distributions,
        set,
        [&](std::span<const LiftedPick> picks)
        { return evaluator.executor.is_applicable_if_fires(effects, views(picks), state_context, grounder, fdr, for_each_effect_binding); },
        [&](std::span<const LiftedPick> picks, auto& outcome)
        {
            const auto picked = views(picks);
            return make(
                [&](LiftedStateBuilder& state)
                {
                    const auto auxiliary_value = evaluator.executor.apply_effects_unregistered(state_context,
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
                    evaluator.scheduled = !impl.definition->temporal || evaluator.stepper.add(urd::built(state.temporal_storage).parts, happening, temporal, makespan);
                    return time_metric ? std::max(auxiliary_value, makespan) : auxiliary_value;
                },
                outcome,
                [&](const auto& successor_state) { return evaluator.scheduled && lifted_over_all_holds(impl, successor_state); });
        });
}

/// Whether a durative action started now can end, as the ground task decides it statically (it keeps no durative action
/// whose over-all or end condition no reachable atom satisfies): each static literal of those conditions holds, and
/// each positive fluent one holds now or names a predicate some effect or timed initial literal adds.
// ponytail: a predicate's adders stand in for the ground task's relaxed reachability of each atom; prunes less where that
// reachability is stronger, and reads no derived literal.
template<typename Impl, typename Context>
bool can_end(const Impl& impl, formalism::TemporalPartView<LiftedTag> part, const Context& context)
{
    const auto may_hold = [&](formalism::ConjunctiveConditionView<LiftedTag> condition)
    {
        return std::ranges::all_of(condition.template get_literals<formalism::StaticTag>(), [&](const auto literal) { return is_applicable(literal, context); })
               && std::ranges::all_of(condition.template get_literals<formalism::FluentTag>(),
                                      [&](const auto literal)
                                      {
                                          return !literal.get_polarity() || impl.definition->added_predicates.contains(literal.get_atom().get_predicate().index())
                                                 || is_applicable(literal, context);
                                      });
    };
    return may_hold(part.get_over_all()) && may_hold(part.get_end_condition());
}

/// The label's binding of a borrowed action binding not interned: a row built in the label, from the pool of `repository`
/// (the repository the label is read through: its values are objects of any of its layers).
template<formalism::ActionRowViewConcept Binding>
void build_label_binding(SuccessorBuilder<LiftedTag>& out, const StateRepository<LiftedTag>& repository, Binding binding)
{
    auto& label = urd::builder_of(out.label);
    auto& row = ygg::formalism::builder_of(label.binding.has_value() ? *label.binding : label.binding.emplace(), repository.builders());
    urd::store(row.relation, action_of(binding));
    row.values.clear();
    for (const auto value : binding.get_values())
        row.values.push_back(value);
}

/// Writes into `out` the successor of applying the instantaneous action binding, or of starting the durative one (none if
/// its duration is not a nonnegative number), without grounding the action: its effect bindings and its choices'
/// instances are those the state's effect atoms report. Starting interns a borrowed binding, as the state's open actions
/// name it by its key; applying keeps it in the label uninterned.
template<typename Impl, formalism::ActionRowViewConcept Binding, typename Make>
void lifted_action_transition(Impl& impl, const NodeBuilderView<LiftedTag>& node, Binding binding, Make make, SuccessorBuilder<LiftedTag>& out)
{
    auto& evaluator = impl.evaluator;
    const auto action = action_of(binding);
    auto tools = LiftedTools<Impl>(impl);
    auto& grounder = tools.grounder;
    set_binding(grounder.binding(), binding);
    impl.require_action_facts(node, action);
    const auto state_context = state_context_of(node);

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

    auto label = urd::data<Transition>(TransitionKind::APPLY, urd::flat_optional<ActionRowKey>(), 0);
    if constexpr (std::same_as<Binding, formalism::ActionRowView>)
        label.binding = binding.index();
    auto temporal = TemporalStepper::Kind {};
    auto applicable = true;
    if (const auto temporal_part = action.get_temporal_part(); temporal_part.has_value())
    {
        if (!label.binding.has_value())
        {
            auto row = evaluator.scratch_builder.template scratch<ActionRow>();
            row->relation = action.index();
            row->values.clear();
            for (const auto value : binding.get_values())
                row->values.push_back(value);
            label.binding = impl.definition->task->get_repository()->insert(*row).first.index();
        }
        label.kind = TransitionKind::START;
        const auto [min_duration, max_duration] =
            duration_bounds(temporal_part->get_min_durations(), temporal_part->get_max_durations(), ApplicabilityContext { state_context, grounder, *tools.fdr });
        applicable = min_duration <= max_duration && can_end(impl, *temporal_part, ApplicabilityContext { state_context, grounder, *tools.fdr });
        temporal = TemporalStepper::Kind { TransitionKind::START, 0, *label.binding, min_duration, max_duration, 0 };
        add_lifted_reads(temporal_part->get_over_all(), grounder, *tools.fdr, happening.over_all, happening.numeric_over_all);
    }

    if (!applicable)
        start_successor(out, node.context(), label);
    else
    {
        evaluator.outcome_distributions.clear();
        evaluator.outcome_builder.clear();
        const auto set =
            evaluator.outcome_builder.build(evaluator.outcome_distributions, action, action.get_choices(), grounder.binding(), for_each_own_effect_binding);
        const auto effects = std::array { LiftedEffects { action, action.get_effects(), objects } };
        lifted_transition(impl, node, label, std::span<const LiftedEffects>(effects), set, for_each_effect_binding, temporal, make, out);
    }
    if (!label.binding.has_value())
        build_label_binding(out, node.context(), binding);
    impl.end_single_binding_work(node);
}

/// Writes into `out` the successor of ending the open durative action at position `open` (none if its end condition does
/// not hold): its end effects and choices apply for the bindings of their variables whose conditions hold, and it reads
/// its over-all condition up to here.
template<typename Impl, typename Make>
void lifted_end_transition(Impl& impl, const NodeBuilderView<LiftedTag>& node, std::uint32_t open, Make make, SuccessorBuilder<LiftedTag>& out)
{
    auto& evaluator = impl.evaluator;
    auto tools = LiftedTools<Impl>(impl);
    auto& grounder = tools.grounder;
    const auto state_context = state_context_of(node);
    const auto row = urd::resolve(state_context.temporal_storage().get_parts().open[open].action, *impl.definition->task->get_repository());
    const auto action = row.get_relation();
    const auto label = urd::data<Transition>(TransitionKind::END, urd::flat_optional<ActionRowKey>(row.index()), open);
    set_binding(grounder.binding(), row);
    const auto applicability_context = ApplicabilityContext { state_context, grounder, *tools.fdr };
    const auto temporal_part = running_temporal_part(action);
    if (!is_applicable(temporal_part.get_end_condition(), applicability_context))
    {
        start_successor(out, node.context(), label);
        return;
    }

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
    const auto set =
        evaluator.outcome_builder.build(evaluator.outcome_distributions, action, temporal_part.get_end_choices(), grounder.binding(), for_each_own_effect_binding);
    const auto effects = std::array { LiftedEffects { action, temporal_part.get_end_effects(), objects } };
    const auto [end_min, end_max] = duration_bounds(temporal_part.get_end_min_durations(), temporal_part.get_end_max_durations(), applicability_context);
    if (!(end_min <= end_max))
    {
        start_successor(out, node.context(), label);
        return;
    }
    const auto temporal = TemporalStepper::Kind { TransitionKind::END, open, {}, 0, 0, 0, 0, end_min, end_max };
    lifted_transition(impl, node, label, std::span<const LiftedEffects>(effects), set, for_each_effect_binding, temporal, make, out);
}

/// Fills an outcome by writing the applied state into a state builder of `node`'s repository, closed under the axioms.
inline auto building(const NodeBuilderView<LiftedTag>& node, AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    return [&node, &axiom_evaluator](auto apply, auto& outcome, auto valid) { return build_outcome(node, axiom_evaluator, outcome, apply, valid); };
}
}

template<formalism::ActionRowViewConcept Binding>
void SuccessorGenerator<LiftedTag>::generate_successor(const Source& node, Binding binding, AxiomEvaluator<LiftedTag>& axiom_evaluator, SuccessorBuilder<LiftedTag>& out)
{
    validate_task(m_impl->definition->task, node);
    validate_task(m_impl->definition->task, axiom_evaluator);
    lifted_action_transition(*m_impl, node, binding, building(node, axiom_evaluator), out);
}

void SuccessorGenerator<LiftedTag>::generate_end_successor(const Source& node,
                                                           std::uint32_t open,
                                                           AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                           SuccessorBuilder<LiftedTag>& out)
{
    validate_task(m_impl->definition->task, node);
    validate_task(m_impl->definition->task, axiom_evaluator);
    lifted_end_transition(*m_impl, node, open, building(node, axiom_evaluator), out);
}

void SuccessorGenerator<LiftedTag>::generate_timed_literal_successor(const Source& node,
                                                                     AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                     SuccessorBuilder<LiftedTag>& out)
{
    validate_task(m_impl->definition->task, node);
    validate_task(m_impl->definition->task, axiom_evaluator);
    timed_literal_transition(
        m_impl->evaluator,
        node,
        building(node, axiom_evaluator),
        [&](const auto& state) { return lifted_over_all_holds(*m_impl, state); },
        out);
}

bool SuccessorGenerator<LiftedTag>::for_each_successor(const Source& node,
                                                       AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                       const std::function<bool(SuccessorBuilder<LiftedTag>&)>& callback)
{
    validate_task(m_impl->definition->task, node);
    validate_task(m_impl->definition->task, axiom_evaluator);
    const auto& repository = *m_impl->definition->task->get_repository();
    // One of the node's repository's builders, reused across its successors: back to its pool when this returns.
    auto checked_out = ygg::formalism::checkout<Successor<LiftedTag>>(node.context().builders());
    auto& successor = *checked_out;
    const auto exhausted = m_impl->for_each_applicable_action_binding(node,
                                                                      std::nullopt,
                                                                      [&](auto& binding)
                                                                      {
                                                                          lifted_action_transition(*m_impl,
                                                                                                   node,
                                                                                                   urd::resolve(binding, repository),
                                                                                                   building(node, axiom_evaluator),
                                                                                                   successor);
                                                                          return successor.distributions.empty() || callback(successor);
                                                                      });
    if (!exhausted)
        return false;
    const auto open_count = static_cast<std::uint32_t>(state_context_of(node).temporal_storage().get_parts().open.size());
    for (std::uint32_t open = 0; open < open_count; ++open)
    {
        generate_end_successor(node, open, axiom_evaluator, successor);
        if (!successor.distributions.empty() && !callback(successor))
            return false;
    }
    generate_timed_literal_successor(node, axiom_evaluator, successor);
    return successor.distributions.empty() || callback(successor);
}

template ActionBindingStatus SuccessorGenerator<LiftedTag>::check_action_binding(const Source& node, formalism::ActionRowView binding);
template ActionBindingStatus SuccessorGenerator<LiftedTag>::check_action_binding(const Source& node, formalism::ActionRowBuilderView binding);
template ygg::float_t
SuccessorGenerator<LiftedTag>::generate_successor_state(const Source& node, formalism::ActionRowView binding, Builder& out_state, StateDelta& out_delta);
template ygg::float_t
SuccessorGenerator<LiftedTag>::generate_successor_state(const Source& node, formalism::ActionRowBuilderView binding, Builder& out_state, StateDelta& out_delta);

template void SuccessorGenerator<LiftedTag>::generate_successor(const Source& node,
                                                              formalism::ActionRowView binding,
                                                              AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                              SuccessorBuilder<LiftedTag>& out);
template void SuccessorGenerator<LiftedTag>::generate_successor(const Source& node,
                                                              formalism::ActionRowBuilderView binding,
                                                              AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                              SuccessorBuilder<LiftedTag>& out);

}
