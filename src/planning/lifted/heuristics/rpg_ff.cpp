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

#include "tyr/planning/lifted/heuristics/rpg_ff.hpp"

#include "tyr/datalog/lifted/policies/annotation.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/formalism/datalog/expression_properties.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/grounder.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/grounder.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"
#include "tyr/planning/applicability.hpp"
#include "tyr/planning/lifted/heuristics/rpg.hpp"

#include <boost/dynamic_bitset.hpp>
#include <cassert>

namespace f = tyr::formalism;

namespace tyr::planning
{
FFRPGHeuristic<LiftedTag>::FFRPGHeuristic(TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) :
    RPGBase<LiftedTag,
            FFRPGHeuristic<LiftedTag>,
            datalog::OrAnnotationPolicy<LiftedTag>,
            datalog::AndAnnotationPolicy<LiftedTag, datalog::SumAggregation>,
            datalog::TerminationPolicy<LiftedTag, datalog::SumAggregation>>(task,
                                                                            std::move(execution_context),
                                                                            datalog::OrAnnotationPolicy<LiftedTag>(),
                                                                            datalog::AndAnnotationPolicy<LiftedTag, datalog::SumAggregation>(),
                                                                            cost_mode),
    m_markings(m_rpg_program.get_datalog_program().get_program().get_predicates<::tyr::formalism::FluentTag>().size()),
    m_function_markings(m_rpg_program.get_datalog_program().get_program().get_functions<::tyr::formalism::FluentTag>().size()),
    m_binding(),
    m_iter_workspace(),
    m_grounder_cache(),
    m_effect_families(),
    m_numeric_support_selector_workspace(),
    m_relaxed_plan(),
    m_preferred_actions(),
    m_preferred_action_views(),
    m_preferred_action_views_dirty(true)
{
}

FFRPGHeuristicPtr<LiftedTag> FFRPGHeuristic<LiftedTag>::create(TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode)
{
    return std::make_shared<FFRPGHeuristic<LiftedTag>>(std::move(task), std::move(execution_context), cost_mode);
}

ygg::float_t FFRPGHeuristic<LiftedTag>::extract_cost_and_set_preferred_actions_impl(const StateView<LiftedTag>& state)
{
    m_preferred_action_views_dirty = true;
    m_relaxed_plan.clear();
    m_preferred_actions.clear();
    m_numeric_support_selector_workspace.clear();
    for (auto& bitset : m_markings)
        bitset.reset();
    for (auto& bitset : m_function_markings)
        bitset.reset();

    auto state_context = StateContext<LiftedTag>(*this->m_task, state.get_unpacked_state(), ygg::float_t(0));
    auto grounder_context = ::tyr::formalism::planning::GrounderContext { this->m_workspace.planning_builder, *this->m_task->get_repository(), m_binding };

    if (const auto& goal = m_workspace.tp.get_goal())
    {
        for (const auto literal : goal->get_literals<::tyr::formalism::FluentTag>())
        {
            assert(literal.get_polarity());

            extract_relaxed_plan_and_preferred_actions(literal.get_atom().get_row(), state_context, grounder_context);
        }

        for (const auto constraint : goal->get_numeric_constraints())
            extract_numeric_constraint_support(constraint, state_context, grounder_context);
    }

    return m_relaxed_plan.size();
}

const ygg::UnorderedSet<ygg::Index<::tyr::formalism::planning::GroundAction>>& FFRPGHeuristic<LiftedTag>::get_preferred_actions()
{
    return m_preferred_actions;
}

const ygg::UnorderedSet<::tyr::formalism::planning::GroundActionView>& FFRPGHeuristic<LiftedTag>::get_preferred_action_views()
{
    if (m_preferred_action_views_dirty)
    {
        m_preferred_action_views_dirty = false;
        m_preferred_action_views.clear();
        const auto& repository = *this->m_task->get_repository();
        for (const auto action_index : m_preferred_actions)
            m_preferred_action_views.insert(ygg::make_view(action_index, repository));
    }

    return m_preferred_action_views;
}

bool FFRPGHeuristic<LiftedTag>::mark_atom(::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> binding)
{
    const auto g = ygg::uint_t(binding.get_index().relation);
    const auto i = ygg::uint_t(binding.get_index().row);

    assert(g < m_markings.size());
    if (ygg::test(i, m_markings[g]))
        return true;
    ygg::set(i, true, m_markings[g]);
    return false;
}

bool FFRPGHeuristic<LiftedTag>::mark_function(::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> binding)
{
    const auto g = ygg::uint_t(binding.get_index().relation);
    const auto i = ygg::uint_t(binding.get_index().row);

    assert(g < m_function_markings.size());
    if (ygg::test(i, m_function_markings[g]))
        return true;
    ygg::set(i, true, m_function_markings[g]);
    return false;
}

void FFRPGHeuristic<LiftedTag>::extract_relaxed_plan_and_preferred_actions(::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> binding,
                                                                           const StateContext<LiftedTag>& state_context,
                                                                           ::tyr::formalism::planning::GrounderContext& grounder_context)
{
    // Base case 1: atom is already marked => do not recurse again
    if (mark_atom(binding))
        return;

    // Base case 2: atom is initially true, i.e., has no witness => do not recurse again
    const auto* annotation = m_workspace.and_annot.find(binding);
    if (!annotation)
        return;

    const auto* witness = std::get_if<datalog::WitnessAnnotation<LiftedTag>>(annotation);
    if (!witness)
        return;

    extract_relaxed_plan_and_preferred_actions(*witness, state_context, grounder_context);
}

void FFRPGHeuristic<LiftedTag>::extract_relaxed_plan_and_preferred_actions(::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> binding,
                                                                           const StateContext<LiftedTag>& state_context,
                                                                           ::tyr::formalism::planning::GrounderContext& grounder_context)
{
    const auto* annotation = m_workspace.numeric_and_annot.find(binding);
    if (!annotation)
        return;

    extract_relaxed_plan_and_preferred_actions(binding, *annotation, state_context, grounder_context);
}

void FFRPGHeuristic<LiftedTag>::extract_relaxed_plan_and_preferred_actions(::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> binding,
                                                                           const datalog::Annotation<LiftedTag, f::FunctionTag>& annotation,
                                                                           const StateContext<LiftedTag>& state_context,
                                                                           ::tyr::formalism::planning::GrounderContext& grounder_context)
{
    // Base case 1: function binding is already marked => do not recurse again
    if (mark_function(binding))
        return;

    // Base case 2: function binding is initially assigned, i.e., has no witness => do not recurse again
    const auto* witness = std::get_if<datalog::WitnessAnnotation<LiftedTag, f::FunctionTag>>(&annotation);
    if (!witness)
        return;

    extract_relaxed_plan_and_preferred_actions(*witness, state_context, grounder_context);
}

void FFRPGHeuristic<LiftedTag>::extract_numeric_constraint_support(::tyr::formalism::datalog::GroundBooleanOperatorView constraint,
                                                                   const StateContext<LiftedTag>& state_context,
                                                                   ::tyr::formalism::planning::GrounderContext& grounder_context)
{
    m_workspace.numeric_support_selector->for_each_constraint_support(
        constraint,
        m_numeric_support_selector_workspace,
        datalog::SumAggregation {},
        [&](const auto binding, const auto, const auto& annotation)
        { extract_relaxed_plan_and_preferred_actions(binding, annotation, state_context, grounder_context); });
}

template<f::RelationKind R>
void FFRPGHeuristic<LiftedTag>::extract_relaxed_plan_and_preferred_actions(const datalog::WitnessAnnotation<LiftedTag, R>& witness,
                                                                           const StateContext<LiftedTag>& state_context,
                                                                           ::tyr::formalism::planning::GrounderContext& grounder_context)
{
    const auto& mapping = this->m_rpg_program.template get_rule_to_action_mapping<R>();

    const auto rule_row = witness.get_rule_key();
    const auto rule = rule_row.get_relation();
    const auto row = rule_row.get_objects();

    if (const auto it = mapping.find(rule); it != mapping.end())
    {
        const auto action = it->second;

        grounder_context.binding.clear();
        for (const auto object : row)
            grounder_context.binding.push_back(object.get_index());

        const auto ground_action = ::tyr::formalism::planning::ground(action,
                                                                      grounder_context,
                                                                      m_grounder_cache,
                                                                      m_task->get_formalism_task().get_variable_domains().action_domains.at(action.get_index()),
                                                                      m_iter_workspace,
                                                                      *m_task->get_fdr_context())
                                       .first;

        const auto ground_action_index = ground_action.get_index();

        m_relaxed_plan.insert(ground_action_index);

        if (is_applicable(ground_action, state_context, m_effect_families))
            m_preferred_actions.insert(ground_action_index);
    }

    // Divide case: recursively call for preconditions.

    auto datalog_grounder_context =
        ::tyr::formalism::datalog::GrounderContext { m_workspace.datalog_builder, m_workspace.workspace_repository, m_workspace.binding };
    const auto& const_rule_workspace = *m_rpg_program.get_const_program_workspace().template get_rules<R>()[ygg::uint_t(rule.get_index())];

    const auto witness_condition = const_rule_workspace.get_witness_rule().get_body();

    for (const auto literal : witness_condition.template get_literals<::tyr::formalism::FluentTag>())
    {
        // Cannot do this before the loop because of overwrites during recursion; we could binding from a builder and place it into the grounder context.
        datalog_grounder_context.binding.clear();
        for (const auto object : row)
            datalog_grounder_context.binding.push_back(object.get_index());

        const auto witness_atom = ::tyr::formalism::datalog::ground(literal.get_atom(), datalog_grounder_context).first;

        extract_relaxed_plan_and_preferred_actions(witness_atom.get_row(), state_context, grounder_context);
    }

    for (const auto constraint : witness_condition.get_numeric_constraints())
    {
        // Cannot do this before the loop because of overwrites during recursion; we could binding from a builder and place it into the grounder context.
        datalog_grounder_context.binding.clear();
        for (const auto object : row)
            datalog_grounder_context.binding.push_back(object.get_index());

        const auto ground_constraint_data = ::tyr::formalism::datalog::ground(constraint, datalog_grounder_context);
        const auto ground_constraint = ygg::make_view(ground_constraint_data, datalog_grounder_context.destination);
        extract_numeric_constraint_support(ground_constraint, state_context, grounder_context);
    }
}

}
