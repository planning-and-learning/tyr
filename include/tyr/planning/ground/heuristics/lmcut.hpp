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

#ifndef TYR_PLANNING_GROUND_HEURISTICS_LMCUT_HPP_
#define TYR_PLANNING_GROUND_HEURISTICS_LMCUT_HPP_

#include "tyr/datalog/ground/policies/annotation.hpp"
#include "tyr/datalog/ground/policies/cost.hpp"
#include "tyr/datalog/ground/policies/numeric_support.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/planning/ground/heuristics/rpg.hpp"
#include "tyr/planning/heuristics/lmcut.hpp"

#include <deque>
#include <tuple>
#include <variant>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/semantics/comparison.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::planning
{

struct GroundLMCutNumericNode : ygg::comparison::Mixin<GroundLMCutNumericNode>
{
    ::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::FluentTag> term;
    ygg::ClosedInterval<ygg::float_t> interval;

    GroundLMCutNumericNode() = default;
    GroundLMCutNumericNode(
        ::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::FluentTag> term,
        ygg::ClosedInterval<ygg::float_t> interval) :
        term(term),
        interval(interval)
    {
    }

    auto identifying_members() const noexcept { return std::make_tuple(term, lower(interval), upper(interval)); }
};

struct GroundLMCutRuleEdge : ygg::comparison::Mixin<GroundLMCutRuleEdge>
{
    ::tyr::formalism::datalog::GroundRuleView rule;

    GroundLMCutRuleEdge() = default;
    explicit GroundLMCutRuleEdge(::tyr::formalism::datalog::GroundRuleView rule) : rule(rule) {}

    auto identifying_members() const noexcept { return std::tie(rule); }
};

struct GroundLMCutNumericEdge : ygg::comparison::Mixin<GroundLMCutNumericEdge>
{
    ::tyr::formalism::datalog::GroundRuleView rule;
    ::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::FluentTag> term;
    ygg::ClosedInterval<ygg::float_t> interval;

    GroundLMCutNumericEdge() = default;
    GroundLMCutNumericEdge(::tyr::formalism::datalog::GroundRuleView rule,
                           ::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::FluentTag> term,
                           ygg::ClosedInterval<ygg::float_t> interval) :
        rule(rule),
        term(term),
        interval(interval)
    {
    }

    auto identifying_members() const noexcept { return std::make_tuple(rule, term, lower(interval), upper(interval)); }
};

template<>
class LMCutHeuristic<GroundTag> :
    public RPGBase<GroundTag,
                   LMCutHeuristic<GroundTag>,
                   datalog::OrAnnotationPolicy<GroundTag>,
                   datalog::AchieverAndAnnotationPolicy<GroundTag, datalog::MaxAggregation>,
                   datalog::TerminationPolicy<GroundTag, datalog::MaxAggregation>,
                   datalog::RuleCostOverridePolicy<GroundTag>>
{
public:
    using Base = RPGBase<GroundTag,
                         LMCutHeuristic<GroundTag>,
                         datalog::OrAnnotationPolicy<GroundTag>,
                         datalog::AchieverAndAnnotationPolicy<GroundTag, datalog::MaxAggregation>,
                         datalog::TerminationPolicy<GroundTag, datalog::MaxAggregation>,
                         datalog::RuleCostOverridePolicy<GroundTag>>;

    LMCutHeuristic(TaskPtr<GroundTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode = CostMode::GENERAL);

    static LMCutHeuristicPtr<GroundTag> create(TaskPtr<GroundTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode = CostMode::GENERAL);

    ygg::float_t evaluate(const StateView<GroundTag>& state) override;

    ygg::float_t extract_cost_and_set_preferred_actions_impl(const StateView<GroundTag>& state);

private:
    using Rule = ::tyr::formalism::datalog::GroundRuleView;
    using CostKey = ::tyr::formalism::planning::ActionBindingView;
    using Atom = ::tyr::formalism::datalog::GroundAtomView<::tyr::formalism::FluentTag>;

    using NumericNode = GroundLMCutNumericNode;
    using RuleEdge = GroundLMCutRuleEdge;
    using NumericEdge = GroundLMCutNumericEdge;
    using Precondition = std::variant<Atom, NumericNode>;

    datalog::Cost get_residual_cost(CostKey action_binding) const;
    datalog::Cost get_residual_cost(Rule rule) const;
    datalog::Cost get_witness_body_cost(const datalog::WitnessAnnotation<GroundTag>& witness) const;
    datalog::Cost get_witness_edge_residual_cost(const datalog::WitnessAnnotation<GroundTag>& witness) const;
    void set_residual_cost(CostKey action_binding, datalog::Cost cost);
    void set_residual_cost(Rule rule, datalog::Cost cost);
    void use_rule_edge_cost(Rule rule, datalog::Cost cost);
    void use_numeric_edge_cost(NumericEdge edge, datalog::Cost cost);
    ygg::float_t evaluate_impl(const StateView<GroundTag>& state);
    void apply_residual_costs();
    datalog::Cost get_numeric_cost(NumericNode node) const noexcept;
    const datalog::WitnessAnnotation<GroundTag>* get_numeric_witness(NumericNode node) const noexcept;
    const std::vector<Precondition>& get_witness_max_preconditions(const datalog::WitnessAnnotation<GroundTag>& witness, datalog::Cost edge_cost);
    void release_witness_max_preconditions();
    void mark_goal_zone(Atom atom);
    void mark_goal_zone(NumericNode node);
    void mark_goal_zone(Precondition precondition);
    bool is_before_goal_zone(Atom atom);
    bool is_before_goal_zone(NumericNode node);
    bool is_before_goal_zone(Precondition precondition);
    void extract_cut();
    void extract_expanded_cut();

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
    bool m_use_expanded_edges;
};

}

#endif
