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

#ifndef TYR_SRC_PLANNING_GROUND_HEURISTICS_RPG_HPP_
#define TYR_SRC_PLANNING_GROUND_HEURISTICS_RPG_HPP_

#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/ground/contexts/program.hpp"
#include "tyr/datalog/ground/policies/cost.hpp"
#include "tyr/datalog/ground/policies/numeric_support.hpp"
#include "tyr/datalog/ground/solver.hpp"
#include "tyr/datalog/ground/workspaces/program.hpp"
#include "tyr/datalog/policies/annotation_concept.hpp"
#include "tyr/datalog/policies/cost_concept.hpp"
#include "tyr/datalog/policies/termination_concept.hpp"
#include "tyr/formalism/datalog/builder.hpp"
#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"
#include "tyr/planning/ground/programs/rpg.hpp"
#include "tyr/planning/ground/state_builder.hpp"
#include "tyr/planning/ground/state_iterators.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/state_iterators.hpp"

#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::planning::detail
{

inline bool needs_expanded_lmcut(const ::tyr::formalism::datalog::ProgramView<GroundTag> program)
{
    for (const auto rule : program.template get_ground_rules<::tyr::formalism::PredicateTag>())
        if (!rule.get_body().get_numeric_constraints().empty())
            return true;
    return !program.template get_ground_rules<::tyr::formalism::FunctionTag>().empty();
}

struct GroundRPGDefinition
{
    GroundRPGDefinition(TaskPtr<GroundTag> task, CostMode cost_mode, bool compute_expanded_lmcut) :
        task(std::move(task)),
        rpg_program(this->task->get_task(), cost_mode),
        action_binding_to_ground_action(),
        goal(rpg_program.get_goal()),
        cost_mode(cost_mode),
        use_expanded_lmcut(compute_expanded_lmcut && needs_expanded_lmcut(rpg_program.get_datalog_program().get_program()))
    {
        for (const auto action : this->task->get_task().get_ground_actions())
            action_binding_to_ground_action.emplace(action.get_row(), action);
    }

    TaskPtr<GroundTag> task;
    RPGProgram<GroundTag> rpg_program;
    ygg::UnorderedMap<::tyr::formalism::planning::ActionBindingView, ::tyr::formalism::planning::GroundActionView> action_binding_to_ground_action;
    ::tyr::formalism::datalog::GroundConjunctiveConditionView goal;
    CostMode cost_mode;
    bool use_expanded_lmcut;
};

template<typename Derived,
         datalog::OrAnnotationPolicyConcept<GroundTag> OrAP,
         datalog::AndAnnotationPolicyConcept<GroundTag> AndAP,
         datalog::TerminationPolicyConcept<GroundTag> TP,
         datalog::RuleCostPolicyConcept<GroundTag> CP>
class GroundRPGEvaluator
{
public:
    using Definition = GroundRPGDefinition;
    using Workspace = datalog::ProgramWorkspace<GroundTag, OrAP, AndAP, TP, CP>;

    GroundRPGEvaluator(TaskPtr<GroundTag> task,
                       ygg::ExecutionContextPtr execution_context,
                       const OrAP& or_ap,
                       const AndAP& and_ap,
                       CostMode cost_mode = CostMode::GENERAL,
                       bool compute_expanded_lmcut = false) :
        GroundRPGEvaluator(std::make_shared<Definition>(std::move(task), cost_mode, compute_expanded_lmcut), std::move(execution_context), or_ap, and_ap)
    {
    }

    GroundRPGEvaluator(std::shared_ptr<Definition> definition, ygg::ExecutionContextPtr execution_context, const OrAP& or_ap, const AndAP& and_ap) :
        m_definition(std::move(definition)),
        m_execution_context(std::move(execution_context)),
        m_workspace(m_definition->rpg_program.get_datalog_program(), or_ap, and_ap, TP()),
        m_queue_workspace(m_definition->rpg_program.get_datalog_program().get_program())
    {
        m_workspace.tp.set_goals(m_definition->goal);
    }

    void set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal)
    {
        if (m_definition.use_count() != 1)
            throw std::logic_error("Cannot change the goal of a shared RPG definition");

        namespace fd = ::tyr::formalism::datalog;
        auto builder = fd::Builder();
        auto& repository = m_definition->rpg_program.get_datalog_program().get_program_repository();
        auto merge_context = ::tyr::formalism::planning::MergeDatalogContext(builder, repository);
        auto condition_ptr = builder.get_builder<fd::GroundConjunctiveCondition>();
        auto& condition = *condition_ptr;
        condition.clear();

        const auto& p2d = m_definition->rpg_program.get_translation_context().p2d.fluent_to_fluent_atom;
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
        m_definition->goal = repository.get_or_create(condition).first;
        m_workspace.tp.set_goals(m_definition->goal);
    }

    ygg::float_t evaluate(const ygg::Builder<State<GroundTag>>& state) { return evaluate_impl(state, true); }

protected:
    constexpr const auto& self() const { return static_cast<const Derived&>(*this); }
    constexpr auto& self() { return static_cast<Derived&>(*this); }

    ygg::float_t evaluate_impl(const ygg::Builder<State<GroundTag>>& state, bool initialize_costs)
    {
        if (initialize_costs)
            initialize_rule_costs();
        m_workspace.facts.reset();
        const auto& p2d = m_definition->rpg_program.get_translation_context().p2d;
        const auto& repository = *m_definition->task->get_repository();
        const auto fluent_facts = FDRFactRange<GroundTag, ::tyr::formalism::FluentTag>(state.template get_atoms<::tyr::formalism::FluentTag>().values);
        for (const auto fact : fluent_facts)
        {
            const auto fact_view = ygg::make_view(fact, repository);
            if (const auto atom = fact_view.get_atom())
                m_workspace.facts.fact_sets.predicate.insert(p2d.fluent_to_fluent_atom.at(*atom));
        }

        const auto fluent_fterm_values = FunctionTermValueRange<::tyr::formalism::FluentTag>(state.get_numeric_variables().values);
        for (const auto& [fterm_index, value] : fluent_fterm_values)
        {
            const auto fterm = ygg::make_view(fterm_index, repository);
            if (const auto it = p2d.fluent_to_fluent_fterm.find(fterm); it != p2d.fluent_to_fluent_fterm.end())
                m_workspace.facts.fact_sets.function.insert(it->second, value);
        }

        auto ctx = datalog::ProgramExecutionContext(m_workspace, m_queue_workspace);
        ctx.initialize();

        m_execution_context->arena().execute([&] { datalog::compute_model(ctx); });

        return m_workspace.tp.check(datalog::FactSets { m_workspace.const_workspace.facts.fact_sets, m_workspace.facts.fact_sets }) ?
                   self().compute_result(state) :
                   std::numeric_limits<ygg::float_t>::infinity();
    }

    ygg::float_t compute_result(const ygg::Builder<State<GroundTag>>&) const noexcept { return get_goal_cost(); }

    void initialize_rule_costs() { m_workspace.clear_costs(); }

    void set_action_binding_cost(::tyr::formalism::planning::ActionBindingView action_binding, datalog::Cost cost)
    {
        const auto action_it = m_definition->action_binding_to_ground_action.find(action_binding);
        if (action_it == m_definition->action_binding_to_ground_action.end())
            return;

        const auto set_costs = [&]<::tyr::formalism::RelationKind R>()
        {
            for (const auto& [rule, mapped_action] : m_definition->rpg_program.template get_rule_to_action_mapping<R>())
                if (mapped_action.get_index() == action_it->second.get_index())
                    m_workspace.cost_policy.set_cost(rule, cost);
        };
        set_costs.template operator()<::tyr::formalism::PredicateTag>();
        set_costs.template operator()<::tyr::formalism::FunctionTag>();
    }

    datalog::Cost get_atom_cost(::tyr::formalism::datalog::GroundAtomView<::tyr::formalism::FluentTag> atom) const noexcept
    {
        const auto* annotation = m_workspace.and_annot.find(atom.get_row());
        return annotation ? datalog::get_cost(*annotation) : datalog::Cost(0);
    }

    datalog::Cost get_goal_cost() const noexcept
    {
        const auto numeric_support_selector =
            datalog::GroundNumericSupportSelector(m_workspace.const_workspace.facts, m_workspace.facts, m_workspace.numeric_and_annot);
        return m_workspace.tp.get_total_cost(datalog::FactSets { m_workspace.const_workspace.facts.fact_sets, m_workspace.facts.fact_sets },
                                             m_workspace.and_annot,
                                             m_workspace.numeric_and_annot,
                                             numeric_support_selector);
    }

    const Task<GroundTag>& get_task() const noexcept { return *m_definition->task; }
    const RPGProgram<GroundTag>& get_program() const noexcept { return m_definition->rpg_program; }

    std::shared_ptr<Definition> m_definition;
    ygg::ExecutionContextPtr m_execution_context;
    Workspace m_workspace;
    datalog::QueueWorkspace<GroundTag> m_queue_workspace;
};

}

#endif
