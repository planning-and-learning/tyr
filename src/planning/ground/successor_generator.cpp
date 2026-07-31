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

#include "tyr/planning/ground/successor_generator.hpp"

#include "../metric.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"  // for ygg::View
#include "tyr/planning/applicability.hpp"    // for StateC...
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/match_tree/match_tree.hpp"
#include "tyr/planning/ground/state_builder.hpp"
#include "tyr/planning/ground/state_repository.hpp"
#include "tyr/planning/ground/state_view.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/state_index.hpp"
#include "tyr/planning/task_utils.hpp"

#include <cassert>

namespace fp = tyr::formalism::planning;

namespace tyr::planning
{

SuccessorGenerator<GroundTag>::SuccessorGenerator(ygg::uint_t index,
                                                  TaskPtr<GroundTag> task,
                                                  ygg::ExecutionContextPtr execution_context,
                                                  StateRepositoryPtr<GroundTag> state_repository) :
    m_index(index),
    m_task(std::move(task)),
    m_action_match_tree(match_tree::MatchTree<fp::GroundAction>::create(
        fp::GroundActionViewList(m_task->get_task().get_ground_actions().begin(), m_task->get_task().get_ground_actions().end()),
        m_task->get_task().get_context())),
    m_action_binding_to_ground_action(),
    m_applicable_actions(),
    m_state_repository(std::move(state_repository)),
    m_executor()
{
    static_cast<void>(execution_context);
    for (const auto action : m_task->get_task().get_ground_actions())
        m_action_binding_to_ground_action.emplace(action.get_row(), action);
}

Node<GroundTag> SuccessorGenerator<GroundTag>::get_initial_node()
{
    auto initial_state = m_state_repository->get_initial_state();

    const auto state_context = StateContext<GroundTag>(*m_task, initial_state.get_state_builder(), 0);

    const auto state_metric = evaluate_metric(m_task->get_task().get_metric(), m_task->get_task().get_auxiliary_fterm_value(), state_context);

    return Node<GroundTag>(std::move(initial_state), state_metric);
}

NodeList<GroundTag> SuccessorGenerator<GroundTag>::get_successor_nodes(const Node<GroundTag>& node)
{
    auto result = NodeList<GroundTag> {};

    get_successor_nodes(node, result);

    return result;
}

void SuccessorGenerator<GroundTag>::get_successor_nodes(const Node<GroundTag>& node, NodeList<GroundTag>& out_nodes)
{
    out_nodes.clear();

    const auto state = node.get_state();

    const auto state_context = StateContext<GroundTag>(*m_task, state.get_state_builder(), node.get_metric());

    m_action_match_tree->generate(state_context, m_applicable_actions);

    for (const auto ground_action : m_applicable_actions)
    {
        assert(is_applicable(ground_action.get_condition(), state_context));

        if (!m_executor.is_applicable_if_fires(ground_action, state_context))
            continue;

        assert(m_executor.is_applicable(ground_action, state_context));
        out_nodes.emplace_back(m_executor.apply_action(state_context, ground_action, *m_state_repository));
    }
}

LabeledNodeList<GroundTag> SuccessorGenerator<GroundTag>::get_labeled_successor_nodes(const Node<GroundTag>& node)
{
    auto result = LabeledNodeList<GroundTag> {};

    get_labeled_successor_nodes(node, result);

    return result;
}

void SuccessorGenerator<GroundTag>::get_labeled_successor_nodes(const Node<GroundTag>& node, LabeledNodeList<GroundTag>& out_nodes)
{
    out_nodes.clear();

    const auto state = node.get_state();

    const auto state_context = StateContext<GroundTag>(*m_task, state.get_state_builder(), node.get_metric());

    m_action_match_tree->generate(state_context, m_applicable_actions);

    for (const auto ground_action : m_applicable_actions)
    {
        assert(is_applicable(ground_action.get_condition(), state_context));

        if (!m_executor.is_applicable_if_fires(ground_action, state_context))
            continue;

        assert(m_executor.is_applicable(ground_action, state_context));
        out_nodes.emplace_back(ground_action.get_row(), m_executor.apply_action(state_context, ground_action, *m_state_repository));
    }
}

Node<GroundTag> SuccessorGenerator<GroundTag>::get_successor_node(const Node<GroundTag>& node, fp::ActionBindingView binding)
{
    return get_successor_node(node, ground_action(binding));
}

Node<GroundTag> SuccessorGenerator<GroundTag>::get_successor_node(const Node<GroundTag>& node, fp::GroundActionView action)
{
    const auto& state = node.get_state();
    const auto state_context = StateContext<GroundTag>(*m_task, state.get_state_builder(), node.get_metric());

    return m_executor.apply_action(state_context, action, *m_state_repository);
}

fp::GroundActionView SuccessorGenerator<GroundTag>::ground_action(fp::ActionBindingView binding) const
{
    const auto it = m_action_binding_to_ground_action.find(binding);
    assert(it != m_action_binding_to_ground_action.end() && "Ground action binding not found.");
    return it->second;
}

Node<GroundTag> SuccessorGenerator<GroundTag>::get_node(ygg::Index<State<GroundTag>> state_index)
{
    auto state = m_state_repository->get_registered_state(state_index);
    const auto state_context = StateContext<GroundTag>(*m_task, state.get_state_builder(), 0);
    const auto state_metric = evaluate_metric(m_task->get_task().get_metric(), m_task->get_task().get_auxiliary_fterm_value(), state_context);

    return Node<GroundTag>(std::move(state), state_metric);
}

static_assert(SuccessorGeneratorConcept<SuccessorGenerator<GroundTag>, GroundTag>);

}
