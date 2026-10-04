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
#include "tyr/datalog/formatter.hpp"
#include "tyr/datalog/lifted/contexts/program.hpp"
#include "tyr/datalog/solver.hpp"
#include "tyr/formalism/planning/grounder.hpp"
#include "tyr/planning/action_executor.hpp"
#include "tyr/planning/applicability_lifted.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/match_tree/match_tree.hpp"
#include "tyr/planning/lifted/axiom_evaluator.hpp"
#include "tyr/planning/lifted/programs/action.hpp"
#include "tyr/planning/lifted/state_builder.hpp"
#include "tyr/planning/lifted/state_repository.hpp"
#include "tyr/planning/lifted/state_view.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/successor_generator.hpp"
#include "tyr/planning/task_utils.hpp"

#include <algorithm>
#include <cassert>
#include <fmt/ostream.h>
#include <stdexcept>
#include <utility>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/execution/onetbb.hpp>
#include <yggdrasil/formalism/membership.hpp>

namespace d = tyr::datalog;
namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;
namespace df = tyr::formalism::datalog;

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
    using ActionBindingMap = ygg::UnorderedMap<fp::ActionBindingView, fp::ActionView<GroundTag>>;

    struct SchemaEvaluator
    {
        df::ProgramView<LiftedTag> program;
        std::vector<d::Scheduler<LiftedTag>> schedulers;
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
        fp::Builder scratch_builder;
        ygg::UniqueObjectPoolPtr<ygg::Data<f::RelationBinding<fp::Action<LiftedTag>>>> scratch_action_binding;
        ActionBindingMap action_binding_to_ground_action;
        datalog::ProgramWorkspace<LiftedTag> workspace;
        ygg::UnorderedMap<fp::ActionView<LiftedTag>, SchemaEvaluator> schema_evaluators;
        analysis::CompatibilityWorkspace compatibility_workspace;
        ActionExecutor executor;
    };

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

    SchemaEvaluator& get_schema_evaluator(fp::ActionView<LiftedTag> action)
    {
        const auto it = evaluator.schema_evaluators.find(action);
        // Repository indices can collide across independent factories.
        if (it == evaluator.schema_evaluators.end() || &action.get_context() != &it->first.get_context())
            throw std::invalid_argument("SuccessorGenerator: action schema does not belong to the task domain.");
        return it->second;
    }

    template<StateViewConcept<LiftedTag> S>
    void compute_action_facts(const Node<S>& node, std::vector<d::Scheduler<LiftedTag>>& schedulers);

    template<StateViewConcept<LiftedTag> S, typename Callback>
    bool for_each_applicable_action_binding(const Node<S>& node,
                                            ygg::Data<f::RelationBinding<fp::Action<LiftedTag>>>& scratch_binding,
                                            df::ProgramView<LiftedTag> program,
                                            std::vector<d::Scheduler<LiftedTag>>& schedulers,
                                            Callback&& callback);

    template<StateViewConcept<LiftedTag> S>
    ygg::float_t generate_successor_state(const Node<S>& node,
                                          const ygg::Data<f::RelationBinding<fp::Action<LiftedTag>>>& binding,
                                          ygg::Builder<State<LiftedTag>>& out_state);

    ygg::uint_t index;
    std::shared_ptr<std::atomic<ygg::uint_t>> next_index;
    std::shared_ptr<const Definition> definition;
    Evaluator evaluator;
};

SuccessorGenerator<LiftedTag>::Impl::Definition::Definition(TaskPtr<LiftedTag> task_) : task(std::move(task_)), action_program(task->get_task()) {}

SuccessorGenerator<LiftedTag>::Impl::Evaluator::Evaluator(const Definition& definition, ygg::ExecutionContextPtr execution_context_) :
    execution_context(std::move(execution_context_)),
    scratch_builder(),
    scratch_action_binding(fp::checkout<f::RelationBinding<fp::Action<LiftedTag>>>(scratch_builder)),
    action_binding_to_ground_action(),
    workspace(definition.action_program.get_datalog_program()),
    compatibility_workspace(),
    executor()
{
    assert(execution_context);
    for (const auto& [action, schema] : definition.action_program.get_schema_programs())
        schema_evaluators.emplace(action,
                                  SchemaEvaluator { schema.program,
                                                    d::create_schedulers(schema.strata,
                                                                         schema.listeners,
                                                                         schema.program.get_context(),
                                                                         schema.program.get_predicates<f::FluentTag>().size(),
                                                                         schema.program.get_functions<f::FluentTag>().size()) });
}

template<StateViewConcept<LiftedTag> S>
void SuccessorGenerator<LiftedTag>::Impl::compute_action_facts(const Node<S>& node, std::vector<d::Scheduler<LiftedTag>>& schedulers)
{
    evaluator.workspace.reset_evaluation();

    const auto state = node.get_state();
    const auto& program = definition->action_program;

    insert_extended_state(state.get_state_builder(), *definition->task->get_repository(), program.get_translation_context().p2d, evaluator.workspace);

    auto ctx = d::ProgramExecutionContext(evaluator.workspace, schedulers);
    d::execute_model(ctx, *evaluator.execution_context);
}

template<StateViewConcept<LiftedTag> S, typename Callback>
bool SuccessorGenerator<LiftedTag>::Impl::for_each_applicable_action_binding(const Node<S>& node,
                                                                             ygg::Data<f::RelationBinding<fp::Action<LiftedTag>>>& scratch_binding,
                                                                             df::ProgramView<LiftedTag> program,
                                                                             std::vector<d::Scheduler<LiftedTag>>& schedulers,
                                                                             Callback&& callback)
{
    compute_action_facts(node, schedulers);

    const auto state_context = StateContext<LiftedTag>(*definition->task, node.get_state().get_state_builder(), node.get_metric());
    auto grounder_context = fp::GrounderContext { evaluator.workspace.planning_builder, *definition->task->get_repository(), scratch_binding.objects };
    const auto& mapping = definition->action_program.get_predicate_to_action_mapping();

    for (const auto rule : program.get_rules<f::PredicateTag>())
    {
        const auto predicate = rule.get_head().get_predicate();
        const auto action = mapping.at(predicate);
        const auto& set = evaluator.workspace.facts.fact_sets.predicate.get_sets()[ygg::uint_t(predicate.get_index())];
        for (const auto& binding : set.get_bindings())
        {
            scratch_binding.relation = action.get_index();
            scratch_binding.objects.clear();
            ygg::extend(binding.get_objects(), scratch_binding.objects);

            assert(is_applicable(action.get_condition(), ApplicabilityContext { state_context, grounder_context, *definition->task->get_fdr_context() })
                   && "ApplicableActionProgram emitted an action binding whose condition is not satisfied.");

            // Datalog certifies the action condition, not whether its grounded numeric effects are valid and mutually compatible.
            if (!evaluator.executor.is_applicable_if_fires(action, state_context, grounder_context, *definition->task->get_fdr_context()))
                continue;

            assert(evaluator.executor.is_applicable(action, state_context, grounder_context, *definition->task->get_fdr_context()));
            if (!callback(scratch_binding))
                return false;
        }
    }
    return true;
}

template<StateViewConcept<LiftedTag> S>
ygg::float_t SuccessorGenerator<LiftedTag>::Impl::generate_successor_state(const Node<S>& node,
                                                                           const ygg::Data<f::RelationBinding<fp::Action<LiftedTag>>>& binding,
                                                                           ygg::Builder<State<LiftedTag>>& out_state)
{
    evaluator.workspace.binding.clear();
    for (const auto object : binding.objects)
        evaluator.workspace.binding.push_back(object);

    auto grounder_context = fp::GrounderContext { evaluator.workspace.planning_builder, *definition->task->get_repository(), evaluator.workspace.binding };
    const auto state_context = StateContext<LiftedTag>(*definition->task, node.get_state().get_state_builder(), node.get_metric());
    const auto action = ygg::make_view(binding.relation, *definition->task->get_repository());

    return evaluator.executor.apply_action_unregistered(state_context, action, grounder_context, *definition->task->get_fdr_context(), out_state);
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

Node<StateView<LiftedTag>> SuccessorGenerator<LiftedTag>::get_initial_node(StateRepository<LiftedTag>& state_repository,
                                                                           AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    validate_task(m_impl->definition->task, state_repository);
    validate_task(m_impl->definition->task, axiom_evaluator);
    auto initial_state = state_repository.get_initial_state(axiom_evaluator);
    const auto state_context = StateContext<LiftedTag>(*m_impl->definition->task, initial_state.get_state_builder(), 0);
    const auto state_metric =
        evaluate_metric(m_impl->definition->task->get_task().get_metric(), m_impl->definition->task->get_task().get_auxiliary_fterm_value(), state_context);
    return Node<StateView<LiftedTag>>(std::move(initial_state), state_metric);
}

template<StateViewConcept<LiftedTag> S>
NodeList<S>
SuccessorGenerator<LiftedTag>::get_successor_nodes(const Node<S>& node, SuccessorListStorage<S>& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = NodeList<S> {};
    get_successor_nodes(node, storage, axiom_evaluator, result);
    return result;
}

template<StateViewConcept<LiftedTag> S>
void SuccessorGenerator<LiftedTag>::get_successor_nodes(const Node<S>& node,
                                                        SuccessorListStorage<S>& storage,
                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                        NodeList<S>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, axiom_evaluator);
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
        validate_task(m_impl->definition->task, storage);
    out_nodes.clear();
    m_impl->for_each_applicable_action_binding(node,
                                               *m_impl->evaluator.scratch_action_binding,
                                               m_impl->definition->action_program.get_datalog_program().get_program(),
                                               m_impl->evaluator.workspace.schedulers,
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
NodeList<S> SuccessorGenerator<LiftedTag>::get_successor_nodes(const Node<S>& node,
                                                               fp::ActionView<LiftedTag> action,
                                                               SuccessorListStorage<S>& storage,
                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = NodeList<S> {};
    get_successor_nodes(node, action, storage, axiom_evaluator, result);
    return result;
}

template<StateViewConcept<LiftedTag> S>
void SuccessorGenerator<LiftedTag>::get_successor_nodes(const Node<S>& node,
                                                        fp::ActionView<LiftedTag> action,
                                                        SuccessorListStorage<S>& storage,
                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                        NodeList<S>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, axiom_evaluator);
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
        validate_task(m_impl->definition->task, storage);
    auto& schema = m_impl->get_schema_evaluator(action);
    out_nodes.clear();
    m_impl->for_each_applicable_action_binding(node,
                                               *m_impl->evaluator.scratch_action_binding,
                                               schema.program,
                                               schema.schedulers,
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
LabeledNodeList<S>
SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes(const Node<S>& node, SuccessorListStorage<S>& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = LabeledNodeList<S> {};
    get_labeled_successor_nodes(node, storage, axiom_evaluator, result);
    return result;
}

template<StateViewConcept<LiftedTag> S>
void SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes(const Node<S>& node,
                                                                SuccessorListStorage<S>& storage,
                                                                AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                LabeledNodeList<S>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, axiom_evaluator);
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
        validate_task(m_impl->definition->task, storage);
    out_nodes.clear();
    m_impl->for_each_applicable_action_binding(node,
                                               *m_impl->evaluator.scratch_action_binding,
                                               m_impl->definition->action_program.get_datalog_program().get_program(),
                                               m_impl->evaluator.workspace.schedulers,
                                               [&](auto& binding)
                                               {
                                                   const auto action_binding = fp::insert(*m_impl->definition->task->get_repository(), binding).first;
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
LabeledNodeList<S> SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes(const Node<S>& node,
                                                                              fp::ActionView<LiftedTag> action,
                                                                              SuccessorListStorage<S>& storage,
                                                                              AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = LabeledNodeList<S> {};
    get_labeled_successor_nodes(node, action, storage, axiom_evaluator, result);
    return result;
}

template<StateViewConcept<LiftedTag> S>
void SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes(const Node<S>& node,
                                                                fp::ActionView<LiftedTag> action,
                                                                SuccessorListStorage<S>& storage,
                                                                AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                LabeledNodeList<S>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, axiom_evaluator);
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
        validate_task(m_impl->definition->task, storage);
    auto& schema = m_impl->get_schema_evaluator(action);
    out_nodes.clear();
    m_impl->for_each_applicable_action_binding(node,
                                               *m_impl->evaluator.scratch_action_binding,
                                               schema.program,
                                               schema.schedulers,
                                               [&](auto& binding)
                                               {
                                                   const auto action_binding = fp::insert(*m_impl->definition->task->get_repository(), binding).first;
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
ActionBindingStatus SuccessorGenerator<LiftedTag>::check_action_binding(const Node<S>& node, fp::ActionView<LiftedTag> action, fp::ObjectSpanView objects)
{
    const auto& task = m_impl->definition->task;
    validate_task(task, node.get_state());
    m_impl->get_schema_evaluator(action);
    if (objects.size() != action.get_arity())
        throw std::invalid_argument("SuccessorGenerator::check_action_binding(...): object count does not match action arity.");

    const auto indices = objects.get_data();
    const auto& source_repository = objects.get_context();
    const auto& target_repository = *task->get_repository();
    if (!ygg::formalism::contains_all(source_repository, indices) || !ygg::formalism::contains_all(target_repository, indices))
        return ActionBindingStatus::OUTSIDE_PARAMETER_DOMAIN;
    if (!ygg::formalism::contains_all(target_repository, objects))
        throw std::invalid_argument("SuccessorGenerator::check_action_binding(...): object does not belong to the task repository.");

    const auto& domains = task->get_formalism_task().get_variable_domains().action_domains.at(action.get_index()).payload.precondition_domain.payload;
    for (size_t i = 0; i < objects.size(); ++i)
        if (i >= domains.size() || !std::binary_search(domains[i].objects.begin(), domains[i].objects.end(), indices[i]))
            return ActionBindingStatus::OUTSIDE_PARAMETER_DOMAIN;

    // Single-binding work has separate scratch from the tuple held by an outer enumeration callback.
    auto& workspace = m_impl->evaluator.workspace;
    workspace.binding.clear();
    for (const auto index : indices)
        workspace.binding.push_back(index);
    auto grounder = fp::GrounderContext { workspace.planning_builder, *task->get_repository(), workspace.binding };
    const auto state = StateContext<LiftedTag>(*task, node.get_state().get_state_builder(), node.get_metric());
    return m_impl->evaluator.executor.is_applicable(action, state, grounder, *task->get_fdr_context()) ? ActionBindingStatus::APPLICABLE :
                                                                                                         ActionBindingStatus::INAPPLICABLE;
}

template<StateViewConcept<LiftedTag> S>
Node<S> SuccessorGenerator<LiftedTag>::get_successor_node(const Node<S>& node,
                                                          fp::ActionView<GroundTag> action,
                                                          SuccessorStorage<S>& storage,
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
    const auto generate = [&](auto& target) { return m_impl->evaluator.executor.apply_action_unregistered(state_context, action, target); };
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
    {
        auto successor = storage.get_state_builder();
        const auto auxiliary_value = generate(*successor);
        return finalize_successor_state(storage, axiom_evaluator, std::move(successor), auxiliary_value);
    }
    else
    {
        storage.clear();
        const auto auxiliary_value = generate(storage);
        const auto metric = evaluate_successor_metric(*m_impl->definition->task, storage, auxiliary_value);
        axiom_evaluator.compute_extended_state(storage);
        return Node<S>(S(storage, *m_impl->definition->task), metric);
    }
}

fp::ActionView<GroundTag> SuccessorGenerator<LiftedTag>::ground_action(fp::ActionBindingView binding)
{
    if (const auto it = m_impl->evaluator.action_binding_to_ground_action.find(binding); it != m_impl->evaluator.action_binding_to_ground_action.end())
        return it->second;

    m_impl->evaluator.workspace.binding.clear();
    ygg::extend(binding.get_objects(), m_impl->evaluator.workspace.binding);

    auto grounder_context =
        fp::GrounderContext { m_impl->evaluator.workspace.planning_builder, *m_impl->definition->task->get_repository(), m_impl->evaluator.workspace.binding };
    const auto action = binding.get_relation();
    const auto ground_action = fp::ground(action,
                                          grounder_context,
                                          m_impl->definition->task->get_formalism_task().get_variable_domains().action_domains.at(action.get_index()),
                                          m_impl->evaluator.compatibility_workspace,
                                          *m_impl->definition->task->get_fdr_context())
                                   .first;
    m_impl->evaluator.action_binding_to_ground_action.emplace(binding, ground_action);
    return ground_action;
}

// Interned action-binding input; state storage follows S.
template<StateViewConcept<LiftedTag> S>
Node<S> SuccessorGenerator<LiftedTag>::get_successor_node(const Node<S>& node,
                                                          formalism::planning::ActionBindingView binding,
                                                          SuccessorStorage<S>& storage,
                                                          AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    m_impl->evaluator.scratch_action_binding->relation = binding.get_relation().get_index();
    m_impl->evaluator.scratch_action_binding->objects.clear();
    ygg::extend(binding.get_objects(), m_impl->evaluator.scratch_action_binding->objects);

    return get_successor_node(node, *m_impl->evaluator.scratch_action_binding, storage, axiom_evaluator);
}

template<StateViewConcept<LiftedTag> S>
std::vector<formalism::planning::ActionBindingView> SuccessorGenerator<LiftedTag>::get_applicable_action_bindings(const Node<S>& node)
{
    auto result = std::vector<formalism::planning::ActionBindingView> {};
    get_applicable_action_bindings(node, result);
    return result;
}

template<StateViewConcept<LiftedTag> S>
void SuccessorGenerator<LiftedTag>::get_applicable_action_bindings(const Node<S>& node, std::vector<formalism::planning::ActionBindingView>& out_bindings)
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
std::vector<fp::ActionBindingView> SuccessorGenerator<LiftedTag>::get_applicable_action_bindings(const Node<S>& node, fp::ActionView<LiftedTag> action)
{
    auto result = std::vector<fp::ActionBindingView> {};
    get_applicable_action_bindings(node, action, result);
    return result;
}

template<StateViewConcept<LiftedTag> S>
void SuccessorGenerator<LiftedTag>::get_applicable_action_bindings(const Node<S>& node,
                                                                   fp::ActionView<LiftedTag> action,
                                                                   std::vector<fp::ActionBindingView>& out_bindings)
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
ygg::float_t
SuccessorGenerator<LiftedTag>::generate_successor_state(const Node<S>& node, fp::ActionBindingView binding, ygg::Builder<State<LiftedTag>>& out_state)
{
    validate_task(m_impl->definition->task, node.get_state());
    m_impl->evaluator.scratch_action_binding->relation = binding.get_relation().get_index();
    m_impl->evaluator.scratch_action_binding->objects.clear();
    ygg::extend(binding.get_objects(), m_impl->evaluator.scratch_action_binding->objects);

    return m_impl->generate_successor_state(node, *m_impl->evaluator.scratch_action_binding, out_state);
}

Node<StateView<LiftedTag>> SuccessorGenerator<LiftedTag>::finalize_successor_state(StateRepository<LiftedTag>& state_repository,
                                                                                   AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                   ygg::SharedObjectPoolPtr<ygg::Builder<State<LiftedTag>>, true> state,
                                                                                   ygg::float_t auxiliary_value)
{
    validate_task(m_impl->definition->task, state_repository);
    validate_task(m_impl->definition->task, axiom_evaluator);
    const auto metric = evaluate_successor_metric(*m_impl->definition->task, *state, auxiliary_value);
    return Node<StateView<LiftedTag>>(state_repository.register_state(axiom_evaluator, std::move(state)), metric);
}

// Raw action-binding input; bindings are not interned and state storage follows S.
template<StateViewConcept<LiftedTag> S>
Node<S> SuccessorGenerator<LiftedTag>::get_successor_node(const Node<S>& node,
                                                          const ygg::Data<formalism::RelationBinding<formalism::planning::Action<LiftedTag>>>& binding,
                                                          SuccessorStorage<S>& storage,
                                                          AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, axiom_evaluator);
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
        validate_task(m_impl->definition->task, storage);
    if constexpr (std::same_as<S, BuilderStateView<LiftedTag>>)
        if (&node.get_state().get_state_builder() == &storage)
            throw std::invalid_argument("SuccessorGenerator: source and output builder must not alias.");
    const auto generate = [&](auto& target) { return m_impl->generate_successor_state(node, binding, target); };
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
    {
        auto successor = storage.get_state_builder();
        const auto auxiliary_value = generate(*successor);
        return finalize_successor_state(storage, axiom_evaluator, std::move(successor), auxiliary_value);
    }
    else
    {
        storage.clear();
        const auto auxiliary_value = generate(storage);
        const auto metric = evaluate_successor_metric(*m_impl->definition->task, storage, auxiliary_value);
        axiom_evaluator.compute_extended_state(storage);
        return Node<S>(S(storage, *m_impl->definition->task), metric);
    }
}

// Lookup
Node<StateView<LiftedTag>> SuccessorGenerator<LiftedTag>::get_node(StateRepository<LiftedTag>& state_repository, ygg::Index<State<LiftedTag>> state_index)
{
    validate_task(m_impl->definition->task, state_repository);
    auto state = state_repository.get_registered_state(state_index);
    const auto state_context = StateContext<LiftedTag>(*m_impl->definition->task, state.get_state_builder(), 0);
    const auto state_metric =
        evaluate_metric(m_impl->definition->task->get_task().get_metric(), m_impl->definition->task->get_task().get_auxiliary_fterm_value(), state_context);
    return Node<StateView<LiftedTag>>(std::move(state), state_metric);
}

template<StateViewConcept<LiftedTag> S>
bool SuccessorGenerator<LiftedTag>::for_each_successor_node(const Node<S>& node,
                                                            SuccessorStorage<S>& storage,
                                                            AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                            const std::type_identity_t<std::function<bool(Node<S>)>>& callback)
{
    validate_task(m_impl->definition->task, node.get_state());
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
        validate_task(m_impl->definition->task, storage);
    if constexpr (std::same_as<S, BuilderStateView<LiftedTag>>)
        if (&node.get_state().get_state_builder() == &storage)
            throw std::invalid_argument("SuccessorGenerator: source and output builder must not alias.");
    validate_task(m_impl->definition->task, axiom_evaluator);
    return m_impl->for_each_applicable_action_binding(node,
                                                      *m_impl->evaluator.scratch_action_binding,
                                                      m_impl->definition->action_program.get_datalog_program().get_program(),
                                                      m_impl->evaluator.workspace.schedulers,
                                                      [&](auto& binding) { return callback(get_successor_node(node, binding, storage, axiom_evaluator)); });
}

template<StateViewConcept<LiftedTag> S>
PackedNodeList<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_nodes(const Node<S>& node,
                                                                                    StateRepository<LiftedTag>& state_repository,
                                                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = PackedNodeList<LiftedTag> {};
    get_packed_successor_nodes(node, state_repository, axiom_evaluator, result);
    return result;
}

template<StateViewConcept<LiftedTag> S>
void SuccessorGenerator<LiftedTag>::get_packed_successor_nodes(const Node<S>& node,
                                                               StateRepository<LiftedTag>& state_repository,
                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                               PackedNodeList<LiftedTag>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, axiom_evaluator);
    validate_task(m_impl->definition->task, state_repository);
    out_nodes.clear();
    m_impl->for_each_applicable_action_binding(node,
                                               *m_impl->evaluator.scratch_action_binding,
                                               m_impl->definition->action_program.get_datalog_program().get_program(),
                                               m_impl->evaluator.workspace.schedulers,
                                               [&](auto& binding)
                                               {
                                                   out_nodes.push_back(get_packed_successor_node(node, binding, state_repository, axiom_evaluator));
                                                   return true;
                                               });
}

template<StateViewConcept<LiftedTag> S>
bool SuccessorGenerator<LiftedTag>::for_each_labeled_successor_node(const Node<S>& node,
                                                                    SuccessorStorage<S>& storage,
                                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                    const std::type_identity_t<std::function<bool(LabeledNode<S>)>>& callback)
{
    validate_task(m_impl->definition->task, node.get_state());
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
        validate_task(m_impl->definition->task, storage);
    if constexpr (std::same_as<S, BuilderStateView<LiftedTag>>)
        if (&node.get_state().get_state_builder() == &storage)
            throw std::invalid_argument("SuccessorGenerator: source and output builder must not alias.");
    validate_task(m_impl->definition->task, axiom_evaluator);
    return m_impl->for_each_applicable_action_binding(node,
                                                      *m_impl->evaluator.scratch_action_binding,
                                                      m_impl->definition->action_program.get_datalog_program().get_program(),
                                                      m_impl->evaluator.workspace.schedulers,
                                                      [&](auto& binding)
                                                      {
                                                          const auto action_binding = fp::insert(*m_impl->definition->task->get_repository(), binding).first;
                                                          return callback({ action_binding, get_successor_node(node, binding, storage, axiom_evaluator) });
                                                      });
}

template<StateViewConcept<LiftedTag> S>
PackedLabeledNodeList<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes(const Node<S>& node,
                                                                                                   StateRepository<LiftedTag>& state_repository,
                                                                                                   AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = PackedLabeledNodeList<LiftedTag> {};
    get_packed_labeled_successor_nodes(node, state_repository, axiom_evaluator, result);
    return result;
}

template<StateViewConcept<LiftedTag> S>
void SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes(const Node<S>& node,
                                                                       StateRepository<LiftedTag>& state_repository,
                                                                       AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                       PackedLabeledNodeList<LiftedTag>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, axiom_evaluator);
    validate_task(m_impl->definition->task, state_repository);
    out_nodes.clear();
    m_impl->for_each_applicable_action_binding(node,
                                               *m_impl->evaluator.scratch_action_binding,
                                               m_impl->definition->action_program.get_datalog_program().get_program(),
                                               m_impl->evaluator.workspace.schedulers,
                                               [&](auto& binding)
                                               {
                                                   const auto action_binding = fp::insert(*m_impl->definition->task->get_repository(), binding).first;
                                                   out_nodes.push_back(
                                                       { action_binding, get_packed_successor_node(node, binding, state_repository, axiom_evaluator) });
                                                   return true;
                                               });
}

template<StateViewConcept<LiftedTag> S>
bool SuccessorGenerator<LiftedTag>::for_each_applicable_action_binding(const Node<S>& node, const std::function<bool(fp::ActionBindingView)>& callback)
{
    validate_task(m_impl->definition->task, node.get_state());
    return m_impl->for_each_applicable_action_binding(node,
                                                      *m_impl->evaluator.scratch_action_binding,
                                                      m_impl->definition->action_program.get_datalog_program().get_program(),
                                                      m_impl->evaluator.workspace.schedulers,
                                                      [&](auto& binding)
                                                      { return callback(fp::insert(*m_impl->definition->task->get_repository(), binding).first); });
}

template<StateViewConcept<LiftedTag> S>
bool SuccessorGenerator<LiftedTag>::for_each_successor_node(const Node<S>& node,
                                                            fp::ActionView<LiftedTag> action,
                                                            SuccessorStorage<S>& storage,
                                                            AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                            const std::type_identity_t<std::function<bool(Node<S>)>>& callback)
{
    validate_task(m_impl->definition->task, node.get_state());
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
        validate_task(m_impl->definition->task, storage);
    if constexpr (std::same_as<S, BuilderStateView<LiftedTag>>)
        if (&node.get_state().get_state_builder() == &storage)
            throw std::invalid_argument("SuccessorGenerator: source and output builder must not alias.");
    validate_task(m_impl->definition->task, axiom_evaluator);
    auto& schema = m_impl->get_schema_evaluator(action);
    return m_impl->for_each_applicable_action_binding(node,
                                                      *m_impl->evaluator.scratch_action_binding,
                                                      schema.program,
                                                      schema.schedulers,
                                                      [&](auto& binding) { return callback(get_successor_node(node, binding, storage, axiom_evaluator)); });
}

template<StateViewConcept<LiftedTag> S>
PackedNodeList<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_nodes(const Node<S>& node,
                                                                                    fp::ActionView<LiftedTag> action,
                                                                                    StateRepository<LiftedTag>& state_repository,
                                                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = PackedNodeList<LiftedTag> {};
    get_packed_successor_nodes(node, action, state_repository, axiom_evaluator, result);
    return result;
}

template<StateViewConcept<LiftedTag> S>
void SuccessorGenerator<LiftedTag>::get_packed_successor_nodes(const Node<S>& node,
                                                               fp::ActionView<LiftedTag> action,
                                                               StateRepository<LiftedTag>& state_repository,
                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                               PackedNodeList<LiftedTag>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, axiom_evaluator);
    validate_task(m_impl->definition->task, state_repository);
    auto& schema = m_impl->get_schema_evaluator(action);
    out_nodes.clear();
    m_impl->for_each_applicable_action_binding(node,
                                               *m_impl->evaluator.scratch_action_binding,
                                               schema.program,
                                               schema.schedulers,
                                               [&](auto& binding)
                                               {
                                                   out_nodes.push_back(get_packed_successor_node(node, binding, state_repository, axiom_evaluator));
                                                   return true;
                                               });
}

template<StateViewConcept<LiftedTag> S>
bool SuccessorGenerator<LiftedTag>::for_each_labeled_successor_node(const Node<S>& node,
                                                                    fp::ActionView<LiftedTag> action,
                                                                    SuccessorStorage<S>& storage,
                                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                    const std::type_identity_t<std::function<bool(LabeledNode<S>)>>& callback)
{
    validate_task(m_impl->definition->task, node.get_state());
    if constexpr (std::same_as<S, StateView<LiftedTag>>)
        validate_task(m_impl->definition->task, storage);
    if constexpr (std::same_as<S, BuilderStateView<LiftedTag>>)
        if (&node.get_state().get_state_builder() == &storage)
            throw std::invalid_argument("SuccessorGenerator: source and output builder must not alias.");
    validate_task(m_impl->definition->task, axiom_evaluator);
    auto& schema = m_impl->get_schema_evaluator(action);
    return m_impl->for_each_applicable_action_binding(node,
                                                      *m_impl->evaluator.scratch_action_binding,
                                                      schema.program,
                                                      schema.schedulers,
                                                      [&](auto& binding)
                                                      {
                                                          const auto action_binding = fp::insert(*m_impl->definition->task->get_repository(), binding).first;
                                                          return callback({ action_binding, get_successor_node(node, binding, storage, axiom_evaluator) });
                                                      });
}

template<StateViewConcept<LiftedTag> S>
PackedLabeledNodeList<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes(const Node<S>& node,
                                                                                                   fp::ActionView<LiftedTag> action,
                                                                                                   StateRepository<LiftedTag>& state_repository,
                                                                                                   AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    auto result = PackedLabeledNodeList<LiftedTag> {};
    get_packed_labeled_successor_nodes(node, action, state_repository, axiom_evaluator, result);
    return result;
}

template<StateViewConcept<LiftedTag> S>
void SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes(const Node<S>& node,
                                                                       fp::ActionView<LiftedTag> action,
                                                                       StateRepository<LiftedTag>& state_repository,
                                                                       AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                       PackedLabeledNodeList<LiftedTag>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, axiom_evaluator);
    validate_task(m_impl->definition->task, state_repository);
    auto& schema = m_impl->get_schema_evaluator(action);
    out_nodes.clear();
    m_impl->for_each_applicable_action_binding(node,
                                               *m_impl->evaluator.scratch_action_binding,
                                               schema.program,
                                               schema.schedulers,
                                               [&](auto& binding)
                                               {
                                                   const auto action_binding = fp::insert(*m_impl->definition->task->get_repository(), binding).first;
                                                   out_nodes.push_back(
                                                       { action_binding, get_packed_successor_node(node, binding, state_repository, axiom_evaluator) });
                                                   return true;
                                               });
}

template<StateViewConcept<LiftedTag> S>
bool SuccessorGenerator<LiftedTag>::for_each_applicable_action_binding(const Node<S>& node,
                                                                       fp::ActionView<LiftedTag> action,
                                                                       const std::function<bool(fp::ActionBindingView)>& callback)
{
    validate_task(m_impl->definition->task, node.get_state());
    auto& schema = m_impl->get_schema_evaluator(action);
    return m_impl->for_each_applicable_action_binding(node,
                                                      *m_impl->evaluator.scratch_action_binding,
                                                      schema.program,
                                                      schema.schedulers,
                                                      [&](auto& binding)
                                                      { return callback(fp::insert(*m_impl->definition->task->get_repository(), binding).first); });
}

PackedNode<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_initial_node(StateRepository<LiftedTag>& state_repository,
                                                                             AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    return get_initial_node(state_repository, axiom_evaluator).pack();
}

PackedNode<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_node(StateRepository<LiftedTag>& state_repository, ygg::Index<State<LiftedTag>> state_index)
{
    return get_node(state_repository, state_index).pack();
}

template<StateViewConcept<LiftedTag> S>
PackedNode<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_node(const Node<S>& node,
                                                                               fp::ActionBindingView binding,
                                                                               StateRepository<LiftedTag>& state_repository,
                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    m_impl->evaluator.scratch_action_binding->relation = binding.get_relation().get_index();
    m_impl->evaluator.scratch_action_binding->objects.clear();
    ygg::extend(binding.get_objects(), m_impl->evaluator.scratch_action_binding->objects);
    return get_packed_successor_node(node, *m_impl->evaluator.scratch_action_binding, state_repository, axiom_evaluator);
}

template<StateViewConcept<LiftedTag> S>
PackedNode<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_node(const Node<S>& node,
                                                                               fp::ActionView<GroundTag> binding,
                                                                               StateRepository<LiftedTag>& state_repository,
                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, axiom_evaluator);
    validate_task(m_impl->definition->task, state_repository);
    auto successor = state_repository.get_state_builder();
    const auto state_context = StateContext<LiftedTag>(*m_impl->definition->task, node.get_state().get_state_builder(), node.get_metric());
    const auto auxiliary_value = m_impl->evaluator.executor.apply_action_unregistered(state_context, binding, *successor);
    return finalize_successor_state(state_repository, axiom_evaluator, std::move(successor), auxiliary_value).pack();
}

template<StateViewConcept<LiftedTag> S>
PackedNode<LiftedTag>
SuccessorGenerator<LiftedTag>::get_packed_successor_node(const Node<S>& node,
                                                         const ygg::Data<formalism::RelationBinding<formalism::planning::Action<LiftedTag>>>& binding,
                                                         StateRepository<LiftedTag>& state_repository,
                                                         AxiomEvaluator<LiftedTag>& axiom_evaluator)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, axiom_evaluator);
    validate_task(m_impl->definition->task, state_repository);
    auto successor = state_repository.get_state_builder();
    const auto auxiliary_value = m_impl->generate_successor_state(node, binding, *successor);
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

    std::cout << "[Successor generator] Summary" << std::endl;
    fmt::print(std::cout, "{}\n", m_impl->evaluator.workspace.statistics);
    auto successor_generator_rule_statistics = std::vector<datalog::RuleStatistics> {};
    for (const auto& ws_rule : m_impl->evaluator.workspace.template get_rules<f::PredicateTag>())
        successor_generator_rule_statistics.push_back(ws_rule->common.statistics);
    fmt::print(std::cout, "{}\n", datalog::compute_aggregated_rule_statistics(successor_generator_rule_statistics));
    auto successor_generator_rule_worker_statistics = std::vector<datalog::RuleWorkerStatistics> {};
    for (const auto& ws_rule : m_impl->evaluator.workspace.template get_rules<f::PredicateTag>())
        for (const auto& worker : ws_rule->worker)
            successor_generator_rule_worker_statistics.push_back(worker.solve.statistics);
    fmt::print(std::cout, "{}\n", datalog::compute_aggregated_rule_worker_statistics(successor_generator_rule_worker_statistics));
}

template ActionBindingStatus
SuccessorGenerator<LiftedTag>::check_action_binding(const Node<StateView<LiftedTag>>& node, fp::ActionView<LiftedTag> action, fp::ObjectSpanView objects);

template ActionBindingStatus SuccessorGenerator<LiftedTag>::check_action_binding(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                 fp::ActionView<LiftedTag> action,
                                                                                 fp::ObjectSpanView objects);

template NodeList<StateView<LiftedTag>> SuccessorGenerator<LiftedTag>::get_successor_nodes<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                                                 StateRepository<LiftedTag>& state_repository,
                                                                                                                 AxiomEvaluator<LiftedTag>& axiom_evaluator);

template NodeList<BuilderStateView<LiftedTag>>
SuccessorGenerator<LiftedTag>::get_successor_nodes<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                std::deque<ygg::Builder<State<LiftedTag>>>& storage,
                                                                                AxiomEvaluator<LiftedTag>& axiom_evaluator);

template void SuccessorGenerator<LiftedTag>::get_successor_nodes<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                       StateRepository<LiftedTag>& state_repository,
                                                                                       AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                       NodeList<StateView<LiftedTag>>& out_nodes);

template void SuccessorGenerator<LiftedTag>::get_successor_nodes<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                              std::deque<ygg::Builder<State<LiftedTag>>>& storage,
                                                                                              AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                              NodeList<BuilderStateView<LiftedTag>>& out_nodes);

template NodeList<StateView<LiftedTag>> SuccessorGenerator<LiftedTag>::get_successor_nodes<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                                                 fp::ActionView<LiftedTag> action,
                                                                                                                 StateRepository<LiftedTag>& state_repository,
                                                                                                                 AxiomEvaluator<LiftedTag>& axiom_evaluator);

template NodeList<BuilderStateView<LiftedTag>>
SuccessorGenerator<LiftedTag>::get_successor_nodes<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                fp::ActionView<LiftedTag> action,
                                                                                std::deque<ygg::Builder<State<LiftedTag>>>& storage,
                                                                                AxiomEvaluator<LiftedTag>& axiom_evaluator);

template void SuccessorGenerator<LiftedTag>::get_successor_nodes<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                       fp::ActionView<LiftedTag> action,
                                                                                       StateRepository<LiftedTag>& state_repository,
                                                                                       AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                       NodeList<StateView<LiftedTag>>& out_nodes);

template void SuccessorGenerator<LiftedTag>::get_successor_nodes<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                              fp::ActionView<LiftedTag> action,
                                                                                              std::deque<ygg::Builder<State<LiftedTag>>>& storage,
                                                                                              AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                              NodeList<BuilderStateView<LiftedTag>>& out_nodes);

template LabeledNodeList<StateView<LiftedTag>>
SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                 StateRepository<LiftedTag>& state_repository,
                                                                                 AxiomEvaluator<LiftedTag>& axiom_evaluator);

template LabeledNodeList<BuilderStateView<LiftedTag>>
SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                        std::deque<ygg::Builder<State<LiftedTag>>>& storage,
                                                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator);

template void SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                               StateRepository<LiftedTag>& state_repository,
                                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                               LabeledNodeList<StateView<LiftedTag>>& out_nodes);

template void SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                                      std::deque<ygg::Builder<State<LiftedTag>>>& storage,
                                                                                                      AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                                      LabeledNodeList<BuilderStateView<LiftedTag>>& out_nodes);

template LabeledNodeList<StateView<LiftedTag>>
SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                 fp::ActionView<LiftedTag> action,
                                                                                 StateRepository<LiftedTag>& state_repository,
                                                                                 AxiomEvaluator<LiftedTag>& axiom_evaluator);

template LabeledNodeList<BuilderStateView<LiftedTag>>
SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                        fp::ActionView<LiftedTag> action,
                                                                                        std::deque<ygg::Builder<State<LiftedTag>>>& storage,
                                                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator);

template void SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                               fp::ActionView<LiftedTag> action,
                                                                                               StateRepository<LiftedTag>& state_repository,
                                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                               LabeledNodeList<StateView<LiftedTag>>& out_nodes);

template void SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                                      fp::ActionView<LiftedTag> action,
                                                                                                      std::deque<ygg::Builder<State<LiftedTag>>>& storage,
                                                                                                      AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                                      LabeledNodeList<BuilderStateView<LiftedTag>>& out_nodes);

template Node<StateView<LiftedTag>> SuccessorGenerator<LiftedTag>::get_successor_node<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                                            fp::ActionView<GroundTag> action,
                                                                                                            StateRepository<LiftedTag>& state_repository,
                                                                                                            AxiomEvaluator<LiftedTag>& axiom_evaluator);

template Node<BuilderStateView<LiftedTag>>
SuccessorGenerator<LiftedTag>::get_successor_node<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                               fp::ActionView<GroundTag> action,
                                                                               ygg::Builder<State<LiftedTag>>& storage,
                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator);

template Node<StateView<LiftedTag>> SuccessorGenerator<LiftedTag>::get_successor_node<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                                            formalism::planning::ActionBindingView binding,
                                                                                                            StateRepository<LiftedTag>& state_repository,
                                                                                                            AxiomEvaluator<LiftedTag>& axiom_evaluator);

template Node<BuilderStateView<LiftedTag>>
SuccessorGenerator<LiftedTag>::get_successor_node<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                               formalism::planning::ActionBindingView binding,
                                                                               ygg::Builder<State<LiftedTag>>& storage,
                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator);

template std::vector<formalism::planning::ActionBindingView>
SuccessorGenerator<LiftedTag>::get_applicable_action_bindings<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node);

template std::vector<formalism::planning::ActionBindingView>
SuccessorGenerator<LiftedTag>::get_applicable_action_bindings<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node);

template void
SuccessorGenerator<LiftedTag>::get_applicable_action_bindings<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                    std::vector<formalism::planning::ActionBindingView>& out_bindings);

template void
SuccessorGenerator<LiftedTag>::get_applicable_action_bindings<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                           std::vector<formalism::planning::ActionBindingView>& out_bindings);

template std::vector<fp::ActionBindingView>
SuccessorGenerator<LiftedTag>::get_applicable_action_bindings<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node, fp::ActionView<LiftedTag> action);

template std::vector<fp::ActionBindingView>
SuccessorGenerator<LiftedTag>::get_applicable_action_bindings<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                           fp::ActionView<LiftedTag> action);

template void SuccessorGenerator<LiftedTag>::get_applicable_action_bindings<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                                  fp::ActionView<LiftedTag> action,
                                                                                                  std::vector<fp::ActionBindingView>& out_bindings);

template void SuccessorGenerator<LiftedTag>::get_applicable_action_bindings<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                                         fp::ActionView<LiftedTag> action,
                                                                                                         std::vector<fp::ActionBindingView>& out_bindings);

template ygg::float_t SuccessorGenerator<LiftedTag>::generate_successor_state<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                                    fp::ActionBindingView binding,
                                                                                                    ygg::Builder<State<LiftedTag>>& out_state);

template ygg::float_t SuccessorGenerator<LiftedTag>::generate_successor_state<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                                           fp::ActionBindingView binding,
                                                                                                           ygg::Builder<State<LiftedTag>>& out_state);

template Node<StateView<LiftedTag>> SuccessorGenerator<LiftedTag>::get_successor_node<StateView<LiftedTag>>(
    const Node<StateView<LiftedTag>>& node,
    const ygg::Data<formalism::RelationBinding<formalism::planning::Action<LiftedTag>>>& binding,
    StateRepository<LiftedTag>& state_repository,
    AxiomEvaluator<LiftedTag>& axiom_evaluator);

template Node<BuilderStateView<LiftedTag>> SuccessorGenerator<LiftedTag>::get_successor_node<BuilderStateView<LiftedTag>>(
    const Node<BuilderStateView<LiftedTag>>& node,
    const ygg::Data<formalism::RelationBinding<formalism::planning::Action<LiftedTag>>>& binding,
    ygg::Builder<State<LiftedTag>>& storage,
    AxiomEvaluator<LiftedTag>& axiom_evaluator);

template bool SuccessorGenerator<LiftedTag>::for_each_successor_node<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                           StateRepository<LiftedTag>& state_repository,
                                                                                           AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                           const std::function<bool(Node<StateView<LiftedTag>>)>& callback);

template bool
SuccessorGenerator<LiftedTag>::for_each_successor_node<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                    ygg::Builder<State<LiftedTag>>& storage,
                                                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                    const std::function<bool(Node<BuilderStateView<LiftedTag>>)>& callback);

template PackedNodeList<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_nodes<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                                                   StateRepository<LiftedTag>& state_repository,
                                                                                                                   AxiomEvaluator<LiftedTag>& axiom_evaluator);

template PackedNodeList<LiftedTag>
SuccessorGenerator<LiftedTag>::get_packed_successor_nodes<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                       StateRepository<LiftedTag>& state_repository,
                                                                                       AxiomEvaluator<LiftedTag>& axiom_evaluator);

template void SuccessorGenerator<LiftedTag>::get_packed_successor_nodes<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                              StateRepository<LiftedTag>& state_repository,
                                                                                              AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                              PackedNodeList<LiftedTag>& out_nodes);

template void SuccessorGenerator<LiftedTag>::get_packed_successor_nodes<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                                     StateRepository<LiftedTag>& state_repository,
                                                                                                     AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                                     PackedNodeList<LiftedTag>& out_nodes);

template bool
SuccessorGenerator<LiftedTag>::for_each_labeled_successor_node<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                     StateRepository<LiftedTag>& state_repository,
                                                                                     AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                     const std::function<bool(LabeledNode<StateView<LiftedTag>>)>& callback);

template bool SuccessorGenerator<LiftedTag>::for_each_labeled_successor_node<BuilderStateView<LiftedTag>>(
    const Node<BuilderStateView<LiftedTag>>& node,
    ygg::Builder<State<LiftedTag>>& storage,
    AxiomEvaluator<LiftedTag>& axiom_evaluator,
    const std::function<bool(LabeledNode<BuilderStateView<LiftedTag>>)>& callback);

template PackedLabeledNodeList<LiftedTag>
SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                        StateRepository<LiftedTag>& state_repository,
                                                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator);

template PackedLabeledNodeList<LiftedTag>
SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                               StateRepository<LiftedTag>& state_repository,
                                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator);

template void SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                                      StateRepository<LiftedTag>& state_repository,
                                                                                                      AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                                      PackedLabeledNodeList<LiftedTag>& out_nodes);

template void SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                                             StateRepository<LiftedTag>& state_repository,
                                                                                                             AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                                             PackedLabeledNodeList<LiftedTag>& out_nodes);

template bool
SuccessorGenerator<LiftedTag>::for_each_applicable_action_binding<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                        const std::function<bool(fp::ActionBindingView)>& callback);

template bool
SuccessorGenerator<LiftedTag>::for_each_applicable_action_binding<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                               const std::function<bool(fp::ActionBindingView)>& callback);

template bool SuccessorGenerator<LiftedTag>::for_each_successor_node<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                           fp::ActionView<LiftedTag> action,
                                                                                           StateRepository<LiftedTag>& state_repository,
                                                                                           AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                           const std::function<bool(Node<StateView<LiftedTag>>)>& callback);

template bool
SuccessorGenerator<LiftedTag>::for_each_successor_node<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                    fp::ActionView<LiftedTag> action,
                                                                                    ygg::Builder<State<LiftedTag>>& storage,
                                                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                    const std::function<bool(Node<BuilderStateView<LiftedTag>>)>& callback);

template PackedNodeList<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_nodes<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                                                   fp::ActionView<LiftedTag> action,
                                                                                                                   StateRepository<LiftedTag>& state_repository,
                                                                                                                   AxiomEvaluator<LiftedTag>& axiom_evaluator);

template PackedNodeList<LiftedTag>
SuccessorGenerator<LiftedTag>::get_packed_successor_nodes<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                       fp::ActionView<LiftedTag> action,
                                                                                       StateRepository<LiftedTag>& state_repository,
                                                                                       AxiomEvaluator<LiftedTag>& axiom_evaluator);

template void SuccessorGenerator<LiftedTag>::get_packed_successor_nodes<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                              fp::ActionView<LiftedTag> action,
                                                                                              StateRepository<LiftedTag>& state_repository,
                                                                                              AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                              PackedNodeList<LiftedTag>& out_nodes);

template void SuccessorGenerator<LiftedTag>::get_packed_successor_nodes<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                                     fp::ActionView<LiftedTag> action,
                                                                                                     StateRepository<LiftedTag>& state_repository,
                                                                                                     AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                                     PackedNodeList<LiftedTag>& out_nodes);

template bool
SuccessorGenerator<LiftedTag>::for_each_labeled_successor_node<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                     fp::ActionView<LiftedTag> action,
                                                                                     StateRepository<LiftedTag>& state_repository,
                                                                                     AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                     const std::function<bool(LabeledNode<StateView<LiftedTag>>)>& callback);

template bool SuccessorGenerator<LiftedTag>::for_each_labeled_successor_node<BuilderStateView<LiftedTag>>(
    const Node<BuilderStateView<LiftedTag>>& node,
    fp::ActionView<LiftedTag> action,
    ygg::Builder<State<LiftedTag>>& storage,
    AxiomEvaluator<LiftedTag>& axiom_evaluator,
    const std::function<bool(LabeledNode<BuilderStateView<LiftedTag>>)>& callback);

template PackedLabeledNodeList<LiftedTag>
SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                        fp::ActionView<LiftedTag> action,
                                                                                        StateRepository<LiftedTag>& state_repository,
                                                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator);

template PackedLabeledNodeList<LiftedTag>
SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                               fp::ActionView<LiftedTag> action,
                                                                                               StateRepository<LiftedTag>& state_repository,
                                                                                               AxiomEvaluator<LiftedTag>& axiom_evaluator);

template void SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                                      fp::ActionView<LiftedTag> action,
                                                                                                      StateRepository<LiftedTag>& state_repository,
                                                                                                      AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                                      PackedLabeledNodeList<LiftedTag>& out_nodes);

template void SuccessorGenerator<LiftedTag>::get_packed_labeled_successor_nodes<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                                             fp::ActionView<LiftedTag> action,
                                                                                                             StateRepository<LiftedTag>& state_repository,
                                                                                                             AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                                                                             PackedLabeledNodeList<LiftedTag>& out_nodes);

template bool
SuccessorGenerator<LiftedTag>::for_each_applicable_action_binding<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                        fp::ActionView<LiftedTag> action,
                                                                                        const std::function<bool(fp::ActionBindingView)>& callback);

template bool
SuccessorGenerator<LiftedTag>::for_each_applicable_action_binding<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                               fp::ActionView<LiftedTag> action,
                                                                                               const std::function<bool(fp::ActionBindingView)>& callback);

template PackedNode<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_node<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                                              fp::ActionBindingView binding,
                                                                                                              StateRepository<LiftedTag>& state_repository,
                                                                                                              AxiomEvaluator<LiftedTag>& axiom_evaluator);

template PackedNode<LiftedTag>
SuccessorGenerator<LiftedTag>::get_packed_successor_node<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                      fp::ActionBindingView binding,
                                                                                      StateRepository<LiftedTag>& state_repository,
                                                                                      AxiomEvaluator<LiftedTag>& axiom_evaluator);

template PackedNode<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_node<StateView<LiftedTag>>(const Node<StateView<LiftedTag>>& node,
                                                                                                              fp::ActionView<GroundTag> binding,
                                                                                                              StateRepository<LiftedTag>& state_repository,
                                                                                                              AxiomEvaluator<LiftedTag>& axiom_evaluator);

template PackedNode<LiftedTag>
SuccessorGenerator<LiftedTag>::get_packed_successor_node<BuilderStateView<LiftedTag>>(const Node<BuilderStateView<LiftedTag>>& node,
                                                                                      fp::ActionView<GroundTag> binding,
                                                                                      StateRepository<LiftedTag>& state_repository,
                                                                                      AxiomEvaluator<LiftedTag>& axiom_evaluator);

template PackedNode<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_node<StateView<LiftedTag>>(
    const Node<StateView<LiftedTag>>& node,
    const ygg::Data<formalism::RelationBinding<formalism::planning::Action<LiftedTag>>>& binding,
    StateRepository<LiftedTag>& state_repository,
    AxiomEvaluator<LiftedTag>& axiom_evaluator);

template PackedNode<LiftedTag> SuccessorGenerator<LiftedTag>::get_packed_successor_node<BuilderStateView<LiftedTag>>(
    const Node<BuilderStateView<LiftedTag>>& node,
    const ygg::Data<formalism::RelationBinding<formalism::planning::Action<LiftedTag>>>& binding,
    StateRepository<LiftedTag>& state_repository,
    AxiomEvaluator<LiftedTag>& axiom_evaluator);

static_assert(SuccessorGeneratorConcept<SuccessorGenerator<LiftedTag>, LiftedTag>);
}
