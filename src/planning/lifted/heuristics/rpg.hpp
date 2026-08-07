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
#include "tyr/formalism/planning/merge_planning.hpp"
#include "tyr/planning/lifted/programs/rpg.hpp"
#include "tyr/planning/lifted/state_builder.hpp"
#include "tyr/planning/lifted/state_data.hpp"
#include "tyr/planning/lifted/state_view.hpp"
#include "tyr/planning/lifted/task.hpp"

#include <cassert>
#include <fmt/ostream.h>
#include <optional>
#include <vector>

namespace tyr::planning::detail
{

template<>
struct RPGPolicy<LiftedTag>
{
    using Action = ::tyr::formalism::planning::ActionBindingView;

    template<typename Workspace>
    static std::optional<::tyr::formalism::planning::GroundAtomView<::tyr::formalism::FluentTag>>
    translate_cut_atom(const RPGDefinition<LiftedTag>& definition, Workspace& workspace, datalog::PredicateAnnotationHead<LiftedTag> head)
    {
        const auto& mapping = definition.rpg_program.get_translation_context().d2p.fluent_to_fluent_predicate;
        if (!mapping.contains(head.get_relation()))
            return std::nullopt;

        auto merge_context = ::tyr::formalism::planning::MergePlanningContext { workspace.planning_builder, *definition.task->get_repository() };
        return ::tyr::formalism::planning::merge_atom_d2p<::tyr::formalism::FluentTag, ::tyr::formalism::FluentTag>(head, mapping, merge_context).first;
    }

    template<typename Workspace>
    static void set_goal(RPGDefinition<LiftedTag>&, Workspace&, ::tyr::formalism::planning::GroundConjunctiveConditionView) noexcept
    {
        // Lifted ground bindings live in the evaluation repository, which reset_evaluation()
        // clears. The goal is therefore materialized in begin_state_evaluation() after the reset.
    }

    template<typename Workspace>
    static void
    begin_state_evaluation(RPGDefinition<LiftedTag>& definition, Workspace& workspace, ::tyr::formalism::planning::GroundConjunctiveConditionView source_goal)
    {
        workspace.reset_evaluation();
        // Lifted ground bindings live in the evaluation repository and must be restored after it is cleared.
        materialize_goal(definition, workspace, source_goal);
    }

    template<::tyr::formalism::RelationKind R, typename Workspace>
    static std::optional<Action>
    get_action(const RPGDefinition<LiftedTag>& definition, Workspace& workspace, const datalog::WitnessAnnotation<LiftedTag, R>& witness)
    {
        const auto rule_binding = witness.get_rule_key();
        const auto& mapping = definition.rpg_program.template get_rule_to_action_mapping<R>();
        const auto it = mapping.find(rule_binding.get_relation());
        if (it == mapping.end())
            return std::nullopt;

        auto grounder_context =
            ::tyr::formalism::planning::GrounderContext { workspace.planning_builder, *definition.task->get_repository(), workspace.binding };
        workspace.binding.clear();
        ygg::extend(rule_binding.get_objects(), workspace.binding);
        return ::tyr::formalism::planning::ground(it->second, grounder_context).first;
    }

    template<typename Workspace, typename Executor>
    static bool is_action_applicable(const RPGDefinition<LiftedTag>& definition,
                                     Workspace& workspace,
                                     Executor& executor,
                                     Action action,
                                     const StateContext<LiftedTag>& state_context)
    {
        workspace.binding.clear();
        ygg::extend(action.get_objects(), workspace.binding);

        auto grounder_context =
            ::tyr::formalism::planning::GrounderContext { workspace.planning_builder, *definition.task->get_repository(), workspace.binding };
        return executor.is_applicable(action.get_relation(), state_context, grounder_context, *definition.task->get_fdr_context());
    }

    template<::tyr::formalism::RelationKind R, typename Workspace, typename PredicateCallback, typename NumericCallback>
    static void for_each_witness_precondition(Workspace& workspace,
                                              const datalog::WitnessAnnotation<LiftedTag, R>& witness,
                                              PredicateCallback&& predicate_callback,
                                              NumericCallback&& numeric_callback)
    {
        const auto rule_binding = witness.get_rule_key();
        const auto row = rule_binding.get_objects();
        const auto& const_rule_workspace = *workspace.const_workspace.template get_rules<R>()[ygg::uint_t(rule_binding.get_relation().get_index())];
        const auto witness_condition = const_rule_workspace.get_witness_rule().get_body();
        auto grounder_context = ::tyr::formalism::datalog::GrounderContext { workspace.datalog_builder, workspace.workspace_repository, workspace.binding };

        for (const auto literal : witness_condition.template get_literals<::tyr::formalism::FluentTag>())
        {
            if (!literal.get_polarity())
                continue;

            workspace.binding.clear();
            ygg::extend(row, workspace.binding);
            predicate_callback(::tyr::formalism::datalog::ground(literal.get_atom(), grounder_context).first.get_row());
        }

        for (const auto constraint : witness_condition.get_numeric_constraints())
        {
            workspace.binding.clear();
            ygg::extend(row, workspace.binding);
            numeric_callback(::tyr::formalism::datalog::ground(constraint, grounder_context));
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
};

}

#endif
