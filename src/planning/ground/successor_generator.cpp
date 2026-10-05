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

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/formalism/membership.hpp>

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

template<StateViewConcept<GroundTag> S>
void validate_task(const TaskPtr<GroundTag>& task, const S& state)
{
    if (&state.get_task() != task.get())
        throw std::invalid_argument("SuccessorGenerator: state belongs to a different task.");
}
}

struct SuccessorGenerator<GroundTag>::Impl
{
    using ActionBindingMap = ygg::UnorderedMap<fp::ActionBindingView, fp::ActionView<GroundTag>>;
    using ActionMatchTrees = ygg::UnorderedMap<fp::ActionView<LiftedTag>, match_tree::MatchTreePtr<fp::Action<GroundTag>>>;

    struct Definition
    {
        explicit Definition(TaskPtr<GroundTag> task);

        TaskPtr<GroundTag> task;
        match_tree::MatchTreePtr<fp::Action<GroundTag>> action_match_tree_prototype;
        ActionMatchTrees schema_match_tree_prototypes;
        ActionBindingMap action_binding_to_ground_action;
    };

    struct Evaluator
    {
        explicit Evaluator(const Definition& definition);

        match_tree::MatchTreePtr<fp::Action<GroundTag>> action_match_tree;
        ActionMatchTrees schema_match_trees;
        fp::ActionViewList<GroundTag> applicable_actions;
        ActionExecutor executor;
        ygg::Data<formalism::RelationBinding<fp::Action<LiftedTag>>> checked_binding;
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

    match_tree::MatchTree<fp::Action<GroundTag>>& get_schema_match_tree(fp::ActionView<LiftedTag> action)
    {
        const auto it = evaluator.schema_match_trees.find(action);
        // Repository indices can collide across independent factories.
        if (it == evaluator.schema_match_trees.end() || &action.get_context() != &it->first.get_context())
            throw std::invalid_argument("SuccessorGenerator: action schema does not belong to the task domain.");
        return *it->second;
    }

    template<StateViewConcept<GroundTag> S, typename Objects>
    ActionBindingResult try_get_applicable_action_binding(const Node<GroundTag, S>& node, fp::ActionView<LiftedTag> action, Objects objects);

    template<StateViewConcept<GroundTag> S, typename Callback>
    bool for_each_applicable_action(const Node<GroundTag, S>& node, match_tree::MatchTree<fp::Action<GroundTag>>& tree, Callback&& callback)
    {
        const auto state_context = StateContext<GroundTag>(*definition->task, node.get_state().get_state_builder(), node.get_metric());
        tree.generate(state_context, evaluator.applicable_actions);
        for (const auto action : evaluator.applicable_actions)
        {
            assert(is_applicable(action.get_condition(), state_context));
            if (!evaluator.executor.is_applicable_if_fires(action, state_context))
                continue;
            assert(evaluator.executor.is_applicable(action, state_context));
            if (!callback(action))
                return false;
        }
        return true;
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
    auto schema_actions = ygg::UnorderedMap<fp::ActionView<LiftedTag>, fp::ActionViewList<GroundTag>> {};
    for (const auto action : task->get_task().get_domain().get_actions())
        schema_actions.try_emplace(action);
    for (const auto action : task->get_task().get_ground_actions())
    {
        action_binding_to_ground_action.emplace(action.get_row(), action);
        schema_actions.at(action.get_action()).push_back(action);
    }
    for (auto& [schema, actions] : schema_actions)
        schema_match_tree_prototypes.emplace(schema, match_tree::MatchTree<fp::Action<GroundTag>>::create(std::move(actions), task->get_task().get_context()));
}

SuccessorGenerator<GroundTag>::Impl::Evaluator::Evaluator(const Definition& definition) :
    action_match_tree(definition.action_match_tree_prototype->make_worker()),
    applicable_actions(),
    executor()
{
    for (const auto& [schema, tree] : definition.schema_match_tree_prototypes)
        schema_match_trees.emplace(schema, tree->make_worker());
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

template<StateViewConcept<GroundTag> S>
NodeList<GroundTag, S>
SuccessorGenerator<GroundTag>::get_successor_nodes(const Node<GroundTag, S>& node, SuccessorListStorage<S>& storage, AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    auto result = NodeList<GroundTag, S> {};

    get_successor_nodes(node, storage, axiom_evaluator, result);

    return result;
}

template<StateViewConcept<GroundTag> S>
void SuccessorGenerator<GroundTag>::get_successor_nodes(const Node<GroundTag, S>& node,
                                                        SuccessorListStorage<S>& storage,
                                                        AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                        NodeList<GroundTag, S>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    if constexpr (std::same_as<S, StateView<GroundTag>>)
        validate_task(m_impl->definition->task, storage);
    validate_task(m_impl->definition->task, axiom_evaluator);
    auto& tree = *m_impl->evaluator.action_match_tree;
    out_nodes.clear();
    m_impl->for_each_applicable_action(node,
                                       tree,
                                       [&](const auto ground_action)
                                       {
                                           if constexpr (std::same_as<S, StateView<GroundTag>>)
                                           {
                                               auto successor = get_successor_node(node, ground_action, storage, axiom_evaluator);
                                               out_nodes.push_back(std::move(successor));
                                           }
                                           else
                                           {
                                               auto successor = get_successor_node(node, ground_action, storage.emplace_back(), axiom_evaluator);
                                               out_nodes.push_back(std::move(successor));
                                           }
                                           return true;
                                       });
}

template<StateViewConcept<GroundTag> S>
NodeList<GroundTag, S> SuccessorGenerator<GroundTag>::get_successor_nodes(const Node<GroundTag, S>& node,
                                                                          fp::ActionView<LiftedTag> action,
                                                                          SuccessorListStorage<S>& storage,
                                                                          AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    auto result = NodeList<GroundTag, S> {};
    get_successor_nodes(node, action, storage, axiom_evaluator, result);
    return result;
}

template<StateViewConcept<GroundTag> S>
void SuccessorGenerator<GroundTag>::get_successor_nodes(const Node<GroundTag, S>& node,
                                                        fp::ActionView<LiftedTag> action,
                                                        SuccessorListStorage<S>& storage,
                                                        AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                        NodeList<GroundTag, S>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    if constexpr (std::same_as<S, StateView<GroundTag>>)
        validate_task(m_impl->definition->task, storage);
    validate_task(m_impl->definition->task, axiom_evaluator);
    auto& tree = m_impl->get_schema_match_tree(action);
    out_nodes.clear();
    m_impl->for_each_applicable_action(node,
                                       tree,
                                       [&](const auto ground_action)
                                       {
                                           if constexpr (std::same_as<S, StateView<GroundTag>>)
                                           {
                                               auto successor = get_successor_node(node, ground_action, storage, axiom_evaluator);
                                               out_nodes.push_back(std::move(successor));
                                           }
                                           else
                                           {
                                               auto successor = get_successor_node(node, ground_action, storage.emplace_back(), axiom_evaluator);
                                               out_nodes.push_back(std::move(successor));
                                           }
                                           return true;
                                       });
}

template<StateViewConcept<GroundTag> S>
LabeledNodeList<GroundTag, S> SuccessorGenerator<GroundTag>::get_labeled_successor_nodes(const Node<GroundTag, S>& node,
                                                                                         SuccessorListStorage<S>& storage,
                                                                                         AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    auto result = LabeledNodeList<GroundTag, S> {};

    get_labeled_successor_nodes(node, storage, axiom_evaluator, result);

    return result;
}

template<StateViewConcept<GroundTag> S>
void SuccessorGenerator<GroundTag>::get_labeled_successor_nodes(const Node<GroundTag, S>& node,
                                                                SuccessorListStorage<S>& storage,
                                                                AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                LabeledNodeList<GroundTag, S>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    if constexpr (std::same_as<S, StateView<GroundTag>>)
        validate_task(m_impl->definition->task, storage);
    validate_task(m_impl->definition->task, axiom_evaluator);
    auto& tree = *m_impl->evaluator.action_match_tree;
    out_nodes.clear();
    m_impl->for_each_applicable_action(node,
                                       tree,
                                       [&](const auto ground_action)
                                       {
                                           if constexpr (std::same_as<S, StateView<GroundTag>>)
                                           {
                                               auto successor = get_successor_node(node, ground_action, storage, axiom_evaluator);
                                               out_nodes.push_back(LabeledNode<GroundTag, S> { ground_action.get_row(), std::move(successor) });
                                           }
                                           else
                                           {
                                               auto successor = get_successor_node(node, ground_action, storage.emplace_back(), axiom_evaluator);
                                               out_nodes.push_back(LabeledNode<GroundTag, S> { ground_action.get_row(), std::move(successor) });
                                           }
                                           return true;
                                       });
}

template<StateViewConcept<GroundTag> S>
LabeledNodeList<GroundTag, S> SuccessorGenerator<GroundTag>::get_labeled_successor_nodes(const Node<GroundTag, S>& node,
                                                                                         fp::ActionView<LiftedTag> action,
                                                                                         SuccessorListStorage<S>& storage,
                                                                                         AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    auto result = LabeledNodeList<GroundTag, S> {};
    get_labeled_successor_nodes(node, action, storage, axiom_evaluator, result);
    return result;
}

template<StateViewConcept<GroundTag> S>
void SuccessorGenerator<GroundTag>::get_labeled_successor_nodes(const Node<GroundTag, S>& node,
                                                                fp::ActionView<LiftedTag> action,
                                                                SuccessorListStorage<S>& storage,
                                                                AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                LabeledNodeList<GroundTag, S>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    if constexpr (std::same_as<S, StateView<GroundTag>>)
        validate_task(m_impl->definition->task, storage);
    validate_task(m_impl->definition->task, axiom_evaluator);
    auto& tree = m_impl->get_schema_match_tree(action);
    out_nodes.clear();
    m_impl->for_each_applicable_action(node,
                                       tree,
                                       [&](const auto ground_action)
                                       {
                                           if constexpr (std::same_as<S, StateView<GroundTag>>)
                                           {
                                               auto successor = get_successor_node(node, ground_action, storage, axiom_evaluator);
                                               out_nodes.push_back(LabeledNode<GroundTag, S> { ground_action.get_row(), std::move(successor) });
                                           }
                                           else
                                           {
                                               auto successor = get_successor_node(node, ground_action, storage.emplace_back(), axiom_evaluator);
                                               out_nodes.push_back(LabeledNode<GroundTag, S> { ground_action.get_row(), std::move(successor) });
                                           }
                                           return true;
                                       });
}

template<StateViewConcept<GroundTag> S>
ActionBindingStatus
SuccessorGenerator<GroundTag>::check_action_binding(const Node<GroundTag, S>& node, fp::ActionView<LiftedTag> action, fp::ObjectSpanView objects)
{
    // Ground applicability only resolves bindings already present in the compiled action set.
    return try_get_applicable_action_binding(node, action, objects).status;
}

template<StateViewConcept<GroundTag> S, typename Objects>
ActionBindingResult
SuccessorGenerator<GroundTag>::Impl::try_get_applicable_action_binding(const Node<GroundTag, S>& node, fp::ActionView<LiftedTag> action, Objects objects)
{
    const auto& task = definition->task;
    validate_task(task, node.get_state());
    get_schema_match_tree(action);
    if (objects.size() != action.get_arity())
        throw std::invalid_argument("SuccessorGenerator::check_action_binding(...): object count does not match action arity.");

    const auto indices = objects.get_data();
    const auto& source_repository = objects.get_context();
    const auto& target_repository = *task->get_repository();
    if (!ygg::formalism::contains_all(source_repository, indices) || !ygg::formalism::contains_all(target_repository, indices))
        return { ActionBindingStatus::INAPPLICABLE, std::nullopt };
    if (!ygg::formalism::contains_all(target_repository, objects))
        throw std::invalid_argument("SuccessorGenerator::check_action_binding(...): object does not belong to the task repository.");

    const auto& constants = task->get_task().get_domain().get_data().constants;
    const auto& task_objects = task->get_task().get_data().objects;
    for (const auto index : indices)
        if (!std::binary_search(constants.begin(), constants.end(), index) && !std::binary_search(task_objects.begin(), task_objects.end(), index))
            return { ActionBindingStatus::INAPPLICABLE, std::nullopt };

    auto& scratch = evaluator.checked_binding;
    scratch.relation = action.get_index();
    scratch.objects.clear();
    for (const auto index : indices)
        scratch.objects.push_back(index);
    const auto binding = task->get_repository()->find(scratch);
    if (!binding)
        return { ActionBindingStatus::INAPPLICABLE, std::nullopt };
    const auto found = definition->action_binding_to_ground_action.find(*binding);
    if (found == definition->action_binding_to_ground_action.end())
        return { ActionBindingStatus::INAPPLICABLE, std::nullopt };
    const auto state = StateContext<GroundTag>(*task, node.get_state().get_state_builder(), node.get_metric());
    if (!evaluator.executor.is_applicable(found->second, state))
        return { ActionBindingStatus::INAPPLICABLE, std::nullopt };
    return { ActionBindingStatus::APPLICABLE, *binding };
}

template<StateViewConcept<GroundTag> S>
ActionBindingResult
SuccessorGenerator<GroundTag>::try_get_applicable_action_binding(const Node<GroundTag, S>& node, fp::ActionView<LiftedTag> action, fp::ObjectSpanView objects)
{
    return m_impl->try_get_applicable_action_binding(node, action, objects);
}

template<StateViewConcept<GroundTag> S>
ActionBindingStatus SuccessorGenerator<GroundTag>::check_action_binding(const Node<GroundTag, S>& node, fp::ActionBindingView binding)
{
    if (!binding.get_context().contains(binding.get_index()))
        throw std::invalid_argument("SuccessorGenerator: action binding does not belong to its source repository.");
    return m_impl->try_get_applicable_action_binding(node, binding.get_relation(), binding.get_objects()).status;
}

fp::ActionBindingView SuccessorGenerator<GroundTag>::materialize_action_binding(BorrowedActionBindingView<GroundTag> binding)
{
    const auto& repository = *m_impl->definition->task->get_repository();
    if (!ygg::formalism::contains(repository, binding))
        throw std::invalid_argument("SuccessorGenerator: action binding does not belong to the task repository.");
    // Bindings in the grounded task already have validated schemas and objects.
    if (m_impl->definition->action_binding_to_ground_action.contains(binding))
        return binding;
    const auto action = binding.get_relation();
    m_impl->get_schema_match_tree(action);
    const auto objects = binding.get_objects();
    if (objects.size() != action.get_arity())
        throw std::invalid_argument("SuccessorGenerator: object count does not match action arity.");
    if (!ygg::formalism::contains_all(repository, objects))
        throw std::invalid_argument("SuccessorGenerator: object does not belong to the task repository.");
    return binding;
}

template<StateViewConcept<GroundTag> S>
Node<GroundTag, S> SuccessorGenerator<GroundTag>::get_successor_node(const Node<GroundTag, S>& node,
                                                                     fp::ActionBindingView binding,
                                                                     SuccessorStorage<S>& storage,
                                                                     AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    return get_successor_node(node, ground_action(binding), storage, axiom_evaluator);
}

template<StateViewConcept<GroundTag> S>
Node<GroundTag, S> SuccessorGenerator<GroundTag>::get_successor_node(const Node<GroundTag, S>& node,
                                                                     fp::ActionView<GroundTag> action,
                                                                     SuccessorStorage<S>& storage,
                                                                     AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, axiom_evaluator);
    const auto state_context = StateContext<GroundTag>(*m_impl->definition->task, node.get_state().get_state_builder(), node.get_metric());
    if constexpr (std::same_as<S, StateView<GroundTag>>)
    {
        validate_task(m_impl->definition->task, storage);
        auto successor_state = storage.get_state_builder();
        const auto auxiliary_value = m_impl->evaluator.executor.apply_action_unregistered(state_context, action, *successor_state);
        return finalize_successor_state(storage, axiom_evaluator, std::move(successor_state), auxiliary_value);
    }
    else
    {
        if (&storage == &node.get_state().get_state_builder())
            throw std::invalid_argument("SuccessorGenerator: source and successor builder must be distinct.");
        storage.clear();
        storage.resize_derived_atoms(m_impl->definition->task->get_task().template get_atoms<formalism::DerivedTag>().size());
        const auto auxiliary_value = m_impl->evaluator.executor.apply_action_unregistered(state_context, action, storage);
        const auto metric = evaluate_successor_metric(*m_impl->definition->task, storage, auxiliary_value);
        axiom_evaluator.compute_extended_state(storage);
        return Node<GroundTag, S>(S(storage, *m_impl->definition->task), metric);
    }
}

template<StateViewConcept<GroundTag> S>
std::vector<fp::ActionBindingView> SuccessorGenerator<GroundTag>::get_applicable_action_bindings(const Node<GroundTag, S>& node)
{
    auto result = std::vector<fp::ActionBindingView> {};
    get_applicable_action_bindings(node, result);
    return result;
}

template<StateViewConcept<GroundTag> S>
void SuccessorGenerator<GroundTag>::get_applicable_action_bindings(const Node<GroundTag, S>& node, std::vector<fp::ActionBindingView>& out_bindings)
{
    validate_task(m_impl->definition->task, node.get_state());
    out_bindings.clear();
    for_each_applicable_action_binding(node,
                                       [&](const auto binding)
                                       {
                                           out_bindings.push_back(binding);
                                           return true;
                                       });
}

template<StateViewConcept<GroundTag> S>
std::vector<fp::ActionBindingView> SuccessorGenerator<GroundTag>::get_applicable_action_bindings(const Node<GroundTag, S>& node,
                                                                                                 fp::ActionView<LiftedTag> action)
{
    auto result = std::vector<fp::ActionBindingView> {};
    get_applicable_action_bindings(node, action, result);
    return result;
}

template<StateViewConcept<GroundTag> S>
void SuccessorGenerator<GroundTag>::get_applicable_action_bindings(const Node<GroundTag, S>& node,
                                                                   fp::ActionView<LiftedTag> action,
                                                                   std::vector<fp::ActionBindingView>& out_bindings)
{
    validate_task(m_impl->definition->task, node.get_state());
    static_cast<void>(m_impl->get_schema_match_tree(action));
    out_bindings.clear();
    for_each_applicable_action_binding(node,
                                       action,
                                       [&](const auto binding)
                                       {
                                           out_bindings.push_back(binding);
                                           return true;
                                       });
}

template<StateViewConcept<GroundTag> S>
bool SuccessorGenerator<GroundTag>::for_each_applicable_action_binding(const Node<GroundTag, S>& node,
                                                                       const std::function<bool(fp::ActionBindingView)>& callback)
{
    validate_task(m_impl->definition->task, node.get_state());
    return m_impl->for_each_applicable_action(node, *m_impl->evaluator.action_match_tree, [&](const auto action) { return callback(action.get_row()); });
}

template<StateViewConcept<GroundTag> S>
bool SuccessorGenerator<GroundTag>::for_each_borrowed_applicable_action_binding(const Node<GroundTag, S>& node,
                                                                                const std::function<bool(BorrowedActionBindingView<GroundTag>)>& callback)
{
    return for_each_applicable_action_binding(node, callback);
}

template<StateViewConcept<GroundTag> S>
bool SuccessorGenerator<GroundTag>::for_each_successor_node(const Node<GroundTag, S>& node,
                                                            SuccessorStorage<S>& storage,
                                                            AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                            const std::type_identity_t<std::function<bool(Node<GroundTag, S>)>>& callback)
{
    validate_task(m_impl->definition->task, node.get_state());
    if constexpr (std::same_as<S, StateView<GroundTag>>)
        validate_task(m_impl->definition->task, storage);
    else if (&storage == &node.get_state().get_state_builder())
        throw std::invalid_argument("SuccessorGenerator: source and successor builder must be distinct.");
    validate_task(m_impl->definition->task, axiom_evaluator);
    return m_impl->for_each_applicable_action(node,
                                              *m_impl->evaluator.action_match_tree,
                                              [&](const auto ground_action)
                                              { return callback(get_successor_node(node, ground_action, storage, axiom_evaluator)); });
}

template<StateViewConcept<GroundTag> S>
bool SuccessorGenerator<GroundTag>::for_each_labeled_successor_node(const Node<GroundTag, S>& node,
                                                                    SuccessorStorage<S>& storage,
                                                                    AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                    const std::type_identity_t<std::function<bool(LabeledNode<GroundTag, S>)>>& callback)
{
    validate_task(m_impl->definition->task, node.get_state());
    if constexpr (std::same_as<S, StateView<GroundTag>>)
        validate_task(m_impl->definition->task, storage);
    else if (&storage == &node.get_state().get_state_builder())
        throw std::invalid_argument("SuccessorGenerator: source and successor builder must be distinct.");
    validate_task(m_impl->definition->task, axiom_evaluator);
    return m_impl->for_each_applicable_action(
        node,
        *m_impl->evaluator.action_match_tree,
        [&](const auto ground_action)
        { return callback(LabeledNode<GroundTag, S> { ground_action.get_row(), get_successor_node(node, ground_action, storage, axiom_evaluator) }); });
}

template<StateViewConcept<GroundTag> S>
bool SuccessorGenerator<GroundTag>::for_each_applicable_action_binding(const Node<GroundTag, S>& node,
                                                                       fp::ActionView<LiftedTag> action,
                                                                       const std::function<bool(fp::ActionBindingView)>& callback)
{
    validate_task(m_impl->definition->task, node.get_state());
    auto& tree = m_impl->get_schema_match_tree(action);
    return m_impl->for_each_applicable_action(node, tree, [&](const auto action) { return callback(action.get_row()); });
}

template<StateViewConcept<GroundTag> S>
bool SuccessorGenerator<GroundTag>::for_each_borrowed_applicable_action_binding(const Node<GroundTag, S>& node,
                                                                                fp::ActionView<LiftedTag> action,
                                                                                const std::function<bool(BorrowedActionBindingView<GroundTag>)>& callback)
{
    return for_each_applicable_action_binding(node, action, callback);
}

template<StateViewConcept<GroundTag> S>
bool SuccessorGenerator<GroundTag>::for_each_successor_node(const Node<GroundTag, S>& node,
                                                            fp::ActionView<LiftedTag> action,
                                                            SuccessorStorage<S>& storage,
                                                            AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                            const std::type_identity_t<std::function<bool(Node<GroundTag, S>)>>& callback)
{
    validate_task(m_impl->definition->task, node.get_state());
    if constexpr (std::same_as<S, StateView<GroundTag>>)
        validate_task(m_impl->definition->task, storage);
    else if (&storage == &node.get_state().get_state_builder())
        throw std::invalid_argument("SuccessorGenerator: source and successor builder must be distinct.");
    validate_task(m_impl->definition->task, axiom_evaluator);
    auto& tree = m_impl->get_schema_match_tree(action);
    return m_impl->for_each_applicable_action(node,
                                              tree,
                                              [&](const auto ground_action)
                                              { return callback(get_successor_node(node, ground_action, storage, axiom_evaluator)); });
}

template<StateViewConcept<GroundTag> S>
bool SuccessorGenerator<GroundTag>::for_each_labeled_successor_node(const Node<GroundTag, S>& node,
                                                                    fp::ActionView<LiftedTag> action,
                                                                    SuccessorStorage<S>& storage,
                                                                    AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                    const std::type_identity_t<std::function<bool(LabeledNode<GroundTag, S>)>>& callback)
{
    validate_task(m_impl->definition->task, node.get_state());
    if constexpr (std::same_as<S, StateView<GroundTag>>)
        validate_task(m_impl->definition->task, storage);
    else if (&storage == &node.get_state().get_state_builder())
        throw std::invalid_argument("SuccessorGenerator: source and successor builder must be distinct.");
    validate_task(m_impl->definition->task, axiom_evaluator);
    auto& tree = m_impl->get_schema_match_tree(action);
    return m_impl->for_each_applicable_action(
        node,
        tree,
        [&](const auto ground_action)
        { return callback(LabeledNode<GroundTag, S> { ground_action.get_row(), get_successor_node(node, ground_action, storage, axiom_evaluator) }); });
}

PackedNode<GroundTag> SuccessorGenerator<GroundTag>::get_packed_initial_node(StateRepository<GroundTag>& state_repository,
                                                                             AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    return get_initial_node(state_repository, axiom_evaluator).pack();
}

PackedNode<GroundTag> SuccessorGenerator<GroundTag>::get_packed_node(StateRepository<GroundTag>& state_repository, ygg::Index<State<GroundTag>> state_index)
{
    return get_node(state_repository, state_index).pack();
}

template<StateViewConcept<GroundTag> S>
PackedNode<GroundTag> SuccessorGenerator<GroundTag>::get_packed_successor_node(const Node<GroundTag, S>& node,
                                                                               fp::ActionBindingView binding,
                                                                               StateRepository<GroundTag>& state_repository,
                                                                               AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    return get_packed_successor_node(node, ground_action(binding), state_repository, axiom_evaluator);
}

template<StateViewConcept<GroundTag> S>
PackedNode<GroundTag> SuccessorGenerator<GroundTag>::get_packed_successor_node(const Node<GroundTag, S>& node,
                                                                               fp::ActionView<GroundTag> action,
                                                                               StateRepository<GroundTag>& state_repository,
                                                                               AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, state_repository);
    validate_task(m_impl->definition->task, axiom_evaluator);
    const auto state_context = StateContext<GroundTag>(*m_impl->definition->task, node.get_state().get_state_builder(), node.get_metric());
    auto successor_state = state_repository.get_state_builder();
    const auto auxiliary_value = m_impl->evaluator.executor.apply_action_unregistered(state_context, action, *successor_state);
    return finalize_successor_state(state_repository, axiom_evaluator, std::move(successor_state), auxiliary_value).pack();
}

template<StateViewConcept<GroundTag> S>
PackedNodeList<GroundTag> SuccessorGenerator<GroundTag>::get_packed_successor_nodes(const Node<GroundTag, S>& node,
                                                                                    StateRepository<GroundTag>& state_repository,
                                                                                    AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    auto result = PackedNodeList<GroundTag> {};
    get_packed_successor_nodes(node, state_repository, axiom_evaluator, result);
    return result;
}

template<StateViewConcept<GroundTag> S>
void SuccessorGenerator<GroundTag>::get_packed_successor_nodes(const Node<GroundTag, S>& node,
                                                               StateRepository<GroundTag>& state_repository,
                                                               AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                               PackedNodeList<GroundTag>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, state_repository);
    validate_task(m_impl->definition->task, axiom_evaluator);
    auto& tree = *m_impl->evaluator.action_match_tree;
    out_nodes.clear();
    m_impl->for_each_applicable_action(node,
                                       tree,
                                       [&](const auto ground_action)
                                       {
                                           out_nodes.push_back(get_packed_successor_node(node, ground_action, state_repository, axiom_evaluator));
                                           return true;
                                       });
}

template<StateViewConcept<GroundTag> S>
PackedNodeList<GroundTag> SuccessorGenerator<GroundTag>::get_packed_successor_nodes(const Node<GroundTag, S>& node,
                                                                                    fp::ActionView<LiftedTag> action,
                                                                                    StateRepository<GroundTag>& state_repository,
                                                                                    AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    auto result = PackedNodeList<GroundTag> {};
    get_packed_successor_nodes(node, action, state_repository, axiom_evaluator, result);
    return result;
}

template<StateViewConcept<GroundTag> S>
void SuccessorGenerator<GroundTag>::get_packed_successor_nodes(const Node<GroundTag, S>& node,
                                                               fp::ActionView<LiftedTag> action,
                                                               StateRepository<GroundTag>& state_repository,
                                                               AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                               PackedNodeList<GroundTag>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, state_repository);
    validate_task(m_impl->definition->task, axiom_evaluator);
    auto& tree = m_impl->get_schema_match_tree(action);
    out_nodes.clear();
    m_impl->for_each_applicable_action(node,
                                       tree,
                                       [&](const auto ground_action)
                                       {
                                           out_nodes.push_back(get_packed_successor_node(node, ground_action, state_repository, axiom_evaluator));
                                           return true;
                                       });
}

template<StateViewConcept<GroundTag> S>
PackedLabeledNodeList<GroundTag> SuccessorGenerator<GroundTag>::get_packed_labeled_successor_nodes(const Node<GroundTag, S>& node,
                                                                                                   StateRepository<GroundTag>& state_repository,
                                                                                                   AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    auto result = PackedLabeledNodeList<GroundTag> {};
    get_packed_labeled_successor_nodes(node, state_repository, axiom_evaluator, result);
    return result;
}

template<StateViewConcept<GroundTag> S>
void SuccessorGenerator<GroundTag>::get_packed_labeled_successor_nodes(const Node<GroundTag, S>& node,
                                                                       StateRepository<GroundTag>& state_repository,
                                                                       AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                       PackedLabeledNodeList<GroundTag>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, state_repository);
    validate_task(m_impl->definition->task, axiom_evaluator);
    auto& tree = *m_impl->evaluator.action_match_tree;
    out_nodes.clear();
    m_impl->for_each_applicable_action(
        node,
        tree,
        [&](const auto ground_action)
        {
            out_nodes.push_back(
                PackedLabeledNode<GroundTag> { ground_action.get_row(), get_packed_successor_node(node, ground_action, state_repository, axiom_evaluator) });
            return true;
        });
}

template<StateViewConcept<GroundTag> S>
PackedLabeledNodeList<GroundTag> SuccessorGenerator<GroundTag>::get_packed_labeled_successor_nodes(const Node<GroundTag, S>& node,
                                                                                                   fp::ActionView<LiftedTag> action,
                                                                                                   StateRepository<GroundTag>& state_repository,
                                                                                                   AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    auto result = PackedLabeledNodeList<GroundTag> {};
    get_packed_labeled_successor_nodes(node, action, state_repository, axiom_evaluator, result);
    return result;
}

template<StateViewConcept<GroundTag> S>
void SuccessorGenerator<GroundTag>::get_packed_labeled_successor_nodes(const Node<GroundTag, S>& node,
                                                                       fp::ActionView<LiftedTag> action,
                                                                       StateRepository<GroundTag>& state_repository,
                                                                       AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                       PackedLabeledNodeList<GroundTag>& out_nodes)
{
    validate_task(m_impl->definition->task, node.get_state());
    validate_task(m_impl->definition->task, state_repository);
    validate_task(m_impl->definition->task, axiom_evaluator);
    auto& tree = m_impl->get_schema_match_tree(action);
    out_nodes.clear();
    m_impl->for_each_applicable_action(
        node,
        tree,
        [&](const auto ground_action)
        {
            out_nodes.push_back(
                PackedLabeledNode<GroundTag> { ground_action.get_row(), get_packed_successor_node(node, ground_action, state_repository, axiom_evaluator) });
            return true;
        });
}

template<StateViewConcept<GroundTag> S>
ygg::float_t SuccessorGenerator<GroundTag>::generate_successor_state(const Node<GroundTag, S>& node,
                                                                     fp::ActionBindingView binding,
                                                                     ygg::Builder<State<GroundTag>>& out_state)
{
    validate_task(m_impl->definition->task, node.get_state());
    const auto state_context = StateContext<GroundTag>(*m_impl->definition->task, node.get_state().get_state_builder(), node.get_metric());
    return m_impl->evaluator.executor.apply_action_unregistered(state_context, ground_action(binding), out_state);
}

Node<GroundTag> SuccessorGenerator<GroundTag>::finalize_successor_state(StateRepository<GroundTag>& state_repository,
                                                                        AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                        ygg::SharedObjectPoolPtr<ygg::Builder<State<GroundTag>>, true> state,
                                                                        ygg::float_t auxiliary_value)
{
    validate_task(m_impl->definition->task, state_repository);
    validate_task(m_impl->definition->task, axiom_evaluator);
    const auto metric = evaluate_successor_metric(*m_impl->definition->task, *state, auxiliary_value);
    return Node<GroundTag>(state_repository.register_state(axiom_evaluator, std::move(state)), metric);
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

template ActionBindingStatus SuccessorGenerator<GroundTag>::check_action_binding(const Node<GroundTag>& node, fp::ActionBindingView binding);

template bool
SuccessorGenerator<GroundTag>::for_each_borrowed_applicable_action_binding(const Node<GroundTag>& node,
                                                                           const std::function<bool(BorrowedActionBindingView<GroundTag>)>& callback);

template bool
SuccessorGenerator<GroundTag>::for_each_borrowed_applicable_action_binding(const Node<GroundTag>& node,
                                                                           fp::ActionView<LiftedTag> action,
                                                                           const std::function<bool(BorrowedActionBindingView<GroundTag>)>& callback);

template ActionBindingStatus SuccessorGenerator<GroundTag>::check_action_binding(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                                 fp::ActionBindingView binding);

template bool
SuccessorGenerator<GroundTag>::for_each_borrowed_applicable_action_binding(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                           const std::function<bool(BorrowedActionBindingView<GroundTag>)>& callback);

template bool
SuccessorGenerator<GroundTag>::for_each_borrowed_applicable_action_binding(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                           fp::ActionView<LiftedTag> action,
                                                                           const std::function<bool(BorrowedActionBindingView<GroundTag>)>& callback);

template ActionBindingResult
SuccessorGenerator<GroundTag>::try_get_applicable_action_binding(const Node<GroundTag>& node, fp::ActionView<LiftedTag> action, fp::ObjectSpanView objects);
template ActionBindingResult SuccessorGenerator<GroundTag>::try_get_applicable_action_binding(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                                              fp::ActionView<LiftedTag> action,
                                                                                              fp::ObjectSpanView objects);

template ActionBindingStatus
SuccessorGenerator<GroundTag>::check_action_binding(const Node<GroundTag>& node, fp::ActionView<LiftedTag> action, fp::ObjectSpanView objects);

template ActionBindingStatus SuccessorGenerator<GroundTag>::check_action_binding(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                                 fp::ActionView<LiftedTag> action,
                                                                                 fp::ObjectSpanView objects);

template NodeList<GroundTag> SuccessorGenerator<GroundTag>::get_successor_nodes(const Node<GroundTag>& node,
                                                                                StateRepository<GroundTag>& state_repository,
                                                                                AxiomEvaluator<GroundTag>& axiom_evaluator);

template NodeList<GroundTag, BuilderStateView<GroundTag>>
SuccessorGenerator<GroundTag>::get_successor_nodes(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                   std::deque<ygg::Builder<State<GroundTag>>>& storage,
                                                   AxiomEvaluator<GroundTag>& axiom_evaluator);

template void SuccessorGenerator<GroundTag>::get_successor_nodes(const Node<GroundTag>& node,
                                                                 StateRepository<GroundTag>& state_repository,
                                                                 AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                 NodeList<GroundTag>& out_nodes);

template void SuccessorGenerator<GroundTag>::get_successor_nodes(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                 std::deque<ygg::Builder<State<GroundTag>>>& storage,
                                                                 AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                 NodeList<GroundTag, BuilderStateView<GroundTag>>& out_nodes);

template NodeList<GroundTag> SuccessorGenerator<GroundTag>::get_successor_nodes(const Node<GroundTag>& node,
                                                                                fp::ActionView<LiftedTag> action,
                                                                                StateRepository<GroundTag>& state_repository,
                                                                                AxiomEvaluator<GroundTag>& axiom_evaluator);

template NodeList<GroundTag, BuilderStateView<GroundTag>>
SuccessorGenerator<GroundTag>::get_successor_nodes(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                   fp::ActionView<LiftedTag> action,
                                                   std::deque<ygg::Builder<State<GroundTag>>>& storage,
                                                   AxiomEvaluator<GroundTag>& axiom_evaluator);

template void SuccessorGenerator<GroundTag>::get_successor_nodes(const Node<GroundTag>& node,
                                                                 fp::ActionView<LiftedTag> action,
                                                                 StateRepository<GroundTag>& state_repository,
                                                                 AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                 NodeList<GroundTag>& out_nodes);

template void SuccessorGenerator<GroundTag>::get_successor_nodes(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                 fp::ActionView<LiftedTag> action,
                                                                 std::deque<ygg::Builder<State<GroundTag>>>& storage,
                                                                 AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                 NodeList<GroundTag, BuilderStateView<GroundTag>>& out_nodes);

template LabeledNodeList<GroundTag> SuccessorGenerator<GroundTag>::get_labeled_successor_nodes(const Node<GroundTag>& node,
                                                                                               StateRepository<GroundTag>& state_repository,
                                                                                               AxiomEvaluator<GroundTag>& axiom_evaluator);

template LabeledNodeList<GroundTag, BuilderStateView<GroundTag>>
SuccessorGenerator<GroundTag>::get_labeled_successor_nodes(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                           std::deque<ygg::Builder<State<GroundTag>>>& storage,
                                                           AxiomEvaluator<GroundTag>& axiom_evaluator);

template void SuccessorGenerator<GroundTag>::get_labeled_successor_nodes(const Node<GroundTag>& node,
                                                                         StateRepository<GroundTag>& state_repository,
                                                                         AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                         LabeledNodeList<GroundTag>& out_nodes);

template void SuccessorGenerator<GroundTag>::get_labeled_successor_nodes(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                         std::deque<ygg::Builder<State<GroundTag>>>& storage,
                                                                         AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                         LabeledNodeList<GroundTag, BuilderStateView<GroundTag>>& out_nodes);

template LabeledNodeList<GroundTag> SuccessorGenerator<GroundTag>::get_labeled_successor_nodes(const Node<GroundTag>& node,
                                                                                               fp::ActionView<LiftedTag> action,
                                                                                               StateRepository<GroundTag>& state_repository,
                                                                                               AxiomEvaluator<GroundTag>& axiom_evaluator);

template LabeledNodeList<GroundTag, BuilderStateView<GroundTag>>
SuccessorGenerator<GroundTag>::get_labeled_successor_nodes(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                           fp::ActionView<LiftedTag> action,
                                                           std::deque<ygg::Builder<State<GroundTag>>>& storage,
                                                           AxiomEvaluator<GroundTag>& axiom_evaluator);

template void SuccessorGenerator<GroundTag>::get_labeled_successor_nodes(const Node<GroundTag>& node,
                                                                         fp::ActionView<LiftedTag> action,
                                                                         StateRepository<GroundTag>& state_repository,
                                                                         AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                         LabeledNodeList<GroundTag>& out_nodes);

template void SuccessorGenerator<GroundTag>::get_labeled_successor_nodes(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                         fp::ActionView<LiftedTag> action,
                                                                         std::deque<ygg::Builder<State<GroundTag>>>& storage,
                                                                         AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                         LabeledNodeList<GroundTag, BuilderStateView<GroundTag>>& out_nodes);

template Node<GroundTag> SuccessorGenerator<GroundTag>::get_successor_node(const Node<GroundTag>& node,
                                                                           fp::ActionBindingView binding,
                                                                           StateRepository<GroundTag>& state_repository,
                                                                           AxiomEvaluator<GroundTag>& axiom_evaluator);

template Node<GroundTag, BuilderStateView<GroundTag>>
SuccessorGenerator<GroundTag>::get_successor_node(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                  fp::ActionBindingView binding,
                                                  ygg::Builder<State<GroundTag>>& storage,
                                                  AxiomEvaluator<GroundTag>& axiom_evaluator);

template Node<GroundTag> SuccessorGenerator<GroundTag>::get_successor_node(const Node<GroundTag>& node,
                                                                           fp::ActionView<GroundTag> action,
                                                                           StateRepository<GroundTag>& state_repository,
                                                                           AxiomEvaluator<GroundTag>& axiom_evaluator);

template Node<GroundTag, BuilderStateView<GroundTag>>
SuccessorGenerator<GroundTag>::get_successor_node(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                  fp::ActionView<GroundTag> action,
                                                  ygg::Builder<State<GroundTag>>& storage,
                                                  AxiomEvaluator<GroundTag>& axiom_evaluator);

template std::vector<fp::ActionBindingView> SuccessorGenerator<GroundTag>::get_applicable_action_bindings(const Node<GroundTag>& node);

template std::vector<fp::ActionBindingView>
SuccessorGenerator<GroundTag>::get_applicable_action_bindings(const Node<GroundTag, BuilderStateView<GroundTag>>& node);

template void SuccessorGenerator<GroundTag>::get_applicable_action_bindings(const Node<GroundTag>& node, std::vector<fp::ActionBindingView>& out_bindings);

template void SuccessorGenerator<GroundTag>::get_applicable_action_bindings(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                            std::vector<fp::ActionBindingView>& out_bindings);

template std::vector<fp::ActionBindingView> SuccessorGenerator<GroundTag>::get_applicable_action_bindings(const Node<GroundTag>& node,
                                                                                                          fp::ActionView<LiftedTag> action);

template std::vector<fp::ActionBindingView>
SuccessorGenerator<GroundTag>::get_applicable_action_bindings(const Node<GroundTag, BuilderStateView<GroundTag>>& node, fp::ActionView<LiftedTag> action);

template void SuccessorGenerator<GroundTag>::get_applicable_action_bindings(const Node<GroundTag>& node,
                                                                            fp::ActionView<LiftedTag> action,
                                                                            std::vector<fp::ActionBindingView>& out_bindings);

template void SuccessorGenerator<GroundTag>::get_applicable_action_bindings(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                            fp::ActionView<LiftedTag> action,
                                                                            std::vector<fp::ActionBindingView>& out_bindings);

template bool SuccessorGenerator<GroundTag>::for_each_applicable_action_binding(const Node<GroundTag>& node,
                                                                                const std::function<bool(fp::ActionBindingView)>& callback);

template bool SuccessorGenerator<GroundTag>::for_each_applicable_action_binding(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                                const std::function<bool(fp::ActionBindingView)>& callback);

template bool SuccessorGenerator<GroundTag>::for_each_successor_node(const Node<GroundTag>& node,
                                                                     StateRepository<GroundTag>& state_repository,
                                                                     AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                     const std::function<bool(Node<GroundTag>)>& callback);

template bool SuccessorGenerator<GroundTag>::for_each_successor_node(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                     ygg::Builder<State<GroundTag>>& storage,
                                                                     AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                     const std::function<bool(Node<GroundTag, BuilderStateView<GroundTag>>)>& callback);

template bool SuccessorGenerator<GroundTag>::for_each_labeled_successor_node(const Node<GroundTag>& node,
                                                                             StateRepository<GroundTag>& state_repository,
                                                                             AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                             const std::function<bool(LabeledNode<GroundTag>)>& callback);

template bool
SuccessorGenerator<GroundTag>::for_each_labeled_successor_node(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                               ygg::Builder<State<GroundTag>>& storage,
                                                               AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                               const std::function<bool(LabeledNode<GroundTag, BuilderStateView<GroundTag>>)>& callback);

template bool SuccessorGenerator<GroundTag>::for_each_applicable_action_binding(const Node<GroundTag>& node,
                                                                                fp::ActionView<LiftedTag> action,
                                                                                const std::function<bool(fp::ActionBindingView)>& callback);

template bool SuccessorGenerator<GroundTag>::for_each_applicable_action_binding(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                                fp::ActionView<LiftedTag> action,
                                                                                const std::function<bool(fp::ActionBindingView)>& callback);

template bool SuccessorGenerator<GroundTag>::for_each_successor_node(const Node<GroundTag>& node,
                                                                     fp::ActionView<LiftedTag> action,
                                                                     StateRepository<GroundTag>& state_repository,
                                                                     AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                     const std::function<bool(Node<GroundTag>)>& callback);

template bool SuccessorGenerator<GroundTag>::for_each_successor_node(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                     fp::ActionView<LiftedTag> action,
                                                                     ygg::Builder<State<GroundTag>>& storage,
                                                                     AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                     const std::function<bool(Node<GroundTag, BuilderStateView<GroundTag>>)>& callback);

template bool SuccessorGenerator<GroundTag>::for_each_labeled_successor_node(const Node<GroundTag>& node,
                                                                             fp::ActionView<LiftedTag> action,
                                                                             StateRepository<GroundTag>& state_repository,
                                                                             AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                             const std::function<bool(LabeledNode<GroundTag>)>& callback);

template bool
SuccessorGenerator<GroundTag>::for_each_labeled_successor_node(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                               fp::ActionView<LiftedTag> action,
                                                               ygg::Builder<State<GroundTag>>& storage,
                                                               AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                               const std::function<bool(LabeledNode<GroundTag, BuilderStateView<GroundTag>>)>& callback);

template PackedNode<GroundTag> SuccessorGenerator<GroundTag>::get_packed_successor_node(const Node<GroundTag>& node,
                                                                                        fp::ActionBindingView binding,
                                                                                        StateRepository<GroundTag>& state_repository,
                                                                                        AxiomEvaluator<GroundTag>& axiom_evaluator);

template PackedNode<GroundTag> SuccessorGenerator<GroundTag>::get_packed_successor_node(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                                        fp::ActionBindingView binding,
                                                                                        StateRepository<GroundTag>& state_repository,
                                                                                        AxiomEvaluator<GroundTag>& axiom_evaluator);

template PackedNode<GroundTag> SuccessorGenerator<GroundTag>::get_packed_successor_node(const Node<GroundTag>& node,
                                                                                        fp::ActionView<GroundTag> action,
                                                                                        StateRepository<GroundTag>& state_repository,
                                                                                        AxiomEvaluator<GroundTag>& axiom_evaluator);

template PackedNode<GroundTag> SuccessorGenerator<GroundTag>::get_packed_successor_node(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                                        fp::ActionView<GroundTag> action,
                                                                                        StateRepository<GroundTag>& state_repository,
                                                                                        AxiomEvaluator<GroundTag>& axiom_evaluator);

template PackedNodeList<GroundTag> SuccessorGenerator<GroundTag>::get_packed_successor_nodes(const Node<GroundTag>& node,
                                                                                             StateRepository<GroundTag>& state_repository,
                                                                                             AxiomEvaluator<GroundTag>& axiom_evaluator);

template PackedNodeList<GroundTag> SuccessorGenerator<GroundTag>::get_packed_successor_nodes(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                                             StateRepository<GroundTag>& state_repository,
                                                                                             AxiomEvaluator<GroundTag>& axiom_evaluator);

template void SuccessorGenerator<GroundTag>::get_packed_successor_nodes(const Node<GroundTag>& node,
                                                                        StateRepository<GroundTag>& state_repository,
                                                                        AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                        PackedNodeList<GroundTag>& out_nodes);

template void SuccessorGenerator<GroundTag>::get_packed_successor_nodes(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                        StateRepository<GroundTag>& state_repository,
                                                                        AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                        PackedNodeList<GroundTag>& out_nodes);

template PackedNodeList<GroundTag> SuccessorGenerator<GroundTag>::get_packed_successor_nodes(const Node<GroundTag>& node,
                                                                                             fp::ActionView<LiftedTag> action,
                                                                                             StateRepository<GroundTag>& state_repository,
                                                                                             AxiomEvaluator<GroundTag>& axiom_evaluator);

template PackedNodeList<GroundTag> SuccessorGenerator<GroundTag>::get_packed_successor_nodes(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                                             fp::ActionView<LiftedTag> action,
                                                                                             StateRepository<GroundTag>& state_repository,
                                                                                             AxiomEvaluator<GroundTag>& axiom_evaluator);

template void SuccessorGenerator<GroundTag>::get_packed_successor_nodes(const Node<GroundTag>& node,
                                                                        fp::ActionView<LiftedTag> action,
                                                                        StateRepository<GroundTag>& state_repository,
                                                                        AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                        PackedNodeList<GroundTag>& out_nodes);

template void SuccessorGenerator<GroundTag>::get_packed_successor_nodes(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                        fp::ActionView<LiftedTag> action,
                                                                        StateRepository<GroundTag>& state_repository,
                                                                        AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                        PackedNodeList<GroundTag>& out_nodes);

template PackedLabeledNodeList<GroundTag> SuccessorGenerator<GroundTag>::get_packed_labeled_successor_nodes(const Node<GroundTag>& node,
                                                                                                            StateRepository<GroundTag>& state_repository,
                                                                                                            AxiomEvaluator<GroundTag>& axiom_evaluator);

template PackedLabeledNodeList<GroundTag>
SuccessorGenerator<GroundTag>::get_packed_labeled_successor_nodes(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                  StateRepository<GroundTag>& state_repository,
                                                                  AxiomEvaluator<GroundTag>& axiom_evaluator);

template void SuccessorGenerator<GroundTag>::get_packed_labeled_successor_nodes(const Node<GroundTag>& node,
                                                                                StateRepository<GroundTag>& state_repository,
                                                                                AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                                PackedLabeledNodeList<GroundTag>& out_nodes);

template void SuccessorGenerator<GroundTag>::get_packed_labeled_successor_nodes(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                                StateRepository<GroundTag>& state_repository,
                                                                                AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                                PackedLabeledNodeList<GroundTag>& out_nodes);

template PackedLabeledNodeList<GroundTag> SuccessorGenerator<GroundTag>::get_packed_labeled_successor_nodes(const Node<GroundTag>& node,
                                                                                                            fp::ActionView<LiftedTag> action,
                                                                                                            StateRepository<GroundTag>& state_repository,
                                                                                                            AxiomEvaluator<GroundTag>& axiom_evaluator);

template PackedLabeledNodeList<GroundTag>
SuccessorGenerator<GroundTag>::get_packed_labeled_successor_nodes(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                  fp::ActionView<LiftedTag> action,
                                                                  StateRepository<GroundTag>& state_repository,
                                                                  AxiomEvaluator<GroundTag>& axiom_evaluator);

template void SuccessorGenerator<GroundTag>::get_packed_labeled_successor_nodes(const Node<GroundTag>& node,
                                                                                fp::ActionView<LiftedTag> action,
                                                                                StateRepository<GroundTag>& state_repository,
                                                                                AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                                PackedLabeledNodeList<GroundTag>& out_nodes);

template void SuccessorGenerator<GroundTag>::get_packed_labeled_successor_nodes(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                                fp::ActionView<LiftedTag> action,
                                                                                StateRepository<GroundTag>& state_repository,
                                                                                AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                                PackedLabeledNodeList<GroundTag>& out_nodes);

template ygg::float_t
SuccessorGenerator<GroundTag>::generate_successor_state(const Node<GroundTag>& node, fp::ActionBindingView binding, ygg::Builder<State<GroundTag>>& out_state);

template ygg::float_t SuccessorGenerator<GroundTag>::generate_successor_state(const Node<GroundTag, BuilderStateView<GroundTag>>& node,
                                                                              fp::ActionBindingView binding,
                                                                              ygg::Builder<State<GroundTag>>& out_state);

static_assert(SuccessorGeneratorConcept<SuccessorGenerator<GroundTag>, GroundTag>);

}
