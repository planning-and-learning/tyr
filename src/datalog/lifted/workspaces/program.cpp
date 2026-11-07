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

#include "tyr/datalog/lifted/workspaces/program.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

#include "tyr/datalog/formatter.hpp"
#include "tyr/datalog/lifted/program.hpp"
#include "tyr/datalog/policies/aggregation.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/policies/termination.hpp"

#include <fmt/ostream.h>
#include <iostream>

namespace tyr::datalog
{
namespace
{
template<formalism::RelationKind R>
void initialize_rule_workspaces(const ConstProgramWorkspace<LiftedTag>& const_workspace, RuleWorkspaces<R>& workspaces)
{
    const auto& const_workspaces = const_workspace.rules.template get<R>();
    workspaces.reserve(const_workspaces.size());
    for (const auto& workspace : const_workspaces)
        workspaces.emplace_back(workspace ? std::make_unique<RuleWorkspace<LiftedTag, R>>(*workspace) : nullptr);
}

template<formalism::RelationKind R>
void initialize_const_rule_workspaces(Program<LiftedTag>& program,
                                      formalism::datalog::Repository& program_repository,
                                      analysis::RuleCompatibilityGraphMap<R>& graphs,
                                      ConstRuleWorkspaces<R>& workspaces)
{
    const auto rules = program.get_program().template get_rules<R>();
    workspaces.resize(rules.size());
    for (const auto rule : rules)
        workspaces[ygg::uint_t(rule.index().value)].emplace(rule, program_repository, std::move(graphs.at(rule.index())));
}
}

void print_rule_statistics(const urd::utils::Family<RuleWorkspaces, formalism::PredicateTag, formalism::FunctionTag>& rules)
{
    auto rule_statistics = urd::Store<urd::sequence<RuleStatistics>> {};
    auto rule_worker_statistics = urd::Store<urd::sequence<RuleWorkerStatistics>> {};
    rules.for_each(
        [&](const auto& workspaces)
        {
            for (const auto& ws_rule : workspaces)
            {
                if (!ws_rule)
                    continue;
                rule_statistics.push_back(ws_rule->common.statistics);
                for (const auto& worker : ws_rule->worker)
                    rule_worker_statistics.push_back(worker.solve.statistics);
            }
        });
    fmt::print(std::cout, "{}\n", compute_aggregated_rule_statistics(rule_statistics));
    fmt::print(std::cout, "{}\n", compute_aggregated_rule_worker_statistics(rule_worker_statistics));
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
ProgramWorkspace<LiftedTag, AP, TP, CP>::ProgramWorkspace(const Program<LiftedTag>& program, AP annotation_policy_, TP tp_, CP cost_policy_) :
    const_workspace(&program.get_const_program_workspace()),
    program_repository(&program.get_program_repository()),
    workspace_repository(program.get_repository_factory().create_shared(program.get_program().get_objects().size(), program_repository)),
    facts(program.get_program().get_predicates<formalism::FluentTag>(),
          program.get_program().get_functions<formalism::FluentTag>(),
          program.get_domains().fluent_predicate_domains,
          program.get_domains().fluent_function_domains,
          program.get_program().get_objects().size(),
          program.get_program().get_atoms<formalism::FluentTag>(),
          program.get_program().get_fterm_values<formalism::FluentTag>(),
          *workspace_repository),
    annotation_policy(std::move(annotation_policy_)),
    annotations(program.get_program().get_predicates<formalism::FluentTag>().size()),
    numeric_annotations(program.get_program().get_functions<formalism::FluentTag>().size()),
    delta_annotations(program.get_program().get_predicates<formalism::FluentTag>().size()),
    delta_numeric_annotations(program.get_program().get_functions<formalism::FluentTag>().size()),
    numeric_support_selector(),
    tp(std::move(tp_)),
    cost_policy(std::move(cost_policy_)),
    rules(),
    planning_builder(),
    datalog_builder(),
    schedulers(create_schedulers(program.get_strata(),
                                 program.get_listeners(),
                                 *program_repository,
                                 program.get_program().get_predicates<formalism::FluentTag>().size(),
                                 program.get_program().get_functions<formalism::FluentTag>().size())),
    statistics()
{
    if constexpr (AP::records_propositional_achievers)
        this->annotation_policy.initialize(program.get_program().get_predicates<formalism::FluentTag>().size());

    initialize_rule_workspaces<formalism::PredicateTag>(*const_workspace, rules.template get<formalism::PredicateTag>());
    initialize_rule_workspaces<formalism::FunctionTag>(*const_workspace, rules.template get<formalism::FunctionTag>());
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void ProgramWorkspace<LiftedTag, AP, TP, CP>::reset_evaluation()
{
    rules.for_each(
        [](auto& workspaces)
        {
            for (auto& rule : workspaces)
                if (rule)
                    rule->clear();
        });

    cost_buckets.clear();
    tp.clear();
    cost_policy.clear();
    numeric_support_selector.reset();
    annotations.clear();
    numeric_annotations.clear();
    delta_annotations.clear();
    delta_numeric_annotations.clear();
    annotation_policy.clear_achievers();
    facts.reset();
    workspace_repository->clear();
}

template struct ProgramWorkspace<LiftedTag, NoAnnotationPolicy, NoTerminationPolicy>;
template struct ProgramWorkspace<LiftedTag, MinCostAnnotationPolicy<SumAggregation>, NoTerminationPolicy>;
template struct ProgramWorkspace<LiftedTag, MinCostAnnotationPolicy<SumAggregation>, TerminationPolicy<SumAggregation>>;
template struct ProgramWorkspace<LiftedTag, MinCostAnnotationPolicy<MaxAggregation>, NoTerminationPolicy>;
template struct ProgramWorkspace<LiftedTag, MinCostAnnotationPolicy<MaxAggregation>, TerminationPolicy<MaxAggregation>>;
template struct ProgramWorkspace<LiftedTag, MinCostAnnotationWithAchieversPolicy<MaxAggregation>, TerminationPolicy<MaxAggregation>>;
template struct ProgramWorkspace<LiftedTag, MinCostAnnotationPolicy<SumAggregation>, NoTerminationPolicy, RuleCostOverridePolicy<LiftedTag>>;
template struct ProgramWorkspace<LiftedTag, MinCostAnnotationPolicy<SumAggregation>, TerminationPolicy<SumAggregation>, RuleCostOverridePolicy<LiftedTag>>;
template struct ProgramWorkspace<LiftedTag, MinCostAnnotationPolicy<MaxAggregation>, NoTerminationPolicy, RuleCostOverridePolicy<LiftedTag>>;
template struct ProgramWorkspace<LiftedTag, MinCostAnnotationPolicy<MaxAggregation>, TerminationPolicy<MaxAggregation>, RuleCostOverridePolicy<LiftedTag>>;
template struct ProgramWorkspace<LiftedTag,
                                 MinCostAnnotationWithAchieversPolicy<MaxAggregation>,
                                 TerminationPolicy<MaxAggregation>,
                                 RuleCostOverridePolicy<LiftedTag>>;

ConstProgramWorkspace<LiftedTag>::ConstProgramWorkspace(Program<LiftedTag>& program) :
    facts(program.get_program().get_predicates<formalism::StaticTag>(),
          program.get_program().get_functions<formalism::StaticTag>(),
          program.get_domains().static_predicate_domains,
          program.get_domains().static_function_domains,
          program.get_program().get_objects().size(),
          program.get_program().get_atoms<formalism::StaticTag>(),
          program.get_program().get_fterm_values<formalism::StaticTag>(),
          *program.m_program_repository),
    rules()
{
    initialize_const_rule_workspaces<formalism::PredicateTag>(program,
                                                              *program.m_program_repository,
                                                              program.m_analysis.compatibility_graphs.get<formalism::PredicateTag>(),
                                                              rules.get<formalism::PredicateTag>());
    initialize_const_rule_workspaces<formalism::FunctionTag>(program,
                                                             *program.m_program_repository,
                                                             program.m_analysis.compatibility_graphs.get<formalism::FunctionTag>(),
                                                             rules.get<formalism::FunctionTag>());
}

}
