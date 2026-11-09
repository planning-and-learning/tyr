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

#include "tyr/planning/heuristics/lmcut.hpp"

#include "../ground/heuristics/rpg.hpp"
#include "../lifted/heuristics/rpg.hpp"
#include "rpg.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/policies/cost.hpp"
#include "tyr/datalog/policies/numeric_support.hpp"
#include "tyr/datalog/policies/termination.hpp"

#include <algorithm>
#include <cassert>
#include <limits>
#include <memory>
#include <optional>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/semantics/comparison.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::planning
{
namespace
{

/// A numeric vertex of the justification graph: a function binding at an interval.
struct LMCutNumericNode : ygg::comparison::Mixin<LMCutNumericNode>
{
    formalism::datalog::FunctionBindingIndex<formalism::FluentTag> key;
    ygg::ClosedInterval<ygg::float_t> interval;

    LMCutNumericNode() = delete;
    LMCutNumericNode(formalism::datalog::FunctionBindingIndex<formalism::FluentTag> key_, ygg::ClosedInterval<ygg::float_t> interval_) :
        key(key_),
        interval(std::move(interval_))
    {
    }

    [[nodiscard]] auto identifying_members() const noexcept { return std::make_tuple(key, lower(interval), upper(interval)); }
};

template<TaskKind Kind>
bool requires_numeric_edge_cuts(formalism::datalog::ProgramView<Kind> program)
{
    for (const auto rule : program.template get_rules<formalism::PredicateTag>())
        if (!rule.get_body().get_numeric_constraints().empty())
            return true;
    return !program.template get_rules<formalism::FunctionTag>().empty();
}

/// LM-cut (Helmert & Domshlak, ICAPS 2009) on the justification graph of the recorded h^max achievers.
///
/// The vertices are the facts, and every achiever is an arc from its precondition choice, the first body atom of maximal
/// h^max cost, to its head, weighted by the achiever's residual cost. A round seeds the goal zone with the first goal fact of
/// maximal cost and closes it backwards over the zero-cost arcs; the before-goal zone holds the vertices the state reaches
/// without entering the goal zone; the cut holds every action arc from the before-goal zone into the goal zone. The least
/// residual cost of the cut is added to the value and charged to the cut actions, and the rounds continue until the goal
/// costs nothing.
///
/// Tasks with numeric conditions are outside the paper. They cut rule and numeric edges instead of actions, keep the
/// goal-directed evaluation, restrict the graph to the h^max-optimal achievers, and let an arc start at every maximal body
/// atom and numeric support, since a numeric support selection stands for alternatives rather than a conjunction.
template<TaskKind Kind>
struct LMCutImplementation :
    detail::RPGEvaluator<LMCutImplementation<Kind>,
                         Kind,
                         datalog::MinCostAnnotationWithAchieversPolicy<datalog::MaxAggregation>,
                         datalog::TerminationPolicy<datalog::MaxAggregation>,
                         datalog::RuleCostOverridePolicy<Kind>>
{
    using Base = detail::RPGEvaluator<LMCutImplementation<Kind>,
                                      Kind,
                                      datalog::MinCostAnnotationWithAchieversPolicy<datalog::MaxAggregation>,
                                      datalog::TerminationPolicy<datalog::MaxAggregation>,
                                      datalog::RuleCostOverridePolicy<Kind>>;
    using ActionBinding = formalism::planning::ActionBindingView;
    using PredicateHead = formalism::datalog::PredicateBindingView<formalism::FluentTag>;
    using NumericNode = LMCutNumericNode;
    using Node = std::variant<PredicateHead, NumericNode>;
    using RuleEdge = formalism::datalog::RuleBindingIndex<formalism::PredicateTag>;
    using NumericEdge = datalog::NumericTransitionCostKey<formalism::FunctionTag>;
    using CutFrontierAtoms = formalism::planning::AtomViewList<GroundTag, formalism::FluentTag>;

    /// The arcs of an achiever: their residual cost and their sources, empty for a rule without preconditions.
    struct Arc
    {
        datalog::Cost cost;
        std::vector<Node> sources;
    };

    LMCutImplementation(std::shared_ptr<detail::RPGDefinition<Kind>> definition, ygg::ExecutionContextPtr execution_context, bool use_numeric_edge_cuts);
    LMCutImplementation(const LMCutImplementation& source, ygg::ExecutionContextPtr execution_context);
    ygg::float_t evaluate(const ygg::Builder<State<Kind>>& state, CutFrontierAtoms* cut_frontier_atoms = nullptr);

private:
    template<formalism::RelationKind R>
    const Arc& get_arc(const datalog::WitnessAnnotation<R>& witness);
    template<typename Callback>
    void for_each_arc(const Node& head, Callback callback);
    [[nodiscard]] bool is_state_node(const Node& node) const noexcept;
    [[nodiscard]] const datalog::WitnessAnnotation<formalism::FunctionTag>* get_numeric_witness(const NumericNode& node) const noexcept;
    void seed_goal_zone();
    void mark_goal_zone();
    void mark_before_goal_zone();
    void extract_cut(CutFrontierAtoms* cut_frontier_atoms);
    void apply_residual_costs();
    void clear_round();
    [[nodiscard]] bool use_numeric_edge_cuts() const noexcept { return m_use_numeric_edge_cuts; }

    bool m_use_numeric_edge_cuts;
    /// The costs the cuts of this evaluation charged so far.
    ygg::UnorderedMap<ActionBinding, datalog::Cost> m_action_credits;
    ygg::UnorderedMap<RuleEdge, datalog::Cost> m_rule_edge_credits;
    ygg::UnorderedMap<NumericEdge, datalog::Cost> m_numeric_edge_credits;
    /// The zones of the current round. The dependents of a node are the explored nodes with an arc from it.
    ygg::UnorderedSet<Node> m_goal_zone;
    ygg::UnorderedSet<Node> m_before_goal_zone;
    ygg::UnorderedSet<Node> m_explored;
    ygg::UnorderedMap<Node, std::vector<Node>> m_dependents;
    std::vector<Node> m_stack;
    std::vector<Node> m_base;
    Arc m_arc;
    /// The cut of the current round and its least residual cost.
    datalog::Cost m_cut_cost;
    ygg::UnorderedSet<ActionBinding> m_cut;
    ygg::UnorderedSet<RuleEdge> m_rule_cut;
    ygg::UnorderedSet<NumericEdge> m_numeric_cut;
    datalog::NumericSupportSelectorWorkspace m_numeric_support_selector_workspace;
};

template<TaskKind Kind>
LMCutImplementation<Kind>::LMCutImplementation(std::shared_ptr<detail::RPGDefinition<Kind>> definition,
                                               ygg::ExecutionContextPtr execution_context,
                                               bool use_numeric_edge_cuts) :
    Base(std::move(definition), std::move(execution_context)),
    m_use_numeric_edge_cuts(use_numeric_edge_cuts),
    m_cut_cost(std::numeric_limits<datalog::Cost>::max())
{
    // Classic action cuts require every reachable operator arc. Numeric-edge
    // cuts retain goal-directed evaluation to avoid unbounded numeric closure.
    this->workspace().tp.set_early_termination(m_use_numeric_edge_cuts);
}

template<TaskKind Kind>
LMCutImplementation<Kind>::LMCutImplementation(const LMCutImplementation& source, ygg::ExecutionContextPtr execution_context) :
    Base(source, std::move(execution_context)),
    m_use_numeric_edge_cuts(source.m_use_numeric_edge_cuts),
    m_cut_cost(std::numeric_limits<datalog::Cost>::max())
{ this->workspace().tp.set_early_termination(m_use_numeric_edge_cuts); }

template<TaskKind Kind>
ygg::float_t LMCutImplementation<Kind>::evaluate(const ygg::Builder<State<Kind>>& state, CutFrontierAtoms* cut_frontier_atoms)
{
    auto value = static_cast<datalog::Cost>(0);
    m_action_credits.clear();
    m_rule_edge_credits.clear();
    m_numeric_edge_credits.clear();
    this->begin_state_evaluation();

    while (true)
    {
        apply_residual_costs();
        const auto hmax = this->evaluate_current_state(state);
        if (hmax == std::numeric_limits<ygg::float_t>::infinity())
            return hmax;

        const auto hmax_cost = static_cast<datalog::Cost>(hmax);
        if (hmax_cost == 0)
            return static_cast<ygg::float_t>(value);

        extract_cut(cut_frontier_atoms);
        if (m_cut.empty() && m_rule_cut.empty() && m_numeric_cut.empty())
            return static_cast<ygg::float_t>(value + hmax_cost);

        assert(m_cut_cost > 0 && m_cut_cost != std::numeric_limits<datalog::Cost>::max());

        value += m_cut_cost;
        for (const auto action_binding : m_cut)
            m_action_credits[action_binding] += m_cut_cost;
        for (const auto edge : m_rule_cut)
            m_rule_edge_credits[edge] += m_cut_cost;
        for (const auto& edge : m_numeric_cut)
            m_numeric_edge_credits[edge] += m_cut_cost;
    }
}

template<TaskKind Kind>
void LMCutImplementation<Kind>::apply_residual_costs()
{
    this->workspace().clear_costs();
    for (const auto& [action_binding, credit] : m_action_credits)
        this->set_action_binding_cost(action_binding, credit);
    for (const auto& [edge, credit] : m_rule_edge_credits)
        this->workspace().cost_policy.set_cost(edge, credit);
    for (const auto& [edge, credit] : m_numeric_edge_credits)
        this->workspace().cost_policy.set_cost(edge.rule_key, edge.numeric_key, edge.interval, credit);
}

/// The result is valid until the next call.
template<TaskKind Kind>
template<formalism::RelationKind R>
const typename LMCutImplementation<Kind>::Arc& LMCutImplementation<Kind>::get_arc(const datalog::WitnessAnnotation<R>& witness)
{
    auto& sources = m_arc.sources;
    sources.clear();
    auto body_cost = static_cast<datalog::Cost>(0);
    const auto consider = [&](datalog::Cost cost, auto&& node)
    {
        if (!sources.empty() && cost < body_cost)
            return;
        if (sources.empty() || cost > body_cost)
        {
            body_cost = cost;
            sources.clear();
        }
        // The classical path keeps the first maximal precondition, the numeric path every one.
        if (sources.empty() || use_numeric_edge_cuts())
            sources.emplace_back(std::forward<decltype(node)>(node));
    };
    this->for_each_witness_precondition(witness, [&](const auto precondition) { consider(this->get_predicate_cost(precondition), precondition); });
    for (const auto& support : witness.get_numeric_supports())
        consider(support.get_cost(), NumericNode { support.get_key(), support.get_interval() });

    m_arc.cost = witness.get_cost() > body_cost ? witness.get_cost() - body_cost : static_cast<datalog::Cost>(0);
    return m_arc;
}

template<TaskKind Kind>
template<typename Callback>
void LMCutImplementation<Kind>::for_each_arc(const Node& head, Callback callback)
{
    std::visit(
        [&](const auto& key)
        {
            if constexpr (std::same_as<std::remove_cvref_t<decltype(key)>, PredicateHead>)
            {
                const auto head_cost = this->get_predicate_cost(key);
                this->for_each_achiever(key,
                                        [&](const auto& witness)
                                        {
                                            if (use_numeric_edge_cuts() && witness.get_cost() != head_cost)
                                                return;
                                            callback(witness);
                                        });
            }
            else
            {
                if (const auto* witness = get_numeric_witness(key))
                    callback(*witness);
            }
        },
        head);
}

/// A node of the state: a state fact or a numeric node without a derivation. Every path of the justification graph starts here.
template<TaskKind Kind>
bool LMCutImplementation<Kind>::is_state_node(const Node& node) const noexcept
{
    return std::visit(
        [&](const auto& key)
        {
            if constexpr (std::same_as<std::remove_cvref_t<decltype(key)>, PredicateHead>)
            {
                const auto* annotation = this->workspace().annotations.find(key.get_index());
                return annotation && std::holds_alternative<datalog::BaseAnnotation>(*annotation);
            }
            else
                return get_numeric_witness(key) == nullptr;
        },
        node);
}

template<TaskKind Kind>
const datalog::WitnessAnnotation<formalism::FunctionTag>* LMCutImplementation<Kind>::get_numeric_witness(const NumericNode& node) const noexcept
{
    const auto* annotation = this->workspace().numeric_annotations.find(node.key, node.interval);
    return annotation ? std::get_if<datalog::WitnessAnnotation<formalism::FunctionTag>>(annotation) : nullptr;
}

/// Seeds the goal zone with the goal's precondition choice: the first goal fact of maximal cost, else the maximal supports
/// of the maximal numeric constraints.
template<TaskKind Kind>
void LMCutImplementation<Kind>::seed_goal_zone()
{
    const auto seed = [&](Node node)
    {
        if (m_goal_zone.insert(node).second)
            m_stack.push_back(std::move(node));
    };
    const auto& goal = this->workspace().tp.get_goal();
    if (!goal)
        return;

    const auto goal_cost = this->get_goal_cost();
    for (const auto literal : goal->template get_literals<formalism::FluentTag>())
    {
        const auto head = literal.get_atom().get_row();
        if (literal.get_polarity() && this->get_predicate_cost(head) == goal_cost)
        {
            seed(head);
            return;
        }
    }

    const auto& selector = this->workspace().get_numeric_support_selector();
    auto& selection = m_numeric_support_selector_workspace.selection;
    for (const auto constraint : goal->get_numeric_constraints())
    {
        if (selector.get_constraint_cost(constraint, selection, datalog::MaxAggregation {}) != goal_cost)
            continue;

        for (const auto& entry : selection)
        {
            if (entry.cost != goal_cost)
                continue;

            [[maybe_unused]] const auto found = selector.for_each_entry_support(entry,
                                                                                [&](const auto key, const auto& interval, const auto& annotation)
                                                                                {
                                                                                    if (datalog::get_cost(annotation) == goal_cost)
                                                                                        seed(NumericNode { key, interval });
                                                                                });
            assert(found);
        }
    }
}

/// Closes the goal zone backwards over the zero-cost arcs from the seeds on the stack.
template<TaskKind Kind>
void LMCutImplementation<Kind>::mark_goal_zone()
{
    while (!m_stack.empty())
    {
        const auto node = m_stack.back();
        m_stack.pop_back();
        for_each_arc(node,
                     [&](const auto& witness)
                     {
                         const auto& arc = get_arc(witness);
                         if (arc.cost > 0)
                             return;
                         for (const auto& source : arc.sources)
                             if (m_goal_zone.insert(source).second)
                                 m_stack.push_back(source);
                     });
    }
}

/// The before-goal zone: the nodes the state reaches without entering the goal zone, among the nodes the cut queries. The
/// sources of the action arcs into the goal zone are explored backwards once, recording for each node the explored nodes it
/// has an arc to, and reachability is then propagated forwards from the state nodes, so that cycles do not cut it off.
template<TaskKind Kind>
void LMCutImplementation<Kind>::mark_before_goal_zone()
{
    for (const auto& head : m_goal_zone)
        for_each_arc(head,
                     [&](const auto& witness)
                     {
                         if (!this->is_action_witness(witness))
                             return;
                         const auto& arc = get_arc(witness);
                         if (arc.cost == 0)
                             return;
                         for (const auto& source : arc.sources)
                             if (!m_goal_zone.contains(source))
                                 m_stack.push_back(source);
                     });

    while (!m_stack.empty())
    {
        const auto node = m_stack.back();
        m_stack.pop_back();
        if (!m_explored.insert(node).second)
            continue;
        if (is_state_node(node))
        {
            m_base.push_back(node);
            continue;
        }

        auto has_arc = false;
        for_each_arc(node,
                     [&](const auto& witness)
                     {
                         has_arc = true;
                         const auto& arc = get_arc(witness);
                         if (arc.sources.empty())
                         {
                             m_base.push_back(node);
                             return;
                         }
                         for (const auto& source : arc.sources)
                         {
                             if (m_goal_zone.contains(source))
                                 continue;
                             m_dependents[source].push_back(node);
                             m_stack.push_back(source);
                         }
                     });
        if (!has_arc)
            m_base.push_back(node);
    }

    while (!m_base.empty())
    {
        const auto node = m_base.back();
        m_base.pop_back();
        if (!m_before_goal_zone.insert(node).second)
            continue;
        if (const auto dependents = m_dependents.find(node); dependents != m_dependents.end())
            m_base.insert(m_base.end(), dependents->second.begin(), dependents->second.end());
    }
}

/// The cut: the action arcs of positive cost from the before-goal zone into the goal zone.
template<TaskKind Kind>
void LMCutImplementation<Kind>::extract_cut(CutFrontierAtoms* cut_frontier_atoms)
{
    clear_round();
    seed_goal_zone();
    mark_goal_zone();
    mark_before_goal_zone();

    for (const auto& head : m_goal_zone)
    {
        for_each_arc(head,
                     [&](const auto& witness)
                     {
                         if (!this->is_action_witness(witness))
                             return;
                         const auto& arc = get_arc(witness);
                         const auto crosses = arc.sources.empty()
                                              || std::ranges::any_of(arc.sources, [&](const auto& source) { return m_before_goal_zone.contains(source); });
                         if (arc.cost == 0 || !crosses)
                             return;

                         m_cut_cost = std::min(m_cut_cost, arc.cost);
                         if (!use_numeric_edge_cuts())
                             this->for_each_action_binding(witness, [&](const auto action_binding) { m_cut.insert(action_binding); });
                         else if constexpr (std::same_as<std::remove_cvref_t<decltype(witness)>, datalog::WitnessAnnotation<formalism::PredicateTag>>)
                             m_rule_cut.insert(witness.get_rule_key());
                         else
                         {
                             const auto& node = std::get<NumericNode>(head);
                             m_numeric_cut.insert(NumericEdge { witness.get_rule_key(), node.key, node.interval });
                         }

                         if (cut_frontier_atoms)
                             if (const auto* frontier = std::get_if<PredicateHead>(&head))
                                 this->append_planning_cut_frontier_atom(*frontier, *cut_frontier_atoms);
                     });
    }
}

template<TaskKind Kind>
void LMCutImplementation<Kind>::clear_round()
{
    m_goal_zone.clear();
    m_before_goal_zone.clear();
    m_explored.clear();
    m_dependents.clear();
    m_stack.clear();
    m_base.clear();
    m_cut_cost = std::numeric_limits<datalog::Cost>::max();
    m_arc.sources.clear();
    m_cut.clear();
    m_rule_cut.clear();
    m_numeric_cut.clear();
    m_numeric_support_selector_workspace.clear();
}

}

template<TaskKind Kind>
struct LMCutHeuristic<Kind>::Impl
{
    using Implementation = LMCutImplementation<Kind>;
    using CutFrontierAtoms = formalism::planning::AtomViewList<GroundTag, formalism::FluentTag>;

    Impl(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) :
        m_implementation(make_implementation(std::move(task), std::move(execution_context), cost_mode))
    {
    }

    Impl(const Impl& source, ygg::ExecutionContextPtr execution_context) : m_implementation(source.m_implementation, std::move(execution_context)) {}

    void set_goal(formalism::planning::ConjunctiveConditionView<GroundTag> goal) { m_implementation.set_goal(goal); }

    ygg::float_t evaluate(const ygg::Builder<State<Kind>>& state, CutFrontierAtoms* cut_frontier_atoms = nullptr)
    { return m_implementation.evaluate(state, cut_frontier_atoms); }

    void print_summary(size_t verbosity) const { m_implementation.print_summary(verbosity); }

private:
    static Implementation make_implementation(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode)
    {
        auto definition = std::make_shared<detail::RPGDefinition<Kind>>(std::move(task), cost_mode);
        const auto numeric_edge_cuts = requires_numeric_edge_cuts(definition->rpg_program.get_datalog_program().get_program());
        return Implementation(std::move(definition), std::move(execution_context), numeric_edge_cuts);
    }

    Implementation m_implementation;
};

template<TaskKind Kind>
LMCutHeuristic<Kind>::LMCutHeuristic(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) :
    m_impl(std::make_unique<Impl>(std::move(task), std::move(execution_context), cost_mode))
{
}

template<TaskKind Kind>
LMCutHeuristic<Kind>::LMCutHeuristic(std::unique_ptr<Impl> impl) : m_impl(std::move(impl))
{
}

template<TaskKind Kind>
LMCutHeuristic<Kind>::~LMCutHeuristic() = default;

template<TaskKind Kind>
LMCutHeuristic<Kind>::LMCutHeuristic(LMCutHeuristic&&) noexcept = default;

template<TaskKind Kind>
LMCutHeuristic<Kind>& LMCutHeuristic<Kind>::operator=(LMCutHeuristic&&) noexcept = default;

template<TaskKind Kind>
LMCutHeuristicPtr<Kind> LMCutHeuristic<Kind>::create(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode)
{ return std::make_shared<LMCutHeuristic<Kind>>(std::move(task), std::move(execution_context), cost_mode); }

template<TaskKind Kind>
void LMCutHeuristic<Kind>::set_goal(formalism::planning::ConjunctiveConditionView<GroundTag> goal)
{ m_impl->set_goal(goal); }

template<TaskKind Kind>
ygg::float_t LMCutHeuristic<Kind>::evaluate(const ygg::Builder<State<Kind>>& state)
{ return m_impl->evaluate(state); }

template<TaskKind Kind>
formalism::planning::AtomViewList<GroundTag, formalism::FluentTag> LMCutHeuristic<Kind>::compute_cut_frontier_atoms(const ygg::Builder<State<Kind>>& state)
{
    auto atoms = formalism::planning::AtomViewList<GroundTag, formalism::FluentTag> {};
    static_cast<void>(m_impl->evaluate(state, &atoms));
    std::sort(atoms.begin(), atoms.end());
    atoms.erase(std::unique(atoms.begin(), atoms.end()), atoms.end());
    return atoms;
}

template<TaskKind Kind>
HeuristicPtr<Kind> LMCutHeuristic<Kind>::make_worker(ygg::ExecutionContextPtr execution_context) const
{ return HeuristicPtr<Kind>(new LMCutHeuristic(std::make_unique<Impl>(*m_impl, std::move(execution_context)))); }

template<TaskKind Kind>
void LMCutHeuristic<Kind>::print_summary(size_t verbosity) const
{ m_impl->print_summary(verbosity); }

template class LMCutHeuristic<GroundTag>;
template class LMCutHeuristic<LiftedTag>;

}
