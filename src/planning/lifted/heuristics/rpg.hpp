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

#ifndef TYR_SRC_PLANNING_LIFTED_HEURISTICS_RPG_HPP_
#define TYR_SRC_PLANNING_LIFTED_HEURISTICS_RPG_HPP_

#include "../../heuristics/rpg.hpp"
#include "tyr/datalog/formatter.hpp"
#include "tyr/datalog/lifted/contexts/program.hpp"
#include "tyr/datalog/lifted/policies/cost.hpp"
#include "tyr/datalog/lifted/workspaces/program.hpp"
#include "tyr/formalism/datalog/grounder.hpp"
#include "tyr/formalism/planning/grounder.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"
#include "tyr/planning/lifted/programs/rpg.hpp"
#include "tyr/planning/lifted/state_builder.hpp"
#include "tyr/planning/lifted/state_data.hpp"
#include "tyr/planning/lifted/state_view.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/task_utils.hpp"

#include <cassert>
#include <fmt/ostream.h>
#include <optional>
#include <vector>

namespace tyr::planning::detail
{

template<>
struct RPGPolicy<LiftedTag>
{
    template<typename Definition, typename Workspace>
    static void set_goal(Definition&, Workspace&, ::tyr::formalism::planning::GroundConjunctiveConditionView) noexcept
    {
        // Lifted ground bindings live in the evaluation repository, which reset_evaluation()
        // clears. The goal is therefore materialized in begin_state_evaluation() after the reset.
    }

    template<typename Definition, typename Workspace>
    static void
    begin_state_evaluation(const Definition& definition, Workspace& workspace, ::tyr::formalism::planning::GroundConjunctiveConditionView source_goal)
    {
        workspace.reset_evaluation();
        materialize_goal(definition, workspace, source_goal);
    }

    template<typename Definition, typename Workspace>
    static void insert_state_facts(const Definition& definition, Workspace& workspace, const ygg::Builder<State<LiftedTag>>& state)
    {
        auto merge_context = ::tyr::formalism::planning::MergeDatalogContext { workspace.datalog_builder, workspace.workspace_repository };
        insert_fluent_atoms_to_fact_set(state,
                                        *definition.task->get_repository(),
                                        definition.rpg_program.get_translation_context().p2d.fluent_to_fluent_predicate,
                                        merge_context,
                                        workspace.facts.fact_sets);
        insert_numeric_variables_to_fact_set(state, *definition.task->get_repository(), merge_context, workspace.facts.fact_sets);
    }

    template<typename Workspace>
    static void prepare_rule_binding(Workspace& workspace, ::tyr::formalism::planning::ActionBindingView action_binding)
    {
        workspace.binding.clear();
        for (const auto object : action_binding.get_data())
            workspace.binding.push_back(object);
    }

    template<::tyr::formalism::RelationKind R, typename Workspace>
    static std::optional<datalog::WitnessRuleKeyT<LiftedTag, R>> make_rule_cost_key(Workspace& workspace,
                                                                                    ::tyr::formalism::datalog::RuleView<R> rule,
                                                                                    ::tyr::formalism::planning::ActionView mapped_action,
                                                                                    ::tyr::formalism::planning::ActionBindingView action_binding)
    {
        if (mapped_action.get_index() != action_binding.get_relation().get_index())
            return std::nullopt;

        auto grounder_context = ::tyr::formalism::datalog::GrounderContext { workspace.datalog_builder, workspace.workspace_repository, workspace.binding };
        return ::tyr::formalism::datalog::ground_binding(rule, grounder_context).first;
    }

    template<::tyr::formalism::RelationKind R, typename Definition, typename Workspace>
    static std::optional<::tyr::formalism::planning::ActionBindingView>
    get_action_binding(const Definition& definition, Workspace& workspace, const datalog::WitnessAnnotation<LiftedTag, R>& witness)
    {
        const auto rule_binding = witness.get_rule_key();
        const auto& mapping = definition.rpg_program.template get_rule_to_action_mapping<R>();
        const auto it = mapping.find(rule_binding.get_relation());
        if (it == mapping.end())
            return std::nullopt;

        auto grounder_context =
            ::tyr::formalism::planning::GrounderContext { workspace.planning_builder, *definition.task->get_repository(), workspace.binding };
        workspace.binding.clear();
        for (const auto object : rule_binding.get_objects())
            workspace.binding.push_back(object.get_index());
        return ::tyr::formalism::planning::ground(it->second, grounder_context).first;
    }

    template<::tyr::formalism::RelationKind R, typename Definition, typename Workspace, typename Callback>
    static void for_each_witness_precondition(const Definition& definition,
                                              Workspace& workspace,
                                              const datalog::WitnessAnnotation<LiftedTag, R>& witness,
                                              Callback&& callback)
    {
        const auto rule_binding = witness.get_rule_key();
        const auto row = rule_binding.get_objects();
        const auto& const_rule_workspace =
            *definition.rpg_program.get_const_program_workspace().template get_rules<R>()[ygg::uint_t(rule_binding.get_relation().get_index())];
        const auto witness_condition = const_rule_workspace.get_witness_rule().get_body();
        auto grounder_context = ::tyr::formalism::datalog::GrounderContext { workspace.datalog_builder, workspace.workspace_repository, workspace.binding };

        for (const auto literal : witness_condition.template get_literals<::tyr::formalism::FluentTag>())
        {
            if (!literal.get_polarity())
                continue;

            workspace.binding.clear();
            for (const auto object : row)
                workspace.binding.push_back(object.get_index());
            callback(::tyr::formalism::datalog::ground(literal.get_atom(), grounder_context).first.get_row());
        }
    }

    template<typename Workspace>
    static void print_summary(const Workspace& workspace, size_t verbosity)
    {
        if (verbosity < 1)
            return;

        std::cout << "[RPGHeuristic] Summary" << std::endl;
        fmt::print(std::cout, "{}\n", workspace.statistics);
        auto rule_statistics = std::vector<datalog::RuleStatistics> {};
        const auto collect_rule_statistics = [&]<::tyr::formalism::RelationKind R>()
        {
            for (const auto& ws_rule : workspace.template get_rules<R>())
                rule_statistics.push_back(ws_rule->common.statistics);
        };
        collect_rule_statistics.template operator()<::tyr::formalism::PredicateTag>();
        collect_rule_statistics.template operator()<::tyr::formalism::FunctionTag>();
        fmt::print(std::cout, "{}\n", datalog::compute_aggregated_rule_statistics(rule_statistics));
        auto rule_worker_statistics = std::vector<datalog::RuleWorkerStatistics> {};
        const auto collect_worker_statistics = [&]<::tyr::formalism::RelationKind R>()
        {
            for (const auto& ws_rule : workspace.template get_rules<R>())
                for (const auto& worker : ws_rule->worker)
                    rule_worker_statistics.push_back(worker.solve.statistics);
        };
        collect_worker_statistics.template operator()<::tyr::formalism::PredicateTag>();
        collect_worker_statistics.template operator()<::tyr::formalism::FunctionTag>();
        fmt::print(std::cout, "{}\n", datalog::compute_aggregated_rule_worker_statistics(rule_worker_statistics));
    }

private:
    template<typename Definition, typename Workspace>
    static void materialize_goal(const Definition& definition, Workspace& workspace, ::tyr::formalism::planning::GroundConjunctiveConditionView source_goal)
    {
        namespace fd = ::tyr::formalism::datalog;
        auto merge_context = ::tyr::formalism::planning::MergeDatalogContext { workspace.datalog_builder, workspace.workspace_repository };
        auto condition_ptr = workspace.datalog_builder.template get_builder<fd::GroundConjunctiveCondition>();
        auto& condition = *condition_ptr;
        condition.clear();

        const auto& p2d = definition.rpg_program.get_translation_context().p2d;
        for (const auto fact : source_goal.template get_facts<::tyr::formalism::PositiveTag>())
            if (const auto literal = ::tyr::formalism::planning::merge_p2d(fact, true, p2d.fluent_to_fluent_predicate, merge_context))
                condition.fluent_literals.push_back(literal->get_index());

        for (const auto numeric_constraint : source_goal.get_numeric_constraints())
            condition.numeric_constraints.push_back(::tyr::formalism::planning::merge_p2d(numeric_constraint, merge_context));

        fd::canonicalize(condition);
        workspace.tp.set_goals(workspace.workspace_repository.get_or_create(condition).first);
    }
};

}

#endif
