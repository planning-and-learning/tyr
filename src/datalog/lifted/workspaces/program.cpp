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

#include "tyr/datalog/lifted/program.hpp"
#include "tyr/datalog/policies/aggregation.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/policies/termination.hpp"

namespace tyr::datalog
{
namespace
{
template<::tyr::formalism::RelationKind R>
void initialize_rule_workspaces(const ConstProgramWorkspace<LiftedTag>& const_workspace, std::vector<std::unique_ptr<RuleWorkspace<LiftedTag, R>>>& workspaces)
{
    const auto& const_workspaces = const_workspace.template get_rules<R>();
    workspaces.reserve(const_workspaces.size());
    for (const auto& workspace : const_workspaces)
        workspaces.emplace_back(workspace ? std::make_unique<RuleWorkspace<LiftedTag, R>>(*workspace) : nullptr);
}

template<::tyr::formalism::RelationKind R>
void initialize_const_rule_workspaces(Program<LiftedTag>& program,
                                      ::tyr::formalism::datalog::Repository& program_repository,
                                      analysis::RuleCompatibilityGraphMap<R>& graphs,
                                      std::vector<std::optional<ConstRuleWorkspace<LiftedTag, R>>>& workspaces)
{
    const auto rules = program.get_program().template get_rules<R>();
    workspaces.resize(rules.size());
    for (const auto rule : rules)
        workspaces[ygg::uint_t(rule.get_index())].emplace(rule, program_repository, std::move(graphs.at(rule.get_index())));
}
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
ProgramWorkspace<LiftedTag, AP, TP, CP>::ProgramWorkspace(const Program<LiftedTag>& program, AP annotation_policy, TP tp, CP cost_policy) :
    const_workspace(program.get_const_program_workspace()),
    program_repository(program.get_program_repository()),
    m_workspace_repository(program.get_repository_factory().create_shared(program.get_program().get_objects().size(), &program_repository)),
    workspace_repository(*m_workspace_repository),
    facts(program.get_program().get_predicates<::tyr::formalism::FluentTag>(),
          program.get_program().get_functions<::tyr::formalism::FluentTag>(),
          program.get_domains().fluent_predicate_domains,
          program.get_domains().fluent_function_domains,
          program.get_program().get_objects().size(),
          program.get_program().get_atoms<::tyr::formalism::FluentTag>(),
          program.get_program().get_fterm_values<::tyr::formalism::FluentTag>(),
          workspace_repository),
    annotation_policy(std::move(annotation_policy)),
    annotations(program.get_program().get_predicates<::tyr::formalism::FluentTag>().size()),
    numeric_annotations(program.get_program().get_functions<::tyr::formalism::FluentTag>().size()),
    delta_annotations(program.get_program().get_predicates<::tyr::formalism::FluentTag>().size()),
    delta_numeric_annotations(program.get_program().get_functions<::tyr::formalism::FluentTag>().size()),
    numeric_support_selector(),
    tp(tp),
    cost_policy(std::move(cost_policy)),
    predicate_rules(),
    function_rules(),
    planning_builder(),
    datalog_builder(),
    schedulers(create_schedulers(program.get_strata(),
                                 program.get_listeners(),
                                 program_repository,
                                 program.get_program().get_predicates<::tyr::formalism::FluentTag>().size(),
                                 program.get_program().get_functions<::tyr::formalism::FluentTag>().size())),
    cost_buckets(),
    statistics()
{
    if constexpr (AP::records_propositional_achievers)
        this->annotation_policy.initialize(program.get_program().get_predicates<::tyr::formalism::FluentTag>().size());

    initialize_rule_workspaces<::tyr::formalism::PredicateTag>(const_workspace, predicate_rules);
    initialize_rule_workspaces<::tyr::formalism::FunctionTag>(const_workspace, function_rules);
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void ProgramWorkspace<LiftedTag, AP, TP, CP>::reset_evaluation()
{
    const auto clear_rules = [](auto& rules)
    {
        for (auto& rule : rules)
            if (rule)
                rule->clear();
    };
    clear_rules(predicate_rules);
    clear_rules(function_rules);

    tp.clear();
    cost_policy.clear();
    cost_buckets.clear();
    numeric_support_selector.reset();
    annotations.clear();
    numeric_annotations.clear();
    delta_annotations.clear();
    delta_numeric_annotations.clear();
    annotation_policy.clear_achievers();
    facts.reset();
    workspace_repository.clear();
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
    facts(program.get_program().get_predicates<::tyr::formalism::StaticTag>(),
          program.get_program().get_functions<::tyr::formalism::StaticTag>(),
          program.get_domains().static_predicate_domains,
          program.get_domains().static_function_domains,
          program.get_program().get_objects().size(),
          program.get_program().get_atoms<::tyr::formalism::StaticTag>(),
          program.get_program().get_fterm_values<::tyr::formalism::StaticTag>(),
          *program.m_program_repository),
    predicate_rules(),
    function_rules()
{
    initialize_const_rule_workspaces<::tyr::formalism::PredicateTag>(program,
                                                                     *program.m_program_repository,
                                                                     program.m_analysis.compatibility_graphs.predicate_rules,
                                                                     predicate_rules);
    initialize_const_rule_workspaces<::tyr::formalism::FunctionTag>(program,
                                                                    *program.m_program_repository,
                                                                    program.m_analysis.compatibility_graphs.function_rules,
                                                                    function_rules);
}

}
