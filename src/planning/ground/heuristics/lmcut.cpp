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

#include "tyr/planning/ground/heuristics/lmcut.hpp"

#include "rpg.hpp"
#include "tyr/datalog/ground/policies/annotation.hpp"
#include "tyr/datalog/ground/policies/cost.hpp"
#include "tyr/datalog/ground/policies/numeric_support.hpp"
#include "tyr/datalog/policies/termination.hpp"

#include <algorithm>
#include <cassert>
#include <deque>
#include <limits>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/containers/variant.hpp>
#include <yggdrasil/semantics/comparison.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

namespace tyr::planning
{

struct GroundLMCutNumericNode : ygg::comparison::Mixin<GroundLMCutNumericNode>
{
    fd::GroundFunctionTermView<f::FluentTag> term;
    ygg::ClosedInterval<ygg::float_t> interval;

    GroundLMCutNumericNode() = delete;
    GroundLMCutNumericNode(fd::GroundFunctionTermView<f::FluentTag> term, ygg::ClosedInterval<ygg::float_t> interval) : term(term), interval(interval) {}

    auto identifying_members() const noexcept { return std::make_tuple(term, lower(interval), upper(interval)); }
};

struct GroundLMCutRuleEdge : ygg::comparison::Mixin<GroundLMCutRuleEdge>
{
    fd::GroundRuleView<f::PredicateTag> rule;

    GroundLMCutRuleEdge() = delete;
    explicit GroundLMCutRuleEdge(fd::GroundRuleView<f::PredicateTag> rule) : rule(rule) {}

    auto identifying_members() const noexcept { return std::tie(rule); }
};

struct GroundLMCutNumericEdge : ygg::comparison::Mixin<GroundLMCutNumericEdge>
{
    fd::GroundRuleView<f::FunctionTag> rule;
    fd::GroundFunctionTermView<f::FluentTag> term;
    ygg::ClosedInterval<ygg::float_t> interval;

    GroundLMCutNumericEdge() = delete;
    GroundLMCutNumericEdge(fd::GroundRuleView<f::FunctionTag> rule, fd::GroundFunctionTermView<f::FluentTag> term, ygg::ClosedInterval<ygg::float_t> interval) :
        rule(rule),
        term(term),
        interval(interval)
    {
    }

    auto identifying_members() const noexcept { return std::make_tuple(rule, term, lower(interval), upper(interval)); }
};

struct LMCutHeuristic<GroundTag>::Impl :
    detail::GroundRPGEvaluator<Impl,
                               datalog::OrAnnotationPolicy<GroundTag>,
                               datalog::AchieverAndAnnotationPolicy<GroundTag, datalog::MaxAggregation>,
                               datalog::TerminationPolicy<GroundTag, datalog::MaxAggregation>,
                               datalog::RuleCostOverridePolicy<GroundTag>>
{
    using Base = detail::GroundRPGEvaluator<Impl,
                                            datalog::OrAnnotationPolicy<GroundTag>,
                                            datalog::AchieverAndAnnotationPolicy<GroundTag, datalog::MaxAggregation>,
                                            datalog::TerminationPolicy<GroundTag, datalog::MaxAggregation>,
                                            datalog::RuleCostOverridePolicy<GroundTag>>;
    using Rule = fd::GroundRuleView<f::PredicateTag>;
    using CostKey = f::planning::ActionBindingView;
    using Atom = fd::GroundAtomView<f::FluentTag>;
    using NumericNode = GroundLMCutNumericNode;
    using RuleEdge = GroundLMCutRuleEdge;
    using NumericEdge = GroundLMCutNumericEdge;
    using Precondition = std::variant<Atom, NumericNode>;

    Impl(TaskPtr<GroundTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) :
        Base(std::move(task),
             std::move(execution_context),
             datalog::OrAnnotationPolicy<GroundTag>(),
             datalog::AchieverAndAnnotationPolicy<GroundTag, datalog::MaxAggregation>(),
             cost_mode,
             true),
        m_residual_costs(),
        m_rule_edge_used_costs(),
        m_numeric_edge_used_costs(),
        m_goal_zone(),
        m_numeric_goal_zone(),
        m_before_goal_zone(),
        m_numeric_before_goal_zone(),
        m_not_before_goal_zone(),
        m_numeric_not_before_goal_zone(),
        m_cut(),
        m_rule_cut(),
        m_numeric_cut(),
        m_max_precondition_buffers(),
        m_numeric_support_selector_workspace(),
        m_max_precondition_depth(0)
    {
    }

    ygg::float_t evaluate(const StateView<GroundTag>& state);
    datalog::Cost get_residual_cost(CostKey action_binding) const;
    template<f::RelationKind R>
    datalog::Cost get_residual_cost(fd::GroundRuleView<R> rule) const;
    template<f::RelationKind R>
    datalog::Cost get_witness_body_cost(const datalog::WitnessAnnotation<GroundTag, R>& witness) const;
    template<f::RelationKind R>
    datalog::Cost get_witness_edge_residual_cost(const datalog::WitnessAnnotation<GroundTag, R>& witness) const;
    void set_residual_cost(CostKey action_binding, datalog::Cost cost);
    template<f::RelationKind R>
    void set_residual_cost(fd::GroundRuleView<R> rule, datalog::Cost cost);
    void use_rule_edge_cost(Rule rule, datalog::Cost cost);
    void use_numeric_edge_cost(NumericEdge edge, datalog::Cost cost);
    void apply_residual_costs();
    datalog::Cost get_numeric_cost(NumericNode node) const noexcept;
    const datalog::WitnessAnnotation<GroundTag, f::FunctionTag>* get_numeric_witness(NumericNode node) const noexcept;
    template<f::RelationKind R>
    const std::vector<Precondition>& get_witness_max_preconditions(const datalog::WitnessAnnotation<GroundTag, R>& witness, datalog::Cost edge_cost);
    void release_witness_max_preconditions();
    void mark_goal_zone(Atom atom);
    void mark_goal_zone(NumericNode node);
    void mark_goal_zone(Precondition precondition);
    bool is_before_goal_zone(Atom atom);
    bool is_before_goal_zone(NumericNode node);
    bool is_before_goal_zone(Precondition precondition);
    void extract_cut();
    void extract_expanded_cut();

    bool use_expanded_edges() const noexcept { return m_definition->use_expanded_lmcut; }

    ygg::UnorderedMap<CostKey, datalog::Cost> m_residual_costs;
    ygg::UnorderedMap<RuleEdge, datalog::Cost> m_rule_edge_used_costs;
    ygg::UnorderedMap<NumericEdge, datalog::Cost> m_numeric_edge_used_costs;
    ygg::UnorderedSet<Atom> m_goal_zone;
    ygg::UnorderedSet<NumericNode> m_numeric_goal_zone;
    ygg::UnorderedSet<Atom> m_before_goal_zone;
    ygg::UnorderedSet<NumericNode> m_numeric_before_goal_zone;
    ygg::UnorderedSet<Atom> m_not_before_goal_zone;
    ygg::UnorderedSet<NumericNode> m_numeric_not_before_goal_zone;
    ygg::UnorderedSet<CostKey> m_cut;
    ygg::UnorderedMap<RuleEdge, datalog::Cost> m_rule_cut;
    ygg::UnorderedMap<NumericEdge, datalog::Cost> m_numeric_cut;
    std::deque<std::vector<Precondition>> m_max_precondition_buffers;
    datalog::GroundNumericSupportSelectorWorkspace m_numeric_support_selector_workspace;
    size_t m_max_precondition_depth;
};

LMCutHeuristic<GroundTag>::LMCutHeuristic(TaskPtr<GroundTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) :
    m_impl(std::make_unique<Impl>(std::move(task), std::move(execution_context), cost_mode))
{
}

LMCutHeuristic<GroundTag>::~LMCutHeuristic() = default;
LMCutHeuristic<GroundTag>::LMCutHeuristic(LMCutHeuristic&&) noexcept = default;
LMCutHeuristic<GroundTag>& LMCutHeuristic<GroundTag>::operator=(LMCutHeuristic&&) noexcept = default;

LMCutHeuristicPtr<GroundTag> LMCutHeuristic<GroundTag>::create(TaskPtr<GroundTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode)
{
    return std::make_shared<LMCutHeuristic<GroundTag>>(std::move(task), std::move(execution_context), cost_mode);
}

void LMCutHeuristic<GroundTag>::set_goal(f::planning::GroundConjunctiveConditionView goal) { m_impl->set_goal(goal); }

ygg::float_t LMCutHeuristic<GroundTag>::evaluate(const StateView<GroundTag>& state) { return m_impl->evaluate(state); }

ygg::float_t LMCutHeuristic<GroundTag>::Impl::evaluate(const StateView<GroundTag>& state)
{
    auto value = datalog::Cost(0);
    m_residual_costs.clear();
    m_rule_edge_used_costs.clear();
    m_numeric_edge_used_costs.clear();

    while (true)
    {
        apply_residual_costs();
        const auto hmax = Base::evaluate_impl(state, false);
        if (hmax == std::numeric_limits<ygg::float_t>::infinity())
            return hmax;

        const auto hmax_cost = datalog::Cost(hmax);
        if (hmax_cost == 0)
            return ygg::float_t(value);

        auto cut_cost = std::numeric_limits<datalog::Cost>::max();
        if (use_expanded_edges())
        {
            extract_expanded_cut();
            if (m_rule_cut.empty() && m_numeric_cut.empty())
                return ygg::float_t(value + hmax_cost);

            for (const auto& [_, residual] : m_rule_cut)
                cut_cost = std::min(cut_cost, residual);
            for (const auto& [_, residual] : m_numeric_cut)
                cut_cost = std::min(cut_cost, residual);

            assert(cut_cost > 0 && cut_cost != std::numeric_limits<datalog::Cost>::max());

            value += cut_cost;
            for (const auto& [edge, _] : m_rule_cut)
                use_rule_edge_cost(edge.rule, cut_cost);
            for (const auto& [edge, _] : m_numeric_cut)
                use_numeric_edge_cost(edge, cut_cost);
        }
        else
        {
            extract_cut();
            if (m_cut.empty())
                return ygg::float_t(value + hmax_cost);

            for (const auto key : m_cut)
                cut_cost = std::min(cut_cost, get_residual_cost(key));

            assert(cut_cost > 0 && cut_cost != std::numeric_limits<datalog::Cost>::max());

            value += cut_cost;
            for (const auto key : m_cut)
                set_residual_cost(key, get_residual_cost(key) - cut_cost);
        }
    }
}

datalog::Cost LMCutHeuristic<GroundTag>::Impl::get_residual_cost(CostKey action_binding) const
{
    const auto used_it = m_residual_costs.find(action_binding);
    const auto used = used_it == m_residual_costs.end() ? datalog::Cost(0) : used_it->second;
    return used >= datalog::Cost(1) ? datalog::Cost(0) : datalog::Cost(1) - used;
}

template<f::RelationKind R>
datalog::Cost LMCutHeuristic<GroundTag>::Impl::get_residual_cost(fd::GroundRuleView<R> rule) const
{
    const auto& mapping = get_program().template get_rule_to_action_mapping<R>();
    const auto action_it = mapping.find(rule);
    return action_it == mapping.end() ? datalog::Cost(0) : get_residual_cost(action_it->second.get_row());
}

template<f::RelationKind R>
datalog::Cost LMCutHeuristic<GroundTag>::Impl::get_witness_body_cost(const datalog::WitnessAnnotation<GroundTag, R>& witness) const
{
    auto body_cost = datalog::Cost(0);
    for (const auto literal : witness.get_rule_key().get_body().template get_literals<f::FluentTag>())
        if (literal.get_polarity())
            body_cost = std::max(body_cost, get_atom_cost(literal.get_atom()));

    for (const auto& support : witness.get_numeric_supports())
        body_cost = std::max(body_cost, support.get_cost());

    return body_cost;
}

template<f::RelationKind R>
datalog::Cost LMCutHeuristic<GroundTag>::Impl::get_witness_edge_residual_cost(const datalog::WitnessAnnotation<GroundTag, R>& witness) const
{
    const auto body_cost = get_witness_body_cost(witness);
    return witness.get_cost() <= body_cost ? datalog::Cost(0) : witness.get_cost() - body_cost;
}

void LMCutHeuristic<GroundTag>::Impl::set_residual_cost(CostKey action_binding, datalog::Cost cost)
{
    m_residual_costs.insert_or_assign(action_binding, datalog::Cost(1) - cost);
}

template<f::RelationKind R>
void LMCutHeuristic<GroundTag>::Impl::set_residual_cost(fd::GroundRuleView<R> rule, datalog::Cost cost)
{
    const auto& mapping = get_program().template get_rule_to_action_mapping<R>();
    const auto action_it = mapping.find(rule);
    if (action_it != mapping.end())
        set_residual_cost(action_it->second.get_row(), cost);
}

void LMCutHeuristic<GroundTag>::Impl::use_rule_edge_cost(Rule rule, datalog::Cost cost)
{
    auto& used = m_rule_edge_used_costs[RuleEdge { rule }];
    used += cost;
}

void LMCutHeuristic<GroundTag>::Impl::use_numeric_edge_cost(NumericEdge edge, datalog::Cost cost)
{
    auto& used = m_numeric_edge_used_costs[edge];
    used += cost;
}

void LMCutHeuristic<GroundTag>::Impl::apply_residual_costs()
{
    m_workspace.clear_costs();
    for (const auto& [action_binding, used_cost] : m_residual_costs)
        set_action_binding_cost(action_binding, used_cost);
    for (const auto& [edge, used_cost] : m_rule_edge_used_costs)
        m_workspace.cost_policy.set_cost(edge.rule, used_cost);
    for (const auto& [edge, used_cost] : m_numeric_edge_used_costs)
        m_workspace.cost_policy.set_cost(edge.rule, edge.term, edge.interval, used_cost);
}

datalog::Cost LMCutHeuristic<GroundTag>::Impl::get_numeric_cost(NumericNode node) const noexcept
{
    const auto* annotation = m_workspace.numeric_and_annot.find(node.term, node.interval);
    return annotation ? datalog::get_cost(*annotation) : datalog::Cost(0);
}

const datalog::WitnessAnnotation<GroundTag, f::FunctionTag>* LMCutHeuristic<GroundTag>::Impl::get_numeric_witness(NumericNode node) const noexcept
{
    const auto* annotation = m_workspace.numeric_and_annot.find(node.term, node.interval);
    return annotation ? std::get_if<datalog::WitnessAnnotation<GroundTag, f::FunctionTag>>(annotation) : nullptr;
}

template<f::RelationKind R>
auto LMCutHeuristic<GroundTag>::Impl::get_witness_max_preconditions(const datalog::WitnessAnnotation<GroundTag, R>& witness,
                                                                    datalog::Cost edge_cost) -> const std::vector<Precondition>&
{
    if (m_max_precondition_depth == m_max_precondition_buffers.size())
        m_max_precondition_buffers.emplace_back();

    auto& result = m_max_precondition_buffers[m_max_precondition_depth++];
    result.clear();
    const auto rule = witness.get_rule_key();
    if (witness.get_cost() < edge_cost)
        return result;

    const auto body_cost = witness.get_cost() - edge_cost;
    for (const auto literal : rule.get_body().template get_literals<f::FluentTag>())
        if (literal.get_polarity() && get_atom_cost(literal.get_atom()) == body_cost)
            result.emplace_back(literal.get_atom());

    for (const auto& support : witness.get_numeric_supports())
        if (support.get_cost() == body_cost)
            result.emplace_back(NumericNode { support.get_key(), support.get_interval() });

    return result;
}

void LMCutHeuristic<GroundTag>::Impl::release_witness_max_preconditions()
{
    assert(m_max_precondition_depth > 0);
    --m_max_precondition_depth;
}

void LMCutHeuristic<GroundTag>::Impl::mark_goal_zone(Precondition precondition)
{
    std::visit([&](auto node) { mark_goal_zone(node); }, precondition);
}

void LMCutHeuristic<GroundTag>::Impl::mark_goal_zone(Atom atom)
{
    if (!m_goal_zone.insert(atom).second)
        return;

    const auto atom_cost = get_atom_cost(atom);
    if (const auto* achievers = m_workspace.and_ap.find_achievers(atom))
    {
        for (const auto& witness : *achievers)
        {
            if (witness.get_cost() != atom_cost)
                continue;

            const auto& mapping = get_program().get_rule_to_action_mapping<f::PredicateTag>();
            const auto action_it = mapping.find(witness.get_rule_key());
            const auto residual = use_expanded_edges() ? (action_it != mapping.end() ? get_witness_edge_residual_cost(witness) : datalog::Cost(0)) :
                                                         get_residual_cost(witness.get_rule_key());
            if (action_it != mapping.end() && residual > 0)
                continue;

            const auto& preconditions = get_witness_max_preconditions(witness, residual);
            for (const auto& precondition : preconditions)
                mark_goal_zone(precondition);
            release_witness_max_preconditions();
        }
    }
}

void LMCutHeuristic<GroundTag>::Impl::mark_goal_zone(NumericNode node)
{
    if (!m_numeric_goal_zone.insert(node).second)
        return;

    const auto numeric_cost = get_numeric_cost(node);
    const auto* witness = get_numeric_witness(node);
    if (!witness || witness->get_cost() != numeric_cost)
        return;

    const auto& mapping = get_program().get_rule_to_action_mapping<f::FunctionTag>();
    const auto action_it = mapping.find(witness->get_rule_key());
    const auto residual = use_expanded_edges() ? (action_it != mapping.end() ? get_witness_edge_residual_cost(*witness) : datalog::Cost(0)) :
                                                 get_residual_cost(witness->get_rule_key());
    if (action_it != mapping.end() && residual > 0)
        return;

    const auto& preconditions = get_witness_max_preconditions(*witness, residual);
    for (const auto& precondition : preconditions)
        mark_goal_zone(precondition);
    release_witness_max_preconditions();
}

bool LMCutHeuristic<GroundTag>::Impl::is_before_goal_zone(Precondition precondition)
{
    return std::visit([&](auto node) { return is_before_goal_zone(node); }, precondition);
}

bool LMCutHeuristic<GroundTag>::Impl::is_before_goal_zone(Atom atom)
{
    if (m_goal_zone.contains(atom))
        return false;
    if (m_before_goal_zone.contains(atom))
        return true;
    if (m_not_before_goal_zone.contains(atom))
        return false;

    m_not_before_goal_zone.insert(atom);

    auto has_optimal_achiever = false;
    auto before = false;
    const auto atom_cost = get_atom_cost(atom);
    if (const auto* achievers = m_workspace.and_ap.find_achievers(atom))
    {
        for (const auto& witness : *achievers)
        {
            if (before || witness.get_cost() != atom_cost)
                continue;

            has_optimal_achiever = true;
            const auto& mapping = get_program().get_rule_to_action_mapping<f::PredicateTag>();
            const auto action_it = mapping.find(witness.get_rule_key());
            const auto residual = use_expanded_edges() ? (action_it != mapping.end() ? get_witness_edge_residual_cost(witness) : datalog::Cost(0)) :
                                                         get_residual_cost(witness.get_rule_key());
            const auto& preconditions = get_witness_max_preconditions(witness, residual);
            before = preconditions.empty() || std::ranges::any_of(preconditions, [&](const auto precondition) { return is_before_goal_zone(precondition); });
            release_witness_max_preconditions();
        }
    }

    if (!has_optimal_achiever)
        before = true;

    if (before)
    {
        m_not_before_goal_zone.erase(atom);
        m_before_goal_zone.insert(atom);
        return true;
    }

    return false;
}

bool LMCutHeuristic<GroundTag>::Impl::is_before_goal_zone(NumericNode node)
{
    if (m_numeric_goal_zone.contains(node))
        return false;
    if (m_numeric_before_goal_zone.contains(node))
        return true;
    if (m_numeric_not_before_goal_zone.contains(node))
        return false;

    m_numeric_not_before_goal_zone.insert(node);

    auto before = false;
    const auto* witness = get_numeric_witness(node);
    if (witness && witness->get_cost() == get_numeric_cost(node))
    {
        const auto& mapping = get_program().get_rule_to_action_mapping<f::FunctionTag>();
        const auto action_it = mapping.find(witness->get_rule_key());
        const auto residual = use_expanded_edges() ? (action_it != mapping.end() ? get_witness_edge_residual_cost(*witness) : datalog::Cost(0)) :
                                                     get_residual_cost(witness->get_rule_key());
        const auto& preconditions = get_witness_max_preconditions(*witness, residual);
        before = preconditions.empty() || std::ranges::any_of(preconditions, [&](const auto precondition) { return is_before_goal_zone(precondition); });
        release_witness_max_preconditions();
    }
    else
    {
        before = true;
    }

    if (before)
    {
        m_numeric_not_before_goal_zone.erase(node);
        m_numeric_before_goal_zone.insert(node);
        return true;
    }

    return false;
}

void LMCutHeuristic<GroundTag>::Impl::extract_cut()
{
    m_goal_zone.clear();
    m_numeric_goal_zone.clear();
    m_before_goal_zone.clear();
    m_numeric_before_goal_zone.clear();
    m_not_before_goal_zone.clear();
    m_numeric_not_before_goal_zone.clear();
    m_cut.clear();

    const auto goal_cost = get_goal_cost();
    if (const auto& goal = m_workspace.tp.get_goal())
    {
        for (const auto literal : goal->get_literals<f::FluentTag>())
        {
            if (literal.get_polarity() && get_atom_cost(literal.get_atom()) == goal_cost)
            {
                mark_goal_zone(literal.get_atom());
                break;
            }
        }

        auto selector = datalog::GroundNumericSupportSelector(m_workspace.const_workspace.facts, m_workspace.facts, m_workspace.numeric_and_annot);
        for (const auto numeric_constraint : goal->get_numeric_constraints())
        {
            if (selector.get_constraint_cost(numeric_constraint, m_numeric_support_selector_workspace, datalog::MaxAggregation {}) != goal_cost)
                continue;

            for (const auto& entry : m_numeric_support_selector_workspace.selection)
                if (entry.cost == goal_cost)
                    mark_goal_zone(NumericNode { entry.key, entry.interval });
        }
    }

    auto inspect_witness = [&](const auto& witness)
    {
        using R = typename std::decay_t<decltype(witness)>::Relation;
        const auto& mapping = get_program().get_rule_to_action_mapping<R>();
        const auto action_it = mapping.find(witness.get_rule_key());
        if (action_it == mapping.end() || get_residual_cost(action_it->second.get_row()) == 0)
            return;

        const auto& preconditions = get_witness_max_preconditions(witness, get_residual_cost(witness.get_rule_key()));
        const auto crosses_cut =
            preconditions.empty() || std::ranges::any_of(preconditions, [&](const auto precondition) { return is_before_goal_zone(precondition); });
        release_witness_max_preconditions();
        if (crosses_cut)
            m_cut.insert(action_it->second.get_row());
    };

    for (const auto atom : m_goal_zone)
    {
        const auto atom_cost = get_atom_cost(atom);
        if (const auto* achievers = m_workspace.and_ap.find_achievers(atom))
            for (const auto& witness : *achievers)
                if (witness.get_cost() == atom_cost)
                    inspect_witness(witness);
    }

    for (const auto& node : m_numeric_goal_zone)
        if (const auto* witness = get_numeric_witness(node); witness && witness->get_cost() == get_numeric_cost(node))
            inspect_witness(*witness);
}

void LMCutHeuristic<GroundTag>::Impl::extract_expanded_cut()
{
    m_goal_zone.clear();
    m_numeric_goal_zone.clear();
    m_before_goal_zone.clear();
    m_numeric_before_goal_zone.clear();
    m_not_before_goal_zone.clear();
    m_numeric_not_before_goal_zone.clear();
    m_rule_cut.clear();
    m_numeric_cut.clear();

    const auto goal_cost = get_goal_cost();
    if (const auto& goal = m_workspace.tp.get_goal())
    {
        for (const auto literal : goal->get_literals<f::FluentTag>())
        {
            if (literal.get_polarity() && get_atom_cost(literal.get_atom()) == goal_cost)
            {
                mark_goal_zone(literal.get_atom());
                break;
            }
        }

        auto selector = datalog::GroundNumericSupportSelector(m_workspace.const_workspace.facts, m_workspace.facts, m_workspace.numeric_and_annot);
        for (const auto numeric_constraint : goal->get_numeric_constraints())
        {
            if (selector.get_constraint_cost(numeric_constraint, m_numeric_support_selector_workspace, datalog::MaxAggregation {}) != goal_cost)
                continue;

            for (const auto& entry : m_numeric_support_selector_workspace.selection)
                if (entry.cost == goal_cost)
                    mark_goal_zone(NumericNode { entry.key, entry.interval });
        }
    }

    auto inspect_rule_witness = [&](const auto& witness)
    {
        using R = typename std::decay_t<decltype(witness)>::Relation;
        const auto& mapping = get_program().get_rule_to_action_mapping<R>();
        if (mapping.find(witness.get_rule_key()) == mapping.end())
            return;
        const auto residual = get_witness_edge_residual_cost(witness);
        if (residual == datalog::Cost(0))
            return;

        const auto& preconditions = get_witness_max_preconditions(witness, residual);
        const auto crosses_cut =
            preconditions.empty() || std::ranges::any_of(preconditions, [&](const auto precondition) { return is_before_goal_zone(precondition); });
        release_witness_max_preconditions();
        if (crosses_cut)
        {
            const auto edge = RuleEdge { witness.get_rule_key() };
            const auto [it, inserted] = m_rule_cut.emplace(edge, residual);
            if (!inserted)
                it->second = std::min(it->second, residual);
        }
    };

    auto inspect_numeric_witness = [&](const NumericNode node, const auto& witness)
    {
        using R = typename std::decay_t<decltype(witness)>::Relation;
        const auto& mapping = get_program().get_rule_to_action_mapping<R>();
        if (mapping.find(witness.get_rule_key()) == mapping.end())
            return;
        const auto edge = NumericEdge { witness.get_rule_key(), node.term, node.interval };
        const auto residual = get_witness_edge_residual_cost(witness);
        if (residual == datalog::Cost(0))
            return;

        const auto& preconditions = get_witness_max_preconditions(witness, residual);
        const auto crosses_cut =
            preconditions.empty() || std::ranges::any_of(preconditions, [&](const auto precondition) { return is_before_goal_zone(precondition); });
        release_witness_max_preconditions();
        if (crosses_cut)
        {
            const auto [it, inserted] = m_numeric_cut.emplace(edge, residual);
            if (!inserted)
                it->second = std::min(it->second, residual);
        }
    };

    for (const auto atom : m_goal_zone)
    {
        const auto atom_cost = get_atom_cost(atom);
        if (const auto* achievers = m_workspace.and_ap.find_achievers(atom))
            for (const auto& witness : *achievers)
                if (witness.get_cost() == atom_cost)
                    inspect_rule_witness(witness);
    }

    for (const auto& node : m_numeric_goal_zone)
        if (const auto* witness = get_numeric_witness(node); witness && witness->get_cost() == get_numeric_cost(node))
            inspect_numeric_witness(node, *witness);
}

}
