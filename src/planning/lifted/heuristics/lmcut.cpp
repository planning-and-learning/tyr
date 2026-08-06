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

#include "tyr/planning/lifted/heuristics/lmcut.hpp"

#include "rpg.hpp"
#include "tyr/datalog/lifted/policies/annotation.hpp"
#include "tyr/datalog/lifted/policies/cost.hpp"
#include "tyr/datalog/lifted/policies/numeric_support.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/formalism/planning/merge_planning.hpp"

#include <algorithm>
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

struct LiftedLMCutNumericNode : ygg::comparison::Mixin<LiftedLMCutNumericNode>
{
    fd::FunctionBindingView<f::FluentTag> binding;
    ygg::ClosedInterval<ygg::float_t> interval;

    LiftedLMCutNumericNode() = delete;
    LiftedLMCutNumericNode(fd::FunctionBindingView<f::FluentTag> binding, ygg::ClosedInterval<ygg::float_t> interval) : binding(binding), interval(interval) {}

    auto identifying_members() const noexcept { return std::make_tuple(binding, lower(interval), upper(interval)); }
};

struct LiftedLMCutRuleEdge : ygg::comparison::Mixin<LiftedLMCutRuleEdge>
{
    ygg::Index<fd::Rule<f::PredicateTag>> rule;
    ygg::IndexList<f::Object> objects;

    LiftedLMCutRuleEdge() = default;
    LiftedLMCutRuleEdge(ygg::Index<fd::Rule<f::PredicateTag>> rule, ygg::IndexList<f::Object> objects) : rule(rule), objects(std::move(objects)) {}

    auto identifying_members() const noexcept { return std::tie(rule, objects); }
};

struct LiftedLMCutNumericEdge : ygg::comparison::Mixin<LiftedLMCutNumericEdge>
{
    ygg::Index<fd::Rule<f::FunctionTag>> rule;
    ygg::IndexList<f::Object> rule_objects;
    ygg::Index<f::Function<f::FluentTag>> function;
    ygg::IndexList<f::Object> function_objects;
    ygg::ClosedInterval<ygg::float_t> interval;

    LiftedLMCutNumericEdge() = default;
    LiftedLMCutNumericEdge(ygg::Index<fd::Rule<f::FunctionTag>> rule,
                           ygg::IndexList<f::Object> rule_objects,
                           ygg::Index<f::Function<f::FluentTag>> function,
                           ygg::IndexList<f::Object> function_objects,
                           ygg::ClosedInterval<ygg::float_t> interval) :
        rule(rule),
        rule_objects(std::move(rule_objects)),
        function(function),
        function_objects(std::move(function_objects)),
        interval(interval)
    {
    }

    auto identifying_members() const noexcept { return std::make_tuple(rule, rule_objects, function, function_objects, lower(interval), upper(interval)); }
};

}

struct LMCutHeuristic<LiftedTag>::Impl :
    detail::RPGEvaluator<LiftedTag,
                         Impl,
                         datalog::OrAnnotationPolicy<LiftedTag>,
                         datalog::AchieverAndAnnotationPolicy<LiftedTag, datalog::MaxAggregation>,
                         datalog::TerminationPolicy<LiftedTag, datalog::MaxAggregation>,
                         datalog::RuleCostOverridePolicy<LiftedTag>>
{
    using Base = detail::RPGEvaluator<LiftedTag,
                                      Impl,
                                      datalog::OrAnnotationPolicy<LiftedTag>,
                                      datalog::AchieverAndAnnotationPolicy<LiftedTag, datalog::MaxAggregation>,
                                      datalog::TerminationPolicy<LiftedTag, datalog::MaxAggregation>,
                                      datalog::RuleCostOverridePolicy<LiftedTag>>;
    using ActionBinding = ::tyr::formalism::planning::ActionBindingView;
    using PredicateBinding = fd::PredicateBindingView<f::FluentTag>;
    using NumericNode = LiftedLMCutNumericNode;
    using RuleEdge = LiftedLMCutRuleEdge;
    using NumericEdge = LiftedLMCutNumericEdge;
    using Precondition = std::variant<PredicateBinding, NumericNode>;
    using CutFrontierAtoms = f::planning::GroundAtomViewList<f::FluentTag>;

    Impl(TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode);
    Impl(const Impl& source, ygg::ExecutionContextPtr execution_context);
    ygg::float_t evaluate(const ygg::Builder<State<LiftedTag>>& state, CutFrontierAtoms* cut_frontier_atoms = nullptr);

private:
    RuleEdge make_rule_edge(const datalog::WitnessAnnotation<LiftedTag, f::PredicateTag>& witness) const;
    NumericEdge make_numeric_edge(const datalog::WitnessAnnotation<LiftedTag, f::FunctionTag>& witness, NumericNode node) const;
    template<f::RelationKind R>
    datalog::Cost get_witness_body_cost(const datalog::WitnessAnnotation<LiftedTag, R>& witness);
    template<f::RelationKind R>
    datalog::Cost get_witness_edge_residual_cost(const datalog::WitnessAnnotation<LiftedTag, R>& witness);
    bool is_target_support(const datalog::NumericSupport<LiftedTag>& support, NumericNode node) const noexcept;
    datalog::Cost get_expanded_numeric_support_cost(const datalog::NumericSupport<LiftedTag>& support) const;
    void append_expanded_numeric_support_preconditions(const datalog::NumericSupport<LiftedTag>& support,
                                                       datalog::Cost body_cost,
                                                       std::vector<Precondition>& result) const;
    datalog::Cost get_numeric_witness_body_cost(const datalog::WitnessAnnotation<LiftedTag, f::FunctionTag>& witness, NumericNode node);
    datalog::Cost get_numeric_witness_edge_residual_cost(const datalog::WitnessAnnotation<LiftedTag, f::FunctionTag>& witness, NumericNode node);
    void use_action_cost(ActionBinding action_binding, datalog::Cost cost);
    void use_rule_edge_cost(RuleEdge edge, datalog::Cost cost);
    void use_numeric_edge_cost(NumericEdge edge, datalog::Cost cost);
    void apply_residual_costs();
    datalog::Cost get_numeric_cost(NumericNode node) const noexcept;
    const datalog::WitnessAnnotation<LiftedTag, f::FunctionTag>* get_numeric_witness(NumericNode node) const noexcept;
    template<f::RelationKind R>
    const std::vector<Precondition>& get_witness_max_preconditions(const datalog::WitnessAnnotation<LiftedTag, R>& witness, datalog::Cost edge_cost);
    const std::vector<Precondition>&
    get_numeric_witness_max_preconditions(const datalog::WitnessAnnotation<LiftedTag, f::FunctionTag>& witness, NumericNode node, datalog::Cost edge_cost);
    void release_witness_max_preconditions();
    void mark_goal_zone(PredicateBinding binding);
    void mark_goal_zone(NumericNode node);
    void mark_goal_zone(Precondition precondition);
    bool is_before_goal_zone(PredicateBinding binding);
    bool is_before_goal_zone(NumericNode node);
    bool is_before_goal_zone(Precondition precondition);
    void clear_zones() noexcept;
    void clear_repository_views() noexcept;
    void append_cut_frontier_atom(PredicateBinding binding, CutFrontierAtoms* cut_frontier_atoms);
    void extract_cut(CutFrontierAtoms* cut_frontier_atoms);
    void extract_expanded_cut(CutFrontierAtoms* cut_frontier_atoms);

    ygg::UnorderedMap<ActionBinding, datalog::Cost> m_action_used_costs;
    ygg::UnorderedMap<RuleEdge, datalog::Cost> m_rule_edge_used_costs;
    ygg::UnorderedMap<NumericEdge, datalog::Cost> m_numeric_edge_used_costs;
    ygg::UnorderedSet<PredicateBinding> m_goal_zone;
    ygg::UnorderedSet<NumericNode> m_numeric_goal_zone;
    ygg::UnorderedSet<PredicateBinding> m_before_goal_zone;
    ygg::UnorderedSet<NumericNode> m_numeric_before_goal_zone;
    ygg::UnorderedSet<PredicateBinding> m_not_before_goal_zone;
    ygg::UnorderedSet<NumericNode> m_numeric_not_before_goal_zone;
    ygg::UnorderedMap<ActionBinding, datalog::Cost> m_cut;
    ygg::UnorderedMap<RuleEdge, datalog::Cost> m_rule_cut;
    ygg::UnorderedMap<NumericEdge, datalog::Cost> m_numeric_cut;
    std::deque<std::vector<Precondition>> m_max_precondition_buffers;
    datalog::NumericSupportSelectorWorkspace<LiftedTag> m_numeric_support_selector_workspace;
    size_t m_max_precondition_depth;
    bool m_use_expanded_edges;
};

LMCutHeuristic<LiftedTag>::Impl::Impl(TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) :
    Base(std::move(task),
         std::move(execution_context),
         datalog::OrAnnotationPolicy<LiftedTag> {},
         datalog::AchieverAndAnnotationPolicy<LiftedTag, datalog::MaxAggregation> {},
         cost_mode,
         true),
    m_action_used_costs(),
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
    m_max_precondition_depth(0),
    m_use_expanded_edges(m_definition->use_expanded_lmcut)
{
    // The propositional justification graph contains every reachable operator arc, including arcs discovered after the goal first becomes reachable.
    m_workspace.tp.set_early_termination(m_use_expanded_edges);
}

LMCutHeuristic<LiftedTag>::Impl::Impl(const Impl& source, ygg::ExecutionContextPtr execution_context) :
    Base(source,
         std::move(execution_context),
         datalog::OrAnnotationPolicy<LiftedTag> {},
         datalog::AchieverAndAnnotationPolicy<LiftedTag, datalog::MaxAggregation> {}),
    m_max_precondition_depth(0),
    m_use_expanded_edges(m_definition->use_expanded_lmcut)
{
    m_workspace.tp.set_early_termination(m_use_expanded_edges);
}

ygg::float_t LMCutHeuristic<LiftedTag>::Impl::evaluate(const ygg::Builder<State<LiftedTag>>& state, CutFrontierAtoms* cut_frontier_atoms)
{
    auto value = datalog::Cost(0);
    clear_repository_views();
    m_action_used_costs.clear();
    m_rule_edge_used_costs.clear();
    m_numeric_edge_used_costs.clear();
    begin_state_evaluation();

    while (true)
    {
        apply_residual_costs();
        m_numeric_support_selector_workspace.clear();
        const auto hmax = evaluate_current_state(state);
        if (hmax == std::numeric_limits<ygg::float_t>::infinity())
            return hmax;

        const auto hmax_cost = datalog::Cost(hmax);
        if (hmax_cost == 0)
            return ygg::float_t(value);

        auto cut_cost = std::numeric_limits<datalog::Cost>::max();
        if (m_use_expanded_edges)
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

LMCutHeuristic<LiftedTag>::Impl::RuleEdge
LMCutHeuristic<LiftedTag>::Impl::make_rule_edge(const datalog::WitnessAnnotation<LiftedTag, f::PredicateTag>& witness) const
{
    const auto rule_binding = witness.get_rule_key();
    auto objects = ygg::IndexList<f::Object> {};
    for (const auto object : rule_binding.get_objects())
        objects.push_back(object.get_index());
    return RuleEdge { rule_binding.get_relation().get_index(), std::move(objects) };
}

LMCutHeuristic<LiftedTag>::Impl::NumericEdge
LMCutHeuristic<LiftedTag>::Impl::make_numeric_edge(const datalog::WitnessAnnotation<LiftedTag, f::FunctionTag>& witness, NumericNode node) const
{
    const auto rule_binding = witness.get_rule_key();
    auto rule_objects = ygg::IndexList<f::Object> {};
    for (const auto object : rule_binding.get_objects())
        rule_objects.push_back(object.get_index());

    auto function_objects = ygg::IndexList<f::Object> {};
    for (const auto object : node.binding.get_objects())
        function_objects.push_back(object.get_index());

    return NumericEdge { rule_binding.get_relation().get_index(),
                         std::move(rule_objects),
                         node.binding.get_relation().get_index(),
                         std::move(function_objects),
                         node.interval };
}

void LMCutHeuristic<LiftedTag>::Impl::use_action_cost(ActionBinding action_binding, datalog::Cost cost) { m_action_used_costs[action_binding] += cost; }

template<f::RelationKind R>
datalog::Cost LMCutHeuristic<LiftedTag>::Impl::get_witness_body_cost(const datalog::WitnessAnnotation<LiftedTag, R>& witness)
{
    auto body_cost = datalog::Cost(0);
    for_each_witness_precondition(witness, [&](const auto precondition) { body_cost = std::max(body_cost, get_predicate_cost(precondition)); });

    for (const auto& support : witness.get_numeric_supports())
        body_cost = std::max(body_cost, support.get_cost());

    return body_cost;
}

template<f::RelationKind R>
datalog::Cost LMCutHeuristic<LiftedTag>::Impl::get_witness_edge_residual_cost(const datalog::WitnessAnnotation<LiftedTag, R>& witness)
{
    const auto body_cost = get_witness_body_cost(witness);
    return witness.get_cost() <= body_cost ? datalog::Cost(0) : witness.get_cost() - body_cost;
}

bool LMCutHeuristic<LiftedTag>::Impl::is_target_support(const datalog::NumericSupport<LiftedTag>& support, NumericNode node) const noexcept
{
    const auto binding = support.get_key();
    return binding.get_relation().get_index() == node.binding.get_relation().get_index()
           && binding.get_objects().get_data() == node.binding.get_objects().get_data() && support.get_interval() == node.interval;
}

datalog::Cost LMCutHeuristic<LiftedTag>::Impl::get_expanded_numeric_support_cost(const datalog::NumericSupport<LiftedTag>& support) const
{
    const auto binding = support.get_key();
    const auto* entries = m_workspace.numeric_and_annot.find_entries(binding.get_index().relation, binding.get_index().row);
    if (!entries)
        return support.get_cost();

    auto expanded = false;
    auto cost = datalog::Cost(0);
    const auto current = m_workspace.facts.fact_sets.function[binding];
    for (const auto& candidate : *entries)
    {
        const auto candidate_cost = datalog::get_cost(candidate.annotation);
        if (candidate_cost <= support.get_cost() && subset(candidate.interval, support.get_interval()) && subset(candidate.interval, current))
        {
            cost = std::max(cost, candidate_cost);
            expanded = true;
        }
    }

    return expanded ? cost : support.get_cost();
}

void LMCutHeuristic<LiftedTag>::Impl::append_expanded_numeric_support_preconditions(const datalog::NumericSupport<LiftedTag>& support,
                                                                                    datalog::Cost body_cost,
                                                                                    std::vector<Precondition>& result) const
{
    const auto binding = support.get_key();
    const auto* entries = m_workspace.numeric_and_annot.find_entries(binding.get_index().relation, binding.get_index().row);
    if (!entries)
    {
        if (support.get_cost() == body_cost)
            result.emplace_back(NumericNode { binding, support.get_interval() });
        return;
    }

    auto expanded = false;
    const auto current = m_workspace.facts.fact_sets.function[binding];
    for (const auto& candidate : *entries)
    {
        const auto candidate_cost = datalog::get_cost(candidate.annotation);
        if (candidate_cost <= support.get_cost() && subset(candidate.interval, support.get_interval()) && subset(candidate.interval, current))
        {
            if (candidate_cost == body_cost)
                result.emplace_back(NumericNode { binding, candidate.interval });
            expanded = true;
        }
    }

    if (!expanded && support.get_cost() == body_cost)
        result.emplace_back(NumericNode { binding, support.get_interval() });
}

datalog::Cost LMCutHeuristic<LiftedTag>::Impl::get_numeric_witness_body_cost(const datalog::WitnessAnnotation<LiftedTag, f::FunctionTag>& witness,
                                                                             NumericNode node)
{
    auto body_cost = datalog::Cost(0);
    for_each_witness_precondition(witness, [&](const auto precondition) { body_cost = std::max(body_cost, get_predicate_cost(precondition)); });

    for (const auto& support : witness.get_numeric_supports())
        if (!is_target_support(support, node))
            body_cost = std::max(body_cost, get_expanded_numeric_support_cost(support));

    return body_cost;
}

datalog::Cost LMCutHeuristic<LiftedTag>::Impl::get_numeric_witness_edge_residual_cost(const datalog::WitnessAnnotation<LiftedTag, f::FunctionTag>& witness,
                                                                                      NumericNode node)
{
    const auto body_cost = get_numeric_witness_body_cost(witness, node);
    return witness.get_cost() <= body_cost ? datalog::Cost(0) : witness.get_cost() - body_cost;
}

void LMCutHeuristic<LiftedTag>::Impl::use_rule_edge_cost(RuleEdge edge, datalog::Cost cost)
{
    auto& used = m_rule_edge_used_costs[edge];
    used += cost;
}

void LMCutHeuristic<LiftedTag>::Impl::use_numeric_edge_cost(NumericEdge edge, datalog::Cost cost)
{
    auto& used = m_numeric_edge_used_costs[edge];
    used += cost;
}

void LMCutHeuristic<LiftedTag>::Impl::apply_residual_costs()
{
    m_workspace.clear_costs();
    for (const auto& [action_binding, used_cost] : m_action_used_costs)
        set_action_binding_cost(action_binding, used_cost);
    auto make_rule_binding = [&]<f::RelationKind R>(ygg::Index<fd::Rule<R>> rule, const ygg::IndexList<f::Object>& objects)
    {
        auto binding_ptr = m_workspace.datalog_builder.template get_builder<f::RelationBinding<fd::Rule<R>>>();
        auto& binding = *binding_ptr;
        binding.clear();
        binding.relation = rule;
        binding.objects = objects;
        f::canonicalize(binding);
        return m_workspace.workspace_repository.get_or_create(binding).first;
    };

    auto make_function_binding = [&](const NumericEdge& edge)
    {
        auto binding_ptr = m_workspace.datalog_builder.template get_builder<f::RelationBinding<f::Function<f::FluentTag>>>();
        auto& binding = *binding_ptr;
        binding.clear();
        binding.relation = edge.function;
        binding.objects = edge.function_objects;
        f::canonicalize(binding);
        return m_workspace.workspace_repository.get_or_create(binding).first;
    };

    for (const auto& [edge, used_cost] : m_rule_edge_used_costs)
        m_workspace.cost_policy.set_cost(make_rule_binding(edge.rule, edge.objects), used_cost);
    for (const auto& [edge, used_cost] : m_numeric_edge_used_costs)
        m_workspace.cost_policy.set_cost(make_rule_binding(edge.rule, edge.rule_objects), make_function_binding(edge), edge.interval, used_cost);
}

datalog::Cost LMCutHeuristic<LiftedTag>::Impl::get_numeric_cost(NumericNode node) const noexcept
{
    const auto* annotation = m_workspace.numeric_and_annot.find(node.binding, node.interval);
    return annotation ? datalog::get_cost(*annotation) : datalog::Cost(0);
}

const datalog::WitnessAnnotation<LiftedTag, f::FunctionTag>* LMCutHeuristic<LiftedTag>::Impl::get_numeric_witness(NumericNode node) const noexcept
{
    const auto* annotation = m_workspace.numeric_and_annot.find(node.binding, node.interval);
    return annotation ? std::get_if<datalog::WitnessAnnotation<LiftedTag, f::FunctionTag>>(annotation) : nullptr;
}

template<f::RelationKind R>
const std::vector<LMCutHeuristic<LiftedTag>::Impl::Precondition>&
LMCutHeuristic<LiftedTag>::Impl::get_witness_max_preconditions(const datalog::WitnessAnnotation<LiftedTag, R>& witness, datalog::Cost edge_cost)
{
    if (m_max_precondition_depth == m_max_precondition_buffers.size())
        m_max_precondition_buffers.emplace_back();

    auto& result = m_max_precondition_buffers[m_max_precondition_depth++];
    result.clear();

    if (witness.get_cost() < edge_cost)
        return result;

    const auto body_cost = witness.get_cost() - edge_cost;
    for_each_witness_precondition(witness,
                                  [&](const auto precondition)
                                  {
                                      if (get_predicate_cost(precondition) == body_cost)
                                          result.emplace_back(precondition);
                                  });

    for (const auto& support : witness.get_numeric_supports())
        if (support.get_cost() == body_cost)
            result.emplace_back(NumericNode { support.get_key(), support.get_interval() });

    return result;
}

const std::vector<LMCutHeuristic<LiftedTag>::Impl::Precondition>&
LMCutHeuristic<LiftedTag>::Impl::get_numeric_witness_max_preconditions(const datalog::WitnessAnnotation<LiftedTag, f::FunctionTag>& witness,
                                                                       NumericNode node,
                                                                       datalog::Cost edge_cost)
{
    if (m_max_precondition_depth == m_max_precondition_buffers.size())
        m_max_precondition_buffers.emplace_back();

    auto& result = m_max_precondition_buffers[m_max_precondition_depth++];
    result.clear();

    if (witness.get_cost() < edge_cost)
        return result;

    const auto body_cost = witness.get_cost() - edge_cost;
    for_each_witness_precondition(witness,
                                  [&](const auto precondition)
                                  {
                                      if (get_predicate_cost(precondition) == body_cost)
                                          result.emplace_back(precondition);
                                  });

    for (const auto& support : witness.get_numeric_supports())
        if (!is_target_support(support, node))
            append_expanded_numeric_support_preconditions(support, body_cost, result);

    return result;
}

void LMCutHeuristic<LiftedTag>::Impl::release_witness_max_preconditions()
{
    assert(m_max_precondition_depth > 0);
    --m_max_precondition_depth;
}

void LMCutHeuristic<LiftedTag>::Impl::mark_goal_zone(Precondition precondition)
{
    std::visit([&](auto node) { mark_goal_zone(node); }, precondition);
}

void LMCutHeuristic<LiftedTag>::Impl::mark_goal_zone(PredicateBinding binding)
{
    if (!m_goal_zone.insert(binding).second)
        return;

    const auto binding_cost = get_predicate_cost(binding);
    for_each_achiever(binding,
                      [&](const auto& witness)
                      {
                          if (m_use_expanded_edges && witness.get_cost() != binding_cost)
                              return;

                          const auto action_binding = get_action_binding(witness);
                          const auto residual = action_binding ? get_witness_edge_residual_cost(witness) : datalog::Cost(0);
                          if (action_binding && residual > 0)
                              return;

                          const auto& preconditions = get_witness_max_preconditions(witness, residual);
                          for (const auto& precondition : preconditions)
                              mark_goal_zone(precondition);
                          release_witness_max_preconditions();
                      });
}

void LMCutHeuristic<LiftedTag>::Impl::mark_goal_zone(NumericNode node)
{
    if (!m_numeric_goal_zone.insert(node).second)
        return;

    const auto* witness = get_numeric_witness(node);
    if (!witness || witness->get_cost() != get_numeric_cost(node))
        return;

    const auto action_binding = get_action_binding(*witness);
    const auto use_numeric_target = m_use_expanded_edges && m_definition->cost_mode == CostMode::GENERAL;
    const auto residual = action_binding ?
                              (use_numeric_target ? get_numeric_witness_edge_residual_cost(*witness, node) : get_witness_edge_residual_cost(*witness)) :
                              datalog::Cost(0);
    if (action_binding && residual > 0)
        return;

    const auto& preconditions =
        use_numeric_target ? get_numeric_witness_max_preconditions(*witness, node, residual) : get_witness_max_preconditions(*witness, residual);
    for (const auto& precondition : preconditions)
        mark_goal_zone(precondition);
    release_witness_max_preconditions();
}

bool LMCutHeuristic<LiftedTag>::Impl::is_before_goal_zone(Precondition precondition)
{
    return std::visit([&](auto node) { return is_before_goal_zone(node); }, precondition);
}

bool LMCutHeuristic<LiftedTag>::Impl::is_before_goal_zone(PredicateBinding binding)
{
    if (m_goal_zone.contains(binding))
        return false;
    if (m_before_goal_zone.contains(binding))
        return true;
    if (m_not_before_goal_zone.contains(binding))
        return false;

    m_not_before_goal_zone.insert(binding);

    auto has_achiever = false;
    auto before = false;
    const auto binding_cost = get_predicate_cost(binding);
    for_each_achiever(binding,
                      [&](const auto& witness)
                      {
                          if (before || (m_use_expanded_edges && witness.get_cost() != binding_cost))
                              return;

                          has_achiever = true;

                          const auto action_binding = get_action_binding(witness);
                          const auto residual = action_binding ? get_witness_edge_residual_cost(witness) : datalog::Cost(0);
                          const auto& preconditions = get_witness_max_preconditions(witness, residual);
                          before = preconditions.empty()
                                   || std::ranges::any_of(preconditions, [&](const auto precondition) { return is_before_goal_zone(precondition); });
                          release_witness_max_preconditions();
                      });

    if (!has_achiever)
        before = true;

    if (before)
    {
        m_not_before_goal_zone.erase(binding);
        m_before_goal_zone.insert(binding);
        return true;
    }

    return false;
}

bool LMCutHeuristic<LiftedTag>::Impl::is_before_goal_zone(NumericNode node)
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
        const auto action_binding = get_action_binding(*witness);
        const auto use_numeric_target = m_use_expanded_edges && m_definition->cost_mode == CostMode::GENERAL;
        const auto residual = action_binding ?
                                  (use_numeric_target ? get_numeric_witness_edge_residual_cost(*witness, node) : get_witness_edge_residual_cost(*witness)) :
                                  datalog::Cost(0);
        const auto& preconditions =
            use_numeric_target ? get_numeric_witness_max_preconditions(*witness, node, residual) : get_witness_max_preconditions(*witness, residual);
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

void LMCutHeuristic<LiftedTag>::Impl::clear_zones() noexcept
{
    m_goal_zone.clear();
    m_numeric_goal_zone.clear();
    m_before_goal_zone.clear();
    m_numeric_before_goal_zone.clear();
    m_not_before_goal_zone.clear();
    m_numeric_not_before_goal_zone.clear();
}

void LMCutHeuristic<LiftedTag>::Impl::clear_repository_views() noexcept
{
    clear_zones();
    for (auto& buffer : m_max_precondition_buffers)
        buffer.clear();
    m_numeric_support_selector_workspace.clear();
    m_max_precondition_depth = 0;
}

void LMCutHeuristic<LiftedTag>::Impl::append_cut_frontier_atom(PredicateBinding binding, CutFrontierAtoms* cut_frontier_atoms)
{
    if (!cut_frontier_atoms)
        return;

    const auto& mapping = m_definition->rpg_program.get_translation_context().d2p.fluent_to_fluent_predicate;
    if (!mapping.contains(binding.get_relation()))
        return;

    auto merge_context = f::planning::MergePlanningContext { m_workspace.planning_builder, *get_task().get_repository() };
    cut_frontier_atoms->push_back(f::planning::merge_atom_d2p<f::FluentTag, f::FluentTag>(binding, mapping, merge_context).first);
}

void LMCutHeuristic<LiftedTag>::Impl::extract_cut(CutFrontierAtoms* cut_frontier_atoms)
{
    clear_zones();
    m_cut.clear();

    const auto goal_cost = get_goal_cost();
    if (const auto& goal = m_workspace.tp.get_goal())
    {
        for (const auto literal : goal->get_literals<::tyr::formalism::FluentTag>())
        {
            if (literal.get_polarity() && get_predicate_cost(literal.get_atom().get_row()) == goal_cost)
            {
                mark_goal_zone(literal.get_atom().get_row());
                break;
            }
        }

        for (const auto constraint : goal->get_numeric_constraints())
        {
            if (m_workspace.numeric_support_selector->get_constraint_cost(constraint, m_numeric_support_selector_workspace, datalog::MaxAggregation {})
                != goal_cost)
                continue;

            for (const auto& entry : m_numeric_support_selector_workspace.selection)
                if (entry.cost == goal_cost)
                    mark_goal_zone(NumericNode { entry.key, entry.interval });
        }
    }

    auto inspect_witness = [&](const auto& witness, const PredicateBinding* frontier_binding)
    {
        const auto action_binding = get_action_binding(witness);
        if (!action_binding)
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
            const auto [it, inserted] = m_cut.emplace(*action_binding, residual);
            if (!inserted)
                it->second = std::min(it->second, residual);
            if (frontier_binding)
                append_cut_frontier_atom(*frontier_binding, cut_frontier_atoms);
        }
    };

    for (const auto binding : m_goal_zone)
        for_each_achiever(binding, [&](const auto& witness) { inspect_witness(witness, &binding); });

    for (const auto& node : m_numeric_goal_zone)
        if (const auto* witness = get_numeric_witness(node); witness && witness->get_cost() == get_numeric_cost(node))
            inspect_witness(*witness, nullptr);
}

void LMCutHeuristic<LiftedTag>::Impl::extract_expanded_cut(CutFrontierAtoms* cut_frontier_atoms)
{
    clear_zones();
    m_rule_cut.clear();
    m_numeric_cut.clear();

    const auto goal_cost = get_goal_cost();
    if (const auto& goal = m_workspace.tp.get_goal())
    {
        for (const auto literal : goal->get_literals<::tyr::formalism::FluentTag>())
        {
            if (literal.get_polarity() && get_predicate_cost(literal.get_atom().get_row()) == goal_cost)
            {
                mark_goal_zone(literal.get_atom().get_row());
                break;
            }
        }

        for (const auto constraint : goal->get_numeric_constraints())
        {
            const auto constraint_cost =
                m_workspace.numeric_support_selector->get_constraint_cost(constraint, m_numeric_support_selector_workspace, datalog::MaxAggregation {});
            if (constraint_cost != goal_cost)
                continue;

            for (const auto& entry : m_numeric_support_selector_workspace.selection)
            {
                if (entry.cost == goal_cost)
                    mark_goal_zone(NumericNode { entry.key, entry.interval });
            }
        }
    }

    auto inspect_rule_witness = [&](const auto& witness, PredicateBinding frontier_binding)
    {
        if (!get_action_binding(witness))
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
            auto edge = make_rule_edge(witness);
            const auto [it, inserted] = m_rule_cut.emplace(std::move(edge), residual);
            if (!inserted)
                it->second = std::min(it->second, residual);
            append_cut_frontier_atom(frontier_binding, cut_frontier_atoms);
        }
    };

    auto inspect_numeric_witness = [&](const NumericNode node, const auto& witness)
    {
        if (!get_action_binding(witness))
            return;
        const auto residual =
            m_definition->cost_mode == CostMode::GENERAL ? get_numeric_witness_edge_residual_cost(witness, node) : get_witness_edge_residual_cost(witness);
        if (residual == datalog::Cost(0))
            return;

        const auto& preconditions = m_definition->cost_mode == CostMode::GENERAL ? get_numeric_witness_max_preconditions(witness, node, residual) :
                                                                                   get_witness_max_preconditions(witness, residual);
        const auto crosses_cut =
            preconditions.empty() || std::ranges::any_of(preconditions, [&](const auto precondition) { return is_before_goal_zone(precondition); });
        release_witness_max_preconditions();
        if (crosses_cut)
        {
            auto edge = make_numeric_edge(witness, node);
            const auto [it, inserted] = m_numeric_cut.emplace(std::move(edge), residual);
            if (!inserted)
                it->second = std::min(it->second, residual);
        }
    };

    for (const auto binding : m_goal_zone)
    {
        const auto binding_cost = get_predicate_cost(binding);
        for_each_achiever(binding,
                          [&](const auto& witness)
                          {
                              if (witness.get_cost() == binding_cost)
                                  inspect_rule_witness(witness, binding);
                          });
    }

    for (const auto& node : m_numeric_goal_zone)
        if (const auto* witness = get_numeric_witness(node); witness && witness->get_cost() == get_numeric_cost(node))
            inspect_numeric_witness(node, *witness);
}

LMCutHeuristic<LiftedTag>::LMCutHeuristic(TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) :
    m_impl(std::make_unique<Impl>(std::move(task), std::move(execution_context), cost_mode))
{
}

LMCutHeuristic<LiftedTag>::LMCutHeuristic(std::unique_ptr<Impl> impl) : m_impl(std::move(impl)) {}

LMCutHeuristic<LiftedTag>::~LMCutHeuristic() = default;
LMCutHeuristic<LiftedTag>::LMCutHeuristic(LMCutHeuristic&&) noexcept = default;
LMCutHeuristic<LiftedTag>& LMCutHeuristic<LiftedTag>::operator=(LMCutHeuristic&&) noexcept = default;

LMCutHeuristicPtr<LiftedTag> LMCutHeuristic<LiftedTag>::create(TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode)
{
    return std::make_shared<LMCutHeuristic<LiftedTag>>(std::move(task), std::move(execution_context), cost_mode);
}

void LMCutHeuristic<LiftedTag>::set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal) { m_impl->set_goal(goal); }
ygg::float_t LMCutHeuristic<LiftedTag>::evaluate(const ygg::Builder<State<LiftedTag>>& state) { return m_impl->evaluate(state); }
f::planning::GroundAtomViewList<f::FluentTag> LMCutHeuristic<LiftedTag>::compute_cut_frontier_atoms(const ygg::Builder<State<LiftedTag>>& state)
{
    auto atoms = f::planning::GroundAtomViewList<f::FluentTag> {};
    static_cast<void>(m_impl->evaluate(state, &atoms));
    std::sort(atoms.begin(), atoms.end());
    atoms.erase(std::unique(atoms.begin(), atoms.end()), atoms.end());
    return atoms;
}
HeuristicPtr<LiftedTag> LMCutHeuristic<LiftedTag>::make_worker(ygg::ExecutionContextPtr execution_context) const
{
    return HeuristicPtr<LiftedTag>(new LMCutHeuristic(std::make_unique<Impl>(*m_impl, std::move(execution_context))));
}
void LMCutHeuristic<LiftedTag>::print_summary(size_t verbosity) const { m_impl->print_summary(verbosity); }

}
