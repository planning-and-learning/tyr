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

#ifndef TYR_PLANNING_GROUND_HEURISTICS_RPG_HPP_
#define TYR_PLANNING_GROUND_HEURISTICS_RPG_HPP_

#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/ground/contexts/program.hpp"
#include "tyr/datalog/ground/policies/cost.hpp"
#include "tyr/datalog/ground/policies/numeric_support.hpp"
#include "tyr/datalog/ground/queue.hpp"
#include "tyr/datalog/ground/workspaces/program.hpp"
#include "tyr/datalog/policies/annotation_concept.hpp"
#include "tyr/datalog/policies/cost_concept.hpp"
#include "tyr/datalog/policies/termination_concept.hpp"
#include "tyr/formalism/datalog/builder.hpp"
#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"
#include "tyr/planning/applicability.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/programs/rpg.hpp"
#include "tyr/planning/ground/state_view.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/heuristic.hpp"
#include "tyr/planning/heuristics/rpg.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

template<typename Derived,
         datalog::OrAnnotationPolicyConcept<GroundTag> OrAP,
         datalog::AndAnnotationPolicyConcept<GroundTag> AndAP,
         datalog::TerminationPolicyConcept<GroundTag> TP,
         datalog::RuleCostPolicyConcept<GroundTag> CP>
class RPGBase<GroundTag, Derived, OrAP, AndAP, TP, CP> : public Heuristic<GroundTag>
{
private:
    constexpr const auto& self() const { return static_cast<const Derived&>(*this); }
    constexpr auto& self() { return static_cast<Derived&>(*this); }

public:
    RPGBase(TaskPtr<GroundTag> task,
            ygg::ExecutionContextPtr execution_context,
            const OrAP& or_ap,
            const AndAP& and_ap,
            CostMode cost_mode = CostMode::GENERAL);

    void set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal) override;

    ygg::float_t evaluate(const StateView<GroundTag>& state) override;

    const auto& get_workspace() const noexcept { return m_workspace; }
    const auto& get_rpg_program() const noexcept { return m_rpg_program; }

protected:
    void set_action_binding_cost(::tyr::formalism::planning::ActionBindingView action_binding, datalog::Cost cost);
    ygg::float_t evaluate_impl(const StateView<GroundTag>& state, bool initialize_costs);
    void initialize_rule_costs(const StateView<GroundTag>& state);

    datalog::Cost get_atom_cost(::tyr::formalism::datalog::GroundAtomView<::tyr::formalism::FluentTag> atom) const noexcept;

    datalog::Cost get_goal_cost() const noexcept;

private:
    TP make_termination_policy() const { return TP(); }

protected:
    TaskPtr<GroundTag> m_task;
    ygg::ExecutionContextPtr m_execution_context;
    RPGProgram<GroundTag> m_rpg_program;
    ygg::UnorderedMap<::tyr::formalism::planning::ActionBindingView, ::tyr::formalism::planning::GroundActionView> m_action_binding_to_ground_action;
    datalog::ProgramWorkspace<GroundTag, OrAP, AndAP, TP, CP> m_workspace;
    datalog::QueueWorkspace<GroundTag> m_queue_workspace;
    CostMode m_cost_mode;
};

template<typename Derived,
         datalog::OrAnnotationPolicyConcept<GroundTag> OrAP,
         datalog::AndAnnotationPolicyConcept<GroundTag> AndAP,
         datalog::TerminationPolicyConcept<GroundTag> TP,
         datalog::RuleCostPolicyConcept<GroundTag> CP>
RPGBase<GroundTag, Derived, OrAP, AndAP, TP, CP>::RPGBase(TaskPtr<GroundTag> task,
                                                          ygg::ExecutionContextPtr execution_context,
                                                          const OrAP& or_ap,
                                                          const AndAP& and_ap,
                                                          CostMode cost_mode) :
    m_task(std::move(task)),
    m_execution_context(std::move(execution_context)),
    m_rpg_program(m_task->get_task(), cost_mode),
    m_action_binding_to_ground_action(),
    m_workspace(m_rpg_program.get_datalog_program(), or_ap, and_ap, make_termination_policy()),
    m_queue_workspace(m_rpg_program.get_datalog_program().get_program()),
    m_cost_mode(cost_mode)
{
    for (const auto action : m_task->get_task().get_ground_actions())
        m_action_binding_to_ground_action.emplace(action.get_row(), action);
    m_workspace.tp.set_goals(m_rpg_program.get_goal());
}

template<typename Derived,
         datalog::OrAnnotationPolicyConcept<GroundTag> OrAP,
         datalog::AndAnnotationPolicyConcept<GroundTag> AndAP,
         datalog::TerminationPolicyConcept<GroundTag> TP,
         datalog::RuleCostPolicyConcept<GroundTag> CP>
void RPGBase<GroundTag, Derived, OrAP, AndAP, TP, CP>::set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal)
{
    namespace fd = ::tyr::formalism::datalog;
    auto builder = fd::Builder();
    auto& repository = m_rpg_program.get_datalog_program().get_program_repository();
    auto merge_context = ::tyr::formalism::planning::MergeDatalogContext(builder, repository);
    auto condition_ptr = builder.get_builder<fd::GroundConjunctiveCondition>();
    auto& condition = *condition_ptr;
    condition.clear();

    const auto& p2d = m_rpg_program.get_translation_context().p2d.fluent_to_fluent_atom;
    for (const auto fact : goal.template get_facts<::tyr::formalism::PositiveTag>())
    {
        if (const auto atom = fact.get_atom())
        {
            auto literal_ptr = builder.get_builder<fd::GroundLiteral<::tyr::formalism::FluentTag>>();
            auto& literal = *literal_ptr;
            literal.clear();
            literal.atom = p2d.at(*atom).get_index();
            literal.polarity = true;
            fd::canonicalize(literal);
            condition.fluent_literals.push_back(repository.get_or_create(literal).first.get_index());
        }
    }

    for (const auto numeric_constraint : goal.get_numeric_constraints())
        condition.numeric_constraints.push_back(::tyr::formalism::planning::merge_p2d(numeric_constraint, merge_context));

    fd::canonicalize(condition);
    m_workspace.tp.set_goals(repository.get_or_create(condition).first);
}

template<typename Derived,
         datalog::OrAnnotationPolicyConcept<GroundTag> OrAP,
         datalog::AndAnnotationPolicyConcept<GroundTag> AndAP,
         datalog::TerminationPolicyConcept<GroundTag> TP,
         datalog::RuleCostPolicyConcept<GroundTag> CP>
ygg::float_t RPGBase<GroundTag, Derived, OrAP, AndAP, TP, CP>::evaluate(const StateView<GroundTag>& state)
{
    return evaluate_impl(state, true);
}

template<typename Derived,
         datalog::OrAnnotationPolicyConcept<GroundTag> OrAP,
         datalog::AndAnnotationPolicyConcept<GroundTag> AndAP,
         datalog::TerminationPolicyConcept<GroundTag> TP,
         datalog::RuleCostPolicyConcept<GroundTag> CP>
ygg::float_t RPGBase<GroundTag, Derived, OrAP, AndAP, TP, CP>::evaluate_impl(const StateView<GroundTag>& state, bool initialize_costs)
{
    if (initialize_costs)
        initialize_rule_costs(state);
    m_workspace.facts.fluent_atoms.clear();
    m_workspace.facts.fluent_fterm_intervals.clear();
    const auto& p2d = m_rpg_program.get_translation_context().p2d;
    for (const auto fact : state.get_fluent_facts_view())
        if (const auto atom = fact.get_atom())
            m_workspace.facts.fluent_atoms.insert(p2d.fluent_to_fluent_atom.at(*atom));

    for (const auto& [fterm, value] : state.get_fluent_fterm_values_view())
        if (const auto it = p2d.fluent_to_fluent_fterm.find(fterm); it != p2d.fluent_to_fluent_fterm.end())
            m_workspace.facts.fluent_fterm_intervals.insert_or_assign(it->second, ygg::ClosedInterval<ygg::float_t>(value, value));

    auto ctx = datalog::ProgramExecutionContext(m_workspace, m_queue_workspace);
    ctx.initialize();

    m_execution_context->arena().execute([&] { datalog::solve_ground_queue(ctx); });

    return m_workspace.tp.check(datalog::FactSets { m_workspace.facts.static_fact_sets, m_workspace.facts.fluent_fact_sets }) ?
               self().extract_cost_and_set_preferred_actions_impl(state) :
               std::numeric_limits<ygg::float_t>::infinity();
}

template<typename Derived,
         datalog::OrAnnotationPolicyConcept<GroundTag> OrAP,
         datalog::AndAnnotationPolicyConcept<GroundTag> AndAP,
         datalog::TerminationPolicyConcept<GroundTag> TP,
         datalog::RuleCostPolicyConcept<GroundTag> CP>
void RPGBase<GroundTag, Derived, OrAP, AndAP, TP, CP>::initialize_rule_costs(const StateView<GroundTag>&)
{
    m_workspace.clear_costs();
}

template<typename Derived,
         datalog::OrAnnotationPolicyConcept<GroundTag> OrAP,
         datalog::AndAnnotationPolicyConcept<GroundTag> AndAP,
         datalog::TerminationPolicyConcept<GroundTag> TP,
         datalog::RuleCostPolicyConcept<GroundTag> CP>
void RPGBase<GroundTag, Derived, OrAP, AndAP, TP, CP>::set_action_binding_cost(::tyr::formalism::planning::ActionBindingView action_binding, datalog::Cost cost)
{
    const auto action_it = m_action_binding_to_ground_action.find(action_binding);
    if (action_it == m_action_binding_to_ground_action.end())
        return;

    const auto set_costs = [&]<::tyr::formalism::RelationKind R>()
    {
        for (const auto& [rule, mapped_action] : m_rpg_program.template get_rule_to_action_mapping<R>())
            if (mapped_action.get_index() == action_it->second.get_index())
                m_workspace.cost_policy.set_cost(rule, cost);
    };
    set_costs.template operator()<::tyr::formalism::PredicateTag>();
    set_costs.template operator()<::tyr::formalism::FunctionTag>();
}

template<typename Derived,
         datalog::OrAnnotationPolicyConcept<GroundTag> OrAP,
         datalog::AndAnnotationPolicyConcept<GroundTag> AndAP,
         datalog::TerminationPolicyConcept<GroundTag> TP,
         datalog::RuleCostPolicyConcept<GroundTag> CP>
datalog::Cost
RPGBase<GroundTag, Derived, OrAP, AndAP, TP, CP>::get_atom_cost(::tyr::formalism::datalog::GroundAtomView<::tyr::formalism::FluentTag> atom) const noexcept
{
    const auto* annotation = m_workspace.and_annot.find(atom.get_row());
    return annotation ? datalog::get_cost(*annotation) : datalog::Cost(0);
}

template<typename Derived,
         datalog::OrAnnotationPolicyConcept<GroundTag> OrAP,
         datalog::AndAnnotationPolicyConcept<GroundTag> AndAP,
         datalog::TerminationPolicyConcept<GroundTag> TP,
         datalog::RuleCostPolicyConcept<GroundTag> CP>
datalog::Cost RPGBase<GroundTag, Derived, OrAP, AndAP, TP, CP>::get_goal_cost() const noexcept
{
    const auto numeric_support_selector = datalog::GroundNumericSupportSelector(m_workspace.facts, m_workspace.numeric_and_annot);
    return m_workspace.tp.get_total_cost(datalog::FactSets { m_workspace.facts.static_fact_sets, m_workspace.facts.fluent_fact_sets },
                                         m_workspace.and_annot,
                                         m_workspace.numeric_and_annot,
                                         numeric_support_selector);
}

}

#endif
