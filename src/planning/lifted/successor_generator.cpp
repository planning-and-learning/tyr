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

namespace d = tyr::datalog;
namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;
namespace df = tyr::formalism::datalog;

namespace tyr::planning
{
SuccessorGenerator<LiftedTag>::SuccessorGenerator(ygg::uint_t index,
                                                  TaskPtr<LiftedTag> task,
                                                  ygg::ExecutionContextPtr execution_context,
                                                  StateRepositoryPtr<LiftedTag> state_repository) :
    m_index(index),
    m_task(std::move(task)),
    m_execution_context(std::move(execution_context)),
    m_action_program(m_task->get_task()),
    m_action_binding_to_ground_action(),
    m_workspace(m_action_program.get_datalog_program()),
    m_state_repository(std::move(state_repository)),
    m_executor()
{
    assert(m_execution_context);
}

Node<LiftedTag> SuccessorGenerator<LiftedTag>::get_initial_node()
{
    auto initial_state = m_state_repository->get_initial_state();
    const auto state_context = StateContext<LiftedTag>(*m_task, initial_state.get_state_builder(), 0);
    const auto state_metric = evaluate_metric(m_task->get_task().get_metric(), m_task->get_task().get_auxiliary_fterm_value(), state_context);
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

    for_each_applicable_action_binding(node, m_scratch_action_binding, [&](const auto& binding) { out_nodes.emplace_back(get_successor_node(node, binding)); });
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

    for_each_applicable_action_binding(node,
                                       m_scratch_action_binding,
                                       [&](const auto& binding)
                                       {
                                           const auto action_binding = m_task->get_repository()->get_or_create(binding).first;
                                           out_nodes.emplace_back(action_binding, get_successor_node(node, binding));
                                       });
}

Node<LiftedTag> SuccessorGenerator<LiftedTag>::get_successor_node(const Node<LiftedTag>& node, fp::GroundActionView action)
{
    const auto& state = node.get_state();
    const auto state_context = StateContext<LiftedTag>(*m_task, state.get_state_builder(), node.get_metric());
    return m_executor.apply_action(state_context, action, *m_state_repository);
}

fp::GroundActionView SuccessorGenerator<LiftedTag>::ground_action(fp::ActionBindingView binding)
{
    if (const auto it = m_action_binding_to_ground_action.find(binding); it != m_action_binding_to_ground_action.end())
        return it->second;

    m_workspace.binding.clear();
    for (const auto object : binding.get_data())
        m_workspace.binding.push_back(object);

    auto grounder_context = fp::GrounderContext { m_workspace.planning_builder, *m_task->get_repository(), m_workspace.binding };
    const auto action = binding.get_relation();
    const auto ground_action = fp::ground(action,
                                          grounder_context,
                                          m_task->get_formalism_task().get_variable_domains().action_domains.at(action.get_index()),
                                          m_cartesian_workspace,
                                          *m_task->get_fdr_context())
                                   .first;
    m_action_binding_to_ground_action.emplace(binding, ground_action);
    return ground_action;
}

// Action binding API (interning)
Node<LiftedTag> SuccessorGenerator<LiftedTag>::get_successor_node(const Node<LiftedTag>& node, ::tyr::formalism::planning::ActionBindingView binding)
{
    m_scratch_action_binding.relation = binding.get_relation().get_index();
    m_scratch_action_binding.objects.clear();
    for (const auto object : binding.get_data())
        m_scratch_action_binding.objects.push_back(object);

    return get_successor_node(node, m_scratch_action_binding);
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

    for_each_applicable_action_binding(node,
                                       m_scratch_action_binding,
                                       [&](const auto& binding) { out_bindings.emplace_back(m_task->get_repository()->get_or_create(binding).first); });
}

// Action binding API (no interning)
Node<LiftedTag>
SuccessorGenerator<LiftedTag>::get_successor_node(const Node<LiftedTag>& node,
                                                  const ygg::Data<::tyr::formalism::RelationBinding<::tyr::formalism::planning::Action>>& binding)
{
    m_workspace.binding.clear();
    for (const auto object : binding.objects)
        m_workspace.binding.push_back(object);

    auto grounder_context = fp::GrounderContext { m_workspace.planning_builder, *m_task->get_repository(), m_workspace.binding };
    const auto state_context = StateContext<LiftedTag>(*m_task, node.get_state().get_state_builder(), node.get_metric());
    const auto action = ygg::make_view(binding.relation, *m_task->get_repository());

    return m_executor.apply_action(state_context, action, grounder_context, *m_task->get_fdr_context(), *m_state_repository);
}

void SuccessorGenerator<LiftedTag>::for_each_applicable_action_binding_impl(
    const Node<LiftedTag>& node,
    ygg::Data<::tyr::formalism::RelationBinding<::tyr::formalism::planning::Action>>& scratch_binding,
    ActionBindingCallback callback,
    void* callback_data)
{
    compute_action_facts(node);

    const auto state_context = StateContext<LiftedTag>(*m_task, node.get_state().get_state_builder(), node.get_metric());
    auto grounder_context = fp::GrounderContext { m_workspace.planning_builder, *m_task->get_repository(), scratch_binding.objects };
    const auto& mapping = m_action_program.get_predicate_to_action_mapping();

    for (const auto& set : m_workspace.facts.fact_sets.predicate.get_sets())
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

            assert(is_applicable(it->second.get_condition(), ApplicabilityContext { state_context, grounder_context, *m_task->get_fdr_context() })
                   && "ApplicableActionProgram emitted an action binding whose condition is not satisfied.");

            // Datalog certifies the action condition, not whether its grounded numeric effects are valid and mutually compatible.
            if (!m_executor.is_applicable_if_fires(it->second, state_context, grounder_context, *m_task->get_fdr_context()))
                continue;

            assert(m_executor.is_applicable(it->second, state_context, grounder_context, *m_task->get_fdr_context()));
            callback(scratch_binding, callback_data);
        }
    }
}

// Lookup
Node<LiftedTag> SuccessorGenerator<LiftedTag>::get_node(ygg::Index<State<LiftedTag>> state_index)
{
    auto state = m_state_repository->get_registered_state(state_index);
    const auto state_context = StateContext<LiftedTag>(*m_task, state.get_state_builder(), 0);
    const auto state_metric = evaluate_metric(m_task->get_task().get_metric(), m_task->get_task().get_auxiliary_fterm_value(), state_context);
    return Node<LiftedTag>(std::move(state), state_metric);
}

// Diagnostics
void SuccessorGenerator<LiftedTag>::print_summary(size_t verbosity) const
{
    if (verbosity < 1)
        return;

    std::cout << "[Successor generator] Summary" << std::endl;
    fmt::print(std::cout, "{}\n", m_workspace.statistics);
    auto successor_generator_rule_statistics = std::vector<datalog::RuleStatistics> {};
    for (const auto& ws_rule : m_workspace.template get_rules<f::PredicateTag>())
        successor_generator_rule_statistics.push_back(ws_rule->common.statistics);
    fmt::print(std::cout, "{}\n", datalog::compute_aggregated_rule_statistics(successor_generator_rule_statistics));
    auto successor_generator_rule_worker_statistics = std::vector<datalog::RuleWorkerStatistics> {};
    for (const auto& ws_rule : m_workspace.template get_rules<f::PredicateTag>())
        for (const auto& worker : ws_rule->worker)
            successor_generator_rule_worker_statistics.push_back(worker.solve.statistics);
    fmt::print(std::cout, "{}\n", datalog::compute_aggregated_rule_worker_statistics(successor_generator_rule_worker_statistics));
}

void SuccessorGenerator<LiftedTag>::compute_action_facts(const Node<LiftedTag>& node)
{
    m_workspace.reset_evaluation();

    const auto state = node.get_state();
    auto merge_context = fp::MergeDatalogContext { m_workspace.datalog_builder, m_workspace.workspace_repository };
    const auto& program = m_action_program;

    insert_extended_state(state.get_state_builder(),
                          *m_task->get_repository(),
                          program.get_translation_context().p2d,
                          merge_context,
                          m_workspace.facts.fact_sets,
                          m_workspace.facts.assignment_sets);

    auto ctx = d::ProgramExecutionContext(m_workspace);
    m_execution_context->arena().execute([&] { d::solve_bottom_up(ctx); });
}

static_assert(SuccessorGeneratorConcept<SuccessorGenerator<LiftedTag>, LiftedTag>);
}
