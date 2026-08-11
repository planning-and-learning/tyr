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

namespace tyr::planning
{
namespace
{
namespace f = ::tyr::formalism;
namespace fd = ::tyr::formalism::datalog;

template<TaskKind Kind>
struct LMCutNumericNode : ygg::comparison::Mixin<LMCutNumericNode<Kind>>
{
    fd::FunctionBindingView<f::FluentTag> key;
    ygg::ClosedInterval<ygg::float_t> interval;

    LMCutNumericNode() = delete;
    LMCutNumericNode(fd::FunctionBindingView<f::FluentTag> key, ygg::ClosedInterval<ygg::float_t> interval) : key(key), interval(interval) {}

    auto identifying_members() const noexcept { return std::make_tuple(key, lower(interval), upper(interval)); }
};

template<TaskKind Kind>
bool requires_numeric_edge_cuts(fd::ProgramView<Kind> program)
{
    for (const auto rule : program.template get_rules<f::PredicateTag>())
        if (!rule.get_body().get_numeric_constraints().empty())
            return true;
    return !program.template get_rules<f::FunctionTag>().empty();
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
struct LMCutImplementation :
    detail::RPGEvaluator<LMCutImplementation<Kind, TP>,
                         Kind,
                         datalog::MinCostAnnotationWithAchieversPolicy<Kind, datalog::MaxAggregation>,
                         TP,
                         datalog::RuleCostOverridePolicy<Kind>>
{
    using Base = detail::RPGEvaluator<LMCutImplementation<Kind, TP>,
                                      Kind,
                                      datalog::MinCostAnnotationWithAchieversPolicy<Kind, datalog::MaxAggregation>,
                                      TP,
                                      datalog::RuleCostOverridePolicy<Kind>>;
    using ActionBinding = ::tyr::formalism::planning::ActionBindingView;
    using PredicateHead = fd::PredicateBindingView<f::FluentTag>;
    using NumericNode = LMCutNumericNode<Kind>;
    using RuleEdge = fd::RuleBindingView<f::PredicateTag>;
    using NumericEdge = datalog::NumericTransitionCostKey<f::FunctionTag>;
    using Precondition = std::variant<PredicateHead, NumericNode>;
    using CutFrontierAtoms = f::planning::GroundAtomViewList<f::FluentTag>;

    LMCutImplementation(std::shared_ptr<detail::RPGDefinition<Kind>> definition, ygg::ExecutionContextPtr execution_context, CostMode cost_mode);
    LMCutImplementation(const LMCutImplementation& source, ygg::ExecutionContextPtr execution_context);
    ygg::float_t evaluate(const ygg::Builder<State<Kind>>& state, CutFrontierAtoms* cut_frontier_atoms = nullptr);

private:
    template<f::RelationKind R>
    datalog::Cost get_witness_body_cost(const datalog::WitnessAnnotation<Kind, R>& witness);
    template<f::RelationKind R>
    datalog::Cost get_witness_edge_residual_cost(const datalog::WitnessAnnotation<Kind, R>& witness);
    bool is_target_support(const datalog::NumericSupport<Kind>& support, NumericNode node) const noexcept;
    datalog::Cost get_numeric_support_cost(const datalog::NumericSupport<Kind>& support);
    void append_numeric_support_preconditions(const datalog::NumericSupport<Kind>& support, datalog::Cost body_cost, std::vector<Precondition>& result);
    datalog::Cost get_numeric_witness_body_cost(const datalog::WitnessAnnotation<Kind, f::FunctionTag>& witness, NumericNode node);
    datalog::Cost get_numeric_witness_edge_residual_cost(const datalog::WitnessAnnotation<Kind, f::FunctionTag>& witness, NumericNode node);
    void use_action_cost(ActionBinding action_binding, datalog::Cost cost);
    void use_rule_edge_cost(RuleEdge edge, datalog::Cost cost);
    void use_numeric_edge_cost(NumericEdge edge, datalog::Cost cost);
    void apply_residual_costs();
    datalog::Cost get_numeric_cost(NumericNode node) const noexcept;
    const datalog::WitnessAnnotation<Kind, f::FunctionTag>* get_numeric_witness(NumericNode node) const noexcept;
    template<f::RelationKind R>
    const std::vector<Precondition>& get_witness_max_preconditions(const datalog::WitnessAnnotation<Kind, R>& witness, datalog::Cost edge_cost, size_t depth);
    const std::vector<Precondition>& get_numeric_witness_max_preconditions(const datalog::WitnessAnnotation<Kind, f::FunctionTag>& witness,
                                                                           NumericNode node,
                                                                           datalog::Cost edge_cost,
                                                                           size_t depth);
    void mark_goal_zone(PredicateHead head, size_t depth);
    void mark_goal_zone(NumericNode node, size_t depth);
    void mark_goal_zone(Precondition precondition, size_t depth);
    bool is_before_goal_zone(PredicateHead head, size_t depth);
    bool is_before_goal_zone(NumericNode node, size_t depth);
    bool is_before_goal_zone(Precondition precondition, size_t depth);
    void clear_zones() noexcept;
    void seed_goal_zone();
    void clear_evaluation_views() noexcept;
    void append_cut_frontier_atom(PredicateHead head, CutFrontierAtoms* cut_frontier_atoms);
    void extract_cut(CutFrontierAtoms* cut_frontier_atoms);
    void extract_expanded_cut(CutFrontierAtoms* cut_frontier_atoms);
    static constexpr bool use_numeric_edge_cuts() noexcept { return std::same_as<TP, datalog::TerminationPolicy<Kind, datalog::MaxAggregation>>; }
    bool use_expanded_numeric_target() const noexcept { return use_numeric_edge_cuts() && m_cost_mode == CostMode::GENERAL; }

    CostMode m_cost_mode;
    ygg::UnorderedMap<ActionBinding, datalog::Cost> m_action_used_costs;
    ygg::UnorderedMap<RuleEdge, datalog::Cost> m_rule_edge_used_costs;
    ygg::UnorderedMap<NumericEdge, datalog::Cost> m_numeric_edge_used_costs;
    ygg::UnorderedSet<PredicateHead> m_goal_zone;
    ygg::UnorderedSet<NumericNode> m_numeric_goal_zone;
    ygg::UnorderedSet<PredicateHead> m_before_goal_zone;
    ygg::UnorderedSet<NumericNode> m_numeric_before_goal_zone;
    ygg::UnorderedSet<PredicateHead> m_not_before_goal_zone;
    ygg::UnorderedSet<NumericNode> m_numeric_not_before_goal_zone;
    ygg::UnorderedMap<ActionBinding, datalog::Cost> m_cut;
    ygg::UnorderedMap<RuleEdge, datalog::Cost> m_rule_cut;
    ygg::UnorderedMap<NumericEdge, datalog::Cost> m_numeric_cut;
    std::deque<std::vector<Precondition>> m_max_precondition_buffers;
    datalog::NumericSupportSelectorWorkspace<Kind> m_numeric_support_selector_workspace;
};

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
LMCutImplementation<Kind, TP>::LMCutImplementation(std::shared_ptr<detail::RPGDefinition<Kind>> definition,
                                                   ygg::ExecutionContextPtr execution_context,
                                                   CostMode cost_mode) :
    Base(std::move(definition), std::move(execution_context)),
    m_cost_mode(cost_mode)
{
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
LMCutImplementation<Kind, TP>::LMCutImplementation(const LMCutImplementation& source, ygg::ExecutionContextPtr execution_context) :
    Base(source, std::move(execution_context)),
    m_cost_mode(source.m_cost_mode)
{
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
ygg::float_t LMCutImplementation<Kind, TP>::evaluate(const ygg::Builder<State<Kind>>& state, CutFrontierAtoms* cut_frontier_atoms)
{
    auto value = datalog::Cost(0);
    clear_evaluation_views();
    this->begin_state_evaluation();

    while (true)
    {
        apply_residual_costs();
        const auto hmax = this->evaluate_current_state(state);
        if (hmax == std::numeric_limits<ygg::float_t>::infinity())
            return hmax;

        const auto hmax_cost = datalog::Cost(hmax);
        if (hmax_cost == 0)
            return ygg::float_t(value);

        auto cut_cost = std::numeric_limits<datalog::Cost>::max();
        if constexpr (use_numeric_edge_cuts())
        {
            extract_expanded_cut(cut_frontier_atoms);
            if (m_rule_cut.empty() && m_numeric_cut.empty())
                return ygg::float_t(value + hmax_cost);

            for (const auto& [_, residual] : m_rule_cut)
                cut_cost = std::min(cut_cost, residual);
            for (const auto& [_, residual] : m_numeric_cut)
                cut_cost = std::min(cut_cost, residual);

            assert(cut_cost > 0 && cut_cost != std::numeric_limits<datalog::Cost>::max());

            value += cut_cost;
            for (const auto& [edge, _] : m_rule_cut)
                use_rule_edge_cost(edge, cut_cost);
            for (const auto& [edge, _] : m_numeric_cut)
                use_numeric_edge_cost(edge, cut_cost);
        }
        else
        {
            extract_cut(cut_frontier_atoms);
            if (m_cut.empty())
                return ygg::float_t(value + hmax_cost);

            for (const auto& [_, residual] : m_cut)
                cut_cost = std::min(cut_cost, residual);

            assert(cut_cost > 0 && cut_cost != std::numeric_limits<datalog::Cost>::max());

            value += cut_cost;
            for (const auto& [action_binding, _] : m_cut)
                use_action_cost(action_binding, cut_cost);
        }
    }
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
void LMCutImplementation<Kind, TP>::use_action_cost(ActionBinding action_binding, datalog::Cost cost)
{
    m_action_used_costs[action_binding] += cost;
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
template<f::RelationKind R>
datalog::Cost LMCutImplementation<Kind, TP>::get_witness_body_cost(const datalog::WitnessAnnotation<Kind, R>& witness)
{
    auto body_cost = datalog::Cost(0);
    this->for_each_witness_precondition(witness, [&](const auto precondition) { body_cost = std::max(body_cost, this->get_predicate_cost(precondition)); });

    for (const auto& support : witness.get_numeric_supports())
        body_cost = std::max(body_cost, get_numeric_support_cost(support));

    return body_cost;
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
template<f::RelationKind R>
datalog::Cost LMCutImplementation<Kind, TP>::get_witness_edge_residual_cost(const datalog::WitnessAnnotation<Kind, R>& witness)
{
    const auto body_cost = get_witness_body_cost(witness);
    return witness.get_cost() <= body_cost ? datalog::Cost(0) : witness.get_cost() - body_cost;
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
bool LMCutImplementation<Kind, TP>::is_target_support(const datalog::NumericSupport<Kind>& support, NumericNode node) const noexcept
{
    return support.get_key() == node.key && support.get_interval() == node.interval;
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
datalog::Cost LMCutImplementation<Kind, TP>::get_numeric_support_cost(const datalog::NumericSupport<Kind>& support)
{
    auto cost = datalog::Cost(0);
    this->for_each_numeric_predecessor(support, [&](const auto, const auto, const auto predecessor_cost) { cost = std::max(cost, predecessor_cost); });
    return cost;
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
void LMCutImplementation<Kind, TP>::append_numeric_support_preconditions(const datalog::NumericSupport<Kind>& support,
                                                                         datalog::Cost body_cost,
                                                                         std::vector<Precondition>& result)
{
    this->for_each_numeric_predecessor(support,
                                       [&](const auto key, const auto interval, const auto cost)
                                       {
                                           if (cost == body_cost)
                                               result.emplace_back(NumericNode { key, interval });
                                       });
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
datalog::Cost LMCutImplementation<Kind, TP>::get_numeric_witness_body_cost(const datalog::WitnessAnnotation<Kind, f::FunctionTag>& witness, NumericNode node)
{
    auto body_cost = datalog::Cost(0);
    this->for_each_witness_precondition(witness, [&](const auto precondition) { body_cost = std::max(body_cost, this->get_predicate_cost(precondition)); });

    for (const auto& support : witness.get_numeric_supports())
        if (!is_target_support(support, node))
            body_cost = std::max(body_cost, get_numeric_support_cost(support));

    return body_cost;
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
datalog::Cost LMCutImplementation<Kind, TP>::get_numeric_witness_edge_residual_cost(const datalog::WitnessAnnotation<Kind, f::FunctionTag>& witness,
                                                                                    NumericNode node)
{
    const auto body_cost = get_numeric_witness_body_cost(witness, node);
    return witness.get_cost() <= body_cost ? datalog::Cost(0) : witness.get_cost() - body_cost;
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
void LMCutImplementation<Kind, TP>::use_rule_edge_cost(RuleEdge edge, datalog::Cost cost)
{
    auto& used = m_rule_edge_used_costs[edge];
    used += cost;
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
void LMCutImplementation<Kind, TP>::use_numeric_edge_cost(NumericEdge edge, datalog::Cost cost)
{
    auto& used = m_numeric_edge_used_costs[edge];
    used += cost;
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
void LMCutImplementation<Kind, TP>::apply_residual_costs()
{
    this->m_workspace.clear_costs();
    for (const auto& [action_binding, used_cost] : m_action_used_costs)
        this->set_action_binding_cost(action_binding, used_cost);

    for (const auto& [edge, used_cost] : m_rule_edge_used_costs)
        this->m_workspace.cost_policy.set_cost(edge, used_cost);
    for (const auto& [edge, used_cost] : m_numeric_edge_used_costs)
        this->m_workspace.cost_policy.set_cost(edge.rule_key, edge.numeric_key, edge.interval, used_cost);
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
datalog::Cost LMCutImplementation<Kind, TP>::get_numeric_cost(NumericNode node) const noexcept
{
    const auto* annotation = this->m_workspace.numeric_annotations.find(node.key, node.interval);
    return annotation ? datalog::get_cost(*annotation) : datalog::Cost(0);
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
const datalog::WitnessAnnotation<Kind, f::FunctionTag>* LMCutImplementation<Kind, TP>::get_numeric_witness(NumericNode node) const noexcept
{
    const auto* annotation = this->m_workspace.numeric_annotations.find(node.key, node.interval);
    return annotation ? std::get_if<datalog::WitnessAnnotation<Kind, f::FunctionTag>>(annotation) : nullptr;
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
template<f::RelationKind R>
const std::vector<typename LMCutImplementation<Kind, TP>::Precondition>&
LMCutImplementation<Kind, TP>::get_witness_max_preconditions(const datalog::WitnessAnnotation<Kind, R>& witness, datalog::Cost edge_cost, size_t depth)
{
    if (m_max_precondition_buffers.size() <= depth)
        m_max_precondition_buffers.emplace_back();

    auto& result = m_max_precondition_buffers[depth];
    result.clear();
    if (witness.get_cost() < edge_cost)
        return result;

    const auto body_cost = witness.get_cost() - edge_cost;
    this->for_each_witness_precondition(witness,
                                        [&](const auto precondition)
                                        {
                                            if (this->get_predicate_cost(precondition) == body_cost)
                                                result.emplace_back(precondition);
                                        });

    for (const auto& support : witness.get_numeric_supports())
        append_numeric_support_preconditions(support, body_cost, result);

    return result;
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
const std::vector<typename LMCutImplementation<Kind, TP>::Precondition>&
LMCutImplementation<Kind, TP>::get_numeric_witness_max_preconditions(const datalog::WitnessAnnotation<Kind, f::FunctionTag>& witness,
                                                                     NumericNode node,
                                                                     datalog::Cost edge_cost,
                                                                     size_t depth)
{
    if (m_max_precondition_buffers.size() <= depth)
        m_max_precondition_buffers.emplace_back();

    auto& result = m_max_precondition_buffers[depth];
    result.clear();
    if (witness.get_cost() < edge_cost)
        return result;

    const auto body_cost = witness.get_cost() - edge_cost;
    this->for_each_witness_precondition(witness,
                                        [&](const auto precondition)
                                        {
                                            if (this->get_predicate_cost(precondition) == body_cost)
                                                result.emplace_back(precondition);
                                        });

    for (const auto& support : witness.get_numeric_supports())
        if (!is_target_support(support, node))
            append_numeric_support_preconditions(support, body_cost, result);

    return result;
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
void LMCutImplementation<Kind, TP>::mark_goal_zone(Precondition precondition, size_t depth)
{
    std::visit([&](auto node) { mark_goal_zone(node, depth); }, precondition);
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
void LMCutImplementation<Kind, TP>::mark_goal_zone(PredicateHead head, size_t depth)
{
    if (!m_goal_zone.insert(head).second)
        return;

    const auto head_cost = this->get_predicate_cost(head);
    this->for_each_achiever(head,
                            [&](const auto& witness)
                            {
                                if (use_numeric_edge_cuts() && witness.get_cost() != head_cost)
                                    return;

                                const auto action_binding = this->get_action_binding(witness);
                                const auto residual = action_binding ? get_witness_edge_residual_cost(witness) : datalog::Cost(0);
                                if (action_binding && residual > 0)
                                    return;

                                const auto& preconditions = get_witness_max_preconditions(witness, residual, depth);
                                for (const auto& precondition : preconditions)
                                    mark_goal_zone(precondition, depth + 1);
                            });
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
void LMCutImplementation<Kind, TP>::mark_goal_zone(NumericNode node, size_t depth)
{
    if (!m_numeric_goal_zone.insert(node).second)
        return;

    const auto* witness = get_numeric_witness(node);
    if (!witness || witness->get_cost() != get_numeric_cost(node))
        return;

    const auto action_binding = this->get_action_binding(*witness);
    const auto use_numeric_target = use_expanded_numeric_target();
    const auto residual = action_binding ?
                              (use_numeric_target ? get_numeric_witness_edge_residual_cost(*witness, node) : get_witness_edge_residual_cost(*witness)) :
                              datalog::Cost(0);
    if (action_binding && residual > 0)
        return;

    const auto& preconditions =
        use_numeric_target ? get_numeric_witness_max_preconditions(*witness, node, residual, depth) : get_witness_max_preconditions(*witness, residual, depth);
    for (const auto& precondition : preconditions)
        mark_goal_zone(precondition, depth + 1);
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
bool LMCutImplementation<Kind, TP>::is_before_goal_zone(Precondition precondition, size_t depth)
{
    return std::visit([&](auto node) { return is_before_goal_zone(node, depth); }, precondition);
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
bool LMCutImplementation<Kind, TP>::is_before_goal_zone(PredicateHead head, size_t depth)
{
    if (m_goal_zone.contains(head))
        return false;
    if (m_before_goal_zone.contains(head))
        return true;
    if (m_not_before_goal_zone.contains(head))
        return false;

    m_not_before_goal_zone.insert(head);

    auto has_achiever = false;
    auto before = false;
    const auto head_cost = this->get_predicate_cost(head);
    this->for_each_achiever(head,
                            [&](const auto& witness)
                            {
                                if (before || (use_numeric_edge_cuts() && witness.get_cost() != head_cost))
                                    return;

                                has_achiever = true;
                                const auto action_binding = this->get_action_binding(witness);
                                const auto residual = action_binding ? get_witness_edge_residual_cost(witness) : datalog::Cost(0);
                                const auto& preconditions = get_witness_max_preconditions(witness, residual, depth);
                                before = preconditions.empty()
                                         || std::ranges::any_of(preconditions,
                                                                [&](const auto precondition) { return is_before_goal_zone(precondition, depth + 1); });
                            });

    if (!has_achiever)
        before = true;

    if (before)
    {
        m_not_before_goal_zone.erase(head);
        m_before_goal_zone.insert(head);
        return true;
    }

    return false;
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
bool LMCutImplementation<Kind, TP>::is_before_goal_zone(NumericNode node, size_t depth)
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
        const auto action_binding = this->get_action_binding(*witness);
        const auto use_numeric_target = use_expanded_numeric_target();
        const auto residual = action_binding ?
                                  (use_numeric_target ? get_numeric_witness_edge_residual_cost(*witness, node) : get_witness_edge_residual_cost(*witness)) :
                                  datalog::Cost(0);
        const auto& preconditions = use_numeric_target ? get_numeric_witness_max_preconditions(*witness, node, residual, depth) :
                                                         get_witness_max_preconditions(*witness, residual, depth);
        before =
            preconditions.empty() || std::ranges::any_of(preconditions, [&](const auto precondition) { return is_before_goal_zone(precondition, depth + 1); });
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

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
void LMCutImplementation<Kind, TP>::clear_zones() noexcept
{
    m_goal_zone.clear();
    m_numeric_goal_zone.clear();
    m_before_goal_zone.clear();
    m_numeric_before_goal_zone.clear();
    m_not_before_goal_zone.clear();
    m_numeric_not_before_goal_zone.clear();
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
void LMCutImplementation<Kind, TP>::seed_goal_zone()
{
    const auto goal_cost = this->get_goal_cost();
    if (const auto& goal = this->m_workspace.tp.get_goal())
    {
        for (const auto literal : goal->template get_literals<f::FluentTag>())
        {
            const auto head = literal.get_atom().get_row();
            if (literal.get_polarity() && this->get_predicate_cost(head) == goal_cost)
            {
                mark_goal_zone(head, 0);
                break;
            }
        }

        const auto& selector = this->m_workspace.get_numeric_support_selector();
        for (const auto constraint : goal->get_numeric_constraints())
        {
            if (selector.get_constraint_cost(constraint, m_numeric_support_selector_workspace.selection, datalog::MaxAggregation {}) != goal_cost)
                continue;

            for (const auto& entry : m_numeric_support_selector_workspace.selection)
                if (entry.cost == goal_cost)
                    mark_goal_zone(NumericNode { entry.key, entry.interval }, 0);
        }
    }
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
void LMCutImplementation<Kind, TP>::clear_evaluation_views() noexcept
{
    clear_zones();
    m_action_used_costs.clear();
    m_rule_edge_used_costs.clear();
    m_numeric_edge_used_costs.clear();
    m_cut.clear();
    m_rule_cut.clear();
    m_numeric_cut.clear();
    for (auto& buffer : m_max_precondition_buffers)
        buffer.clear();
    m_numeric_support_selector_workspace.clear();
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
void LMCutImplementation<Kind, TP>::append_cut_frontier_atom(PredicateHead head, CutFrontierAtoms* cut_frontier_atoms)
{
    if (cut_frontier_atoms)
        this->append_planning_cut_frontier_atom(head, *cut_frontier_atoms);
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
void LMCutImplementation<Kind, TP>::extract_cut(CutFrontierAtoms* cut_frontier_atoms)
{
    clear_zones();
    m_cut.clear();
    seed_goal_zone();

    auto inspect_witness = [&](const auto& witness, const PredicateHead* frontier_head)
    {
        const auto action_binding = this->get_action_binding(witness);
        if (!action_binding)
            return;

        const auto residual = get_witness_edge_residual_cost(witness);
        if (residual == datalog::Cost(0))
            return;

        const auto& preconditions = get_witness_max_preconditions(witness, residual, 0);
        const auto crosses_cut =
            preconditions.empty() || std::ranges::any_of(preconditions, [&](const auto precondition) { return is_before_goal_zone(precondition, 1); });
        if (crosses_cut)
        {
            const auto [it, inserted] = m_cut.emplace(*action_binding, residual);
            if (!inserted)
                it->second = std::min(it->second, residual);
            if (frontier_head)
                append_cut_frontier_atom(*frontier_head, cut_frontier_atoms);
        }
    };

    for (const auto head : m_goal_zone)
        this->for_each_achiever(head, [&](const auto& witness) { inspect_witness(witness, &head); });

    for (const auto& node : m_numeric_goal_zone)
        if (const auto* witness = get_numeric_witness(node); witness && witness->get_cost() == get_numeric_cost(node))
            inspect_witness(*witness, nullptr);
}

template<TaskKind Kind, datalog::TerminationPolicyConcept<Kind> TP>
void LMCutImplementation<Kind, TP>::extract_expanded_cut(CutFrontierAtoms* cut_frontier_atoms)
{
    clear_zones();
    m_rule_cut.clear();
    m_numeric_cut.clear();
    seed_goal_zone();

    auto inspect_rule_witness = [&](const auto& witness, PredicateHead frontier_head)
    {
        if (!this->get_action_binding(witness))
            return;
        const auto residual = get_witness_edge_residual_cost(witness);
        if (residual == datalog::Cost(0))
            return;

        const auto& preconditions = get_witness_max_preconditions(witness, residual, 0);
        const auto crosses_cut =
            preconditions.empty() || std::ranges::any_of(preconditions, [&](const auto precondition) { return is_before_goal_zone(precondition, 1); });
        if (crosses_cut)
        {
            const auto [it, inserted] = m_rule_cut.emplace(witness.get_rule_key(), residual);
            if (!inserted)
                it->second = std::min(it->second, residual);
            append_cut_frontier_atom(frontier_head, cut_frontier_atoms);
        }
    };

    auto inspect_numeric_witness = [&](NumericNode node, const auto& witness)
    {
        if (!this->get_action_binding(witness))
            return;
        const auto residual = use_expanded_numeric_target() ? get_numeric_witness_edge_residual_cost(witness, node) : get_witness_edge_residual_cost(witness);
        if (residual == datalog::Cost(0))
            return;

        const auto& preconditions = use_expanded_numeric_target() ? get_numeric_witness_max_preconditions(witness, node, residual, 0) :
                                                                    get_witness_max_preconditions(witness, residual, 0);
        const auto crosses_cut =
            preconditions.empty() || std::ranges::any_of(preconditions, [&](const auto precondition) { return is_before_goal_zone(precondition, 1); });
        if (crosses_cut)
        {
            const auto edge = NumericEdge { witness.get_rule_key(), node.key, node.interval };
            const auto [it, inserted] = m_numeric_cut.emplace(edge, residual);
            if (!inserted)
                it->second = std::min(it->second, residual);
        }
    };

    for (const auto head : m_goal_zone)
    {
        const auto head_cost = this->get_predicate_cost(head);
        this->for_each_achiever(head,
                                [&](const auto& witness)
                                {
                                    if (witness.get_cost() == head_cost)
                                        inspect_rule_witness(witness, head);
                                });
    }

    for (const auto& node : m_numeric_goal_zone)
        if (const auto* witness = get_numeric_witness(node); witness && witness->get_cost() == get_numeric_cost(node))
            inspect_numeric_witness(node, *witness);
}

}

template<TaskKind Kind>
struct LMCutHeuristic<Kind>::Impl
{
    using FullModelImplementation = LMCutImplementation<Kind, datalog::FullModelGoalPolicy<Kind, datalog::MaxAggregation>>;
    using GoalDirectedImplementation = LMCutImplementation<Kind, datalog::TerminationPolicy<Kind, datalog::MaxAggregation>>;
    using Implementation = std::variant<FullModelImplementation, GoalDirectedImplementation>;
    using CutFrontierAtoms = f::planning::GroundAtomViewList<f::FluentTag>;

    Impl(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) :
        m_implementation(make_implementation(std::move(task), std::move(execution_context), cost_mode))
    {
    }

    Impl(const Impl& source, ygg::ExecutionContextPtr execution_context) :
        m_implementation(std::visit(
            [&](const auto& implementation) -> Implementation
            {
                using T = std::decay_t<decltype(implementation)>;
                return Implementation(std::in_place_type<T>, implementation, std::move(execution_context));
            },
            source.m_implementation))
    {
    }

    void set_goal(f::planning::GroundConjunctiveConditionView goal)
    {
        std::visit([&](auto& implementation) { implementation.set_goal(goal); }, m_implementation);
    }

    ygg::float_t evaluate(const ygg::Builder<State<Kind>>& state, CutFrontierAtoms* cut_frontier_atoms = nullptr)
    {
        return std::visit([&](auto& implementation) { return implementation.evaluate(state, cut_frontier_atoms); }, m_implementation);
    }

    void print_summary(size_t verbosity) const
    {
        std::visit([&](const auto& implementation) { implementation.print_summary(verbosity); }, m_implementation);
    }

private:
    static Implementation make_implementation(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode)
    {
        auto definition = std::make_shared<detail::RPGDefinition<Kind>>(std::move(task), cost_mode);
        if (requires_numeric_edge_cuts(definition->rpg_program.get_datalog_program().get_program()))
            return Implementation(std::in_place_type<GoalDirectedImplementation>, std::move(definition), std::move(execution_context), cost_mode);
        return Implementation(std::in_place_type<FullModelImplementation>, std::move(definition), std::move(execution_context), cost_mode);
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
{
    return std::make_shared<LMCutHeuristic<Kind>>(std::move(task), std::move(execution_context), cost_mode);
}

template<TaskKind Kind>
void LMCutHeuristic<Kind>::set_goal(f::planning::GroundConjunctiveConditionView goal)
{
    m_impl->set_goal(goal);
}

template<TaskKind Kind>
ygg::float_t LMCutHeuristic<Kind>::evaluate(const ygg::Builder<State<Kind>>& state)
{
    return m_impl->evaluate(state);
}

template<TaskKind Kind>
f::planning::GroundAtomViewList<f::FluentTag> LMCutHeuristic<Kind>::compute_cut_frontier_atoms(const ygg::Builder<State<Kind>>& state)
{
    auto atoms = f::planning::GroundAtomViewList<f::FluentTag> {};
    static_cast<void>(m_impl->evaluate(state, &atoms));
    std::sort(atoms.begin(), atoms.end());
    atoms.erase(std::unique(atoms.begin(), atoms.end()), atoms.end());
    return atoms;
}

template<TaskKind Kind>
HeuristicPtr<Kind> LMCutHeuristic<Kind>::make_worker(ygg::ExecutionContextPtr execution_context) const
{
    return HeuristicPtr<Kind>(new LMCutHeuristic(std::make_unique<Impl>(*m_impl, std::move(execution_context))));
}

template<TaskKind Kind>
void LMCutHeuristic<Kind>::print_summary(size_t verbosity) const
{
    m_impl->print_summary(verbosity);
}

template class LMCutHeuristic<GroundTag>;
template class LMCutHeuristic<LiftedTag>;

}
