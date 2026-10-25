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
#include "tyr/datalog/lifted/static_extensions.hpp"
#include "tyr/datalog/solver.hpp"
#include "tyr/formalism/planning/grounder.hpp"
#include "tyr/formalism/planning/interning.hpp"
#include "tyr/planning/action_executor.hpp"
#include "tyr/planning/applicability_lifted.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/match_tree/match_tree.hpp"
#include "tyr/planning/lifted/axiom_evaluator.hpp"
#include "tyr/planning/lifted/programs/action.hpp"
#include "tyr/planning/lifted/state_repository.hpp"
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
#include <utility>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/execution/onetbb.hpp>
#include <yggdrasil/formalism/membership.hpp>
#include <yggdrasil/serialization/cista_equal_to.hpp>
#include <yggdrasil/serialization/cista_hash.hpp>

namespace tyr::planning
{
namespace
{
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

template<StateViewConcept<LiftedTag> S>
void validate_task(const TaskPtr<LiftedTag>& task, const S& state)
{
    if (&state.get_task() != task.get())
        throw std::invalid_argument("SuccessorGenerator: state belongs to a different task.");
}
}

struct SuccessorGenerator<LiftedTag>::Impl
{
    using Program = ApplicableActionProgram<LiftedTag>;
    using ActionBindingMap = ygg::UnorderedMap<formalism::planning::ActionBindingView, formalism::planning::ActionView<GroundTag>>;
    using StateKey = std::pair<const void*, ygg::uint_t>;

    struct SchemaEvaluator
    {
        formalism::datalog::ProgramView<LiftedTag> program;
        const P2DTranslationContext<LiftedTag>* input_translation;
        std::vector<datalog::Scheduler<LiftedTag>> schedulers;
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
        ygg::UniqueObjectPoolPtr<ygg::Data<formalism::RelationBinding<formalism::planning::Action<LiftedTag>>>> scratch_action_binding;
        ActionBindingMap action_binding_to_ground_action;
        ygg::Data<formalism::RelationBinding<formalism::planning::Action<LiftedTag>>> checked_binding;
        datalog::ProgramWorkspace<LiftedTag> workspace;
        ygg::UnorderedMap<formalism::planning::ActionView<LiftedTag>, SchemaEvaluator> schema_evaluators;
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
            std::optional<ygg::Index<formalism::planning::Action<LiftedTag>>> action;
            bool enumerating = false;
        } facts;

        /// The effect atoms of each action in the facts, grouped at the action's first successor.
    };

    /// Computes the facts of the node's state with the action's effects unless the workspace holds them.
    template<StateViewConcept<LiftedTag> S>
    void require_action_facts(const Node<LiftedTag, S>& node, formalism::planning::ActionView<LiftedTag> action);

    /// The repository and index of a registered state, or the address of a state builder.
    template<StateViewConcept<LiftedTag> S>
    static StateKey state_key(const Node<LiftedTag, S>& node)
    {
        if constexpr (requires {
                          node.get_state().get_index();
                          node.get_state().get_state_repository();
                      })
            return { static_cast<const void*>(node.get_state().get_state_repository().get()), static_cast<ygg::uint_t>(node.get_state().get_index()) };
        else
            return { static_cast<const void*>(&node.get_state().get_state_builder()), std::numeric_limits<ygg::uint_t>::max() };
    }

    template<StateViewConcept<LiftedTag> S>
    static constexpr bool is_registered = requires(const Node<LiftedTag, S>& node) { node.get_state().get_index(); };

    /// Forgets the facts of a state builder after single-binding work, since the builder can change.
    template<StateViewConcept<LiftedTag> S>
    void end_single_binding_work()
    {
        if constexpr (!is_registered<S>)
            if (!evaluator.facts.enumerating)
                evaluator.facts.state = std::nullopt;
    }

    Impl(ygg::uint_t index, TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context, std::shared_ptr<std::atomic<ygg::uint_t>> next_index) :
        index(index),
        next_index(std::move(next_index)),
        definition(std::make_shared<Definition>(std::move(task))),
        evaluator(*definition, std::move(execution_context))
    {
    }

    Impl(ygg::uint_t index,
         std::shared_ptr<const Definition> definition,
         ygg::ExecutionContextPtr execution_context,
         std::shared_ptr<std::atomic<ygg::uint_t>> next_index) :
        index(index),
        next_index(std::move(next_index)),
        definition(std::move(definition)),
        evaluator(*this->definition, std::move(execution_context))
    {
    }

    SchemaEvaluator& get_schema_evaluator(formalism::planning::ActionView<LiftedTag> action)
    {
        const auto it = evaluator.schema_evaluators.find(action);
        // Repository indices can collide across independent factories.
        if (it == evaluator.schema_evaluators.end() || &action.get_context() != &it->first.get_context())
            throw std::invalid_argument("SuccessorGenerator: action schema does not belong to the task domain.");
        return it->second;
    }

    template<typename Objects>
    bool validate_binding_objects(formalism::planning::ActionView<LiftedTag> action, Objects objects)
    {
        get_schema_evaluator(action);
        if (objects.size() != action.get_arity())
            throw std::invalid_argument("SuccessorGenerator: object count does not match action arity.");
        const auto indices = objects.get_data();
        const auto& target_repository = *definition->task->get_repository();
        if (!ygg::formalism::contains_all(objects.get_context(), indices) || !ygg::formalism::contains_all(target_repository, indices))
            return false;
        if (!ygg::formalism::contains_all(target_repository, objects))
            throw std::invalid_argument("SuccessorGenerator: object does not belong to the task repository.");
        return true;
    }

    void validate_binding(formalism::planning::ActionBindingDataView binding)
    {
        if (!binding.get_context().contains(binding.get_data().relation))
            throw std::invalid_argument("SuccessorGenerator: action schema does not belong to its source repository.");
        if (!validate_binding_objects(binding.get_relation(), binding.get_objects()))
            throw std::invalid_argument("SuccessorGenerator: object does not belong to the task repository.");
    }

    template<StateViewConcept<LiftedTag> S, typename Objects>
    ActionBindingStatus check_action_binding(const Node<LiftedTag, S>& node, formalism::planning::ActionView<LiftedTag> action, Objects objects);

    /// Computes the facts of the node's state with the action's schema program, or with the full program for all actions.
    template<StateViewConcept<LiftedTag> S>
    void compute_action_facts(const Node<LiftedTag, S>& node, std::optional<formalism::planning::ActionView<LiftedTag>> action);

    /// Calls `callback` with each applicable binding (in scratch_action_binding) of the action, or of all actions.
    template<StateViewConcept<LiftedTag> S, typename Callback>
    bool
    for_each_applicable_action_binding(const Node<LiftedTag, S>& node, std::optional<formalism::planning::ActionView<LiftedTag>> action, Callback&& callback);

    template<StateViewConcept<LiftedTag> S, typename Accept, typename Callback, typename Stop>
    bool for_each_applicable_action_binding(const Node<LiftedTag, S>& node,
                                            std::optional<formalism::planning::ActionView<LiftedTag>> action,
                                            Accept accept,
                                            Callback callback,
                                            Stop stop);

    template<StateViewConcept<LiftedTag> S>
    ygg::float_t generate_successor_state(const Node<LiftedTag, S>& node,
                                          const ygg::Data<formalism::RelationBinding<formalism::planning::Action<LiftedTag>>>& binding,
                                          ygg::Builder<State<LiftedTag>>& out_state,
                                          StateDelta& out_delta);

    ygg::uint_t index;
    std::shared_ptr<std::atomic<ygg::uint_t>> next_index;
    std::shared_ptr<const Definition> definition;
    Evaluator evaluator;
};

SuccessorGenerator<LiftedTag>::Impl::Definition::Definition(TaskPtr<LiftedTag> task_) : task(std::move(task_)), action_program(task->get_task()) {}

SuccessorGenerator<LiftedTag>::Impl::Evaluator::Evaluator(const Definition& definition, ygg::ExecutionContextPtr execution_context_) :
    execution_context(std::move(execution_context_)),
    scratch_builder(),
    scratch_action_binding(formalism::planning::checkout<formalism::RelationBinding<formalism::planning::Action<LiftedTag>>>(scratch_builder)),
    action_binding_to_ground_action(),
    workspace(definition.action_program.get_datalog_program()),
    static_extension_workspace(),
    effect_atom_index(definition.action_program.get_predicate_to_action_mapping(),
                      definition.action_program.get_conditional_effect_to_predicate_mapping(),
                      workspace.facts.fact_sets.predicate,
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
                                                                               schema.program.get_context(),
                                                                               schema.program.get_predicates<formalism::FluentTag>().size(),
                                                                               schema.program.get_functions<formalism::FluentTag>().size()) });
}

template<StateViewConcept<LiftedTag> S>
void SuccessorGenerator<LiftedTag>::Impl::compute_action_facts(const Node<LiftedTag, S>& node, std::optional<formalism::planning::ActionView<LiftedTag>> action)
{
    auto* schema = action ? &get_schema_evaluator(*action) : nullptr;
    evaluator.workspace.reset_evaluation();
    evaluator.facts.state = state_key(node);
    evaluator.facts.action = action ? std::optional(action->get_index()) : std::nullopt;
    evaluator.effect_atom_index.clear();

    const auto& state = node.get_state();
    insert_extended_state(state.get_state_builder(),
                          *definition->task->get_repository(),
                          schema ? *schema->input_translation : definition->action_program.get_translation_context().p2d,
                          evaluator.workspace);

    auto ctx =
        datalog::ProgramExecutionContext<LiftedTag>::from_reset_workspace(evaluator.workspace, schema ? schema->schedulers : evaluator.workspace.schedulers);
    datalog::execute_model(ctx, *evaluator.execution_context);
}

template<StateViewConcept<LiftedTag> S, typename Callback>
bool SuccessorGenerator<LiftedTag>::Impl::for_each_applicable_action_binding(const Node<LiftedTag, S>& node,
                                                                             std::optional<formalism::planning::ActionView<LiftedTag>> action,
                                                                             Callback&& callback)
{
    return for_each_applicable_action_binding(node, action, [](const auto&) { return true; }, std::forward<Callback>(callback), [] { return false; });
}

template<StateViewConcept<LiftedTag> S, typename Accept, typename Callback, typename Stop>
bool SuccessorGenerator<LiftedTag>::Impl::for_each_applicable_action_binding(const Node<LiftedTag, S>& node,
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
    } end_enumeration { evaluator.facts, is_registered<S> };
    auto& scratch_binding = *evaluator.scratch_action_binding;

    const auto state_context = StateContext<LiftedTag>(*definition->task, node.get_state().get_state_builder(), node.get_metric());
    auto grounder_context =
        formalism::planning::GrounderContext { evaluator.workspace.planning_builder, *definition->task->get_repository(), scratch_binding.objects };
    const auto& mapping = definition->action_program.get_predicate_to_action_mapping();

    for (const auto rule : program.get_rules<formalism::PredicateTag>())
    {
        const auto predicate = rule.get_heads().front().get_predicate();
        const auto it = mapping.find(predicate);
        if (it == mapping.end())
            continue;  // A conditional effect's rule.
        const auto action = it->second;
        const auto& set = evaluator.workspace.facts.fact_sets.predicate.get_sets()[static_cast<ygg::uint_t>(predicate.get_index())];
        for (const auto& binding : set.get_bindings())
        {
            if (stop())
                return false;
            scratch_binding.relation = action.get_index();
            scratch_binding.objects.clear();
            ygg::extend(binding.get_objects(), scratch_binding.objects);
            if (!accept(scratch_binding))
                continue;

            assert(is_applicable(action.get_condition(), ApplicabilityContext { state_context, grounder_context, *definition->task->get_fdr_context() })
                   && "ApplicableActionProgram emitted an action binding whose condition is not satisfied.");

            // Datalog certifies the action condition, not whether its grounded numeric effects are valid and mutually compatible.
            if (!evaluator.executor.is_applicable_if_fires(
                    action,
                    state_context,
                    grounder_context,
                    *definition->task->get_fdr_context(),
                    [&](auto effect, auto apply) { evaluator.effect_atom_index.for_each_effect_binding(action, effect, grounder_context.binding(), apply); }))
                continue;
            if (!callback(scratch_binding))
                return false;
        }
    }
    return true;
}

template<StateViewConcept<LiftedTag> S>
ygg::float_t
SuccessorGenerator<LiftedTag>::Impl::generate_successor_state(const Node<LiftedTag, S>& node,
                                                              const ygg::Data<formalism::RelationBinding<formalism::planning::Action<LiftedTag>>>& binding,
                                                              ygg::Builder<State<LiftedTag>>& out_state,
                                                              StateDelta& out_delta)
{
    evaluator.workspace.binding.clear();
    for (const auto object : binding.objects)
        evaluator.workspace.binding.push_back(object);

    auto grounder_context =
        formalism::planning::GrounderContext { evaluator.workspace.planning_builder, *definition->task->get_repository(), evaluator.workspace.binding };
    const auto state_context = StateContext<LiftedTag>(*definition->task, node.get_state().get_state_builder(), node.get_metric());
    const auto action = ygg::make_view(binding.relation, *definition->task->get_repository());

    require_action_facts(node, action);
    const auto auxiliary_value = evaluator.executor.apply_action_unregistered(
        state_context,
        action,
        grounder_context,
        *definition->task->get_fdr_context(),
        [&](auto effect, auto apply) { evaluator.effect_atom_index.for_each_effect_binding(action, effect, grounder_context.binding(), apply); },
        out_state,
        out_delta);
    end_single_binding_work<S>();
    return auxiliary_value;
}

template<StateViewConcept<LiftedTag> S>
void SuccessorGenerator<LiftedTag>::Impl::require_action_facts(const Node<LiftedTag, S>& node, formalism::planning::ActionView<LiftedTag> action)
{
    // The facts of another state, or without the action's effects, come from the action's schema program.
    if (evaluator.facts.state == state_key(node) && (!evaluator.facts.action || *evaluator.facts.action == action.get_index()))
        return;
    if (evaluator.facts.enumerating)
        throw std::logic_error("SuccessorGenerator: the state differs from the one whose applicable action bindings are enumerated.");
    compute_action_facts(node, action);
}

SuccessorGenerator<LiftedTag>::SuccessorGenerator(ygg::uint_t index,
                                                  TaskPtr<LiftedTag> task,
                                                  ygg::ExecutionContextPtr execution_context,
                                                  std::shared_ptr<std::atomic<ygg::uint_t>> next_index) :
    m_impl(std::make_unique<Impl>(index, std::move(task), std::move(execution_context), std::move(next_index)))
{
}

SuccessorGenerator<LiftedTag>::SuccessorGenerator(std::unique_ptr<Impl> impl) noexcept : m_impl(std::move(impl)) {}

SuccessorGenerator<LiftedTag>::~SuccessorGenerator() = default;
SuccessorGenerator<LiftedTag>::SuccessorGenerator(SuccessorGenerator&&) noexcept = default;
SuccessorGenerator<LiftedTag>& SuccessorGenerator<LiftedTag>::operator=(SuccessorGenerator&&) noexcept = default;

SuccessorGeneratorPtr<LiftedTag> SuccessorGenerator<LiftedTag>::make_worker(ygg::ExecutionContextPtr execution_context) const
{
    return SuccessorGeneratorPtr<LiftedTag>(
        new SuccessorGenerator<LiftedTag>(std::make_unique<Impl>(m_impl->next_index->fetch_add(1, std::memory_order_relaxed),
                                                                 m_impl->definition,
                                                                 std::move(execution_context),
                                                                 m_impl->next_index)));
}

Node<LiftedTag> SuccessorGenerator<LiftedTag>::get_initial_node(StateRepository<LiftedTag>& state_repository, AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    validate_task(m_impl->definition->task, state_repository);
    validate_task(m_impl->definition->task, axiom_evaluator);
    auto initial_state = state_repository.get_initial_state(axiom_evaluator);
    const auto state_context = StateContext<LiftedTag>(*m_impl->definition->task, initial_state.get_state_builder(), 0);
    const auto state_metric =
        evaluate_metric(m_impl->definition->task->get_task().get_metric(), m_impl->definition->task->get_task().get_auxiliary_fterm_value(), state_context);
    return Node<LiftedTag>(std::move(initial_state), state_metric);
}

template<StateViewConcept<LiftedTag> S>
NodeList<LiftedTag, S> SuccessorGenerator<LiftedTag>::get_successor_nodes(const Node<LiftedTag, S>& node,
                                                                          SuccessorListStorage<LiftedTag, S>& storage,
                                                                          AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = NodeList<LiftedTag, S> {};
    get_successor_nodes(node, storage, axiom_evaluator, result);
    return result;
}

template<StateViewConcept<LiftedTag> S>
void SuccessorGenerator<LiftedTag>::get_successor_nodes(const Node<LiftedTag, S>& node,
                                                        SuccessorListStorage<LiftedTag, S>& storage,
                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                        NodeList<LiftedTag, S>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, axiom_evaluator);
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
        validate_task(m_impl->definition->task, storage);
    out_nodes.clear();
    m_impl->for_each_applicable_action_binding(node,
                                               std::nullopt,
                                               [&](auto& binding)
                                               {
                                                   auto& target = [&]() -> auto&
                                                   {
                                                       if constexpr (std::same_as<S, StateView<LiftedTag>>)
                                                           return storage;
                                                       else
                                                           return storage.emplace_back();
                                                   }();
                                                   out_nodes.push_back(get_successor_node(node, binding, target, axiom_evaluator));
                                                   return true;
                                               });
}

template<StateViewConcept<LiftedTag> S>
NodeList<LiftedTag, S> SuccessorGenerator<LiftedTag>::get_successor_nodes(const Node<LiftedTag, S>& node,
                                                                          formalism::planning::ActionView<LiftedTag> action,
                                                                          SuccessorListStorage<LiftedTag, S>& storage,
                                                                          AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = NodeList<LiftedTag, S> {};
    get_successor_nodes(node, action, storage, axiom_evaluator, result);
    return result;
}

template<StateViewConcept<LiftedTag> S>
void SuccessorGenerator<LiftedTag>::get_successor_nodes(const Node<LiftedTag, S>& node,
                                                        formalism::planning::ActionView<LiftedTag> action,
                                                        SuccessorListStorage<LiftedTag, S>& storage,
                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                        NodeList<LiftedTag, S>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, axiom_evaluator);
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
        validate_task(m_impl->definition->task, storage);
    out_nodes.clear();
    m_impl->for_each_applicable_action_binding(node,
                                               action,
                                               [&](auto& binding)
                                               {
                                                   auto& target = [&]() -> auto&
                                                   {
                                                       if constexpr (std::same_as<S, StateView<LiftedTag>>)
                                                           return storage;
                                                       else
                                                           return storage.emplace_back();
                                                   }();
                                                   out_nodes.push_back(get_successor_node(node, binding, target, axiom_evaluator));
                                                   return true;
                                               });
}

template<StateViewConcept<LiftedTag> S>
LabeledNodeList<LiftedTag, S> SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes(const Node<LiftedTag, S>& node,
                                                                                         SuccessorListStorage<LiftedTag, S>& storage,
                                                                                         AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = LabeledNodeList<LiftedTag, S> {};
    get_labeled_successor_nodes(node, storage, axiom_evaluator, result);
    return result;
}

template<StateViewConcept<LiftedTag> S>
void SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes(const Node<LiftedTag, S>& node,
                                                                SuccessorListStorage<LiftedTag, S>& storage,
                                                                AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                LabeledNodeList<LiftedTag, S>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, axiom_evaluator);
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
        validate_task(m_impl->definition->task, storage);
    out_nodes.clear();
    m_impl->for_each_applicable_action_binding(node,
                                               std::nullopt,
                                               [&](auto& binding)
                                               {
                                                   const auto action_binding =
                                                       formalism::planning::insert(*m_impl->definition->task->get_repository(), binding).first;
                                                   auto& target = [&]() -> auto&
                                                   {
                                                       if constexpr (std::same_as<S, StateView<LiftedTag>>)
                                                           return storage;
                                                       else
                                                           return storage.emplace_back();
                                                   }();
                                                   out_nodes.push_back({ action_binding, get_successor_node(node, binding, target, axiom_evaluator) });
                                                   return true;
                                               });
}

template<StateViewConcept<LiftedTag> S>
LabeledNodeList<LiftedTag, S> SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes(const Node<LiftedTag, S>& node,
                                                                                         formalism::planning::ActionView<LiftedTag> action,
                                                                                         SuccessorListStorage<LiftedTag, S>& storage,
                                                                                         AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = LabeledNodeList<LiftedTag, S> {};
    get_labeled_successor_nodes(node, action, storage, axiom_evaluator, result);
    return result;
}

template<StateViewConcept<LiftedTag> S>
void SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes(const Node<LiftedTag, S>& node,
                                                                formalism::planning::ActionView<LiftedTag> action,
                                                                SuccessorListStorage<LiftedTag, S>& storage,
                                                                AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                LabeledNodeList<LiftedTag, S>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, axiom_evaluator);
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
        validate_task(m_impl->definition->task, storage);
    out_nodes.clear();
    m_impl->for_each_applicable_action_binding(node,
                                               action,
                                               [&](auto& binding)
                                               {
                                                   const auto action_binding =
                                                       formalism::planning::insert(*m_impl->definition->task->get_repository(), binding).first;
                                                   auto& target = [&]() -> auto&
                                                   {
                                                       if constexpr (std::same_as<S, StateView<LiftedTag>>)
                                                           return storage;
                                                       else
                                                           return storage.emplace_back();
                                                   }();
                                                   out_nodes.push_back({ action_binding, get_successor_node(node, binding, target, axiom_evaluator) });
                                                   return true;
                                               });
}

template<StateViewConcept<LiftedTag> S, typename Objects>
ActionBindingStatus
SuccessorGenerator<LiftedTag>::Impl::check_action_binding(const Node<LiftedTag, S>& node, formalism::planning::ActionView<LiftedTag> action, Objects objects)
{
    const auto& task = definition->task;
    validate_task(task, node.get_state());
    if (!validate_binding_objects(action, objects))
        return ActionBindingStatus::OUTSIDE_PARAMETER_DOMAIN;
    const auto indices = objects.get_data();

    const auto& domains = task->get_formalism_task().get_variable_domains().action_domains.at(action.get_index()).payload.precondition_domain.payload;
    for (size_t i = 0; i < objects.size(); ++i)
        if (i >= domains.size() || !std::binary_search(domains[i].objects.begin(), domains[i].objects.end(), indices[i]))
            return ActionBindingStatus::OUTSIDE_PARAMETER_DOMAIN;

    // Single-binding work has separate scratch from the tuple held by an outer enumeration callback.
    auto& workspace = evaluator.workspace;
    auto& binding = evaluator.checked_binding;
    binding.relation = action.get_index();
    binding.objects.clear();
    for (const auto index : indices)
        binding.objects.push_back(index);
    auto grounder = formalism::planning::GrounderContext { workspace.planning_builder, *task->get_repository(), binding.objects };
    const auto state = StateContext<LiftedTag>(*task, node.get_state().get_state_builder(), node.get_metric());
    if (!is_applicable(action.get_condition(), ApplicabilityContext { state, grounder, *task->get_fdr_context() }))
        return ActionBindingStatus::INAPPLICABLE;
    require_action_facts(node, action);
    const auto applicable = evaluator.executor.is_applicable_if_fires(
        action,
        state,
        grounder,
        *task->get_fdr_context(),
        [&](auto effect, auto apply) { evaluator.effect_atom_index.for_each_effect_binding(action, effect, grounder.binding(), apply); });
    end_single_binding_work<S>();
    return applicable ? ActionBindingStatus::APPLICABLE : ActionBindingStatus::INAPPLICABLE;
}

template<StateViewConcept<LiftedTag> S>
ActionBindingStatus SuccessorGenerator<LiftedTag>::check_action_binding(const Node<LiftedTag, S>& node,
                                                                        formalism::planning::ActionView<LiftedTag> action,
                                                                        formalism::planning::ObjectSpanView objects)
{ return m_impl->check_action_binding(node, action, objects); }

template<StateViewConcept<LiftedTag> S, ygg::formalism::RelationBindingViewConcept<formalism::planning::Action<LiftedTag>, formalism::ObjectTag> Binding>
ActionBindingStatus SuccessorGenerator<LiftedTag>::check_action_binding(const Node<LiftedTag, S>& node, Binding binding)
{
    if constexpr (requires { binding.get_index(); })
        if (!binding.get_context().contains(binding.get_index()))
            throw std::invalid_argument("SuccessorGenerator: action binding does not belong to its source repository.");
    return m_impl->check_action_binding(node, binding.get_relation(), binding.get_objects());
}

formalism::planning::ActionBindingView SuccessorGenerator<LiftedTag>::materialize_action_binding(BorrowedActionBindingView<LiftedTag> binding)
{
    m_impl->validate_binding(binding);
    // Relation bindings preserve object order and are already canonical.
    return m_impl->definition->task->get_repository()->insert(binding.get_data()).first;
}

template<StateViewConcept<LiftedTag> S>
ActionBindingResult SuccessorGenerator<LiftedTag>::try_get_applicable_action_binding(const Node<LiftedTag, S>& node,
                                                                                     formalism::planning::ActionView<LiftedTag> action,
                                                                                     formalism::planning::ObjectSpanView objects)
{
    const auto status = check_action_binding(node, action, objects);
    if (status != ActionBindingStatus::APPLICABLE)
        return { status, std::nullopt };
    return { status, formalism::planning::insert(*m_impl->definition->task->get_repository(), m_impl->evaluator.checked_binding).first };
}

template<StateViewConcept<LiftedTag> S>
Node<LiftedTag, S> SuccessorGenerator<LiftedTag>::get_successor_node(const Node<LiftedTag, S>& node,
                                                                     formalism::planning::ActionView<GroundTag> action,
                                                                     SuccessorStorage<LiftedTag, S>& storage,
                                                                     AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, axiom_evaluator);
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
        validate_task(m_impl->definition->task, storage);
    if constexpr (std::same_as<S, BuilderStateView<LiftedTag>>)
        if (&node.get_state().get_state_builder() == &storage)
            throw std::invalid_argument("SuccessorGenerator: source and output builder must not alias.");
    const auto state_context = StateContext<LiftedTag>(*m_impl->definition->task, node.get_state().get_state_builder(), node.get_metric());
    auto& delta = m_impl->evaluator.successor_delta;
    const auto generate = [&](auto& target) { return m_impl->evaluator.executor.apply_action_unregistered(state_context, action, target, delta); };
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
    {
        auto successor = storage.get_state_builder();
        const auto auxiliary_value = generate(*successor);
        return finalize_successor_state(storage, axiom_evaluator, std::move(successor), auxiliary_value, node.get_state(), delta);
    }
    else
    {
        storage.clear();
        const auto auxiliary_value = generate(storage);
        const auto metric = evaluate_successor_metric(*m_impl->definition->task, storage, auxiliary_value);
        axiom_evaluator.compute_extended_state(storage);
        return Node<LiftedTag, S>(S(storage, *m_impl->definition->task), metric);
    }
}

formalism::planning::ActionView<GroundTag> SuccessorGenerator<LiftedTag>::ground_action(formalism::planning::ActionBindingView binding)
{
    if (const auto it = m_impl->evaluator.action_binding_to_ground_action.find(binding); it != m_impl->evaluator.action_binding_to_ground_action.end())
        return it->second;

    m_impl->evaluator.workspace.binding.clear();
    ygg::extend(binding.get_objects(), m_impl->evaluator.workspace.binding);

    auto grounder_context = formalism::planning::GrounderContext { m_impl->evaluator.workspace.planning_builder,
                                                                   *m_impl->definition->task->get_repository(),
                                                                   m_impl->evaluator.workspace.binding };
    // Without a state, an effect has every completed binding that the static consistency graph of its rule allows.
    const auto action = binding.get_relation();
    const auto objects = ygg::IndexList<formalism::Object>(m_impl->evaluator.workspace.binding.begin(), m_impl->evaluator.workspace.binding.end());
    const auto& mapping = m_impl->definition->action_program.get_conditional_effect_to_predicate_mapping();
    const auto& rules = m_impl->definition->action_program.get_datalog_program().get_const_program_workspace().get_rules<formalism::PredicateTag>();
    const auto schema_rules = m_impl->get_schema_evaluator(action).program.get_rules<formalism::PredicateTag>();
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
                datalog::for_each_static_extension(*rules[static_cast<ygg::uint_t>(rule.get_index())],
                                                   std::span<const ygg::Index<formalism::Object>>(objects.data(), objects.size()),
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

// Interned action-binding input; state storage follows S.
template<StateViewConcept<LiftedTag> S>
Node<LiftedTag, S> SuccessorGenerator<LiftedTag>::get_successor_node(const Node<LiftedTag, S>& node,
                                                                     formalism::planning::ActionBindingView binding,
                                                                     SuccessorStorage<LiftedTag, S>& storage,
                                                                     AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto& data = m_impl->evaluator.checked_binding;
    data.relation = binding.get_relation().get_index();
    data.objects.clear();
    ygg::extend(binding.get_objects(), data.objects);

    return get_successor_node(node, data, storage, axiom_evaluator);
}

template<StateViewConcept<LiftedTag> S>
std::vector<formalism::planning::ActionBindingView> SuccessorGenerator<LiftedTag>::get_applicable_action_bindings(const Node<LiftedTag, S>& node)
{
    auto result = std::vector<formalism::planning::ActionBindingView> {};
    get_applicable_action_bindings(node, result);
    return result;
}

template<StateViewConcept<LiftedTag> S>
void SuccessorGenerator<LiftedTag>::get_applicable_action_bindings(const Node<LiftedTag, S>& node,
                                                                   std::vector<formalism::planning::ActionBindingView>& out_bindings)
{
    validate_task(m_impl->definition->task, node.get_state());
    out_bindings.clear();
    for_each_applicable_action_binding(node,
                                       [&](auto value)
                                       {
                                           out_bindings.push_back(std::move(value));
                                           return true;
                                       });
}

template<StateViewConcept<LiftedTag> S>
std::vector<formalism::planning::ActionBindingView>
SuccessorGenerator<LiftedTag>::get_applicable_action_bindings(const Node<LiftedTag, S>& node, formalism::planning::ActionView<LiftedTag> action)
{
    auto result = std::vector<formalism::planning::ActionBindingView> {};
    get_applicable_action_bindings(node, action, result);
    return result;
}

template<StateViewConcept<LiftedTag> S>
void SuccessorGenerator<LiftedTag>::get_applicable_action_bindings(const Node<LiftedTag, S>& node,
                                                                   formalism::planning::ActionView<LiftedTag> action,
                                                                   std::vector<formalism::planning::ActionBindingView>& out_bindings)
{
    validate_task(m_impl->definition->task, node.get_state());
    m_impl->get_schema_evaluator(action);
    out_bindings.clear();
    for_each_applicable_action_binding(node,
                                       action,
                                       [&](auto value)
                                       {
                                           out_bindings.push_back(std::move(value));
                                           return true;
                                       });
}

template<StateViewConcept<LiftedTag> S>
ygg::float_t SuccessorGenerator<LiftedTag>::generate_successor_state(const Node<LiftedTag, S>& node,
                                                                     formalism::planning::ActionBindingView binding,
                                                                     ygg::Builder<State<LiftedTag>>& out_state,
                                                                     StateDelta& out_delta)
{
    validate_task(m_impl->definition->task, node.get_state());
    auto& data = m_impl->evaluator.checked_binding;
    data.relation = binding.get_relation().get_index();
    data.objects.clear();
    ygg::extend(binding.get_objects(), data.objects);

    return m_impl->generate_successor_state(node, data, out_state, out_delta);
}

Node<LiftedTag> SuccessorGenerator<LiftedTag>::finalize_successor_state(StateRepository<LiftedTag>& state_repository,
                                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                        ygg::SharedObjectPoolPtr<ygg::Builder<State<LiftedTag>>, true> state,
                                                                        ygg::float_t auxiliary_value)
{
    validate_task(m_impl->definition->task, state_repository);
    validate_task(m_impl->definition->task, axiom_evaluator);
    const auto metric = evaluate_successor_metric(*m_impl->definition->task, *state, auxiliary_value);
    return Node<LiftedTag>(state_repository.register_state(axiom_evaluator, std::move(state)), metric);
}

Node<LiftedTag> SuccessorGenerator<LiftedTag>::finalize_successor_state(StateRepository<LiftedTag>& state_repository,
                                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                        ygg::SharedObjectPoolPtr<ygg::Builder<State<LiftedTag>>, true> state,
                                                                        ygg::float_t auxiliary_value,
                                                                        const StateView<LiftedTag>& parent,
                                                                        const StateDelta& delta)
{
    validate_task(m_impl->definition->task, state_repository);
    validate_task(m_impl->definition->task, axiom_evaluator);
    const auto metric = evaluate_successor_metric(*m_impl->definition->task, *state, auxiliary_value);
    return Node<LiftedTag>(state_repository.register_state(axiom_evaluator, std::move(state), parent, delta), metric);
}

template<StateViewConcept<LiftedTag> S>
Node<LiftedTag, S> SuccessorGenerator<LiftedTag>::get_successor_node(const Node<LiftedTag, S>& node,
                                                                     formalism::planning::ActionBindingDataView binding,
                                                                     SuccessorStorage<LiftedTag, S>& storage,
                                                                     AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    m_impl->validate_binding(binding);
    return get_successor_node(node, binding.get_data(), storage, axiom_evaluator);
}

// Raw action-binding input; bindings are not interned and state storage follows S.
template<StateViewConcept<LiftedTag> S>
Node<LiftedTag, S>
SuccessorGenerator<LiftedTag>::get_successor_node(const Node<LiftedTag, S>& node,
                                                  const ygg::Data<formalism::RelationBinding<formalism::planning::Action<LiftedTag>>>& binding,
                                                  SuccessorStorage<LiftedTag, S>& storage,
                                                  AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, axiom_evaluator);
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
        validate_task(m_impl->definition->task, storage);
    if constexpr (std::same_as<S, BuilderStateView<LiftedTag>>)
        if (&node.get_state().get_state_builder() == &storage)
            throw std::invalid_argument("SuccessorGenerator: source and output builder must not alias.");
    auto& delta = m_impl->evaluator.successor_delta;
    const auto generate = [&](auto& target) { return m_impl->generate_successor_state(node, binding, target, delta); };
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
    {
        auto successor = storage.get_state_builder();
        const auto auxiliary_value = generate(*successor);
        return finalize_successor_state(storage, axiom_evaluator, std::move(successor), auxiliary_value, node.get_state(), delta);
    }
    else
    {
        storage.clear();
        const auto auxiliary_value = generate(storage);
        const auto metric = evaluate_successor_metric(*m_impl->definition->task, storage, auxiliary_value);
        axiom_evaluator.compute_extended_state(storage);
        return Node<LiftedTag, S>(S(storage, *m_impl->definition->task), metric);
    }
}

// Lookup
Node<LiftedTag> SuccessorGenerator<LiftedTag>::get_node(StateRepository<LiftedTag>& state_repository, ygg::Index<State<LiftedTag>> state_index)
{
    validate_task(m_impl->definition->task, state_repository);
    auto state = state_repository.get_registered_state(state_index);
    const auto state_context = StateContext<LiftedTag>(*m_impl->definition->task, state.get_state_builder(), 0);
    const auto state_metric =
        evaluate_metric(m_impl->definition->task->get_task().get_metric(), m_impl->definition->task->get_task().get_auxiliary_fterm_value(), state_context);
    return Node<LiftedTag>(std::move(state), state_metric);
}

template<StateViewConcept<LiftedTag> S>
bool SuccessorGenerator<LiftedTag>::for_each_successor_node(const Node<LiftedTag, S>& node,
                                                            SuccessorStorage<LiftedTag, S>& storage,
                                                            AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                            const std::type_identity_t<std::function<bool(Node<LiftedTag, S>)>>& callback)
{
    validate_task(m_impl->definition->task, node.get_state());
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
        validate_task(m_impl->definition->task, storage);
    if constexpr (std::same_as<S, BuilderStateView<LiftedTag>>)
        if (&node.get_state().get_state_builder() == &storage)
            throw std::invalid_argument("SuccessorGenerator: source and output builder must not alias.");
    validate_task(m_impl->definition->task, axiom_evaluator);
    return m_impl->for_each_applicable_action_binding(node,
                                                      std::nullopt,
                                                      [&](auto& binding) { return callback(get_successor_node(node, binding, storage, axiom_evaluator)); });
}

template<StateViewConcept<LiftedTag> S>
PackedNodeList<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_nodes(const Node<LiftedTag, S>& node,
                                                                                    StateRepository<LiftedTag>& state_repository,
                                                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = PackedNodeList<LiftedTag> {};
    get_packed_successor_nodes(node, state_repository, axiom_evaluator, result);
    return result;
}

template<StateViewConcept<LiftedTag> S>
void SuccessorGenerator<LiftedTag>::get_packed_successor_nodes(const Node<LiftedTag, S>& node,
                                                               StateRepository<LiftedTag>& state_repository,
                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                               PackedNodeList<LiftedTag>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, axiom_evaluator);
    validate_task(m_impl->definition->task, state_repository);
    out_nodes.clear();
    m_impl->for_each_applicable_action_binding(node,
                                               std::nullopt,
                                               [&](auto& binding)
                                               {
                                                   out_nodes.push_back(get_packed_successor_node(node, binding, state_repository, axiom_evaluator));
                                                   return true;
                                               });
}

template<StateViewConcept<LiftedTag> S>
bool SuccessorGenerator<LiftedTag>::for_each_labeled_successor_node(const Node<LiftedTag, S>& node,
                                                                    SuccessorStorage<LiftedTag, S>& storage,
                                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                    const std::type_identity_t<std::function<bool(LabeledNode<LiftedTag, S>)>>& callback)
{
    validate_task(m_impl->definition->task, node.get_state());
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
        validate_task(m_impl->definition->task, storage);
    if constexpr (std::same_as<S, BuilderStateView<LiftedTag>>)
        if (&node.get_state().get_state_builder() == &storage)
            throw std::invalid_argument("SuccessorGenerator: source and output builder must not alias.");
    validate_task(m_impl->definition->task, axiom_evaluator);
    return m_impl->for_each_applicable_action_binding(node,
                                                      std::nullopt,
                                                      [&](auto& binding)
                                                      {
                                                          const auto action_binding =
                                                              formalism::planning::insert(*m_impl->definition->task->get_repository(), binding).first;
                                                          return callback({ action_binding, get_successor_node(node, binding, storage, axiom_evaluator) });
                                                      });
}

template<StateViewConcept<LiftedTag> S>
PackedLabeledNodeList<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes(const Node<LiftedTag, S>& node,
                                                                                                   StateRepository<LiftedTag>& state_repository,
                                                                                                   AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = PackedLabeledNodeList<LiftedTag> {};
    get_packed_labeled_successor_nodes(node, state_repository, axiom_evaluator, result);
    return result;
}

template<StateViewConcept<LiftedTag> S>
void SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes(const Node<LiftedTag, S>& node,
                                                                       StateRepository<LiftedTag>& state_repository,
                                                                       AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                       PackedLabeledNodeList<LiftedTag>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, axiom_evaluator);
    validate_task(m_impl->definition->task, state_repository);
    out_nodes.clear();
    m_impl->for_each_applicable_action_binding(
        node,
        std::nullopt,
        [&](auto& binding)
        {
            const auto action_binding = formalism::planning::insert(*m_impl->definition->task->get_repository(), binding).first;
            out_nodes.push_back({ action_binding, get_packed_successor_node(node, binding, state_repository, axiom_evaluator) });
            return true;
        });
}

template<StateViewConcept<LiftedTag> S>
bool SuccessorGenerator<LiftedTag>::for_each_applicable_action_binding(const Node<LiftedTag, S>& node,
                                                                       const std::function<bool(formalism::planning::ActionBindingView)>& callback)
{
    validate_task(m_impl->definition->task, node.get_state());
    return m_impl->for_each_applicable_action_binding(
        node,
        std::nullopt,
        [&](auto& binding) { return callback(formalism::planning::insert(*m_impl->definition->task->get_repository(), binding).first); });
}

template<StateViewConcept<LiftedTag> S>
bool SuccessorGenerator<LiftedTag>::for_each_borrowed_applicable_action_binding(const Node<LiftedTag, S>& node,
                                                                                const std::function<bool(BorrowedActionBindingView<LiftedTag>)>& callback)
{
    validate_task(m_impl->definition->task, node.get_state());
    return m_impl->for_each_applicable_action_binding(node,
                                                      std::nullopt,
                                                      [&](auto& binding)
                                                      { return callback(ygg::make_view(binding, *m_impl->definition->task->get_repository())); });
}

template<StateViewConcept<LiftedTag> S>
bool SuccessorGenerator<LiftedTag>::for_each_successor_node(const Node<LiftedTag, S>& node,
                                                            formalism::planning::ActionView<LiftedTag> action,
                                                            SuccessorStorage<LiftedTag, S>& storage,
                                                            AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                            const std::type_identity_t<std::function<bool(Node<LiftedTag, S>)>>& callback)
{
    validate_task(m_impl->definition->task, node.get_state());
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
        validate_task(m_impl->definition->task, storage);
    if constexpr (std::same_as<S, BuilderStateView<LiftedTag>>)
        if (&node.get_state().get_state_builder() == &storage)
            throw std::invalid_argument("SuccessorGenerator: source and output builder must not alias.");
    validate_task(m_impl->definition->task, axiom_evaluator);
    return m_impl->for_each_applicable_action_binding(node,
                                                      action,
                                                      [&](auto& binding) { return callback(get_successor_node(node, binding, storage, axiom_evaluator)); });
}

template<StateViewConcept<LiftedTag> S>
PackedNodeList<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_nodes(const Node<LiftedTag, S>& node,
                                                                                    formalism::planning::ActionView<LiftedTag> action,
                                                                                    StateRepository<LiftedTag>& state_repository,
                                                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = PackedNodeList<LiftedTag> {};
    get_packed_successor_nodes(node, action, state_repository, axiom_evaluator, result);
    return result;
}

template<StateViewConcept<LiftedTag> S>
void SuccessorGenerator<LiftedTag>::get_packed_successor_nodes(const Node<LiftedTag, S>& node,
                                                               formalism::planning::ActionView<LiftedTag> action,
                                                               StateRepository<LiftedTag>& state_repository,
                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                               PackedNodeList<LiftedTag>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, axiom_evaluator);
    validate_task(m_impl->definition->task, state_repository);
    out_nodes.clear();
    m_impl->for_each_applicable_action_binding(node,
                                               action,
                                               [&](auto& binding)
                                               {
                                                   out_nodes.push_back(get_packed_successor_node(node, binding, state_repository, axiom_evaluator));
                                                   return true;
                                               });
}

template<StateViewConcept<LiftedTag> S>
bool SuccessorGenerator<LiftedTag>::for_each_labeled_successor_node(const Node<LiftedTag, S>& node,
                                                                    formalism::planning::ActionView<LiftedTag> action,
                                                                    SuccessorStorage<LiftedTag, S>& storage,
                                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                    const std::type_identity_t<std::function<bool(LabeledNode<LiftedTag, S>)>>& callback)
{
    validate_task(m_impl->definition->task, node.get_state());
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
        validate_task(m_impl->definition->task, storage);
    if constexpr (std::same_as<S, BuilderStateView<LiftedTag>>)
        if (&node.get_state().get_state_builder() == &storage)
            throw std::invalid_argument("SuccessorGenerator: source and output builder must not alias.");
    validate_task(m_impl->definition->task, axiom_evaluator);
    return m_impl->for_each_applicable_action_binding(node,
                                                      action,
                                                      [&](auto& binding)
                                                      {
                                                          const auto action_binding =
                                                              formalism::planning::insert(*m_impl->definition->task->get_repository(), binding).first;
                                                          return callback({ action_binding, get_successor_node(node, binding, storage, axiom_evaluator) });
                                                      });
}

template<StateViewConcept<LiftedTag> S>
PackedLabeledNodeList<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes(const Node<LiftedTag, S>& node,
                                                                                                   formalism::planning::ActionView<LiftedTag> action,
                                                                                                   StateRepository<LiftedTag>& state_repository,
                                                                                                   AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = PackedLabeledNodeList<LiftedTag> {};
    get_packed_labeled_successor_nodes(node, action, state_repository, axiom_evaluator, result);
    return result;
}

template<StateViewConcept<LiftedTag> S>
void SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes(const Node<LiftedTag, S>& node,
                                                                       formalism::planning::ActionView<LiftedTag> action,
                                                                       StateRepository<LiftedTag>& state_repository,
                                                                       AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                       PackedLabeledNodeList<LiftedTag>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, axiom_evaluator);
    validate_task(m_impl->definition->task, state_repository);
    out_nodes.clear();
    m_impl->for_each_applicable_action_binding(
        node,
        action,
        [&](auto& binding)
        {
            const auto action_binding = formalism::planning::insert(*m_impl->definition->task->get_repository(), binding).first;
            out_nodes.push_back({ action_binding, get_packed_successor_node(node, binding, state_repository, axiom_evaluator) });
            return true;
        });
}

template<StateViewConcept<LiftedTag> S>
bool SuccessorGenerator<LiftedTag>::for_each_applicable_action_binding(const Node<LiftedTag, S>& node,
                                                                       formalism::planning::ActionView<LiftedTag> action,
                                                                       const std::function<bool(formalism::planning::ActionBindingView)>& callback)
{
    validate_task(m_impl->definition->task, node.get_state());
    return m_impl->for_each_applicable_action_binding(
        node,
        action,
        [&](auto& binding) { return callback(formalism::planning::insert(*m_impl->definition->task->get_repository(), binding).first); });
}

template<StateViewConcept<LiftedTag> S>
bool SuccessorGenerator<LiftedTag>::for_each_borrowed_applicable_action_binding(const Node<LiftedTag, S>& node,
                                                                                formalism::planning::ActionView<LiftedTag> action,
                                                                                const std::function<bool(BorrowedActionBindingView<LiftedTag>)>& callback)
{
    validate_task(m_impl->definition->task, node.get_state());
    return m_impl->for_each_applicable_action_binding(node,
                                                      action,
                                                      [&](auto& binding)
                                                      { return callback(ygg::make_view(binding, *m_impl->definition->task->get_repository())); });
}

template<StateViewConcept<LiftedTag> S>
bool SuccessorGenerator<LiftedTag>::for_each_borrowed_applicable_action_binding(const Node<LiftedTag, S>& node,
                                                                                formalism::planning::ActionView<LiftedTag> action,
                                                                                const std::function<bool(BorrowedActionBindingView<LiftedTag>)>& accept,
                                                                                const std::function<bool(BorrowedActionBindingView<LiftedTag>)>& callback,
                                                                                const std::function<bool()>& stop)
{
    validate_task(m_impl->definition->task, node.get_state());
    const auto& repository = *m_impl->definition->task->get_repository();
    return m_impl->for_each_applicable_action_binding(
        node,
        action,
        [&](auto& binding) { return accept(ygg::make_view(binding, repository)); },
        [&](auto& binding) { return callback(ygg::make_view(binding, repository)); },
        [&] { return stop(); });
}

PackedNode<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_initial_node(StateRepository<LiftedTag>& state_repository,
                                                                             AxiomEvaluator<LiftedTag>& axiom_evaluator)
{ return get_initial_node(state_repository, axiom_evaluator).pack(); }

PackedNode<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_node(StateRepository<LiftedTag>& state_repository, ygg::Index<State<LiftedTag>> state_index)
{ return get_node(state_repository, state_index).pack(); }

template<StateViewConcept<LiftedTag> S>
PackedNode<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_node(const Node<LiftedTag, S>& node,
                                                                               formalism::planning::ActionBindingView binding,
                                                                               StateRepository<LiftedTag>& state_repository,
                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto& data = m_impl->evaluator.checked_binding;
    data.relation = binding.get_relation().get_index();
    data.objects.clear();
    ygg::extend(binding.get_objects(), data.objects);
    return get_packed_successor_node(node, data, state_repository, axiom_evaluator);
}

template<StateViewConcept<LiftedTag> S>
PackedNode<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_node(const Node<LiftedTag, S>& node,
                                                                               formalism::planning::ActionView<GroundTag> binding,
                                                                               StateRepository<LiftedTag>& state_repository,
                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, axiom_evaluator);
    validate_task(m_impl->definition->task, state_repository);
    auto successor = state_repository.get_state_builder();
    const auto state_context = StateContext<LiftedTag>(*m_impl->definition->task, node.get_state().get_state_builder(), node.get_metric());
    auto& delta = m_impl->evaluator.successor_delta;
    const auto auxiliary_value = m_impl->evaluator.executor.apply_action_unregistered(state_context, binding, *successor, delta);
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
        return finalize_successor_state(state_repository, axiom_evaluator, std::move(successor), auxiliary_value, node.get_state(), delta).pack();
    else
        return finalize_successor_state(state_repository, axiom_evaluator, std::move(successor), auxiliary_value).pack();
}

template<StateViewConcept<LiftedTag> S>
PackedNode<LiftedTag>
SuccessorGenerator<LiftedTag>::get_packed_successor_node(const Node<LiftedTag, S>& node,
                                                         const ygg::Data<formalism::RelationBinding<formalism::planning::Action<LiftedTag>>>& binding,
                                                         StateRepository<LiftedTag>& state_repository,
                                                         AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, axiom_evaluator);
    validate_task(m_impl->definition->task, state_repository);
    auto successor = state_repository.get_state_builder();
    auto& delta = m_impl->evaluator.successor_delta;
    const auto auxiliary_value = m_impl->generate_successor_state(node, binding, *successor, delta);
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
        return finalize_successor_state(state_repository, axiom_evaluator, std::move(successor), auxiliary_value, node.get_state(), delta).pack();
    else
        return finalize_successor_state(state_repository, axiom_evaluator, std::move(successor), auxiliary_value).pack();
}

const ApplicableActionProgram<LiftedTag>& SuccessorGenerator<LiftedTag>::get_action_program() const noexcept { return m_impl->definition->action_program; }

const TaskPtr<LiftedTag>& SuccessorGenerator<LiftedTag>::get_task() const noexcept { return m_impl->definition->task; }

ygg::uint_t SuccessorGenerator<LiftedTag>::get_index() const noexcept { return m_impl->index; }

// Diagnostics
void SuccessorGenerator<LiftedTag>::print_summary(size_t verbosity) const
{
    if (verbosity < 1)
        return;

    std::cout << "[Successor generator] Summary" << '\n';
    fmt::print(std::cout, "{}\n", m_impl->evaluator.workspace.statistics);
    auto successor_generator_rule_statistics = std::vector<datalog::RuleStatistics> {};
    for (const auto& ws_rule : m_impl->evaluator.workspace.template get_rules<formalism::PredicateTag>())
        successor_generator_rule_statistics.push_back(ws_rule->common.statistics);
    fmt::print(std::cout, "{}\n", datalog::compute_aggregated_rule_statistics(successor_generator_rule_statistics));
    auto successor_generator_rule_worker_statistics = std::vector<datalog::RuleWorkerStatistics> {};
    for (const auto& ws_rule : m_impl->evaluator.workspace.template get_rules<formalism::PredicateTag>())
        for (const auto& worker : ws_rule->worker)
            successor_generator_rule_worker_statistics.push_back(worker.solve.statistics);
    fmt::print(std::cout, "{}\n", datalog::compute_aggregated_rule_worker_statistics(successor_generator_rule_worker_statistics));
}

template ActionBindingStatus SuccessorGenerator<LiftedTag>::check_action_binding(const Node<LiftedTag>& node, formalism::planning::ActionBindingView binding);

template ActionBindingStatus SuccessorGenerator<LiftedTag>::check_action_binding(const Node<LiftedTag>& node,
                                                                                 formalism::planning::ActionBindingDataView binding);

template ActionBindingStatus SuccessorGenerator<LiftedTag>::check_action_binding(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                 formalism::planning::ActionBindingDataView binding);

template bool
SuccessorGenerator<LiftedTag>::for_each_borrowed_applicable_action_binding(const Node<LiftedTag>& node,
                                                                           const std::function<bool(BorrowedActionBindingView<LiftedTag>)>& callback);

template bool
SuccessorGenerator<LiftedTag>::for_each_borrowed_applicable_action_binding(const Node<LiftedTag>& node,
                                                                           formalism::planning::ActionView<LiftedTag> action,
                                                                           const std::function<bool(BorrowedActionBindingView<LiftedTag>)>& callback);

template bool
SuccessorGenerator<LiftedTag>::for_each_borrowed_applicable_action_binding(const Node<LiftedTag>& node,
                                                                           formalism::planning::ActionView<LiftedTag> action,
                                                                           const std::function<bool(BorrowedActionBindingView<LiftedTag>)>& accept,
                                                                           const std::function<bool(BorrowedActionBindingView<LiftedTag>)>& callback,
                                                                           const std::function<bool()>& stop);

template Node<LiftedTag> SuccessorGenerator<LiftedTag>::get_successor_node(const Node<LiftedTag>& node,
                                                                           formalism::planning::ActionBindingDataView binding,
                                                                           StateRepository<LiftedTag>& storage,
                                                                           AxiomEvaluator<LiftedTag>& axiom_evaluator);

template ActionBindingStatus SuccessorGenerator<LiftedTag>::check_action_binding(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                 formalism::planning::ActionBindingView binding);

template bool
SuccessorGenerator<LiftedTag>::for_each_borrowed_applicable_action_binding(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                           const std::function<bool(BorrowedActionBindingView<LiftedTag>)>& callback);

template bool
SuccessorGenerator<LiftedTag>::for_each_borrowed_applicable_action_binding(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                           formalism::planning::ActionView<LiftedTag> action,
                                                                           const std::function<bool(BorrowedActionBindingView<LiftedTag>)>& callback);

template bool
SuccessorGenerator<LiftedTag>::for_each_borrowed_applicable_action_binding(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                           formalism::planning::ActionView<LiftedTag> action,
                                                                           const std::function<bool(BorrowedActionBindingView<LiftedTag>)>& accept,
                                                                           const std::function<bool(BorrowedActionBindingView<LiftedTag>)>& callback,
                                                                           const std::function<bool()>& stop);

template Node<LiftedTag, BuilderStateView<LiftedTag>>
SuccessorGenerator<LiftedTag>::get_successor_node(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                  formalism::planning::ActionBindingDataView binding,
                                                  ygg::Builder<State<LiftedTag>>& storage,
                                                  AxiomEvaluator<LiftedTag>& axiom_evaluator);

template ActionBindingResult SuccessorGenerator<LiftedTag>::try_get_applicable_action_binding(const Node<LiftedTag>& node,
                                                                                              formalism::planning::ActionView<LiftedTag> action,
                                                                                              formalism::planning::ObjectSpanView objects);
template ActionBindingResult SuccessorGenerator<LiftedTag>::try_get_applicable_action_binding(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                              formalism::planning::ActionView<LiftedTag> action,
                                                                                              formalism::planning::ObjectSpanView objects);

template ActionBindingStatus SuccessorGenerator<LiftedTag>::check_action_binding(const Node<LiftedTag>& node,
                                                                                 formalism::planning::ActionView<LiftedTag> action,
                                                                                 formalism::planning::ObjectSpanView objects);

template ActionBindingStatus SuccessorGenerator<LiftedTag>::check_action_binding(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                 formalism::planning::ActionView<LiftedTag> action,
                                                                                 formalism::planning::ObjectSpanView objects);

template NodeList<LiftedTag> SuccessorGenerator<LiftedTag>::get_successor_nodes<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                                      StateRepository<LiftedTag>& state_repository,
                                                                                                      AxiomEvaluator<LiftedTag>& axiom_evaluator);

template NodeList<LiftedTag, BuilderStateView<LiftedTag>>
SuccessorGenerator<LiftedTag>::get_successor_nodes<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                std::deque<ygg::Builder<State<LiftedTag>>>& storage,
                                                                                AxiomEvaluator<LiftedTag>& axiom_evaluator);

template void SuccessorGenerator<LiftedTag>::get_successor_nodes<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                       StateRepository<LiftedTag>& state_repository,
                                                                                       AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                       NodeList<LiftedTag>& out_nodes);

template void SuccessorGenerator<LiftedTag>::get_successor_nodes<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                              std::deque<ygg::Builder<State<LiftedTag>>>& storage,
                                                                                              AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                              NodeList<LiftedTag, BuilderStateView<LiftedTag>>& out_nodes);

template NodeList<LiftedTag> SuccessorGenerator<LiftedTag>::get_successor_nodes<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                                      formalism::planning::ActionView<LiftedTag> action,
                                                                                                      StateRepository<LiftedTag>& state_repository,
                                                                                                      AxiomEvaluator<LiftedTag>& axiom_evaluator);

template NodeList<LiftedTag, BuilderStateView<LiftedTag>>
SuccessorGenerator<LiftedTag>::get_successor_nodes<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                formalism::planning::ActionView<LiftedTag> action,
                                                                                std::deque<ygg::Builder<State<LiftedTag>>>& storage,
                                                                                AxiomEvaluator<LiftedTag>& axiom_evaluator);

template void SuccessorGenerator<LiftedTag>::get_successor_nodes<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                       formalism::planning::ActionView<LiftedTag> action,
                                                                                       StateRepository<LiftedTag>& state_repository,
                                                                                       AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                       NodeList<LiftedTag>& out_nodes);

template void SuccessorGenerator<LiftedTag>::get_successor_nodes<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                              formalism::planning::ActionView<LiftedTag> action,
                                                                                              std::deque<ygg::Builder<State<LiftedTag>>>& storage,
                                                                                              AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                              NodeList<LiftedTag, BuilderStateView<LiftedTag>>& out_nodes);

template LabeledNodeList<LiftedTag>
SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                 StateRepository<LiftedTag>& state_repository,
                                                                                 AxiomEvaluator<LiftedTag>& axiom_evaluator);

template LabeledNodeList<LiftedTag, BuilderStateView<LiftedTag>>
SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                        std::deque<ygg::Builder<State<LiftedTag>>>& storage,
                                                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator);

template void SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                               StateRepository<LiftedTag>& state_repository,
                                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                               LabeledNodeList<LiftedTag>& out_nodes);

template void
SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                        std::deque<ygg::Builder<State<LiftedTag>>>& storage,
                                                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                        LabeledNodeList<LiftedTag, BuilderStateView<LiftedTag>>& out_nodes);

template LabeledNodeList<LiftedTag>
SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                 formalism::planning::ActionView<LiftedTag> action,
                                                                                 StateRepository<LiftedTag>& state_repository,
                                                                                 AxiomEvaluator<LiftedTag>& axiom_evaluator);

template LabeledNodeList<LiftedTag, BuilderStateView<LiftedTag>>
SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                        formalism::planning::ActionView<LiftedTag> action,
                                                                                        std::deque<ygg::Builder<State<LiftedTag>>>& storage,
                                                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator);

template void SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                               formalism::planning::ActionView<LiftedTag> action,
                                                                                               StateRepository<LiftedTag>& state_repository,
                                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                               LabeledNodeList<LiftedTag>& out_nodes);

template void
SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                        formalism::planning::ActionView<LiftedTag> action,
                                                                                        std::deque<ygg::Builder<State<LiftedTag>>>& storage,
                                                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                        LabeledNodeList<LiftedTag, BuilderStateView<LiftedTag>>& out_nodes);

template Node<LiftedTag> SuccessorGenerator<LiftedTag>::get_successor_node<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                                 formalism::planning::ActionView<GroundTag> action,
                                                                                                 StateRepository<LiftedTag>& state_repository,
                                                                                                 AxiomEvaluator<LiftedTag>& axiom_evaluator);

template Node<LiftedTag, BuilderStateView<LiftedTag>>
SuccessorGenerator<LiftedTag>::get_successor_node<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                               formalism::planning::ActionView<GroundTag> action,
                                                                               ygg::Builder<State<LiftedTag>>& storage,
                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator);

template Node<LiftedTag> SuccessorGenerator<LiftedTag>::get_successor_node<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                                 formalism::planning::ActionBindingView binding,
                                                                                                 StateRepository<LiftedTag>& state_repository,
                                                                                                 AxiomEvaluator<LiftedTag>& axiom_evaluator);

template Node<LiftedTag, BuilderStateView<LiftedTag>>
SuccessorGenerator<LiftedTag>::get_successor_node<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                               formalism::planning::ActionBindingView binding,
                                                                               ygg::Builder<State<LiftedTag>>& storage,
                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator);

template std::vector<formalism::planning::ActionBindingView>
SuccessorGenerator<LiftedTag>::get_applicable_action_bindings<StateView<LiftedTag>>(const Node<LiftedTag>& node);

template std::vector<formalism::planning::ActionBindingView>
SuccessorGenerator<LiftedTag>::get_applicable_action_bindings<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node);

template void
SuccessorGenerator<LiftedTag>::get_applicable_action_bindings<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                    std::vector<formalism::planning::ActionBindingView>& out_bindings);

template void
SuccessorGenerator<LiftedTag>::get_applicable_action_bindings<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                           std::vector<formalism::planning::ActionBindingView>& out_bindings);

template std::vector<formalism::planning::ActionBindingView>
SuccessorGenerator<LiftedTag>::get_applicable_action_bindings<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                    formalism::planning::ActionView<LiftedTag> action);

template std::vector<formalism::planning::ActionBindingView>
SuccessorGenerator<LiftedTag>::get_applicable_action_bindings<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                           formalism::planning::ActionView<LiftedTag> action);

template void
SuccessorGenerator<LiftedTag>::get_applicable_action_bindings<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                    formalism::planning::ActionView<LiftedTag> action,
                                                                                    std::vector<formalism::planning::ActionBindingView>& out_bindings);

template void
SuccessorGenerator<LiftedTag>::get_applicable_action_bindings<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                           formalism::planning::ActionView<LiftedTag> action,
                                                                                           std::vector<formalism::planning::ActionBindingView>& out_bindings);

template ygg::float_t SuccessorGenerator<LiftedTag>::generate_successor_state<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                                    formalism::planning::ActionBindingView binding,
                                                                                                    ygg::Builder<State<LiftedTag>>& out_state,
                                                                                                    StateDelta& out_delta);

template ygg::float_t
SuccessorGenerator<LiftedTag>::generate_successor_state<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                     formalism::planning::ActionBindingView binding,
                                                                                     ygg::Builder<State<LiftedTag>>& out_state,
                                                                                     StateDelta& out_delta);

template Node<LiftedTag> SuccessorGenerator<LiftedTag>::get_successor_node<StateView<LiftedTag>>(
    const Node<LiftedTag>& node,
    const ygg::Data<formalism::RelationBinding<formalism::planning::Action<LiftedTag>>>& binding,
    StateRepository<LiftedTag>& state_repository,
    AxiomEvaluator<LiftedTag>& axiom_evaluator);

template Node<LiftedTag, BuilderStateView<LiftedTag>> SuccessorGenerator<LiftedTag>::get_successor_node<BuilderStateView<LiftedTag>>(
    const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
    const ygg::Data<formalism::RelationBinding<formalism::planning::Action<LiftedTag>>>& binding,
    ygg::Builder<State<LiftedTag>>& storage,
    AxiomEvaluator<LiftedTag>& axiom_evaluator);

template bool SuccessorGenerator<LiftedTag>::for_each_successor_node<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                           StateRepository<LiftedTag>& state_repository,
                                                                                           AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                           const std::function<bool(Node<LiftedTag>)>& callback);

template bool SuccessorGenerator<LiftedTag>::for_each_successor_node<BuilderStateView<LiftedTag>>(
    const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
    ygg::Builder<State<LiftedTag>>& storage,
    AxiomEvaluator<LiftedTag>& axiom_evaluator,
    const std::function<bool(Node<LiftedTag, BuilderStateView<LiftedTag>>)>& callback);

template PackedNodeList<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_nodes<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                                                   StateRepository<LiftedTag>& state_repository,
                                                                                                                   AxiomEvaluator<LiftedTag>& axiom_evaluator);

template PackedNodeList<LiftedTag>
SuccessorGenerator<LiftedTag>::get_packed_successor_nodes<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                       StateRepository<LiftedTag>& state_repository,
                                                                                       AxiomEvaluator<LiftedTag>& axiom_evaluator);

template void SuccessorGenerator<LiftedTag>::get_packed_successor_nodes<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                              StateRepository<LiftedTag>& state_repository,
                                                                                              AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                              PackedNodeList<LiftedTag>& out_nodes);

template void SuccessorGenerator<LiftedTag>::get_packed_successor_nodes<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                                     StateRepository<LiftedTag>& state_repository,
                                                                                                     AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                                     PackedNodeList<LiftedTag>& out_nodes);

template bool SuccessorGenerator<LiftedTag>::for_each_labeled_successor_node<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                                   StateRepository<LiftedTag>& state_repository,
                                                                                                   AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                                   const std::function<bool(LabeledNode<LiftedTag>)>& callback);

template bool SuccessorGenerator<LiftedTag>::for_each_labeled_successor_node<BuilderStateView<LiftedTag>>(
    const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
    ygg::Builder<State<LiftedTag>>& storage,
    AxiomEvaluator<LiftedTag>& axiom_evaluator,
    const std::function<bool(LabeledNode<LiftedTag, BuilderStateView<LiftedTag>>)>& callback);

template PackedLabeledNodeList<LiftedTag>
SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                        StateRepository<LiftedTag>& state_repository,
                                                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator);

template PackedLabeledNodeList<LiftedTag>
SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                               StateRepository<LiftedTag>& state_repository,
                                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator);

template void SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                                      StateRepository<LiftedTag>& state_repository,
                                                                                                      AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                                      PackedLabeledNodeList<LiftedTag>& out_nodes);

template void
SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                               StateRepository<LiftedTag>& state_repository,
                                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                               PackedLabeledNodeList<LiftedTag>& out_nodes);

template bool SuccessorGenerator<LiftedTag>::for_each_applicable_action_binding<StateView<LiftedTag>>(
    const Node<LiftedTag>& node,
    const std::function<bool(formalism::planning::ActionBindingView)>& callback);

template bool SuccessorGenerator<LiftedTag>::for_each_applicable_action_binding<BuilderStateView<LiftedTag>>(
    const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
    const std::function<bool(formalism::planning::ActionBindingView)>& callback);

template bool SuccessorGenerator<LiftedTag>::for_each_successor_node<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                           formalism::planning::ActionView<LiftedTag> action,
                                                                                           StateRepository<LiftedTag>& state_repository,
                                                                                           AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                           const std::function<bool(Node<LiftedTag>)>& callback);

template bool SuccessorGenerator<LiftedTag>::for_each_successor_node<BuilderStateView<LiftedTag>>(
    const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
    formalism::planning::ActionView<LiftedTag> action,
    ygg::Builder<State<LiftedTag>>& storage,
    AxiomEvaluator<LiftedTag>& axiom_evaluator,
    const std::function<bool(Node<LiftedTag, BuilderStateView<LiftedTag>>)>& callback);

template PackedNodeList<LiftedTag>
SuccessorGenerator<LiftedTag>::get_packed_successor_nodes<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                formalism::planning::ActionView<LiftedTag> action,
                                                                                StateRepository<LiftedTag>& state_repository,
                                                                                AxiomEvaluator<LiftedTag>& axiom_evaluator);

template PackedNodeList<LiftedTag>
SuccessorGenerator<LiftedTag>::get_packed_successor_nodes<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                       formalism::planning::ActionView<LiftedTag> action,
                                                                                       StateRepository<LiftedTag>& state_repository,
                                                                                       AxiomEvaluator<LiftedTag>& axiom_evaluator);

template void SuccessorGenerator<LiftedTag>::get_packed_successor_nodes<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                              formalism::planning::ActionView<LiftedTag> action,
                                                                                              StateRepository<LiftedTag>& state_repository,
                                                                                              AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                              PackedNodeList<LiftedTag>& out_nodes);

template void SuccessorGenerator<LiftedTag>::get_packed_successor_nodes<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                                     formalism::planning::ActionView<LiftedTag> action,
                                                                                                     StateRepository<LiftedTag>& state_repository,
                                                                                                     AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                                     PackedNodeList<LiftedTag>& out_nodes);

template bool SuccessorGenerator<LiftedTag>::for_each_labeled_successor_node<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                                   formalism::planning::ActionView<LiftedTag> action,
                                                                                                   StateRepository<LiftedTag>& state_repository,
                                                                                                   AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                                   const std::function<bool(LabeledNode<LiftedTag>)>& callback);

template bool SuccessorGenerator<LiftedTag>::for_each_labeled_successor_node<BuilderStateView<LiftedTag>>(
    const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
    formalism::planning::ActionView<LiftedTag> action,
    ygg::Builder<State<LiftedTag>>& storage,
    AxiomEvaluator<LiftedTag>& axiom_evaluator,
    const std::function<bool(LabeledNode<LiftedTag, BuilderStateView<LiftedTag>>)>& callback);

template PackedLabeledNodeList<LiftedTag>
SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                        formalism::planning::ActionView<LiftedTag> action,
                                                                                        StateRepository<LiftedTag>& state_repository,
                                                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator);

template PackedLabeledNodeList<LiftedTag>
SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                               formalism::planning::ActionView<LiftedTag> action,
                                                                                               StateRepository<LiftedTag>& state_repository,
                                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator);

template void SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                                      formalism::planning::ActionView<LiftedTag> action,
                                                                                                      StateRepository<LiftedTag>& state_repository,
                                                                                                      AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                                      PackedLabeledNodeList<LiftedTag>& out_nodes);

template void
SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                               formalism::planning::ActionView<LiftedTag> action,
                                                                                               StateRepository<LiftedTag>& state_repository,
                                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                               PackedLabeledNodeList<LiftedTag>& out_nodes);

template bool SuccessorGenerator<LiftedTag>::for_each_applicable_action_binding<StateView<LiftedTag>>(
    const Node<LiftedTag>& node,
    formalism::planning::ActionView<LiftedTag> action,
    const std::function<bool(formalism::planning::ActionBindingView)>& callback);

template bool SuccessorGenerator<LiftedTag>::for_each_applicable_action_binding<BuilderStateView<LiftedTag>>(
    const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
    formalism::planning::ActionView<LiftedTag> action,
    const std::function<bool(formalism::planning::ActionBindingView)>& callback);

template PackedNode<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_node<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                                                              formalism::planning::ActionBindingView binding,
                                                                                                              StateRepository<LiftedTag>& state_repository,
                                                                                                              AxiomEvaluator<LiftedTag>& axiom_evaluator);

template PackedNode<LiftedTag>
SuccessorGenerator<LiftedTag>::get_packed_successor_node<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                      formalism::planning::ActionBindingView binding,
                                                                                      StateRepository<LiftedTag>& state_repository,
                                                                                      AxiomEvaluator<LiftedTag>& axiom_evaluator);

template PackedNode<LiftedTag>
SuccessorGenerator<LiftedTag>::get_packed_successor_node<StateView<LiftedTag>>(const Node<LiftedTag>& node,
                                                                               formalism::planning::ActionView<GroundTag> binding,
                                                                               StateRepository<LiftedTag>& state_repository,
                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator);

template PackedNode<LiftedTag>
SuccessorGenerator<LiftedTag>::get_packed_successor_node<BuilderStateView<LiftedTag>>(const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
                                                                                      formalism::planning::ActionView<GroundTag> binding,
                                                                                      StateRepository<LiftedTag>& state_repository,
                                                                                      AxiomEvaluator<LiftedTag>& axiom_evaluator);

template PackedNode<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_node<StateView<LiftedTag>>(
    const Node<LiftedTag>& node,
    const ygg::Data<formalism::RelationBinding<formalism::planning::Action<LiftedTag>>>& binding,
    StateRepository<LiftedTag>& state_repository,
    AxiomEvaluator<LiftedTag>& axiom_evaluator);

template PackedNode<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_node<BuilderStateView<LiftedTag>>(
    const Node<LiftedTag, BuilderStateView<LiftedTag>>& node,
    const ygg::Data<formalism::RelationBinding<formalism::planning::Action<LiftedTag>>>& binding,
    StateRepository<LiftedTag>& state_repository,
    AxiomEvaluator<LiftedTag>& axiom_evaluator);

static_assert(SuccessorGeneratorConcept<SuccessorGenerator<LiftedTag>, LiftedTag>);
}
