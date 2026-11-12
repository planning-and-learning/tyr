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
#include "effect_atom_index.hpp"
#include "tyr/datalog/formatter.hpp"
#include "tyr/datalog/lifted/contexts/program.hpp"
#include "tyr/datalog/lifted/kckp/static_extensions.hpp"
#include "tyr/datalog/solver.hpp"
#include "tyr/formalism/planning/grounder.hpp"
#include "tyr/formalism/planning/interning.hpp"
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
#include <yggdrasil/formalism/membership.hpp>

namespace tyr::planning
{
namespace
{
using Source = SourceNode<LiftedTag>;
using Builder = urd::builder<State<LiftedTag>>;
using BuilderNode = Node<LiftedTag, BuilderStateView<LiftedTag>>;
using Bindings = urd::Store<urd::sequence<formalism::planning::ActionRowView>>;
using BindingData = urd::data<ygg::Row<formalism::planning::Action<LiftedTag>>>;

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
    if (&storage == &node.data())
        throw std::invalid_argument("SuccessorGenerator: source and output builder must not alias.");
}

void validate_storage(const TaskPtr<LiftedTag>&, const Source&, const SuccessorBuilders<LiftedTag>&) {}

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
    using ActionBindingMap = urd::Store<urd::map<formalism::planning::ActionRowView, formalism::planning::ActionView<GroundTag>>>;
    using StateKey = std::pair<const void*, ygg::uint_t>;

    struct SchemaEvaluator
    {
        formalism::datalog::ProgramView<LiftedTag> program;
        const P2DTranslationContext<LiftedTag>* input_translation;
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
        formalism::planning::Builder scratch_builder;
        urd::unique_slot_ptr<urd::data<ygg::Row<formalism::planning::Action<LiftedTag>>>> scratch_action_binding;
        ActionBindingMap action_binding_to_ground_action;
        urd::data<ygg::Row<formalism::planning::Action<LiftedTag>>> checked_binding;
        /// The objects of scratch_action_binding and checked_binding, as the grounder binds parameters to them.
        urd::flat_vector<urd::index<formalism::Object>> scratch_objects;
        urd::flat_vector<urd::index<formalism::Object>> checked_objects;
        datalog::ProgramWorkspace<LiftedTag> workspace;
        urd::Store<urd::map<formalism::planning::ActionView<LiftedTag>, SchemaEvaluator>> schema_evaluators;
        datalog::StaticExtensionWorkspace static_extension_workspace;
        /// The effect atoms of the facts, grouped by action binding.
        EffectAtomIndex effect_atom_index;
        ActionExecutor executor;
        StateDelta successor_delta;

        /// The facts the workspace holds. A state builder's contents can change, so its facts hold only while enumerating.
        struct Facts
        {
            std::optional<StateKey> state;
            /// The action of the schema program that computed them, or none for the full program (all actions).
            std::optional<urd::index<formalism::planning::Action<LiftedTag>>> action;
            bool enumerating = false;
        } facts;

        /// The effect atoms of each action in the facts, grouped at the action's first successor.
    };

    /// Computes the facts of the node's state with the action's effects unless the workspace holds them.
    void require_action_facts(const Source& node, formalism::planning::ActionView<LiftedTag> action);

    /// The repository and index of a registered state, or the address of a state builder.
    static StateKey state_key(const Source& node)
    {
        if (const auto* state = node.registered())
            return { static_cast<const void*>(&state->get_state_repository()), static_cast<ygg::uint_t>(state->index().value) };
        return { static_cast<const void*>(&node.data()), std::numeric_limits<ygg::uint_t>::max() };
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

    SchemaEvaluator& get_schema_evaluator(formalism::planning::ActionView<LiftedTag> action)
    {
        const auto it = evaluator.schema_evaluators.find(action);
        // Repository indices can collide across independent factories.
        if (it == evaluator.schema_evaluators.end() || &action.context() != &it->first.context())
            throw std::invalid_argument("SuccessorGenerator: action schema does not belong to the task domain.");
        return it->second;
    }

    template<typename Objects>
    bool validate_binding_objects(formalism::planning::ActionView<LiftedTag> action, Objects objects)
    {
        get_schema_evaluator(action);
        if (objects.size() != action.get_arity())
            throw std::invalid_argument("SuccessorGenerator: object count does not match action arity.");
        const auto indices = objects.base();
        const auto& target_repository = *definition->task->get_repository();
        if (!std::ranges::all_of(indices, [&](const auto& value) { return objects.context().symbols().contains(value); })
            || !std::ranges::all_of(indices, [&](const auto& value) { return target_repository.symbols().contains(value); }))
            return false;
        if (!std::ranges::all_of(objects, [&](const auto& value) { return ygg::formalism::contains(target_repository, value); }))
            throw std::invalid_argument("SuccessorGenerator: object does not belong to the task repository.");
        return true;
    }

    void validate_binding(formalism::planning::ActionRowDataView binding)
    {
        if (!binding.context().symbols().contains(binding.data().relation))
            throw std::invalid_argument("SuccessorGenerator: action schema does not belong to its source repository.");
        // A borrowed binding's action may have no pool yet (no stored types): its values are read as objects directly.
        if (!validate_binding_objects(
                binding.get_relation(),
                urd::resolve(
                    urd::transform(binding.get_values(), [](ygg::Value value) { return urd::index<formalism::Object>(static_cast<std::uint32_t>(value)); }),
                    binding.context())))
            throw std::invalid_argument("SuccessorGenerator: object does not belong to the task repository.");
    }

    template<typename Objects>
    ActionBindingStatus check_action_binding(const Source& node, formalism::planning::ActionView<LiftedTag> action, Objects objects);

    /// Computes the facts of the node's state with the action's schema program, or with the full program for all actions.
    void compute_action_facts(const Source& node, std::optional<formalism::planning::ActionView<LiftedTag>> action);

    /// Calls `callback` with each applicable binding (in scratch_action_binding) of the action, or of all actions.
    template<typename Callback>
    bool for_each_applicable_action_binding(const Source& node, std::optional<formalism::planning::ActionView<LiftedTag>> action, Callback&& callback);

    template<typename Accept, typename Callback, typename Stop>
    bool for_each_applicable_action_binding(const Source& node,
                                            std::optional<formalism::planning::ActionView<LiftedTag>> action,
                                            Accept accept,
                                            Callback callback,
                                            Stop stop);

    /// The core: writes the successor of `node` by binding data into `out_state`, without axiom closure.
    ygg::float_t generate(const Source& node,
                          const urd::data<ygg::Row<formalism::planning::Action<LiftedTag>>>& binding,
                          urd::builder<State<LiftedTag>>& out_state,
                          StateDelta& out_delta);

    /// Like generate(), by a ground action.
    ygg::float_t generate(const Source& node, formalism::planning::ActionView<GroundTag> action, Builder& out_state, StateDelta& out_delta)
    {
        const auto state_context = StateContext<LiftedTag>(*definition->task, node.data(), node.get_metric());
        return evaluator.executor.apply_action_unregistered(state_context, action, out_state, out_delta);
    }

    /// Copies an interned binding into the single-binding scratch.
    const BindingData& stage(formalism::planning::ActionRowView binding)
    {
        auto& data = evaluator.checked_binding;
        data.relation = binding.get_relation().index();
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
        const auto metric = evaluate_successor_metric(*definition->task, storage, auxiliary_value);
        axiom_evaluator.compute_extended_state(storage);
        return BuilderNode(urd::resolve(storage, node.get_state_repository()), metric);
    }

    /// Passes the successor of each applicable binding of the action (or of all actions), generated into `storage`, to `sink`.
    template<typename Storage, typename Sink>
    bool for_each_successor(SuccessorGenerator& self,
                            const Source& node,
                            std::optional<formalism::planning::ActionView<LiftedTag>> action,
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
                                    std::optional<formalism::planning::ActionView<LiftedTag>> action,
                                    Storage& storage,
                                    AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                    const Sink& sink)
    {
        return for_each_applicable_action_binding(
            node,
            action,
            [&](auto& binding)
            {
                const auto label = formalism::planning::insert(*definition->task->get_repository(), binding).first;
                auto successor = self.get_successor_node(node, binding, next_storage(storage), axiom_evaluator);
                return sink(LabeledNode<LiftedTag, std::remove_cvref_t<decltype(successor.get_state())>> { label, std::move(successor) });
            });
    }

    std::shared_ptr<const Definition> definition;
    Evaluator evaluator;
};

SuccessorGenerator<LiftedTag>::Impl::Definition::Definition(TaskPtr<LiftedTag> task_) : task(std::move(task_)), action_program(task->get_task()) {}

SuccessorGenerator<LiftedTag>::Impl::Evaluator::Evaluator(const Definition& definition, ygg::ExecutionContextPtr execution_context_) :
    execution_context(std::move(execution_context_)),
    scratch_builder(),
    scratch_action_binding(formalism::planning::checkout<ygg::Row<formalism::planning::Action<LiftedTag>>>(scratch_builder)),
    action_binding_to_ground_action(),
    workspace(definition.action_program.get_datalog_program()),
    static_extension_workspace(),
    effect_atom_index(definition.action_program.get_predicate_to_action_mapping(),
                      definition.action_program.get_conditional_effect_to_predicate_mapping(),
                      workspace.facts.fact_sets,
                      *workspace.workspace_repository),
    executor()
{
    assert(execution_context);
    for (const auto& [action, schema] : definition.action_program.get_schema_programs())
        schema_evaluators.emplace(action,
                                  SchemaEvaluator { schema.program,
                                                    &schema.input_translation,
                                                    datalog::create_schedulers(schema.strata,
                                                                               schema.listeners,
                                                                               schema.program.context(),
                                                                               schema.program.get_predicates<formalism::FluentTag>().size(),
                                                                               schema.program.get_functions<formalism::FluentTag>().size()) });
}

void SuccessorGenerator<LiftedTag>::Impl::compute_action_facts(const Source& node, std::optional<formalism::planning::ActionView<LiftedTag>> action)
{
    auto* schema = action ? &get_schema_evaluator(*action) : nullptr;
    evaluator.workspace.reset_evaluation();
    evaluator.facts.state = state_key(node);
    evaluator.facts.action = action ? std::optional(action->index()) : std::nullopt;
    evaluator.effect_atom_index.clear();

    insert_extended_state(node.data(),
                          *definition->task->get_repository(),
                          schema ? *schema->input_translation : definition->action_program.get_translation_context().p2d,
                          evaluator.workspace);

    auto ctx =
        datalog::ProgramExecutionContext<LiftedTag>::from_reset_workspace(evaluator.workspace, schema ? schema->schedulers : evaluator.workspace.schedulers);
    datalog::execute_model(ctx, *evaluator.execution_context);
}

template<typename Callback>
bool SuccessorGenerator<LiftedTag>::Impl::for_each_applicable_action_binding(const Source& node,
                                                                             std::optional<formalism::planning::ActionView<LiftedTag>> action,
                                                                             Callback&& callback)
{
    return for_each_applicable_action_binding(node, action, [](const auto&) { return true; }, std::forward<Callback>(callback), [] { return false; });
}

template<typename Accept, typename Callback, typename Stop>
bool SuccessorGenerator<LiftedTag>::Impl::for_each_applicable_action_binding(const Source& node,
                                                                             std::optional<formalism::planning::ActionView<LiftedTag>> action,
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

    const auto state_context = StateContext<LiftedTag>(*definition->task, node.data(), node.get_metric());
    auto grounder_context = formalism::WriteContext { evaluator.workspace.planning_builder, *definition->task->get_repository(), evaluator.scratch_objects };
    const auto& mapping = definition->action_program.get_predicate_to_action_mapping();

    for (const auto rule : program.get_rules())
    {
        const auto predicate = rule.get_heads().front().get_predicate();
        const auto it = mapping.find(predicate);
        if (it == mapping.end())
            continue;  // A conditional effect's rule.
        const auto schema = it->second;
        for (const auto& binding : datalog::rows(evaluator.workspace.facts.fact_sets, predicate.index().value))
        {
            if (stop())
                return false;
            scratch_binding.relation = schema.index();
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
                                                           const urd::data<ygg::Row<formalism::planning::Action<LiftedTag>>>& binding,
                                                           urd::builder<State<LiftedTag>>& out_state,
                                                           StateDelta& out_delta)
{
    evaluator.workspace.binding.clear();
    for (const auto value : binding.values)
        evaluator.workspace.binding.push_back(urd::index<formalism::Object>(static_cast<std::uint32_t>(value)));

    auto grounder_context = formalism::WriteContext { evaluator.workspace.planning_builder, *definition->task->get_repository(), evaluator.workspace.binding };
    const auto state_context = StateContext<LiftedTag>(*definition->task, node.data(), node.get_metric());
    const auto action = urd::resolve(binding.relation, *definition->task->get_repository());

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

void SuccessorGenerator<LiftedTag>::Impl::require_action_facts(const Source& node, formalism::planning::ActionView<LiftedTag> action)
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
    const auto state_context = StateContext<LiftedTag>(*m_impl->definition->task, initial_state.data(), 0);
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
                                                                       formalism::planning::ActionView<LiftedTag> action,
                                                                       StateRepository<LiftedTag>& storage,
                                                                       AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = NodeList<LiftedTag> {};
    get_successor_nodes(node, action, storage, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_successor_nodes(const Source& node,
                                                        formalism::planning::ActionView<LiftedTag> action,
                                                        StateRepository<LiftedTag>& storage,
                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                        NodeList<LiftedTag>& out_nodes)
{
    m_impl->validate(node, storage, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_successor(*this, node, action, storage, axiom_evaluator, append(out_nodes));
}

NodeList<LiftedTag, BuilderStateView<LiftedTag>>
SuccessorGenerator<LiftedTag>::get_successor_nodes(const Source& node, SuccessorBuilders<LiftedTag>& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = NodeList<LiftedTag, BuilderStateView<LiftedTag>> {};
    get_successor_nodes(node, storage, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_successor_nodes(const Source& node,
                                                        SuccessorBuilders<LiftedTag>& storage,
                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                        NodeList<LiftedTag, BuilderStateView<LiftedTag>>& out_nodes)
{
    m_impl->validate(node, storage, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_successor(*this, node, std::nullopt, storage, axiom_evaluator, append(out_nodes));
}

NodeList<LiftedTag, BuilderStateView<LiftedTag>> SuccessorGenerator<LiftedTag>::get_successor_nodes(const Source& node,
                                                                                                    formalism::planning::ActionView<LiftedTag> action,
                                                                                                    SuccessorBuilders<LiftedTag>& storage,
                                                                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = NodeList<LiftedTag, BuilderStateView<LiftedTag>> {};
    get_successor_nodes(node, action, storage, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_successor_nodes(const Source& node,
                                                        formalism::planning::ActionView<LiftedTag> action,
                                                        SuccessorBuilders<LiftedTag>& storage,
                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                        NodeList<LiftedTag, BuilderStateView<LiftedTag>>& out_nodes)
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
                                                                                      formalism::planning::ActionView<LiftedTag> action,
                                                                                      StateRepository<LiftedTag>& storage,
                                                                                      AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = LabeledNodeList<LiftedTag> {};
    get_labeled_successor_nodes(node, action, storage, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes(const Source& node,
                                                                formalism::planning::ActionView<LiftedTag> action,
                                                                StateRepository<LiftedTag>& storage,
                                                                AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                LabeledNodeList<LiftedTag>& out_nodes)
{
    m_impl->validate(node, storage, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_labeled_successor(*this, node, action, storage, axiom_evaluator, append(out_nodes));
}

LabeledNodeList<LiftedTag, BuilderStateView<LiftedTag>> SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes(const Source& node,
                                                                                                                   SuccessorBuilders<LiftedTag>& storage,
                                                                                                                   AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = LabeledNodeList<LiftedTag, BuilderStateView<LiftedTag>> {};
    get_labeled_successor_nodes(node, storage, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes(const Source& node,
                                                                SuccessorBuilders<LiftedTag>& storage,
                                                                AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                LabeledNodeList<LiftedTag, BuilderStateView<LiftedTag>>& out_nodes)
{
    m_impl->validate(node, storage, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_labeled_successor(*this, node, std::nullopt, storage, axiom_evaluator, append(out_nodes));
}

LabeledNodeList<LiftedTag, BuilderStateView<LiftedTag>>
SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes(const Source& node,
                                                           formalism::planning::ActionView<LiftedTag> action,
                                                           SuccessorBuilders<LiftedTag>& storage,
                                                           AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = LabeledNodeList<LiftedTag, BuilderStateView<LiftedTag>> {};
    get_labeled_successor_nodes(node, action, storage, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes(const Source& node,
                                                                formalism::planning::ActionView<LiftedTag> action,
                                                                SuccessorBuilders<LiftedTag>& storage,
                                                                AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                LabeledNodeList<LiftedTag, BuilderStateView<LiftedTag>>& out_nodes)
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
                                                            formalism::planning::ActionView<LiftedTag> action,
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
                                                            formalism::planning::ActionView<LiftedTag> action,
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
                                                                    formalism::planning::ActionView<LiftedTag> action,
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
                                                                    const std::function<bool(LabeledNode<LiftedTag, BuilderStateView<LiftedTag>>)>& callback)
{
    m_impl->validate(node, storage, axiom_evaluator);
    return m_impl->for_each_labeled_successor(*this, node, std::nullopt, storage, axiom_evaluator, callback);
}

bool SuccessorGenerator<LiftedTag>::for_each_labeled_successor_node(const Source& node,
                                                                    formalism::planning::ActionView<LiftedTag> action,
                                                                    Builder& storage,
                                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                    const std::function<bool(LabeledNode<LiftedTag, BuilderStateView<LiftedTag>>)>& callback)
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
                                                                                    formalism::planning::ActionView<LiftedTag> action,
                                                                                    StateRepository<LiftedTag>& state_repository,
                                                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = PackedNodeList<LiftedTag> {};
    get_packed_successor_nodes(node, action, state_repository, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_packed_successor_nodes(const Source& node,
                                                               formalism::planning::ActionView<LiftedTag> action,
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
                                                                                                   formalism::planning::ActionView<LiftedTag> action,
                                                                                                   StateRepository<LiftedTag>& state_repository,
                                                                                                   AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = PackedLabeledNodeList<LiftedTag> {};
    get_packed_labeled_successor_nodes(node, action, state_repository, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes(const Source& node,
                                                                       formalism::planning::ActionView<LiftedTag> action,
                                                                       StateRepository<LiftedTag>& state_repository,
                                                                       AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                       PackedLabeledNodeList<LiftedTag>& out_nodes)
{
    m_impl->validate(node, state_repository, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_labeled_successor(*this, node, action, state_repository, axiom_evaluator, append_packed(out_nodes));
}

Node<LiftedTag> SuccessorGenerator<LiftedTag>::get_successor_node(const Source& node,
                                                                  formalism::planning::ActionView<GroundTag> action,
                                                                  StateRepository<LiftedTag>& storage,
                                                                  AxiomEvaluator<LiftedTag>& axiom_evaluator)
{ return m_impl->successor(*this, node, action, storage, axiom_evaluator); }

BuilderNode SuccessorGenerator<LiftedTag>::get_successor_node(const Source& node,
                                                              formalism::planning::ActionView<GroundTag> action,
                                                              Builder& storage,
                                                              AxiomEvaluator<LiftedTag>& axiom_evaluator)
{ return m_impl->successor(*this, node, action, storage, axiom_evaluator); }

Node<LiftedTag> SuccessorGenerator<LiftedTag>::get_successor_node(const Source& node,
                                                                  formalism::planning::ActionRowView binding,
                                                                  StateRepository<LiftedTag>& storage,
                                                                  AxiomEvaluator<LiftedTag>& axiom_evaluator)
{ return get_successor_node(node, m_impl->stage(binding), storage, axiom_evaluator); }

BuilderNode SuccessorGenerator<LiftedTag>::get_successor_node(const Source& node,
                                                              formalism::planning::ActionRowView binding,
                                                              Builder& storage,
                                                              AxiomEvaluator<LiftedTag>& axiom_evaluator)
{ return get_successor_node(node, m_impl->stage(binding), storage, axiom_evaluator); }

Node<LiftedTag> SuccessorGenerator<LiftedTag>::get_successor_node(const Source& node,
                                                                  formalism::planning::ActionRowDataView binding,
                                                                  StateRepository<LiftedTag>& storage,
                                                                  AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    m_impl->validate_binding(binding);
    return get_successor_node(node, binding.data(), storage, axiom_evaluator);
}

BuilderNode SuccessorGenerator<LiftedTag>::get_successor_node(const Source& node,
                                                              formalism::planning::ActionRowDataView binding,
                                                              Builder& storage,
                                                              AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    m_impl->validate_binding(binding);
    return get_successor_node(node, binding.data(), storage, axiom_evaluator);
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
                                                                               formalism::planning::ActionRowView binding,
                                                                               StateRepository<LiftedTag>& state_repository,
                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator)
{ return get_successor_node(node, binding, state_repository, axiom_evaluator).pack(); }

PackedNode<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_node(const Source& node,
                                                                               formalism::planning::ActionView<GroundTag> action,
                                                                               StateRepository<LiftedTag>& state_repository,
                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator)
{ return get_successor_node(node, action, state_repository, axiom_evaluator).pack(); }

PackedNode<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_node(const Source& node,
                                                                               const BindingData& binding,
                                                                               StateRepository<LiftedTag>& state_repository,
                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator)
{ return get_successor_node(node, binding, state_repository, axiom_evaluator).pack(); }

template<typename Objects>
ActionBindingStatus
SuccessorGenerator<LiftedTag>::Impl::check_action_binding(const Source& node, formalism::planning::ActionView<LiftedTag> action, Objects objects)
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
    binding.relation = action.index();
    binding.values.clear();
    evaluator.checked_objects.clear();
    for (const auto object : indices)
    {
        binding.values.push_back(object.value);
        evaluator.checked_objects.push_back(object);
    }
    auto grounder = formalism::WriteContext { workspace.planning_builder, *task->get_repository(), evaluator.checked_objects };
    const auto state = StateContext<LiftedTag>(*task, node.data(), node.get_metric());
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

template<ygg::RowViewConcept<formalism::planning::Action<LiftedTag>> Binding>
ActionBindingStatus SuccessorGenerator<LiftedTag>::check_action_binding(const Source& node, Binding binding)
{
    if constexpr (requires { binding.context().owner(binding.data()); })
        if (binding.context().owner(binding.data()) == nullptr)
            throw std::invalid_argument("SuccessorGenerator: action binding does not belong to its source repository.");
    // A borrowed binding's action may have no pool yet (no stored types): its values are read as objects directly.
    return m_impl->check_action_binding(
        node,
        binding.get_relation(),
        urd::resolve(urd::transform(binding.get_values(), [](ygg::Value value) { return urd::index<formalism::Object>(static_cast<std::uint32_t>(value)); }),
                     binding.context()));
}

formalism::planning::ActionRowView SuccessorGenerator<LiftedTag>::materialize_action_binding(BorrowedActionRowView<LiftedTag> binding)
{
    m_impl->validate_binding(binding);
    // Relation bindings preserve object order and are already canonical.
    return m_impl->definition->task->get_repository()->insert(binding.data()).first;
}

template<ygg::RowViewConcept<formalism::planning::Action<LiftedTag>> Binding>
ActionBindingResult SuccessorGenerator<LiftedTag>::try_get_applicable_action_binding(const Source& node, Binding binding)
{
    const auto status = check_action_binding(node, binding);
    if (status != ActionBindingStatus::APPLICABLE)
        return { status, std::nullopt };
    return { status, formalism::planning::insert(*m_impl->definition->task->get_repository(), m_impl->evaluator.checked_binding).first };
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

Bindings SuccessorGenerator<LiftedTag>::get_applicable_action_bindings(const Source& node, formalism::planning::ActionView<LiftedTag> action)
{
    auto result = Bindings {};
    get_applicable_action_bindings(node, action, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_applicable_action_bindings(const Source& node,
                                                                   formalism::planning::ActionView<LiftedTag> action,
                                                                   Bindings& out_bindings)
{
    validate_task(m_impl->definition->task, node);
    m_impl->get_schema_evaluator(action);
    out_bindings.clear();
    for_each_applicable_action_binding(node, action, append(out_bindings));
}

ygg::float_t SuccessorGenerator<LiftedTag>::generate_successor_state(const Source& node,
                                                                     formalism::planning::ActionRowView binding,
                                                                     Builder& out_state,
                                                                     StateDelta& out_delta)
{
    validate_task(m_impl->definition->task, node);
    return m_impl->generate(node, m_impl->stage(binding), out_state, out_delta);
}

bool SuccessorGenerator<LiftedTag>::for_each_applicable_action_binding(const Source& node,
                                                                       const std::function<bool(formalism::planning::ActionRowView)>& callback)
{
    validate_task(m_impl->definition->task, node);
    return m_impl->for_each_applicable_action_binding(
        node,
        std::nullopt,
        [&](auto& binding) { return callback(formalism::planning::insert(*m_impl->definition->task->get_repository(), binding).first); });
}

bool SuccessorGenerator<LiftedTag>::for_each_applicable_action_binding(const Source& node,
                                                                       formalism::planning::ActionView<LiftedTag> action,
                                                                       const std::function<bool(formalism::planning::ActionRowView)>& callback)
{
    validate_task(m_impl->definition->task, node);
    return m_impl->for_each_applicable_action_binding(
        node,
        action,
        [&](auto& binding) { return callback(formalism::planning::insert(*m_impl->definition->task->get_repository(), binding).first); });
}

bool SuccessorGenerator<LiftedTag>::for_each_borrowed_applicable_action_binding(const Source& node,
                                                                                const std::function<bool(BorrowedActionRowView<LiftedTag>)>& callback)
{
    validate_task(m_impl->definition->task, node);
    return m_impl->for_each_applicable_action_binding(node,
                                                      std::nullopt,
                                                      [&](auto& binding)
                                                      { return callback(urd::resolve(binding, *m_impl->definition->task->get_repository())); });
}

bool SuccessorGenerator<LiftedTag>::for_each_borrowed_applicable_action_binding(const Source& node,
                                                                                formalism::planning::ActionView<LiftedTag> action,
                                                                                const std::function<bool(BorrowedActionRowView<LiftedTag>)>& callback)
{
    validate_task(m_impl->definition->task, node);
    return m_impl->for_each_applicable_action_binding(node,
                                                      action,
                                                      [&](auto& binding)
                                                      { return callback(urd::resolve(binding, *m_impl->definition->task->get_repository())); });
}

bool SuccessorGenerator<LiftedTag>::for_each_borrowed_applicable_action_binding(const Source& node,
                                                                                formalism::planning::ActionView<LiftedTag> action,
                                                                                const std::function<bool(BorrowedActionRowView<LiftedTag>)>& accept,
                                                                                const std::function<bool(BorrowedActionRowView<LiftedTag>)>& callback,
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

formalism::planning::ActionView<GroundTag> SuccessorGenerator<LiftedTag>::ground_action(formalism::planning::ActionRowView binding)
{
    if (const auto it = m_impl->evaluator.action_binding_to_ground_action.find(binding); it != m_impl->evaluator.action_binding_to_ground_action.end())
        return it->second;

    m_impl->evaluator.workspace.binding.clear();
    for (const auto object : binding.get<urd::index<formalism::Object>>())
        m_impl->evaluator.workspace.binding.push_back(object);

    auto grounder_context = formalism::WriteContext { m_impl->evaluator.workspace.planning_builder,
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
        formalism::planning::ground(
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
                    *std::ranges::find_if(schema_rules, [&](auto schema_rule) { return schema_rule.get_heads().front().get_predicate() == it->second; });
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
                                                                        std::shared_ptr<urd::builder<State<LiftedTag>>> state,
                                                                        ygg::float_t auxiliary_value)
{
    validate_task(m_impl->definition->task, state_repository);
    validate_task(m_impl->definition->task, axiom_evaluator);
    const auto metric = evaluate_successor_metric(*m_impl->definition->task, *state, auxiliary_value);
    return Node<LiftedTag>(state_repository.register_state(axiom_evaluator, std::move(state)), metric);
}

Node<LiftedTag> SuccessorGenerator<LiftedTag>::finalize_successor_state(StateRepository<LiftedTag>& state_repository,
                                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                        std::shared_ptr<urd::builder<State<LiftedTag>>> state,
                                                                        ygg::float_t auxiliary_value,
                                                                        const StateView<LiftedTag>& parent,
                                                                        const StateDelta& delta)
{
    validate_task(m_impl->definition->task, state_repository);
    validate_task(m_impl->definition->task, axiom_evaluator);
    const auto metric = evaluate_successor_metric(*m_impl->definition->task, *state, auxiliary_value);
    return Node<LiftedTag>(state_repository.register_state(axiom_evaluator, std::move(state), parent, delta), metric);
}

// Lookup
Node<LiftedTag> SuccessorGenerator<LiftedTag>::get_node(StateRepository<LiftedTag>& state_repository, urd::index<State<LiftedTag>> state_index)
{
    validate_task(m_impl->definition->task, state_repository);
    auto state = state_repository.get_registered_state(state_index);
    const auto state_context = StateContext<LiftedTag>(*m_impl->definition->task, state.data(), 0);
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

template ActionBindingStatus SuccessorGenerator<LiftedTag>::check_action_binding(const Source& node, formalism::planning::ActionRowView binding);
template ActionBindingStatus SuccessorGenerator<LiftedTag>::check_action_binding(const Source& node, formalism::planning::ActionRowDataView binding);
template ActionBindingResult SuccessorGenerator<LiftedTag>::try_get_applicable_action_binding(const Source& node, formalism::planning::ActionRowView binding);
template ActionBindingResult SuccessorGenerator<LiftedTag>::try_get_applicable_action_binding(const Source& node,
                                                                                              formalism::planning::ActionRowDataView binding);

}
