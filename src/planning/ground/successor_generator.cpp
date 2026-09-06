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
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/action_executor.hpp"
#include "tyr/planning/applicability.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/axiom_evaluator.hpp"
#include "tyr/planning/ground/match_tree/match_tree.hpp"
#include "tyr/planning/ground/state_builder.hpp"
#include "tyr/planning/ground/state_repository.hpp"
#include "tyr/planning/ground/state_view.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/state_index.hpp"
#include "tyr/planning/task_utils.hpp"

#include <cassert>
#include <stdexcept>
#include <yggdrasil/containers/associative_containers.hpp>

namespace fp = tyr::formalism::planning;

namespace tyr::planning
{
namespace
{
void validate_task(const TaskPtr<GroundTag>& task, const StateRepository<GroundTag>& state_repository)
{
    if (state_repository.get_task() != task)
        throw std::invalid_argument("SuccessorGenerator: state repository belongs to a different task.");
}

void validate_task(const TaskPtr<GroundTag>& task, const AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    if (axiom_evaluator.get_task() != task)
        throw std::invalid_argument("SuccessorGenerator: axiom evaluator belongs to a different task.");
}

void validate_task(const TaskPtr<GroundTag>& task, const StateView<GroundTag>& state)
{
    if (state.get_state_repository()->get_task() != task)
        throw std::invalid_argument("SuccessorGenerator: state belongs to a different task.");
}
}

struct SuccessorGenerator<GroundTag>::Impl
{
    using ActionBindingMap = ygg::UnorderedMap<fp::ActionBindingView, fp::ActionView<GroundTag>>;

    struct Definition
    {
        explicit Definition(TaskPtr<GroundTag> task);

        TaskPtr<GroundTag> task;
        match_tree::MatchTreePtr<fp::Action<GroundTag>> action_match_tree_prototype;
        ActionBindingMap action_binding_to_ground_action;
    };

    struct Evaluator
    {
        explicit Evaluator(const Definition& definition);

        match_tree::MatchTreePtr<fp::Action<GroundTag>> action_match_tree;
        fp::ActionViewList<GroundTag> applicable_actions;
        ActionExecutor executor;
    };

    Impl(ygg::uint_t index, TaskPtr<GroundTag> task, std::shared_ptr<std::atomic<ygg::uint_t>> next_index) :
        index(index),
        next_index(std::move(next_index)),
        definition(std::make_shared<Definition>(std::move(task))),
        evaluator(*definition)
    {
    }

    Impl(ygg::uint_t index, std::shared_ptr<const Definition> definition, std::shared_ptr<std::atomic<ygg::uint_t>> next_index) :
        index(index),
        next_index(std::move(next_index)),
        definition(std::move(definition)),
        evaluator(*this->definition)
    {
    }

    ygg::uint_t index;
    std::shared_ptr<std::atomic<ygg::uint_t>> next_index;
    std::shared_ptr<const Definition> definition;
    Evaluator evaluator;
};

SuccessorGenerator<GroundTag>::Impl::Definition::Definition(TaskPtr<GroundTag> task_) :
    task(std::move(task_)),
    action_match_tree_prototype(match_tree::MatchTree<fp::Action<GroundTag>>::create(
        fp::ActionViewList<GroundTag>(task->get_task().get_ground_actions().begin(), task->get_task().get_ground_actions().end()),
        task->get_task().get_context())),
    action_binding_to_ground_action()
{
    for (const auto action : task->get_task().get_ground_actions())
        action_binding_to_ground_action.emplace(action.get_row(), action);
}

SuccessorGenerator<GroundTag>::Impl::Evaluator::Evaluator(const Definition& definition) :
    action_match_tree(definition.action_match_tree_prototype->make_worker()),
    applicable_actions(),
    executor()
{
}

SuccessorGenerator<GroundTag>::SuccessorGenerator(ygg::uint_t index,
                                                  TaskPtr<GroundTag> task,
                                                  ygg::ExecutionContextPtr execution_context,
                                                  std::shared_ptr<std::atomic<ygg::uint_t>> next_index) :
    m_impl(std::make_unique<Impl>(index, std::move(task), std::move(next_index)))
{
    static_cast<void>(execution_context);
}

SuccessorGenerator<GroundTag>::SuccessorGenerator(std::unique_ptr<Impl> impl) noexcept : m_impl(std::move(impl)) {}

SuccessorGenerator<GroundTag>::~SuccessorGenerator() = default;
SuccessorGenerator<GroundTag>::SuccessorGenerator(SuccessorGenerator&&) noexcept = default;
SuccessorGenerator<GroundTag>& SuccessorGenerator<GroundTag>::operator=(SuccessorGenerator&&) noexcept = default;

SuccessorGeneratorPtr<GroundTag> SuccessorGenerator<GroundTag>::make_worker(ygg::ExecutionContextPtr execution_context) const
{
    static_cast<void>(execution_context);
    return SuccessorGeneratorPtr<GroundTag>(new SuccessorGenerator<GroundTag>(
        std::make_unique<Impl>(m_impl->next_index->fetch_add(1, std::memory_order_relaxed), m_impl->definition, m_impl->next_index)));
}

Node<GroundTag> SuccessorGenerator<GroundTag>::get_initial_node(StateRepository<GroundTag>& state_repository, AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    validate_task(m_impl->definition->task, state_repository);
    validate_task(m_impl->definition->task, axiom_evaluator);
    auto initial_state = state_repository.get_initial_state(axiom_evaluator);

    const auto state_context = StateContext<GroundTag>(*m_impl->definition->task, initial_state.get_state_builder(), 0);

    const auto state_metric =
        evaluate_metric(m_impl->definition->task->get_task().get_metric(), m_impl->definition->task->get_task().get_auxiliary_fterm_value(), state_context);

    return Node<GroundTag>(std::move(initial_state), state_metric);
}

NodeList<GroundTag> SuccessorGenerator<GroundTag>::get_successor_nodes(const Node<GroundTag>& node,
                                                                       StateRepository<GroundTag>& state_repository,
                                                                       AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    auto result = NodeList<GroundTag> {};

    get_successor_nodes(node, state_repository, axiom_evaluator, result);

    return result;
}

void SuccessorGenerator<GroundTag>::get_successor_nodes(const Node<GroundTag>& node,
                                                        StateRepository<GroundTag>& state_repository,
                                                        AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                        NodeList<GroundTag>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, state_repository);
    validate_task(m_impl->definition->task, axiom_evaluator);
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
        out_nodes.emplace_back(get_successor_node(node, ground_action, state_repository, axiom_evaluator));
    }
}

LabeledNodeList<GroundTag> SuccessorGenerator<GroundTag>::get_labeled_successor_nodes(const Node<GroundTag>& node,
                                                                                      StateRepository<GroundTag>& state_repository,
                                                                                      AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    auto result = LabeledNodeList<GroundTag> {};

    get_labeled_successor_nodes(node, state_repository, axiom_evaluator, result);

    return result;
}

void SuccessorGenerator<GroundTag>::get_labeled_successor_nodes(const Node<GroundTag>& node,
                                                                StateRepository<GroundTag>& state_repository,
                                                                AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                LabeledNodeList<GroundTag>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, state_repository);
    validate_task(m_impl->definition->task, axiom_evaluator);
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
        out_nodes.emplace_back(ground_action.get_row(), get_successor_node(node, ground_action, state_repository, axiom_evaluator));
    }
}

Node<GroundTag> SuccessorGenerator<GroundTag>::get_successor_node(const Node<GroundTag>& node,
                                                                  fp::ActionBindingView binding,
                                                                  StateRepository<GroundTag>& state_repository,
                                                                  AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    return get_successor_node(node, ground_action(binding), state_repository, axiom_evaluator);
}

Node<GroundTag> SuccessorGenerator<GroundTag>::get_successor_node(const Node<GroundTag>& node,
                                                                  fp::ActionView<GroundTag> action,
                                                                  StateRepository<GroundTag>& state_repository,
                                                                  AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, state_repository);
    validate_task(m_impl->definition->task, axiom_evaluator);
    const auto& state = node.get_state();
    const auto state_context = StateContext<GroundTag>(*m_impl->definition->task, state.get_state_builder(), node.get_metric());
    auto successor_state = state_repository.get_state_builder();
    const auto result = PendingActionResult { m_impl->evaluator.executor.apply_action_unregistered(state_context, action, *successor_state) };
    return finalize_successor_state(state_repository, axiom_evaluator, std::move(successor_state), result);
}

std::vector<fp::ActionBindingView> SuccessorGenerator<GroundTag>::get_applicable_action_bindings(const Node<GroundTag>& node)
{
    auto result = std::vector<fp::ActionBindingView> {};
    get_applicable_action_bindings(node, result);
    return result;
}

void SuccessorGenerator<GroundTag>::get_applicable_action_bindings(const Node<GroundTag>& node, std::vector<fp::ActionBindingView>& out_bindings)
{
    validate_task(m_impl->definition->task, node.get_state());
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
    validate_task(m_impl->definition->task, node.get_state());
    const auto state_context = StateContext<GroundTag>(*m_impl->definition->task, node.get_state().get_state_builder(), node.get_metric());
    return PendingActionResult { m_impl->evaluator.executor.apply_action_unregistered(state_context, ground_action(binding), out_state) };
}

Node<GroundTag> SuccessorGenerator<GroundTag>::finalize_successor_state(StateRepository<GroundTag>& state_repository,
                                                                        AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                        ygg::SharedObjectPoolPtr<ygg::Builder<State<GroundTag>>, true> state,
                                                                        PendingActionResult result)
{
    validate_task(m_impl->definition->task, state_repository);
    validate_task(m_impl->definition->task, axiom_evaluator);
    const auto metric = complete_successor_state(*m_impl->definition->task, axiom_evaluator, *state, result.auxiliary_value);
    return Node<GroundTag>(state_repository.register_extended_state(std::move(state)), metric);
}

fp::ActionView<GroundTag> SuccessorGenerator<GroundTag>::ground_action(fp::ActionBindingView binding) const
{
    const auto it = m_impl->definition->action_binding_to_ground_action.find(binding);
    assert(it != m_impl->definition->action_binding_to_ground_action.end() && "Ground action binding not found.");
    return it->second;
}

Node<GroundTag> SuccessorGenerator<GroundTag>::get_node(StateRepository<GroundTag>& state_repository, ygg::Index<State<GroundTag>> state_index)
{
    validate_task(m_impl->definition->task, state_repository);
    auto state = state_repository.get_registered_state(state_index);
    const auto state_context = StateContext<GroundTag>(*m_impl->definition->task, state.get_state_builder(), 0);
    const auto state_metric =
        evaluate_metric(m_impl->definition->task->get_task().get_metric(), m_impl->definition->task->get_task().get_auxiliary_fterm_value(), state_context);

    return Node<GroundTag>(std::move(state), state_metric);
}

const TaskPtr<GroundTag>& SuccessorGenerator<GroundTag>::get_task() const noexcept { return m_impl->definition->task; }

ygg::uint_t SuccessorGenerator<GroundTag>::get_index() const noexcept { return m_impl->index; }

static_assert(SuccessorGeneratorConcept<SuccessorGenerator<GroundTag>, GroundTag>);

}
