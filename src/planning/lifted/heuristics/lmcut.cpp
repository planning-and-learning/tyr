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

#include <algorithm>
#include <limits>
#include <ranges>
#include <type_traits>
#include <variant>
#include <yggdrasil/containers/variant.hpp>

namespace tyr::planning
{
namespace
{
namespace f = ::tyr::formalism;
namespace fd = ::tyr::formalism::datalog;

template<typename Head>
bool is_numeric_head(const Head&) noexcept
{
    return false;
}

bool is_numeric_head(fd::NumericEffectOperatorView<f::FluentTag>) noexcept { return true; }

bool needs_expanded_lmcut(auto program)
{
    for (const auto rule : program.get_rules())
    {
        if (!rule.get_body().get_numeric_constraints().empty())
            return true;
        if (ygg::visit([](auto&& head) { return is_numeric_head(head); }, rule.get_head()))
            return true;
    }
    return false;
}

}

LMCutHeuristic<LiftedTag>::LMCutHeuristic(TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) :
    Base(std::move(task),
         std::move(execution_context),
         datalog::OrAnnotationPolicy<LiftedTag>(),
         datalog::AchieverAndAnnotationPolicy<LiftedTag, datalog::MaxAggregation>(),
         cost_mode),
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
    m_max_precondition_depth(0),
    m_use_expanded_edges(needs_expanded_lmcut(m_rpg_program.get_datalog_program().get_program()))
{
}

LMCutHeuristicPtr<LiftedTag> LMCutHeuristic<LiftedTag>::create(TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode)
{
    return std::make_shared<LMCutHeuristic<LiftedTag>>(std::move(task), std::move(execution_context), cost_mode);
}

ygg::float_t LMCutHeuristic<LiftedTag>::evaluate(const StateView<LiftedTag>& state)
{
    auto value = datalog::Cost(0);
    m_residual_costs.clear();
    m_rule_edge_used_costs.clear();
    m_numeric_edge_used_costs.clear();

    while (true)
    {
        apply_residual_costs();
        const auto hmax = Base::evaluate(state);
        if (hmax == std::numeric_limits<ygg::float_t>::infinity())
            return hmax;

        const auto hmax_cost = datalog::Cost(hmax);
        if (hmax_cost == 0)
            return ygg::float_t(value);

        auto cut_cost = std::numeric_limits<datalog::Cost>::max();
        if (m_use_expanded_edges)
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
                use_rule_edge_cost(edge, cut_cost);
            for (const auto& [edge, _] : m_numeric_cut)
                use_numeric_edge_cost(edge, cut_cost);
        }
        else
        {
            extract_cut();
            if (m_cut.empty())
                return ygg::float_t(value + hmax_cost);

            for (const auto action_binding : m_cut)
                cut_cost = std::min(cut_cost, get_residual_cost(action_binding));

            assert(cut_cost > 0 && cut_cost != std::numeric_limits<datalog::Cost>::max());

            value += cut_cost;
            for (const auto action_binding : m_cut)
                set_residual_cost(action_binding, get_residual_cost(action_binding) - cut_cost);
        }
    }
}

ygg::float_t LMCutHeuristic<LiftedTag>::extract_cost_and_set_preferred_actions_impl(const StateView<LiftedTag>&) { return get_goal_cost(); }

LMCutHeuristic<LiftedTag>::RuleEdge LMCutHeuristic<LiftedTag>::make_rule_edge(const datalog::WitnessAnnotation<LiftedTag>& witness) const
{
    const auto rule_binding = witness.get_rule_key();
    auto objects = ygg::IndexList<f::Object> {};
    for (const auto object : rule_binding.get_objects())
        objects.push_back(object.get_index());
    return RuleEdge { rule_binding.get_relation().get_index(), std::move(objects) };
}

LMCutHeuristic<LiftedTag>::NumericEdge LMCutHeuristic<LiftedTag>::make_numeric_edge(const datalog::WitnessAnnotation<LiftedTag>& witness,
                                                                                    NumericNode node) const
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

datalog::Cost LMCutHeuristic<LiftedTag>::get_residual_cost(ActionBinding action_binding) const
{
    const auto it = m_residual_costs.find(action_binding);
    return it == m_residual_costs.end() ? datalog::Cost(1) : it->second;
}

void LMCutHeuristic<LiftedTag>::set_residual_cost(ActionBinding action_binding, datalog::Cost cost) { m_residual_costs.insert_or_assign(action_binding, cost); }

datalog::Cost LMCutHeuristic<LiftedTag>::get_witness_body_cost(const datalog::WitnessAnnotation<LiftedTag>& witness)
{
    auto body_cost = datalog::Cost(0);
    for_each_witness_precondition(witness, [&](const auto precondition) { body_cost = std::max(body_cost, get_binding_cost(precondition)); });

    for (const auto& support : witness.get_numeric_supports())
        body_cost = std::max(body_cost, support.get_cost());

    return body_cost;
}

datalog::Cost LMCutHeuristic<LiftedTag>::get_witness_edge_residual_cost(const datalog::WitnessAnnotation<LiftedTag>& witness)
{
    const auto body_cost = get_witness_body_cost(witness);
    return witness.get_cost() <= body_cost ? datalog::Cost(0) : witness.get_cost() - body_cost;
}

bool LMCutHeuristic<LiftedTag>::is_target_support(const datalog::NumericSupport<LiftedTag>& support, NumericNode node) const noexcept
{
    const auto binding = support.get_key();
    return binding.get_relation().get_index() == node.binding.get_relation().get_index()
           && binding.get_objects().get_data() == node.binding.get_objects().get_data() && support.get_interval() == node.interval;
}

datalog::Cost LMCutHeuristic<LiftedTag>::get_expanded_numeric_support_cost(const datalog::NumericSupport<LiftedTag>& support) const
{
    const auto binding = support.get_key();
    const auto* entries = m_workspace.numeric_and_annot.find_entries(binding.get_relation(), binding.get_index().row);
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

void LMCutHeuristic<LiftedTag>::append_expanded_numeric_support_preconditions(const datalog::NumericSupport<LiftedTag>& support,
                                                                              datalog::Cost body_cost,
                                                                              std::vector<Precondition>& result) const
{
    const auto binding = support.get_key();
    const auto* entries = m_workspace.numeric_and_annot.find_entries(binding.get_relation(), binding.get_index().row);
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

datalog::Cost LMCutHeuristic<LiftedTag>::get_numeric_witness_body_cost(const datalog::WitnessAnnotation<LiftedTag>& witness, NumericNode node)
{
    auto body_cost = datalog::Cost(0);
    for_each_witness_precondition(witness, [&](const auto precondition) { body_cost = std::max(body_cost, get_binding_cost(precondition)); });

    for (const auto& support : witness.get_numeric_supports())
        if (!is_target_support(support, node))
            body_cost = std::max(body_cost, get_expanded_numeric_support_cost(support));

    return body_cost;
}

datalog::Cost LMCutHeuristic<LiftedTag>::get_numeric_witness_edge_residual_cost(const datalog::WitnessAnnotation<LiftedTag>& witness, NumericNode node)
{
    const auto body_cost = get_numeric_witness_body_cost(witness, node);
    return witness.get_cost() <= body_cost ? datalog::Cost(0) : witness.get_cost() - body_cost;
}

void LMCutHeuristic<LiftedTag>::use_rule_edge_cost(RuleEdge edge, datalog::Cost cost)
{
    auto& used = m_rule_edge_used_costs[edge];
    used += cost;
}

void LMCutHeuristic<LiftedTag>::use_numeric_edge_cost(NumericEdge edge, datalog::Cost cost)
{
    auto& used = m_numeric_edge_used_costs[edge];
    used += cost;
}

void LMCutHeuristic<LiftedTag>::apply_residual_costs()
{
    m_workspace.clear_costs();
    for (const auto& [action_binding, cost] : m_residual_costs)
        set_action_binding_cost(action_binding, datalog::Cost(1) - cost);
    auto make_rule_binding = [&](const RuleEdge& edge)
    {
        auto binding_ptr = m_workspace.datalog_builder.template get_builder<f::RelationBinding<fd::Rule>>();
        auto& binding = *binding_ptr;
        binding.clear();
        binding.relation = edge.rule;
        binding.objects = edge.objects;
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
        m_workspace.cost_policy.set_cost(make_rule_binding(edge), used_cost);
    for (const auto& [edge, used_cost] : m_numeric_edge_used_costs)
        m_workspace.cost_policy.set_cost(make_rule_binding(RuleEdge { edge.rule, edge.rule_objects }), make_function_binding(edge), edge.interval, used_cost);
}

datalog::Cost LMCutHeuristic<LiftedTag>::get_numeric_cost(NumericNode node) const noexcept
{
    const auto* annotation = m_workspace.numeric_and_annot.find(node.binding, node.interval);
    return annotation ? datalog::get_cost(*annotation) : datalog::Cost(0);
}

const datalog::WitnessAnnotation<LiftedTag>* LMCutHeuristic<LiftedTag>::get_numeric_witness(NumericNode node) const noexcept
{
    const auto* annotation = m_workspace.numeric_and_annot.find(node.binding, node.interval);
    return annotation ? std::get_if<datalog::WitnessAnnotation<LiftedTag>>(annotation) : nullptr;
}

const std::vector<LMCutHeuristic<LiftedTag>::Precondition>&
LMCutHeuristic<LiftedTag>::get_witness_max_preconditions(const datalog::WitnessAnnotation<LiftedTag>& witness, datalog::Cost edge_cost)
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
                                      if (get_binding_cost(precondition) == body_cost)
                                          result.emplace_back(precondition);
                                  });

    for (const auto& support : witness.get_numeric_supports())
        if (support.get_cost() == body_cost)
            result.emplace_back(NumericNode { support.get_key(), support.get_interval() });

    return result;
}

const std::vector<LMCutHeuristic<LiftedTag>::Precondition>&
LMCutHeuristic<LiftedTag>::get_numeric_witness_max_preconditions(const datalog::WitnessAnnotation<LiftedTag>& witness,
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
                                      if (get_binding_cost(precondition) == body_cost)
                                          result.emplace_back(precondition);
                                  });

    for (const auto& support : witness.get_numeric_supports())
        if (!is_target_support(support, node))
            append_expanded_numeric_support_preconditions(support, body_cost, result);

    return result;
}

void LMCutHeuristic<LiftedTag>::release_witness_max_preconditions()
{
    assert(m_max_precondition_depth > 0);
    --m_max_precondition_depth;
}

void LMCutHeuristic<LiftedTag>::mark_goal_zone(Precondition precondition)
{
    std::visit([&](auto node) { mark_goal_zone(node); }, precondition);
}

void LMCutHeuristic<LiftedTag>::mark_goal_zone(PredicateBinding binding)
{
    if (!m_goal_zone.insert(binding).second)
        return;

    const auto binding_cost = get_binding_cost(binding);
    for_each_achiever(binding,
                      [&](const auto& witness)
                      {
                          if (witness.get_cost() != binding_cost)
                              return;

                          const auto action_binding = get_action_binding(witness);
                          const auto residual = m_use_expanded_edges ? (action_binding ? get_witness_edge_residual_cost(witness) : datalog::Cost(0)) :
                                                                       (action_binding ? get_residual_cost(*action_binding) : datalog::Cost(0));
                          if (action_binding && residual > 0)
                              return;

                          const auto& preconditions = get_witness_max_preconditions(witness, residual);
                          for (const auto& precondition : preconditions)
                              mark_goal_zone(precondition);
                          release_witness_max_preconditions();
                      });
}

void LMCutHeuristic<LiftedTag>::mark_goal_zone(NumericNode node)
{
    if (!m_numeric_goal_zone.insert(node).second)
        return;

    const auto* witness = get_numeric_witness(node);
    if (!witness || witness->get_cost() != get_numeric_cost(node))
        return;

    const auto action_binding = get_action_binding(*witness);
    const auto use_numeric_target = m_use_expanded_edges && m_cost_mode == CostMode::GENERAL;
    const auto residual =
        m_use_expanded_edges ?
            (action_binding ? (use_numeric_target ? get_numeric_witness_edge_residual_cost(*witness, node) : get_witness_edge_residual_cost(*witness)) :
                              datalog::Cost(0)) :
            (action_binding ? get_residual_cost(*action_binding) : datalog::Cost(0));
    if (action_binding && residual > 0)
        return;

    const auto& preconditions =
        use_numeric_target ? get_numeric_witness_max_preconditions(*witness, node, residual) : get_witness_max_preconditions(*witness, residual);
    for (const auto& precondition : preconditions)
        mark_goal_zone(precondition);
    release_witness_max_preconditions();
}

bool LMCutHeuristic<LiftedTag>::is_before_goal_zone(Precondition precondition)
{
    return std::visit([&](auto node) { return is_before_goal_zone(node); }, precondition);
}

bool LMCutHeuristic<LiftedTag>::is_before_goal_zone(PredicateBinding binding)
{
    if (m_goal_zone.contains(binding))
        return false;
    if (m_before_goal_zone.contains(binding))
        return true;
    if (m_not_before_goal_zone.contains(binding))
        return false;

    m_not_before_goal_zone.insert(binding);

    auto has_optimal_achiever = false;
    auto before = false;
    const auto binding_cost = get_binding_cost(binding);
    for_each_achiever(binding,
                      [&](const auto& witness)
                      {
                          if (before || witness.get_cost() != binding_cost)
                              return;

                          has_optimal_achiever = true;

                          const auto action_binding = get_action_binding(witness);
                          const auto residual = m_use_expanded_edges ? (action_binding ? get_witness_edge_residual_cost(witness) : datalog::Cost(0)) :
                                                                       (action_binding ? get_residual_cost(*action_binding) : datalog::Cost(0));
                          const auto& preconditions = get_witness_max_preconditions(witness, residual);
                          before = preconditions.empty()
                                   || std::ranges::any_of(preconditions, [&](const auto precondition) { return is_before_goal_zone(precondition); });
                          release_witness_max_preconditions();
                      });

    if (!has_optimal_achiever)
        before = true;

    if (before)
    {
        m_not_before_goal_zone.erase(binding);
        m_before_goal_zone.insert(binding);
        return true;
    }

    return false;
}

bool LMCutHeuristic<LiftedTag>::is_before_goal_zone(NumericNode node)
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
        const auto use_numeric_target = m_use_expanded_edges && m_cost_mode == CostMode::GENERAL;
        const auto residual =
            m_use_expanded_edges ?
                (action_binding ? (use_numeric_target ? get_numeric_witness_edge_residual_cost(*witness, node) : get_witness_edge_residual_cost(*witness)) :
                                  datalog::Cost(0)) :
                (action_binding ? get_residual_cost(*action_binding) : datalog::Cost(0));
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

void LMCutHeuristic<LiftedTag>::extract_cut()
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
        for (const auto literal : goal->get_literals<::tyr::formalism::FluentTag>())
        {
            if (literal.get_polarity() && get_binding_cost(literal.get_atom().get_row()) == goal_cost)
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

    auto inspect_witness = [&](const auto& witness)
    {
        const auto action_binding = get_action_binding(witness);
        if (!action_binding || get_residual_cost(*action_binding) == 0)
            return;

        const auto& preconditions = get_witness_max_preconditions(witness, get_residual_cost(*action_binding));
        const auto crosses_cut =
            preconditions.empty() || std::ranges::any_of(preconditions, [&](const auto precondition) { return is_before_goal_zone(precondition); });
        release_witness_max_preconditions();
        if (crosses_cut)
            m_cut.insert(*action_binding);
    };

    for (const auto binding : m_goal_zone)
    {
        const auto binding_cost = get_binding_cost(binding);
        for_each_achiever(binding,
                          [&](const auto& witness)
                          {
                              if (witness.get_cost() == binding_cost)
                                  inspect_witness(witness);
                          });
    }

    for (const auto& node : m_numeric_goal_zone)
        if (const auto* witness = get_numeric_witness(node); witness && witness->get_cost() == get_numeric_cost(node))
            inspect_witness(*witness);
}

void LMCutHeuristic<LiftedTag>::extract_expanded_cut()
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
        for (const auto literal : goal->get_literals<::tyr::formalism::FluentTag>())
        {
            if (literal.get_polarity() && get_binding_cost(literal.get_atom().get_row()) == goal_cost)
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

    auto inspect_rule_witness = [&](const auto& witness)
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
        }
    };

    auto inspect_numeric_witness = [&](const NumericNode node, const auto& witness)
    {
        if (!get_action_binding(witness))
            return;
        const auto residual =
            m_cost_mode == CostMode::GENERAL ? get_numeric_witness_edge_residual_cost(witness, node) : get_witness_edge_residual_cost(witness);
        if (residual == datalog::Cost(0))
            return;

        const auto& preconditions = m_cost_mode == CostMode::GENERAL ? get_numeric_witness_max_preconditions(witness, node, residual) :
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
        const auto binding_cost = get_binding_cost(binding);
        for_each_achiever(binding,
                          [&](const auto& witness)
                          {
                              if (witness.get_cost() == binding_cost)
                                  inspect_rule_witness(witness);
                          });
    }

    for (const auto& node : m_numeric_goal_zone)
        if (const auto* witness = get_numeric_witness(node); witness && witness->get_cost() == get_numeric_cost(node))
            inspect_numeric_witness(node, *witness);
}

}
