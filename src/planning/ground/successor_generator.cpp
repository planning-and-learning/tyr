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
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/planning/action_executor.hpp"
#include "tyr/planning/applicability.hpp"
#include "tyr/planning/axiom_evaluator.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/match_tree/match_tree.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/state_delta.hpp"
#include "tyr/planning/state_repository.hpp"
#include "tyr/planning/task_utils.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/segmented.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <yggdrasil/formalism/membership.hpp>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::planning
{
namespace
{
using Source = SourceNode<GroundTag>;
using Builder = urd::builder<State<GroundTag>>;
using BuilderNode = Node<GroundTag, BuilderStateView<GroundTag>>;
using Bindings = urd::Store<urd::sequence<formalism::planning::ActionRowView>>;

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

void validate_task(const TaskPtr<GroundTag>& task, const Source& node)
{
    if (&node.get_task() != task.get())
        throw std::invalid_argument("SuccessorGenerator: state belongs to a different task.");
}

/// Successors go into a repository of the task, a builder other than the source's, or fresh builders of a list.
void validate_storage(const TaskPtr<GroundTag>& task, const Source&, const StateRepository<GroundTag>& storage) { validate_task(task, storage); }

void validate_storage(const TaskPtr<GroundTag>&, const Source& node, const Builder& storage)
{
    if (&storage == &node.data())
        throw std::invalid_argument("SuccessorGenerator: source and successor builder must be distinct.");
}

void validate_storage(const TaskPtr<GroundTag>&, const Source&, const SuccessorBuilders<GroundTag>&) {}

/// The storage of a list's next successor: the repository, or a builder appended to the list.
StateRepository<GroundTag>& next_storage(StateRepository<GroundTag>& storage) { return storage; }
Builder& next_storage(Builder& storage) { return storage; }
Builder& next_storage(SuccessorBuilders<GroundTag>& storage) { return storage.emplace_back(); }

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

struct SuccessorGenerator<GroundTag>::Impl
{
    using ActionBindingMap = urd::Store<urd::map<formalism::planning::ActionRowView, formalism::planning::ActionView<GroundTag>>>;
    using ActionMatchTrees = urd::Store<urd::map<formalism::planning::ActionView<LiftedTag>, match_tree::MatchTreePtr<formalism::planning::Action<GroundTag>>>>;
    using MatchTree = match_tree::MatchTree<formalism::planning::Action<GroundTag>>;

    struct Definition
    {
        explicit Definition(TaskPtr<GroundTag> task);

        TaskPtr<GroundTag> task;
        match_tree::MatchTreePtr<formalism::planning::Action<GroundTag>> action_match_tree_prototype;
        ActionMatchTrees schema_match_tree_prototypes;
        ActionBindingMap action_binding_to_ground_action;
    };

    struct Evaluator
    {
        explicit Evaluator(const Definition& definition);

        match_tree::MatchTreePtr<formalism::planning::Action<GroundTag>> action_match_tree;
        ActionMatchTrees schema_match_trees;
        formalism::planning::ActionViewList<GroundTag> applicable_actions;
        ActionExecutor executor;
        StateDelta successor_delta;
        urd::data<ygg::Row<formalism::planning::Action<LiftedTag>>> checked_binding;
    };

    explicit Impl(TaskPtr<GroundTag> task) : definition(std::make_shared<Definition>(std::move(task))), evaluator(*definition) {}

    explicit Impl(std::shared_ptr<const Definition> definition_) : definition(std::move(definition_)), evaluator(*this->definition) {}

    MatchTree& get_schema_match_tree(formalism::planning::ActionView<LiftedTag> action)
    {
        const auto it = evaluator.schema_match_trees.find(action);
        // Repository indices can collide across independent factories.
        if (it == evaluator.schema_match_trees.end() || &action.context() != &it->first.context())
            throw std::invalid_argument("SuccessorGenerator: action schema does not belong to the task domain.");
        return *it->second;
    }

    /// The match tree of the action schema, or of all actions.
    MatchTree& get_match_tree(std::optional<formalism::planning::ActionView<LiftedTag>> action)
    { return action ? get_schema_match_tree(*action) : *evaluator.action_match_tree; }

    template<typename Objects>
    ActionBindingResult try_get_applicable_action_binding(const Source& node, formalism::planning::ActionView<LiftedTag> action, Objects objects);

    template<typename Callback>
    bool for_each_applicable_action(const Source& node, MatchTree& tree, Callback&& callback)
    {
        return for_each_applicable_action(node, tree, [](auto) { return true; }, std::forward<Callback>(callback), [] { return false; });
    }

    template<typename Accept, typename Callback, typename Stop>
    bool for_each_applicable_action(const Source& node, MatchTree& tree, Accept accept, Callback callback, Stop stop)
    {
        if (stop())
            return false;
        const auto state_context = StateContext<GroundTag>(*definition->task, node.data(), node.get_metric());
        tree.generate(state_context, evaluator.applicable_actions);
        for (const auto action : evaluator.applicable_actions)
        {
            if (stop())
                return false;
            if (!accept(action))
                continue;
            assert(is_applicable(action.get_condition(), state_context));
            if (!evaluator.executor.is_applicable_if_fires(action, state_context))
                continue;
            assert(evaluator.executor.is_applicable(action, state_context));
            if (!callback(action))
                return false;
        }
        return true;
    }

    /// Validates the arguments of generating successors of `node` into `storage`, and gives the match tree of the action
    /// schema, or of all actions.
    template<typename Storage>
    MatchTree& prepare(const Source& node,
                       std::optional<formalism::planning::ActionView<LiftedTag>> action,
                       const Storage& storage,
                       const AxiomEvaluator<GroundTag>& axiom_evaluator)
    {
        validate(node, storage, axiom_evaluator);
        return get_match_tree(action);
    }

    template<typename Storage>
    void validate(const Source& node, const Storage& storage, const AxiomEvaluator<GroundTag>& axiom_evaluator) const
    {
        validate_task(definition->task, node);
        validate_storage(definition->task, node, storage);
        validate_task(definition->task, axiom_evaluator);
    }

    /// The core: writes the successor of `node` by `action` into `out_state`, without axiom closure.
    ygg::float_t generate(const Source& node, formalism::planning::ActionView<GroundTag> action, Builder& out_state, StateDelta& out_delta)
    {
        const auto state_context = StateContext<GroundTag>(*definition->task, node.data(), node.get_metric());
        return evaluator.executor.apply_action_unregistered(state_context, action, out_state, out_delta);
    }

    /// Passes the successor of each applicable action of `tree`, generated into `storage`, to `sink`.
    template<typename Storage, typename Sink>
    bool for_each_successor(SuccessorGenerator& self,
                            const Source& node,
                            MatchTree& tree,
                            Storage& storage,
                            AxiomEvaluator<GroundTag>& axiom_evaluator,
                            const Sink& sink)
    {
        return for_each_applicable_action(node,
                                          tree,
                                          [&](const auto ground_action)
                                          { return sink(self.get_successor_node(node, ground_action, next_storage(storage), axiom_evaluator)); });
    }

    /// Like for_each_successor(), with each successor labeled by its action binding.
    template<typename Storage, typename Sink>
    bool for_each_labeled_successor(SuccessorGenerator& self,
                                    const Source& node,
                                    MatchTree& tree,
                                    Storage& storage,
                                    AxiomEvaluator<GroundTag>& axiom_evaluator,
                                    const Sink& sink)
    {
        return for_each_applicable_action(
            node,
            tree,
            [&](const auto ground_action)
            {
                auto successor = self.get_successor_node(node, ground_action, next_storage(storage), axiom_evaluator);
                return sink(LabeledNode<GroundTag, std::remove_cvref_t<decltype(successor.get_state())>> { ground_action.get_row(), std::move(successor) });
            });
    }

    std::shared_ptr<const Definition> definition;
    Evaluator evaluator;
};

SuccessorGenerator<GroundTag>::Impl::Definition::Definition(TaskPtr<GroundTag> task_) :
    task(std::move(task_)),
    action_match_tree_prototype(match_tree::MatchTree<formalism::planning::Action<GroundTag>>::create(
        formalism::planning::ActionViewList<GroundTag>(task->get_task().get_ground_actions().begin(), task->get_task().get_ground_actions().end()),
        task->get_task().context())),
    action_binding_to_ground_action()
{
    auto schema_actions = urd::Store<urd::map<formalism::planning::ActionView<LiftedTag>, formalism::planning::ActionViewList<GroundTag>>> {};
    for (const auto action : task->get_task().get_domain().get_actions())
        schema_actions.try_emplace(action);
    for (const auto action : task->get_task().get_ground_actions())
    {
        action_binding_to_ground_action.emplace(action.get_row(), action);
        schema_actions.at(action.get_action()).push_back(action);
    }
    for (auto& [schema, actions] : schema_actions)
        schema_match_tree_prototypes.emplace(
            schema,
            match_tree::MatchTree<formalism::planning::Action<GroundTag>>::create(std::move(actions), task->get_task().context()));
}

SuccessorGenerator<GroundTag>::Impl::Evaluator::Evaluator(const Definition& definition) :
    action_match_tree(definition.action_match_tree_prototype->make_worker()),
    applicable_actions(),
    executor()
{
    for (const auto& [schema, tree] : definition.schema_match_tree_prototypes)
        schema_match_trees.emplace(schema, tree->make_worker());
}

SuccessorGenerator<GroundTag>::SuccessorGenerator(TaskPtr<GroundTag> task, const ygg::ExecutionContextPtr& execution_context) :
    m_impl(std::make_unique<Impl>(std::move(task)))
{ static_cast<void>(execution_context); }

SuccessorGenerator<GroundTag>::SuccessorGenerator(std::unique_ptr<Impl> impl) noexcept : m_impl(std::move(impl)) {}

SuccessorGenerator<GroundTag>::~SuccessorGenerator() = default;
SuccessorGenerator<GroundTag>::SuccessorGenerator(SuccessorGenerator&&) noexcept = default;
SuccessorGenerator<GroundTag>& SuccessorGenerator<GroundTag>::operator=(SuccessorGenerator&&) noexcept = default;

SuccessorGeneratorPtr<GroundTag> SuccessorGenerator<GroundTag>::make_worker(const ygg::ExecutionContextPtr& execution_context) const
{
    static_cast<void>(execution_context);
    return SuccessorGeneratorPtr<GroundTag>(new SuccessorGenerator<GroundTag>(std::make_unique<Impl>(m_impl->definition)));
}

Node<GroundTag> SuccessorGenerator<GroundTag>::get_initial_node(StateRepository<GroundTag>& state_repository, AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    validate_task(m_impl->definition->task, state_repository);
    validate_task(m_impl->definition->task, axiom_evaluator);
    auto initial_state = state_repository.get_initial_state(axiom_evaluator);

    const auto state_context = StateContext<GroundTag>(*m_impl->definition->task, initial_state.data(), 0);

    const auto state_metric =
        evaluate_metric(m_impl->definition->task->get_task().get_metric(), m_impl->definition->task->get_task().get_auxiliary_fterm_value(), state_context);

    return Node<GroundTag>(std::move(initial_state), state_metric);
}

NodeList<GroundTag>
SuccessorGenerator<GroundTag>::get_successor_nodes(const Source& node, StateRepository<GroundTag>& storage, AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    auto result = NodeList<GroundTag> {};
    get_successor_nodes(node, storage, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<GroundTag>::get_successor_nodes(const Source& node,
                                                        StateRepository<GroundTag>& storage,
                                                        AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                        NodeList<GroundTag>& out_nodes)
{
    auto& tree = m_impl->prepare(node, std::nullopt, storage, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_successor(*this, node, tree, storage, axiom_evaluator, append(out_nodes));
}

NodeList<GroundTag> SuccessorGenerator<GroundTag>::get_successor_nodes(const Source& node,
                                                                       formalism::planning::ActionView<LiftedTag> action,
                                                                       StateRepository<GroundTag>& storage,
                                                                       AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    auto result = NodeList<GroundTag> {};
    get_successor_nodes(node, action, storage, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<GroundTag>::get_successor_nodes(const Source& node,
                                                        formalism::planning::ActionView<LiftedTag> action,
                                                        StateRepository<GroundTag>& storage,
                                                        AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                        NodeList<GroundTag>& out_nodes)
{
    auto& tree = m_impl->prepare(node, action, storage, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_successor(*this, node, tree, storage, axiom_evaluator, append(out_nodes));
}

NodeList<GroundTag, BuilderStateView<GroundTag>>
SuccessorGenerator<GroundTag>::get_successor_nodes(const Source& node, SuccessorBuilders<GroundTag>& storage, AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    auto result = NodeList<GroundTag, BuilderStateView<GroundTag>> {};
    get_successor_nodes(node, storage, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<GroundTag>::get_successor_nodes(const Source& node,
                                                        SuccessorBuilders<GroundTag>& storage,
                                                        AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                        NodeList<GroundTag, BuilderStateView<GroundTag>>& out_nodes)
{
    auto& tree = m_impl->prepare(node, std::nullopt, storage, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_successor(*this, node, tree, storage, axiom_evaluator, append(out_nodes));
}

NodeList<GroundTag, BuilderStateView<GroundTag>> SuccessorGenerator<GroundTag>::get_successor_nodes(const Source& node,
                                                                                                    formalism::planning::ActionView<LiftedTag> action,
                                                                                                    SuccessorBuilders<GroundTag>& storage,
                                                                                                    AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    auto result = NodeList<GroundTag, BuilderStateView<GroundTag>> {};
    get_successor_nodes(node, action, storage, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<GroundTag>::get_successor_nodes(const Source& node,
                                                        formalism::planning::ActionView<LiftedTag> action,
                                                        SuccessorBuilders<GroundTag>& storage,
                                                        AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                        NodeList<GroundTag, BuilderStateView<GroundTag>>& out_nodes)
{
    auto& tree = m_impl->prepare(node, action, storage, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_successor(*this, node, tree, storage, axiom_evaluator, append(out_nodes));
}

LabeledNodeList<GroundTag>
SuccessorGenerator<GroundTag>::get_labeled_successor_nodes(const Source& node, StateRepository<GroundTag>& storage, AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    auto result = LabeledNodeList<GroundTag> {};
    get_labeled_successor_nodes(node, storage, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<GroundTag>::get_labeled_successor_nodes(const Source& node,
                                                                StateRepository<GroundTag>& storage,
                                                                AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                LabeledNodeList<GroundTag>& out_nodes)
{
    auto& tree = m_impl->prepare(node, std::nullopt, storage, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_labeled_successor(*this, node, tree, storage, axiom_evaluator, append(out_nodes));
}

LabeledNodeList<GroundTag> SuccessorGenerator<GroundTag>::get_labeled_successor_nodes(const Source& node,
                                                                                      formalism::planning::ActionView<LiftedTag> action,
                                                                                      StateRepository<GroundTag>& storage,
                                                                                      AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    auto result = LabeledNodeList<GroundTag> {};
    get_labeled_successor_nodes(node, action, storage, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<GroundTag>::get_labeled_successor_nodes(const Source& node,
                                                                formalism::planning::ActionView<LiftedTag> action,
                                                                StateRepository<GroundTag>& storage,
                                                                AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                LabeledNodeList<GroundTag>& out_nodes)
{
    auto& tree = m_impl->prepare(node, action, storage, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_labeled_successor(*this, node, tree, storage, axiom_evaluator, append(out_nodes));
}

LabeledNodeList<GroundTag, BuilderStateView<GroundTag>> SuccessorGenerator<GroundTag>::get_labeled_successor_nodes(const Source& node,
                                                                                                                   SuccessorBuilders<GroundTag>& storage,
                                                                                                                   AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    auto result = LabeledNodeList<GroundTag, BuilderStateView<GroundTag>> {};
    get_labeled_successor_nodes(node, storage, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<GroundTag>::get_labeled_successor_nodes(const Source& node,
                                                                SuccessorBuilders<GroundTag>& storage,
                                                                AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                LabeledNodeList<GroundTag, BuilderStateView<GroundTag>>& out_nodes)
{
    auto& tree = m_impl->prepare(node, std::nullopt, storage, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_labeled_successor(*this, node, tree, storage, axiom_evaluator, append(out_nodes));
}

LabeledNodeList<GroundTag, BuilderStateView<GroundTag>>
SuccessorGenerator<GroundTag>::get_labeled_successor_nodes(const Source& node,
                                                           formalism::planning::ActionView<LiftedTag> action,
                                                           SuccessorBuilders<GroundTag>& storage,
                                                           AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    auto result = LabeledNodeList<GroundTag, BuilderStateView<GroundTag>> {};
    get_labeled_successor_nodes(node, action, storage, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<GroundTag>::get_labeled_successor_nodes(const Source& node,
                                                                formalism::planning::ActionView<LiftedTag> action,
                                                                SuccessorBuilders<GroundTag>& storage,
                                                                AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                LabeledNodeList<GroundTag, BuilderStateView<GroundTag>>& out_nodes)
{
    auto& tree = m_impl->prepare(node, action, storage, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_labeled_successor(*this, node, tree, storage, axiom_evaluator, append(out_nodes));
}

bool SuccessorGenerator<GroundTag>::for_each_successor_node(const Source& node,
                                                            StateRepository<GroundTag>& storage,
                                                            AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                            const std::function<bool(Node<GroundTag>)>& callback)
{
    auto& tree = m_impl->prepare(node, std::nullopt, storage, axiom_evaluator);
    return m_impl->for_each_successor(*this, node, tree, storage, axiom_evaluator, callback);
}

bool SuccessorGenerator<GroundTag>::for_each_successor_node(const Source& node,
                                                            formalism::planning::ActionView<LiftedTag> action,
                                                            StateRepository<GroundTag>& storage,
                                                            AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                            const std::function<bool(Node<GroundTag>)>& callback)
{
    auto& tree = m_impl->prepare(node, action, storage, axiom_evaluator);
    return m_impl->for_each_successor(*this, node, tree, storage, axiom_evaluator, callback);
}

bool SuccessorGenerator<GroundTag>::for_each_successor_node(const Source& node,
                                                            Builder& storage,
                                                            AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                            const std::function<bool(BuilderNode)>& callback)
{
    auto& tree = m_impl->prepare(node, std::nullopt, storage, axiom_evaluator);
    return m_impl->for_each_successor(*this, node, tree, storage, axiom_evaluator, callback);
}

bool SuccessorGenerator<GroundTag>::for_each_successor_node(const Source& node,
                                                            formalism::planning::ActionView<LiftedTag> action,
                                                            Builder& storage,
                                                            AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                            const std::function<bool(BuilderNode)>& callback)
{
    auto& tree = m_impl->prepare(node, action, storage, axiom_evaluator);
    return m_impl->for_each_successor(*this, node, tree, storage, axiom_evaluator, callback);
}

bool SuccessorGenerator<GroundTag>::for_each_labeled_successor_node(const Source& node,
                                                                    StateRepository<GroundTag>& storage,
                                                                    AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                    const std::function<bool(LabeledNode<GroundTag>)>& callback)
{
    auto& tree = m_impl->prepare(node, std::nullopt, storage, axiom_evaluator);
    return m_impl->for_each_labeled_successor(*this, node, tree, storage, axiom_evaluator, callback);
}

bool SuccessorGenerator<GroundTag>::for_each_labeled_successor_node(const Source& node,
                                                                    formalism::planning::ActionView<LiftedTag> action,
                                                                    StateRepository<GroundTag>& storage,
                                                                    AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                    const std::function<bool(LabeledNode<GroundTag>)>& callback)
{
    auto& tree = m_impl->prepare(node, action, storage, axiom_evaluator);
    return m_impl->for_each_labeled_successor(*this, node, tree, storage, axiom_evaluator, callback);
}

bool SuccessorGenerator<GroundTag>::for_each_labeled_successor_node(const Source& node,
                                                                    Builder& storage,
                                                                    AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                    const std::function<bool(LabeledNode<GroundTag, BuilderStateView<GroundTag>>)>& callback)
{
    auto& tree = m_impl->prepare(node, std::nullopt, storage, axiom_evaluator);
    return m_impl->for_each_labeled_successor(*this, node, tree, storage, axiom_evaluator, callback);
}

bool SuccessorGenerator<GroundTag>::for_each_labeled_successor_node(const Source& node,
                                                                    formalism::planning::ActionView<LiftedTag> action,
                                                                    Builder& storage,
                                                                    AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                    const std::function<bool(LabeledNode<GroundTag, BuilderStateView<GroundTag>>)>& callback)
{
    auto& tree = m_impl->prepare(node, action, storage, axiom_evaluator);
    return m_impl->for_each_labeled_successor(*this, node, tree, storage, axiom_evaluator, callback);
}

PackedNodeList<GroundTag> SuccessorGenerator<GroundTag>::get_packed_successor_nodes(const Source& node,
                                                                                    StateRepository<GroundTag>& state_repository,
                                                                                    AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    auto result = PackedNodeList<GroundTag> {};
    get_packed_successor_nodes(node, state_repository, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<GroundTag>::get_packed_successor_nodes(const Source& node,
                                                               StateRepository<GroundTag>& state_repository,
                                                               AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                               PackedNodeList<GroundTag>& out_nodes)
{
    auto& tree = m_impl->prepare(node, std::nullopt, state_repository, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_successor(*this, node, tree, state_repository, axiom_evaluator, append_packed(out_nodes));
}

PackedNodeList<GroundTag> SuccessorGenerator<GroundTag>::get_packed_successor_nodes(const Source& node,
                                                                                    formalism::planning::ActionView<LiftedTag> action,
                                                                                    StateRepository<GroundTag>& state_repository,
                                                                                    AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    auto result = PackedNodeList<GroundTag> {};
    get_packed_successor_nodes(node, action, state_repository, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<GroundTag>::get_packed_successor_nodes(const Source& node,
                                                               formalism::planning::ActionView<LiftedTag> action,
                                                               StateRepository<GroundTag>& state_repository,
                                                               AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                               PackedNodeList<GroundTag>& out_nodes)
{
    auto& tree = m_impl->prepare(node, action, state_repository, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_successor(*this, node, tree, state_repository, axiom_evaluator, append_packed(out_nodes));
}

PackedLabeledNodeList<GroundTag> SuccessorGenerator<GroundTag>::get_packed_labeled_successor_nodes(const Source& node,
                                                                                                   StateRepository<GroundTag>& state_repository,
                                                                                                   AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    auto result = PackedLabeledNodeList<GroundTag> {};
    get_packed_labeled_successor_nodes(node, state_repository, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<GroundTag>::get_packed_labeled_successor_nodes(const Source& node,
                                                                       StateRepository<GroundTag>& state_repository,
                                                                       AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                       PackedLabeledNodeList<GroundTag>& out_nodes)
{
    auto& tree = m_impl->prepare(node, std::nullopt, state_repository, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_labeled_successor(*this, node, tree, state_repository, axiom_evaluator, append_packed(out_nodes));
}

PackedLabeledNodeList<GroundTag> SuccessorGenerator<GroundTag>::get_packed_labeled_successor_nodes(const Source& node,
                                                                                                   formalism::planning::ActionView<LiftedTag> action,
                                                                                                   StateRepository<GroundTag>& state_repository,
                                                                                                   AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    auto result = PackedLabeledNodeList<GroundTag> {};
    get_packed_labeled_successor_nodes(node, action, state_repository, axiom_evaluator, result);
    return result;
}

void SuccessorGenerator<GroundTag>::get_packed_labeled_successor_nodes(const Source& node,
                                                                       formalism::planning::ActionView<LiftedTag> action,
                                                                       StateRepository<GroundTag>& state_repository,
                                                                       AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                       PackedLabeledNodeList<GroundTag>& out_nodes)
{
    auto& tree = m_impl->prepare(node, action, state_repository, axiom_evaluator);
    out_nodes.clear();
    m_impl->for_each_labeled_successor(*this, node, tree, state_repository, axiom_evaluator, append_packed(out_nodes));
}
template<typename Objects>
ActionBindingResult
SuccessorGenerator<GroundTag>::Impl::try_get_applicable_action_binding(const Source& node, formalism::planning::ActionView<LiftedTag> action, Objects objects)
{
    const auto& task = definition->task;
    validate_task(task, node);
    get_schema_match_tree(action);
    if (objects.size() != action.get_arity())
        throw std::invalid_argument("SuccessorGenerator::check_action_binding(...): object count does not match action arity.");

    const auto indices = objects.base();
    const auto& source_repository = objects.context();
    const auto& target_repository = *task->get_repository();
    if (!std::ranges::all_of(indices, [&](const auto& value) { return source_repository.contains(value); })
        || !std::ranges::all_of(indices, [&](const auto& value) { return target_repository.contains(value); }))
        return { ActionBindingStatus::INAPPLICABLE, std::nullopt };
    if (!std::ranges::all_of(objects, [&](const auto& value) { return ygg::formalism::contains(target_repository, value); }))
        throw std::invalid_argument("SuccessorGenerator::check_action_binding(...): object does not belong to the task repository.");

    const auto& constants = task->get_task().get_domain().data().constants;
    const auto& task_objects = task->get_task().data().objects;
    for (const auto object : indices)
        if (!std::binary_search(constants.begin(), constants.end(), object) && !std::binary_search(task_objects.begin(), task_objects.end(), object))
            return { ActionBindingStatus::INAPPLICABLE, std::nullopt };

    auto& scratch = evaluator.checked_binding;
    scratch.relation = action.index();
    scratch.values.clear();
    for (const auto object : indices)
        scratch.values.push_back(object.value);
    const auto binding = task->get_repository()->find(scratch);
    if (!binding)
        return { ActionBindingStatus::INAPPLICABLE, std::nullopt };
    const auto found = definition->action_binding_to_ground_action.find(*binding);
    if (found == definition->action_binding_to_ground_action.end())
        return { ActionBindingStatus::INAPPLICABLE, std::nullopt };
    const auto state = StateContext<GroundTag>(*task, node.data(), node.get_metric());
    if (!evaluator.executor.is_applicable(found->second, state))
        return { ActionBindingStatus::INAPPLICABLE, std::nullopt };
    return { ActionBindingStatus::APPLICABLE, binding };
}

template<ygg::RowViewConcept<formalism::planning::Action<LiftedTag>> Binding>
ActionBindingResult SuccessorGenerator<GroundTag>::try_get_applicable_action_binding(const Source& node, Binding binding)
{
    if constexpr (requires { binding.context().owner(binding.data()); })
        if (binding.context().owner(binding.data()) == nullptr)
            throw std::invalid_argument("SuccessorGenerator: action binding does not belong to its source repository.");
    return m_impl->try_get_applicable_action_binding(
        node,
        binding.get_relation(),
        urd::resolve(urd::transform(binding.get_values(), [](ygg::Value value) { return urd::index<formalism::Object>(static_cast<std::uint32_t>(value)); }),
                     binding.context()));
}

template<ygg::RowViewConcept<formalism::planning::Action<LiftedTag>> Binding>
ActionBindingStatus SuccessorGenerator<GroundTag>::check_action_binding(const Source& node, Binding binding)
{ return try_get_applicable_action_binding(node, binding).status; }

formalism::planning::ActionRowView SuccessorGenerator<GroundTag>::materialize_action_binding(BorrowedActionRowView<GroundTag> binding)
{
    const auto& repository = *m_impl->definition->task->get_repository();
    if (!ygg::formalism::contains(repository, binding))
        throw std::invalid_argument("SuccessorGenerator: action binding does not belong to the task repository.");
    // Bindings in the grounded task already have validated schemas and objects.
    if (m_impl->definition->action_binding_to_ground_action.contains(binding))
        return binding;
    const auto action = binding.get_relation();
    m_impl->get_schema_match_tree(action);
    const auto objects = urd::resolve(binding.get<urd::index<formalism::Object>>(), binding.context());
    if (objects.size() != action.get_arity())
        throw std::invalid_argument("SuccessorGenerator: object count does not match action arity.");
    if (!std::ranges::all_of(objects, [&](const auto& value) { return ygg::formalism::contains(repository, value); }))
        throw std::invalid_argument("SuccessorGenerator: object does not belong to the task repository.");
    return binding;
}

Node<GroundTag> SuccessorGenerator<GroundTag>::get_successor_node(const Source& node,
                                                                  formalism::planning::ActionRowView binding,
                                                                  StateRepository<GroundTag>& storage,
                                                                  AxiomEvaluator<GroundTag>& axiom_evaluator)
{ return get_successor_node(node, ground_action(binding), storage, axiom_evaluator); }

BuilderNode SuccessorGenerator<GroundTag>::get_successor_node(const Source& node,
                                                              formalism::planning::ActionRowView binding,
                                                              Builder& storage,
                                                              AxiomEvaluator<GroundTag>& axiom_evaluator)
{ return get_successor_node(node, ground_action(binding), storage, axiom_evaluator); }

/// The registering wrapper: a fresh builder of the repository, the core, then registration, which closes the state under
/// the axioms only if it is new.
Node<GroundTag> SuccessorGenerator<GroundTag>::get_successor_node(const Source& node,
                                                                  formalism::planning::ActionView<GroundTag> action,
                                                                  StateRepository<GroundTag>& storage,
                                                                  AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    m_impl->validate(node, storage, axiom_evaluator);
    auto successor_state = storage.get_state_builder();
    auto& delta = m_impl->evaluator.successor_delta;
    const auto auxiliary_value = m_impl->generate(node, action, *successor_state, delta);
    if (const auto* parent = node.registered())
        return finalize_successor_state(storage, axiom_evaluator, std::move(successor_state), auxiliary_value, *parent, delta);
    return finalize_successor_state(storage, axiom_evaluator, std::move(successor_state), auxiliary_value);
}

/// The core into a caller's builder, closed under the axioms.
BuilderNode SuccessorGenerator<GroundTag>::get_successor_node(const Source& node,
                                                              formalism::planning::ActionView<GroundTag> action,
                                                              Builder& storage,
                                                              AxiomEvaluator<GroundTag>& axiom_evaluator)
{
    m_impl->validate(node, storage, axiom_evaluator);
    storage.clear();
    const auto auxiliary_value = m_impl->generate(node, action, storage, m_impl->evaluator.successor_delta);
    const auto metric = evaluate_successor_metric(*m_impl->definition->task, storage, auxiliary_value);
    axiom_evaluator.compute_extended_state(storage);
    return BuilderNode(urd::resolve(storage, node.get_state_repository()), metric);
}

Bindings SuccessorGenerator<GroundTag>::get_applicable_action_bindings(const Source& node)
{
    auto result = Bindings {};
    get_applicable_action_bindings(node, result);
    return result;
}

void SuccessorGenerator<GroundTag>::get_applicable_action_bindings(const Source& node, Bindings& out_bindings)
{
    validate_task(m_impl->definition->task, node);
    out_bindings.clear();
    for_each_applicable_action_binding(node, append(out_bindings));
}

Bindings SuccessorGenerator<GroundTag>::get_applicable_action_bindings(const Source& node, formalism::planning::ActionView<LiftedTag> action)
{
    auto result = Bindings {};
    get_applicable_action_bindings(node, action, result);
    return result;
}

void SuccessorGenerator<GroundTag>::get_applicable_action_bindings(const Source& node,
                                                                   formalism::planning::ActionView<LiftedTag> action,
                                                                   Bindings& out_bindings)
{
    validate_task(m_impl->definition->task, node);
    static_cast<void>(m_impl->get_schema_match_tree(action));
    out_bindings.clear();
    for_each_applicable_action_binding(node, action, append(out_bindings));
}

bool SuccessorGenerator<GroundTag>::for_each_applicable_action_binding(const Source& node,
                                                                       const std::function<bool(formalism::planning::ActionRowView)>& callback)
{
    validate_task(m_impl->definition->task, node);
    return m_impl->for_each_applicable_action(node, *m_impl->evaluator.action_match_tree, [&](const auto action) { return callback(action.get_row()); });
}

bool SuccessorGenerator<GroundTag>::for_each_applicable_action_binding(const Source& node,
                                                                       formalism::planning::ActionView<LiftedTag> action,
                                                                       const std::function<bool(formalism::planning::ActionRowView)>& callback)
{
    validate_task(m_impl->definition->task, node);
    auto& tree = m_impl->get_schema_match_tree(action);
    return m_impl->for_each_applicable_action(node, tree, [&](const auto ground_action) { return callback(ground_action.get_row()); });
}

bool SuccessorGenerator<GroundTag>::for_each_borrowed_applicable_action_binding(const Source& node,
                                                                                const std::function<bool(BorrowedActionRowView<GroundTag>)>& callback)
{ return for_each_applicable_action_binding(node, callback); }

bool SuccessorGenerator<GroundTag>::for_each_borrowed_applicable_action_binding(const Source& node,
                                                                                formalism::planning::ActionView<LiftedTag> action,
                                                                                const std::function<bool(BorrowedActionRowView<GroundTag>)>& callback)
{ return for_each_applicable_action_binding(node, action, callback); }

bool SuccessorGenerator<GroundTag>::for_each_borrowed_applicable_action_binding(const Source& node,
                                                                                formalism::planning::ActionView<LiftedTag> action,
                                                                                const std::function<bool(BorrowedActionRowView<GroundTag>)>& accept,
                                                                                const std::function<bool(BorrowedActionRowView<GroundTag>)>& callback,
                                                                                const std::function<bool()>& stop)
{
    validate_task(m_impl->definition->task, node);
    auto& tree = m_impl->get_schema_match_tree(action);
    return m_impl->for_each_applicable_action(
        node,
        tree,
        [&](const auto candidate) { return accept(candidate.get_row()); },
        [&](const auto candidate) { return callback(candidate.get_row()); },
        [&] { return stop(); });
}

PackedNode<GroundTag> SuccessorGenerator<GroundTag>::get_packed_initial_node(StateRepository<GroundTag>& state_repository,
                                                                             AxiomEvaluator<GroundTag>& axiom_evaluator)
{ return get_initial_node(state_repository, axiom_evaluator).pack(); }

PackedNode<GroundTag> SuccessorGenerator<GroundTag>::get_packed_node(StateRepository<GroundTag>& state_repository, urd::index<State<GroundTag>> state_index)
{ return get_node(state_repository, state_index).pack(); }

PackedNode<GroundTag> SuccessorGenerator<GroundTag>::get_packed_successor_node(const Source& node,
                                                                               formalism::planning::ActionRowView binding,
                                                                               StateRepository<GroundTag>& state_repository,
                                                                               AxiomEvaluator<GroundTag>& axiom_evaluator)
{ return get_successor_node(node, binding, state_repository, axiom_evaluator).pack(); }

PackedNode<GroundTag> SuccessorGenerator<GroundTag>::get_packed_successor_node(const Source& node,
                                                                               formalism::planning::ActionView<GroundTag> action,
                                                                               StateRepository<GroundTag>& state_repository,
                                                                               AxiomEvaluator<GroundTag>& axiom_evaluator)
{ return get_successor_node(node, action, state_repository, axiom_evaluator).pack(); }

ygg::float_t SuccessorGenerator<GroundTag>::generate_successor_state(const Source& node,
                                                                     formalism::planning::ActionRowView binding,
                                                                     Builder& out_state,
                                                                     StateDelta& out_delta)
{
    validate_task(m_impl->definition->task, node);
    return m_impl->generate(node, ground_action(binding), out_state, out_delta);
}

Node<GroundTag> SuccessorGenerator<GroundTag>::finalize_successor_state(StateRepository<GroundTag>& state_repository,
                                                                        AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                        std::shared_ptr<urd::builder<State<GroundTag>>> state,
                                                                        ygg::float_t auxiliary_value)
{
    validate_task(m_impl->definition->task, state_repository);
    validate_task(m_impl->definition->task, axiom_evaluator);
    const auto metric = evaluate_successor_metric(*m_impl->definition->task, *state, auxiliary_value);
    return Node<GroundTag>(state_repository.register_state(axiom_evaluator, std::move(state)), metric);
}

Node<GroundTag> SuccessorGenerator<GroundTag>::finalize_successor_state(StateRepository<GroundTag>& state_repository,
                                                                        AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                                        std::shared_ptr<urd::builder<State<GroundTag>>> state,
                                                                        ygg::float_t auxiliary_value,
                                                                        const StateView<GroundTag>& parent,
                                                                        const StateDelta& delta)
{
    validate_task(m_impl->definition->task, state_repository);
    validate_task(m_impl->definition->task, axiom_evaluator);
    const auto metric = evaluate_successor_metric(*m_impl->definition->task, *state, auxiliary_value);
    return Node<GroundTag>(state_repository.register_state(axiom_evaluator, std::move(state), parent, delta), metric);
}

formalism::planning::ActionView<GroundTag> SuccessorGenerator<GroundTag>::ground_action(formalism::planning::ActionRowView binding) const
{
    const auto it = m_impl->definition->action_binding_to_ground_action.find(binding);
    assert(it != m_impl->definition->action_binding_to_ground_action.end() && "Ground action binding not found.");
    return it->second;
}

Node<GroundTag> SuccessorGenerator<GroundTag>::get_node(StateRepository<GroundTag>& state_repository, urd::index<State<GroundTag>> state_index)
{
    validate_task(m_impl->definition->task, state_repository);
    auto state = state_repository.get_registered_state(state_index);
    const auto state_context = StateContext<GroundTag>(*m_impl->definition->task, state.data(), 0);
    const auto state_metric =
        evaluate_metric(m_impl->definition->task->get_task().get_metric(), m_impl->definition->task->get_task().get_auxiliary_fterm_value(), state_context);

    return Node<GroundTag>(std::move(state), state_metric);
}

const TaskPtr<GroundTag>& SuccessorGenerator<GroundTag>::get_task() const noexcept { return m_impl->definition->task; }

template ActionBindingStatus SuccessorGenerator<GroundTag>::check_action_binding(const Source& node, formalism::planning::ActionRowView binding);
template ActionBindingStatus SuccessorGenerator<GroundTag>::check_action_binding(const Source& node, formalism::planning::ActionRowDataView binding);
template ActionBindingResult SuccessorGenerator<GroundTag>::try_get_applicable_action_binding(const Source& node, formalism::planning::ActionRowView binding);
template ActionBindingResult SuccessorGenerator<GroundTag>::try_get_applicable_action_binding(const Source& node,
                                                                                              formalism::planning::ActionRowDataView binding);

}
