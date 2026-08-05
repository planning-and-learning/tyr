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
#include "tyr/planning/action_executor.hpp"
#include "tyr/planning/applicability.hpp"  // for StateC...
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
#include <yggdrasil/containers/associative_containers.hpp>

namespace fp = tyr::formalism::planning;

namespace tyr::planning
{

struct SuccessorGenerator<GroundTag>::Impl
{
    using ActionBindingMap = ygg::UnorderedMap<fp::ActionBindingView, fp::GroundActionView>;

    struct Definition
    {
        explicit Definition(TaskPtr<GroundTag> task);

        TaskPtr<GroundTag> task;
        match_tree::MatchTreePtr<fp::GroundAction> action_match_tree_prototype;
        ActionBindingMap action_binding_to_ground_action;
    };

    struct Evaluator
    {
        Evaluator(const Definition& definition, StateRepositoryPtr<GroundTag> state_repository);

        match_tree::MatchTreePtr<fp::GroundAction> action_match_tree;
        fp::GroundActionViewList applicable_actions;
        StateRepositoryPtr<GroundTag> state_repository;
        ActionExecutor executor;
    };

    Impl(ygg::uint_t index, TaskPtr<GroundTag> task, StateRepositoryPtr<GroundTag> state_repository, std::shared_ptr<std::atomic<ygg::uint_t>> next_index) :
        index(index),
        next_index(std::move(next_index)),
        definition(std::make_shared<Definition>(std::move(task))),
        evaluator(*definition, std::move(state_repository))
    {
    }

    Impl(ygg::uint_t index,
         std::shared_ptr<const Definition> definition,
         StateRepositoryPtr<GroundTag> state_repository,
         std::shared_ptr<std::atomic<ygg::uint_t>> next_index) :
        index(index),
        next_index(std::move(next_index)),
        definition(std::move(definition)),
        evaluator(*this->definition, std::move(state_repository))
    {
    }

    ygg::uint_t index;
    std::shared_ptr<std::atomic<ygg::uint_t>> next_index;
    std::shared_ptr<const Definition> definition;
    Evaluator evaluator;
};

SuccessorGenerator<GroundTag>::Impl::Definition::Definition(TaskPtr<GroundTag> task_) :
    task(std::move(task_)),
    action_match_tree_prototype(match_tree::MatchTree<fp::GroundAction>::create(
        fp::GroundActionViewList(task->get_task().get_ground_actions().begin(), task->get_task().get_ground_actions().end()),
        task->get_task().get_context())),
    action_binding_to_ground_action()
{
    for (const auto action : task->get_task().get_ground_actions())
        action_binding_to_ground_action.emplace(action.get_row(), action);
}

SuccessorGenerator<GroundTag>::Impl::Evaluator::Evaluator(const Definition& definition, StateRepositoryPtr<GroundTag> state_repository_) :
    action_match_tree(definition.action_match_tree_prototype->make_worker()),
    applicable_actions(),
    state_repository(std::move(state_repository_)),
    executor()
{
}

SuccessorGenerator<GroundTag>::SuccessorGenerator(ygg::uint_t index,
                                                  TaskPtr<GroundTag> task,
                                                  ygg::ExecutionContextPtr execution_context,
                                                  StateRepositoryPtr<GroundTag> state_repository,
                                                  std::shared_ptr<std::atomic<ygg::uint_t>> next_index) :
    m_impl(std::make_unique<Impl>(index, std::move(task), std::move(state_repository), std::move(next_index)))
{
    static_cast<void>(execution_context);
}

SuccessorGenerator<GroundTag>::SuccessorGenerator(std::unique_ptr<Impl> impl) noexcept : m_impl(std::move(impl)) {}

SuccessorGenerator<GroundTag>::~SuccessorGenerator() = default;
SuccessorGenerator<GroundTag>::SuccessorGenerator(SuccessorGenerator&&) noexcept = default;
SuccessorGenerator<GroundTag>& SuccessorGenerator<GroundTag>::operator=(SuccessorGenerator&&) noexcept = default;

SuccessorGeneratorPtr<GroundTag> SuccessorGenerator<GroundTag>::make_worker(ygg::ExecutionContextPtr execution_context) const
{
    auto state_repository = m_impl->evaluator.state_repository->make_worker(std::move(execution_context));
    return SuccessorGeneratorPtr<GroundTag>(
        new SuccessorGenerator<GroundTag>(std::make_unique<Impl>(m_impl->next_index->fetch_add(1, std::memory_order_relaxed),
                                                                 m_impl->definition,
                                                                 std::move(state_repository),
                                                                 m_impl->next_index)));
}

std::vector<SuccessorGeneratorPtr<GroundTag>>
SuccessorGenerator<GroundTag>::make_shared_workers(std::span<const ygg::ExecutionContextPtr> execution_contexts) const
{
    auto state_repositories = m_impl->evaluator.state_repository->make_shared_workers(execution_contexts);
    auto workers = std::vector<SuccessorGeneratorPtr<GroundTag>> {};
    workers.reserve(state_repositories.size());
    for (auto& state_repository : state_repositories)
    {
        workers.emplace_back(new SuccessorGenerator<GroundTag>(std::make_unique<Impl>(m_impl->next_index->fetch_add(1, std::memory_order_relaxed),
                                                                                      m_impl->definition,
                                                                                      std::move(state_repository),
                                                                                      m_impl->next_index)));
    }
    return workers;
}

Node<GroundTag> SuccessorGenerator<GroundTag>::get_initial_node()
{
    auto initial_state = m_impl->evaluator.state_repository->get_initial_state();

    const auto state_context = StateContext<GroundTag>(*m_impl->definition->task, initial_state.get_state_builder(), 0);

    const auto state_metric =
        evaluate_metric(m_impl->definition->task->get_task().get_metric(), m_impl->definition->task->get_task().get_auxiliary_fterm_value(), state_context);

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

    const auto state_context = StateContext<GroundTag>(*m_impl->definition->task, state.get_state_builder(), node.get_metric());

    m_impl->evaluator.action_match_tree->generate(state_context, m_impl->evaluator.applicable_actions);

    for (const auto ground_action : m_impl->evaluator.applicable_actions)
    {
        assert(is_applicable(ground_action.get_condition(), state_context));

        if (!m_impl->evaluator.executor.is_applicable_if_fires(ground_action, state_context))
            continue;

        assert(m_impl->evaluator.executor.is_applicable(ground_action, state_context));
        out_nodes.emplace_back(m_impl->evaluator.executor.apply_action(state_context, ground_action, *m_impl->evaluator.state_repository));
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

    const auto state_context = StateContext<GroundTag>(*m_impl->definition->task, state.get_state_builder(), node.get_metric());

    m_impl->evaluator.action_match_tree->generate(state_context, m_impl->evaluator.applicable_actions);

    for (const auto ground_action : m_impl->evaluator.applicable_actions)
    {
        assert(is_applicable(ground_action.get_condition(), state_context));

        if (!m_impl->evaluator.executor.is_applicable_if_fires(ground_action, state_context))
            continue;

        assert(m_impl->evaluator.executor.is_applicable(ground_action, state_context));
        out_nodes.emplace_back(ground_action.get_row(),
                               m_impl->evaluator.executor.apply_action(state_context, ground_action, *m_impl->evaluator.state_repository));
    }
}

Node<GroundTag> SuccessorGenerator<GroundTag>::get_successor_node(const Node<GroundTag>& node, fp::ActionBindingView binding)
{
    return get_successor_node(node, ground_action(binding));
}

Node<GroundTag> SuccessorGenerator<GroundTag>::get_successor_node(const Node<GroundTag>& node, fp::GroundActionView action)
{
    const auto& state = node.get_state();
    const auto state_context = StateContext<GroundTag>(*m_impl->definition->task, state.get_state_builder(), node.get_metric());

    return m_impl->evaluator.executor.apply_action(state_context, action, *m_impl->evaluator.state_repository);
}

std::vector<fp::ActionBindingView> SuccessorGenerator<GroundTag>::get_applicable_action_bindings(const Node<GroundTag>& node)
{
    auto result = std::vector<fp::ActionBindingView> {};
    get_applicable_action_bindings(node, result);
    return result;
}

void SuccessorGenerator<GroundTag>::get_applicable_action_bindings(const Node<GroundTag>& node, std::vector<fp::ActionBindingView>& out_bindings)
{
    out_bindings.clear();

    const auto state_context = StateContext<GroundTag>(*m_impl->definition->task, node.get_state().get_state_builder(), node.get_metric());
    m_impl->evaluator.action_match_tree->generate(state_context, m_impl->evaluator.applicable_actions);

    for (const auto action : m_impl->evaluator.applicable_actions)
    {
        assert(is_applicable(action.get_condition(), state_context));

        if (!m_impl->evaluator.executor.is_applicable_if_fires(action, state_context))
            continue;

        assert(m_impl->evaluator.executor.is_applicable(action, state_context));
        out_bindings.push_back(action.get_row());
    }
}

PendingActionResult
SuccessorGenerator<GroundTag>::generate_successor_state(const Node<GroundTag>& node, fp::ActionBindingView binding, ygg::Builder<State<GroundTag>>& out_state)
{
    const auto state_context = StateContext<GroundTag>(*m_impl->definition->task, node.get_state().get_state_builder(), node.get_metric());
    return PendingActionResult { m_impl->evaluator.executor.apply_action_unregistered(state_context, ground_action(binding), out_state) };
}

Node<GroundTag> SuccessorGenerator<GroundTag>::finalize_successor_state(ygg::SharedObjectPoolPtr<ygg::Builder<State<GroundTag>>, true> state,
                                                                        PendingActionResult result)
{
    return m_impl->evaluator.executor.finalize_action(*m_impl->evaluator.state_repository, std::move(state), result.auxiliary_value);
}

fp::GroundActionView SuccessorGenerator<GroundTag>::ground_action(fp::ActionBindingView binding) const
{
    const auto it = m_impl->definition->action_binding_to_ground_action.find(binding);
    assert(it != m_impl->definition->action_binding_to_ground_action.end() && "Ground action binding not found.");
    return it->second;
}

Node<GroundTag> SuccessorGenerator<GroundTag>::get_node(ygg::Index<State<GroundTag>> state_index)
{
    auto state = m_impl->evaluator.state_repository->get_registered_state(state_index);
    const auto state_context = StateContext<GroundTag>(*m_impl->definition->task, state.get_state_builder(), 0);
    const auto state_metric =
        evaluate_metric(m_impl->definition->task->get_task().get_metric(), m_impl->definition->task->get_task().get_auxiliary_fterm_value(), state_context);

    return Node<GroundTag>(std::move(state), state_metric);
}

const StateRepositoryPtr<GroundTag>& SuccessorGenerator<GroundTag>::get_state_repository() const noexcept { return m_impl->evaluator.state_repository; }

ygg::uint_t SuccessorGenerator<GroundTag>::get_index() const noexcept { return m_impl->index; }

static_assert(SuccessorGeneratorConcept<SuccessorGenerator<GroundTag>, GroundTag>);

}
