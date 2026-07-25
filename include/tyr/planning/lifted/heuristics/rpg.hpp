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

#ifndef TYR_PLANNING_LIFTED_HEURISTICS_RPG_HPP_
#define TYR_PLANNING_LIFTED_HEURISTICS_RPG_HPP_

#include "tyr/datalog/formatter.hpp"
#include "tyr/datalog/lifted/bottom_up.hpp"
#include "tyr/datalog/lifted/contexts/program.hpp"
#include "tyr/datalog/lifted/policies/cost.hpp"
#include "tyr/datalog/lifted/workspaces/program.hpp"
#include "tyr/datalog/policies/annotation_concept.hpp"
#include "tyr/datalog/policies/cost_concept.hpp"
#include "tyr/datalog/policies/termination_concept.hpp"
#include "tyr/formalism/datalog/grounder.hpp"
#include "tyr/formalism/planning/grounder.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/heuristic.hpp"
#include "tyr/planning/heuristics/rpg.hpp"
#include "tyr/planning/lifted/programs/rpg.hpp"
#include "tyr/planning/lifted/state_builder.hpp"
#include "tyr/planning/lifted/state_data.hpp"
#include "tyr/planning/lifted/state_view.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/task_utils.hpp"

#include <cassert>
#include <concepts>
#include <fmt/ostream.h>
#include <optional>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::planning
{

template<typename Derived,
         datalog::OrAnnotationPolicyConcept<LiftedTag> OrAP,
         datalog::AndAnnotationPolicyConcept<LiftedTag> AndAP,
         datalog::TerminationPolicyConcept<LiftedTag> TP,
         datalog::RuleCostPolicyConcept<LiftedTag> CP>
class RPGBase<LiftedTag, Derived, OrAP, AndAP, TP, CP> : public Heuristic<LiftedTag>
{
private:
    /// @brief Helper to cast to Derived.
    constexpr const auto& self() const { return static_cast<const Derived&>(*this); }
    constexpr auto& self() { return static_cast<Derived&>(*this); }

public:
    explicit RPGBase(TaskPtr<LiftedTag> task,
                     ygg::ExecutionContextPtr execution_context,
                     const OrAP& or_ap,
                     const AndAP& and_ap,
                     CostMode cost_mode = CostMode::GENERAL) :
        m_task(std::move(task)),
        m_execution_context(std::move(execution_context)),
        m_rpg_program(m_task->get_task(), cost_mode),
        m_workspace(m_rpg_program.get_datalog_program(), or_ap, and_ap, TP()),
        m_cost_mode(cost_mode)
    {
        m_workspace.tp.set_goals(m_rpg_program.get_goal());
    }

    void set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal) override
    {
        namespace fd = ::tyr::formalism::datalog;
        auto merge_context = ::tyr::formalism::planning::MergeDatalogContext { m_workspace.datalog_builder, m_workspace.workspace_repository };
        auto condition_ptr = m_workspace.datalog_builder.template get_builder<fd::GroundConjunctiveCondition>();
        auto& condition = *condition_ptr;
        condition.clear();

        const auto& p2d = m_rpg_program.get_translation_context().p2d;
        for (const auto fact : goal.template get_facts<::tyr::formalism::PositiveTag>())
            if (const auto literal = ::tyr::formalism::planning::merge_p2d(fact, true, p2d.fluent_to_fluent_predicate, merge_context))
                condition.fluent_literals.push_back(literal->get_index());

        for (const auto numeric_constraint : goal.get_numeric_constraints())
            condition.numeric_constraints.push_back(::tyr::formalism::planning::merge_p2d(numeric_constraint, merge_context));

        fd::canonicalize(condition);
        m_workspace.tp.set_goals(m_workspace.workspace_repository.get_or_create(condition).first);
    }

    ygg::float_t evaluate(const StateView<LiftedTag>& state) override
    {
        m_workspace.facts.reset();

        auto merge_context = ::tyr::formalism::planning::MergeDatalogContext { m_workspace.datalog_builder, m_workspace.workspace_repository };

        insert_fluent_atoms_to_fact_set(state.get_unpacked_state(),
                                        *m_task->get_repository(),
                                        m_rpg_program.get_translation_context().p2d.fluent_to_fluent_predicate,
                                        merge_context,
                                        m_workspace.facts.fact_sets);
        insert_numeric_variables_to_fact_set(state.get_unpacked_state(), *m_task->get_repository(), merge_context, m_workspace.facts.fact_sets);

        auto ctx = datalog::ProgramExecutionContext(m_workspace);

        m_execution_context->arena().execute([&] { datalog::solve_bottom_up(ctx); });

        return (m_workspace.tp.check(datalog::FactSets { m_rpg_program.get_const_program_workspace().facts.fact_sets, m_workspace.facts.fact_sets })) ?
                   self().extract_cost_and_set_preferred_actions_impl(state) :
                   std::numeric_limits<ygg::float_t>::infinity();
    }

    const auto& get_workspace() const noexcept { return m_workspace; }
    const auto& get_rpg_program() const noexcept { return m_rpg_program; }

protected:
    void set_action_binding_cost(::tyr::formalism::planning::ActionBindingView action_binding, datalog::Cost cost)
    {
        const auto action = action_binding.get_relation();
        const auto objects = action_binding.get_data();
        m_workspace.binding.clear();
        for (const auto object : objects)
            m_workspace.binding.push_back(object);
        auto grounder_context =
            ::tyr::formalism::datalog::GrounderContext { m_workspace.datalog_builder, m_workspace.workspace_repository, m_workspace.binding };
        const auto binding = std::span<const ygg::Index<::tyr::formalism::Object>>(m_workspace.binding.data(), m_workspace.binding.size());
        const auto set_costs = [&]<::tyr::formalism::RelationKind R>()
        {
            for (const auto& [rule, mapped_action] : m_rpg_program.template get_rule_to_action_mapping<R>())
            {
                if (mapped_action.get_index() != action.get_index())
                    continue;

                datalog::set_rule_cost(m_workspace.cost_policy, rule, binding, cost, grounder_context);
            }
        };
        set_costs.template operator()<::tyr::formalism::PredicateTag>();
        set_costs.template operator()<::tyr::formalism::FunctionTag>();
    }

    datalog::Cost get_binding_cost(::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> binding) const noexcept
    {
        const auto* annotation = m_workspace.and_annot.find(binding);
        return annotation ? datalog::get_cost(*annotation) : datalog::Cost(0);
    }

    datalog::Cost get_goal_cost() const noexcept
    {
        return m_workspace.tp.get_total_cost(datalog::FactSets { m_rpg_program.get_const_program_workspace().facts.fact_sets, m_workspace.facts.fact_sets },
                                             m_workspace.and_annot,
                                             m_workspace.numeric_and_annot,
                                             *m_workspace.numeric_support_selector);
    }

    template<::tyr::formalism::RelationKind R>
    std::optional<::tyr::formalism::planning::ActionBindingView> get_action_binding(const datalog::WitnessAnnotation<LiftedTag, R>& witness)
    {
        const auto rule_binding = witness.get_rule_key();
        const auto rule = rule_binding.get_relation();
        const auto& mapping = m_rpg_program.template get_rule_to_action_mapping<R>();
        const auto it = mapping.find(rule);
        if (it == mapping.end())
            return std::nullopt;

        auto grounder_context = ::tyr::formalism::planning::GrounderContext { m_workspace.planning_builder, *m_task->get_repository(), m_workspace.binding };
        m_workspace.binding.clear();
        for (const auto object : rule_binding.get_objects())
            m_workspace.binding.push_back(object.get_index());
        return ::tyr::formalism::planning::ground(it->second, grounder_context).first;
    }

    template<::tyr::formalism::RelationKind R, typename Callback>
    void for_each_witness_precondition(const datalog::WitnessAnnotation<LiftedTag, R>& witness, Callback&& callback)
    {
        const auto rule_binding = witness.get_rule_key();
        const auto row = rule_binding.get_objects();
        const auto& const_rule_workspace =
            *m_rpg_program.get_const_program_workspace().template get_rules<R>()[ygg::uint_t(rule_binding.get_relation().get_index())];
        const auto witness_condition = const_rule_workspace.get_witness_rule().get_body();
        auto grounder_context =
            ::tyr::formalism::datalog::GrounderContext { m_workspace.datalog_builder, m_workspace.workspace_repository, m_workspace.binding };

        for (const auto literal : witness_condition.template get_literals<::tyr::formalism::FluentTag>())
        {
            if (!literal.get_polarity())
                continue;

            m_workspace.binding.clear();
            for (const auto object : row)
                m_workspace.binding.push_back(object.get_index());
            const auto atom = ::tyr::formalism::datalog::ground(literal.get_atom(), grounder_context).first;
            callback(atom.get_row());
        }
    }

    template<typename Callback>
    void for_each_achiever(::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> binding, Callback&& callback)
        requires AndAP::records_propositional_achievers
    {
        for (const auto& rule : m_workspace.template get_rules<::tyr::formalism::PredicateTag>())
        {
            for (const auto& worker : rule->worker)
            {
                const auto* achievers = worker.solve.and_ap.find_achievers(binding);
                if (!achievers)
                    continue;

                for (const auto& achiever : *achievers)
                    callback(achiever);
            }
        }
    }

public:
    void print_summary(size_t verbosity) const override
    {
        if (verbosity < 1)
            return;

        std::cout << "[RPGHeuristic] Summary" << std::endl;
        fmt::print(std::cout, "{}\n", m_workspace.statistics);
        auto rule_statistics = std::vector<datalog::RuleStatistics> {};
        const auto collect_rule_statistics = [&]<::tyr::formalism::RelationKind R>()
        {
            for (const auto& ws_rule : m_workspace.template get_rules<R>())
                rule_statistics.push_back(ws_rule->common.statistics);
        };
        collect_rule_statistics.template operator()<::tyr::formalism::PredicateTag>();
        collect_rule_statistics.template operator()<::tyr::formalism::FunctionTag>();
        fmt::print(std::cout, "{}\n", datalog::compute_aggregated_rule_statistics(rule_statistics));
        auto rule_worker_statistics = std::vector<datalog::RuleWorkerStatistics> {};
        const auto collect_worker_statistics = [&]<::tyr::formalism::RelationKind R>()
        {
            for (const auto& ws_rule : m_workspace.template get_rules<R>())
                for (const auto& worker : ws_rule->worker)
                    rule_worker_statistics.push_back(worker.solve.statistics);
        };
        collect_worker_statistics.template operator()<::tyr::formalism::PredicateTag>();
        collect_worker_statistics.template operator()<::tyr::formalism::FunctionTag>();
        fmt::print(std::cout, "{}\n", datalog::compute_aggregated_rule_worker_statistics(rule_worker_statistics));
    }

protected:
    TaskPtr<LiftedTag> m_task;
    ygg::ExecutionContextPtr m_execution_context;
    RPGProgram<LiftedTag> m_rpg_program;

    datalog::ProgramWorkspace<LiftedTag, OrAP, AndAP, TP, CP> m_workspace;
    CostMode m_cost_mode;
};

}

#endif
