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
#include "tyr/datalog/lifted/bottom_up.hpp"
#include "tyr/datalog/lifted/contexts/program.hpp"
#include "tyr/formalism/planning/grounder.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"
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

#include <cassert>
#include <fmt/ostream.h>
#include <utility>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/core/itertools.hpp>
#include <yggdrasil/execution/onetbb.hpp>

namespace d = tyr::datalog;
namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;
namespace df = tyr::formalism::datalog;

namespace tyr::planning
{

struct SuccessorGenerator<LiftedTag>::Impl
{
    using Program = ApplicableActionProgram<LiftedTag>;
    using ActionBindingMap = ygg::UnorderedMap<fp::ActionBindingView, fp::GroundActionView>;

    struct Definition
    {
        explicit Definition(TaskPtr<LiftedTag> task);

        TaskPtr<LiftedTag> task;
        Program action_program;
    };

    struct Evaluator
    {
        Evaluator(const Definition& definition, ygg::ExecutionContextPtr execution_context, StateRepositoryPtr<LiftedTag> state_repository);

        ygg::ExecutionContextPtr execution_context;
        ygg::Data<f::RelationBinding<fp::Action>> scratch_action_binding;
        ActionBindingMap action_binding_to_ground_action;
        datalog::ProgramWorkspace<LiftedTag> workspace;
        ygg::itertools::cartesian_set::Workspace<ygg::Index<f::Object>> cartesian_workspace;
        StateRepositoryPtr<LiftedTag> state_repository;
        ActionExecutor executor;
    };

    Impl(ygg::uint_t index,
         TaskPtr<LiftedTag> task,
         ygg::ExecutionContextPtr execution_context,
         StateRepositoryPtr<LiftedTag> state_repository,
         std::shared_ptr<std::atomic<ygg::uint_t>> next_index) :
        index(index),
        next_index(std::move(next_index)),
        definition(std::make_shared<Definition>(std::move(task))),
        evaluator(*definition, std::move(execution_context), std::move(state_repository))
    {
    }

    Impl(ygg::uint_t index,
         std::shared_ptr<const Definition> definition,
         ygg::ExecutionContextPtr execution_context,
         StateRepositoryPtr<LiftedTag> state_repository,
         std::shared_ptr<std::atomic<ygg::uint_t>> next_index) :
        index(index),
        next_index(std::move(next_index)),
        definition(std::move(definition)),
        evaluator(*this->definition, std::move(execution_context), std::move(state_repository))
    {
    }

    void compute_action_facts(const Node<LiftedTag>& node);

    template<typename Callback>
    void for_each_applicable_action_binding(const Node<LiftedTag>& node, ygg::Data<f::RelationBinding<fp::Action>>& scratch_binding, Callback&& callback);

    ygg::uint_t index;
    std::shared_ptr<std::atomic<ygg::uint_t>> next_index;
    std::shared_ptr<const Definition> definition;
    Evaluator evaluator;
};

SuccessorGenerator<LiftedTag>::Impl::Definition::Definition(TaskPtr<LiftedTag> task_) : task(std::move(task_)), action_program(task->get_task()) {}

SuccessorGenerator<LiftedTag>::Impl::Evaluator::Evaluator(const Definition& definition,
                                                          ygg::ExecutionContextPtr execution_context_,
                                                          StateRepositoryPtr<LiftedTag> state_repository_) :
    execution_context(std::move(execution_context_)),
    scratch_action_binding(),
    action_binding_to_ground_action(),
    workspace(definition.action_program.get_datalog_program()),
    cartesian_workspace(),
    state_repository(std::move(state_repository_)),
    executor()
{
    assert(execution_context);
}

void SuccessorGenerator<LiftedTag>::Impl::compute_action_facts(const Node<LiftedTag>& node)
{
    evaluator.workspace.reset_evaluation();

    const auto state = node.get_state();
    auto merge_context = fp::MergeDatalogContext { evaluator.workspace.datalog_builder, evaluator.workspace.workspace_repository };
    const auto& program = definition->action_program;

    insert_extended_state(state.get_state_builder(),
                          *definition->task->get_repository(),
                          program.get_translation_context().p2d,
                          merge_context,
                          evaluator.workspace.facts.fact_sets,
                          evaluator.workspace.facts.assignment_sets);

    auto ctx = d::ProgramExecutionContext(evaluator.workspace);
    evaluator.execution_context->arena().execute([&] { d::solve_bottom_up(ctx); });
}

template<typename Callback>
void SuccessorGenerator<LiftedTag>::Impl::for_each_applicable_action_binding(const Node<LiftedTag>& node,
                                                                             ygg::Data<f::RelationBinding<fp::Action>>& scratch_binding,
                                                                             Callback&& callback)
{
    compute_action_facts(node);

    const auto state_context = StateContext<LiftedTag>(*definition->task, node.get_state().get_state_builder(), node.get_metric());
    auto grounder_context = fp::GrounderContext { evaluator.workspace.planning_builder, *definition->task->get_repository(), scratch_binding.objects };
    const auto& mapping = definition->action_program.get_predicate_to_action_mapping();

    for (const auto& set : evaluator.workspace.facts.fact_sets.predicate.get_sets())
    {
        for (const auto& binding : set.get_bindings())
        {
            const auto it = mapping.find(binding.get_relation());
            if (it == mapping.end())
                continue;

            scratch_binding.relation = it->second.get_index();
            scratch_binding.objects.clear();
            for (const auto object : binding.get_objects())
                scratch_binding.objects.push_back(object.get_index());

            assert(is_applicable(it->second.get_condition(), ApplicabilityContext { state_context, grounder_context, *definition->task->get_fdr_context() })
                   && "ApplicableActionProgram emitted an action binding whose condition is not satisfied.");

            // Datalog certifies the action condition, not whether its grounded numeric effects are valid and mutually compatible.
            if (!evaluator.executor.is_applicable_if_fires(it->second, state_context, grounder_context, *definition->task->get_fdr_context()))
                continue;

            assert(evaluator.executor.is_applicable(it->second, state_context, grounder_context, *definition->task->get_fdr_context()));
            callback(scratch_binding);
        }
    }
}

SuccessorGenerator<LiftedTag>::SuccessorGenerator(ygg::uint_t index,
                                                  TaskPtr<LiftedTag> task,
                                                  ygg::ExecutionContextPtr execution_context,
                                                  StateRepositoryPtr<LiftedTag> state_repository,
                                                  std::shared_ptr<std::atomic<ygg::uint_t>> next_index) :
    m_impl(std::make_unique<Impl>(index, std::move(task), std::move(execution_context), std::move(state_repository), std::move(next_index)))
{
}

SuccessorGenerator<LiftedTag>::SuccessorGenerator(std::unique_ptr<Impl> impl) noexcept : m_impl(std::move(impl)) {}

SuccessorGenerator<LiftedTag>::~SuccessorGenerator() = default;
SuccessorGenerator<LiftedTag>::SuccessorGenerator(SuccessorGenerator&&) noexcept = default;
SuccessorGenerator<LiftedTag>& SuccessorGenerator<LiftedTag>::operator=(SuccessorGenerator&&) noexcept = default;

SuccessorGeneratorPtr<LiftedTag> SuccessorGenerator<LiftedTag>::make_worker(ygg::ExecutionContextPtr execution_context) const
{
    auto state_repository = m_impl->evaluator.state_repository->make_worker(execution_context);
    return SuccessorGeneratorPtr<LiftedTag>(
        new SuccessorGenerator<LiftedTag>(std::make_unique<Impl>(m_impl->next_index->fetch_add(1, std::memory_order_relaxed),
                                                                 m_impl->definition,
                                                                 std::move(execution_context),
                                                                 std::move(state_repository),
                                                                 m_impl->next_index)));
}

Node<LiftedTag> SuccessorGenerator<LiftedTag>::get_initial_node()
{
    auto initial_state = m_impl->evaluator.state_repository->get_initial_state();
    const auto state_context = StateContext<LiftedTag>(*m_impl->definition->task, initial_state.get_state_builder(), 0);
    const auto state_metric =
        evaluate_metric(m_impl->definition->task->get_task().get_metric(), m_impl->definition->task->get_task().get_auxiliary_fterm_value(), state_context);
    return Node<LiftedTag>(std::move(initial_state), state_metric);
}

NodeList<LiftedTag> SuccessorGenerator<LiftedTag>::get_successor_nodes(const Node<LiftedTag>& node)
{
    auto result = NodeList<LiftedTag> {};
    get_successor_nodes(node, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_successor_nodes(const Node<LiftedTag>& node, NodeList<LiftedTag>& out_nodes)
{
    out_nodes.clear();

    m_impl->for_each_applicable_action_binding(node,
                                               m_impl->evaluator.scratch_action_binding,
                                               [&](const auto& binding) { out_nodes.emplace_back(get_successor_node(node, binding)); });
}

LabeledNodeList<LiftedTag> SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes(const Node<LiftedTag>& node)
{
    auto result = LabeledNodeList<LiftedTag> {};
    get_labeled_successor_nodes(node, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_labeled_successor_nodes(const Node<LiftedTag>& node, LabeledNodeList<LiftedTag>& out_nodes)
{
    out_nodes.clear();

    m_impl->for_each_applicable_action_binding(node,
                                               m_impl->evaluator.scratch_action_binding,
                                               [&](const auto& binding)
                                               {
                                                   const auto action_binding = m_impl->definition->task->get_repository()->get_or_create(binding).first;
                                                   out_nodes.emplace_back(action_binding, get_successor_node(node, binding));
                                               });
}

Node<LiftedTag> SuccessorGenerator<LiftedTag>::get_successor_node(const Node<LiftedTag>& node, fp::GroundActionView action)
{
    const auto& state = node.get_state();
    const auto state_context = StateContext<LiftedTag>(*m_impl->definition->task, state.get_state_builder(), node.get_metric());
    return m_impl->evaluator.executor.apply_action(state_context, action, *m_impl->evaluator.state_repository);
}

fp::GroundActionView SuccessorGenerator<LiftedTag>::ground_action(fp::ActionBindingView binding)
{
    if (const auto it = m_impl->evaluator.action_binding_to_ground_action.find(binding); it != m_impl->evaluator.action_binding_to_ground_action.end())
        return it->second;

    m_impl->evaluator.workspace.binding.clear();
    for (const auto object : binding.get_data())
        m_impl->evaluator.workspace.binding.push_back(object);

    auto grounder_context =
        fp::GrounderContext { m_impl->evaluator.workspace.planning_builder, *m_impl->definition->task->get_repository(), m_impl->evaluator.workspace.binding };
    const auto action = binding.get_relation();
    const auto ground_action = fp::ground(action,
                                          grounder_context,
                                          m_impl->definition->task->get_formalism_task().get_variable_domains().action_domains.at(action.get_index()),
                                          m_impl->evaluator.cartesian_workspace,
                                          *m_impl->definition->task->get_fdr_context())
                                   .first;
    m_impl->evaluator.action_binding_to_ground_action.emplace(binding, ground_action);
    return ground_action;
}

// Action binding API (interning)
Node<LiftedTag> SuccessorGenerator<LiftedTag>::get_successor_node(const Node<LiftedTag>& node, ::tyr::formalism::planning::ActionBindingView binding)
{
    m_impl->evaluator.scratch_action_binding.relation = binding.get_relation().get_index();
    m_impl->evaluator.scratch_action_binding.objects.clear();
    for (const auto object : binding.get_data())
        m_impl->evaluator.scratch_action_binding.objects.push_back(object);

    return get_successor_node(node, m_impl->evaluator.scratch_action_binding);
}

std::vector<::tyr::formalism::planning::ActionBindingView> SuccessorGenerator<LiftedTag>::get_applicable_action_bindings(const Node<LiftedTag>& node)
{
    auto result = std::vector<::tyr::formalism::planning::ActionBindingView> {};
    get_applicable_action_bindings(node, result);
    return result;
}

void SuccessorGenerator<LiftedTag>::get_applicable_action_bindings(const Node<LiftedTag>& node,
                                                                   std::vector<::tyr::formalism::planning::ActionBindingView>& out_bindings)
{
    out_bindings.clear();

    m_impl->for_each_applicable_action_binding(node,
                                               m_impl->evaluator.scratch_action_binding,
                                               [&](const auto& binding)
                                               { out_bindings.emplace_back(m_impl->definition->task->get_repository()->get_or_create(binding).first); });
}

// Action binding API (no interning)
Node<LiftedTag>
SuccessorGenerator<LiftedTag>::get_successor_node(const Node<LiftedTag>& node,
                                                  const ygg::Data<::tyr::formalism::RelationBinding<::tyr::formalism::planning::Action>>& binding)
{
    m_impl->evaluator.workspace.binding.clear();
    for (const auto object : binding.objects)
        m_impl->evaluator.workspace.binding.push_back(object);

    auto grounder_context =
        fp::GrounderContext { m_impl->evaluator.workspace.planning_builder, *m_impl->definition->task->get_repository(), m_impl->evaluator.workspace.binding };
    const auto state_context = StateContext<LiftedTag>(*m_impl->definition->task, node.get_state().get_state_builder(), node.get_metric());
    const auto action = ygg::make_view(binding.relation, *m_impl->definition->task->get_repository());

    return m_impl->evaluator.executor.apply_action(state_context,
                                                   action,
                                                   grounder_context,
                                                   *m_impl->definition->task->get_fdr_context(),
                                                   *m_impl->evaluator.state_repository);
}

// Lookup
Node<LiftedTag> SuccessorGenerator<LiftedTag>::get_node(ygg::Index<State<LiftedTag>> state_index)
{
    auto state = m_impl->evaluator.state_repository->get_registered_state(state_index);
    const auto state_context = StateContext<LiftedTag>(*m_impl->definition->task, state.get_state_builder(), 0);
    const auto state_metric =
        evaluate_metric(m_impl->definition->task->get_task().get_metric(), m_impl->definition->task->get_task().get_auxiliary_fterm_value(), state_context);
    return Node<LiftedTag>(std::move(state), state_metric);
}

const ApplicableActionProgram<LiftedTag>& SuccessorGenerator<LiftedTag>::get_action_program() const noexcept { return m_impl->definition->action_program; }

const StateRepositoryPtr<LiftedTag>& SuccessorGenerator<LiftedTag>::get_state_repository() const noexcept { return m_impl->evaluator.state_repository; }

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

static_assert(SuccessorGeneratorConcept<SuccessorGenerator<LiftedTag>, LiftedTag>);
}
