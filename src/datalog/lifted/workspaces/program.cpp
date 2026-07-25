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

#include "tyr/datalog/lifted/policies/annotation.hpp"
#include "tyr/datalog/lifted/programs/program.hpp"
#include "tyr/datalog/policies/aggregation.hpp"
#include "tyr/datalog/policies/termination.hpp"

namespace tyr::datalog
{
namespace
{
template<::tyr::formalism::RelationKind R, AndAnnotationPolicyConcept<LiftedTag> AndAP>
void initialize_rule_workspaces(Program<LiftedTag>& program,
                                const ConstProgramWorkspace<LiftedTag>& const_workspace,
                                const ::tyr::formalism::datalog::Repository& program_repository,
                                ::tyr::formalism::datalog::Repository& workspace_repository,
                                const AndAP& and_ap,
                                std::vector<std::unique_ptr<typename RuleWorkspace<LiftedTag, R>::template Instance<AndAP>>>& workspaces)
{
    const auto& const_workspaces = const_workspace.template get_rules<R>();
    workspaces.reserve(const_workspaces.size());
    for (const auto& workspace : const_workspaces)
        workspaces.emplace_back(workspace ? std::make_unique<typename RuleWorkspace<LiftedTag, R>::template Instance<AndAP>>(program.get_repository_factory(),
                                                                                                                             program_repository,
                                                                                                                             workspace_repository,
                                                                                                                             *workspace,
                                                                                                                             and_ap) :
                                            nullptr);
}

template<::tyr::formalism::RelationKind R>
void initialize_const_rule_workspaces(Program<LiftedTag>& program,
                                      const ConstFactsWorkspace<LiftedTag>& facts,
                                      std::vector<std::optional<ConstRuleWorkspace<LiftedTag, R>>>& workspaces)
{
    const auto rules = program.get_program().template get_rules<R>();
    const auto& domains = program.get_domains().template get_rule_domains<R>();
    workspaces.resize(rules.size());
    for (const auto rule : rules)
        workspaces[ygg::uint_t(rule.get_index())].emplace(rule,
                                                          program.get_workspace_repository(),
                                                          domains.at(rule.get_index()).payload,
                                                          program.get_program().get_objects().size(),
                                                          program.get_program().template get_predicates<::tyr::formalism::FluentTag>().size(),
                                                          facts.fact_sets,
                                                          facts.assignment_sets);
}
}

template<OrAnnotationPolicyConcept<LiftedTag> OrAP,
         AndAnnotationPolicyConcept<LiftedTag> AndAP,
         TerminationPolicyConcept<LiftedTag> TP,
         RuleCostPolicyConcept<LiftedTag> CP>
ProgramWorkspace<LiftedTag, OrAP, AndAP, TP, CP>::ProgramWorkspace(Program<LiftedTag>& program, OrAP or_ap, AndAP and_ap, TP tp, CP cost_policy) :
    const_workspace(program.get_const_program_workspace()),
    program_repository(program.get_program_repository()),
    workspace_repository(program.get_workspace_repository()),
    facts(program.get_program().get_predicates<::tyr::formalism::FluentTag>(),
          program.get_program().get_functions<::tyr::formalism::FluentTag>(),
          program.get_domains().fluent_predicate_domains,
          program.get_domains().fluent_function_domains,
          program.get_program().get_objects().size(),
          program.get_program().get_atoms<::tyr::formalism::FluentTag>(),
          program.get_program().get_fterm_values<::tyr::formalism::FluentTag>(),
          program.get_workspace_repository()),
    or_ap(or_ap),
    and_annot(),
    numeric_and_annot(),
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
    initialize_rule_workspaces<::tyr::formalism::PredicateTag>(program, const_workspace, program_repository, workspace_repository, and_ap, predicate_rules);
    initialize_rule_workspaces<::tyr::formalism::FunctionTag>(program, const_workspace, program_repository, workspace_repository, and_ap, function_rules);
}

template struct ProgramWorkspace<LiftedTag, NoOrAnnotationPolicy<LiftedTag>, NoAndAnnotationPolicy<LiftedTag>, NoTerminationPolicy<LiftedTag>>;
template struct ProgramWorkspace<LiftedTag, OrAnnotationPolicy<LiftedTag>, AndAnnotationPolicy<LiftedTag, SumAggregation>, NoTerminationPolicy<LiftedTag>>;
template struct ProgramWorkspace<LiftedTag,
                                 OrAnnotationPolicy<LiftedTag>,
                                 AndAnnotationPolicy<LiftedTag, SumAggregation>,
                                 TerminationPolicy<LiftedTag, SumAggregation>>;
template struct ProgramWorkspace<LiftedTag, OrAnnotationPolicy<LiftedTag>, AndAnnotationPolicy<LiftedTag, MaxAggregation>, NoTerminationPolicy<LiftedTag>>;
template struct ProgramWorkspace<LiftedTag,
                                 OrAnnotationPolicy<LiftedTag>,
                                 AndAnnotationPolicy<LiftedTag, MaxAggregation>,
                                 TerminationPolicy<LiftedTag, MaxAggregation>>;
template struct ProgramWorkspace<LiftedTag,
                                 NoOrAnnotationPolicy<LiftedTag>,
                                 NoAndAnnotationPolicy<LiftedTag>,
                                 NoTerminationPolicy<LiftedTag>,
                                 RuleCostOverridePolicy<LiftedTag>>;
template struct ProgramWorkspace<LiftedTag,
                                 OrAnnotationPolicy<LiftedTag>,
                                 AndAnnotationPolicy<LiftedTag, SumAggregation>,
                                 NoTerminationPolicy<LiftedTag>,
                                 RuleCostOverridePolicy<LiftedTag>>;
template struct ProgramWorkspace<LiftedTag,
                                 OrAnnotationPolicy<LiftedTag>,
                                 AndAnnotationPolicy<LiftedTag, SumAggregation>,
                                 TerminationPolicy<LiftedTag, SumAggregation>,
                                 RuleCostOverridePolicy<LiftedTag>>;
template struct ProgramWorkspace<LiftedTag,
                                 OrAnnotationPolicy<LiftedTag>,
                                 AndAnnotationPolicy<LiftedTag, MaxAggregation>,
                                 NoTerminationPolicy<LiftedTag>,
                                 RuleCostOverridePolicy<LiftedTag>>;
template struct ProgramWorkspace<LiftedTag,
                                 OrAnnotationPolicy<LiftedTag>,
                                 AndAnnotationPolicy<LiftedTag, MaxAggregation>,
                                 TerminationPolicy<LiftedTag, MaxAggregation>,
                                 RuleCostOverridePolicy<LiftedTag>>;
template struct ProgramWorkspace<LiftedTag,
                                 OrAnnotationPolicy<LiftedTag>,
                                 AchieverAndAnnotationPolicy<LiftedTag, MaxAggregation>,
                                 TerminationPolicy<LiftedTag, MaxAggregation>,
                                 RuleCostOverridePolicy<LiftedTag>>;

ConstProgramWorkspace<LiftedTag>::ConstProgramWorkspace(Program<LiftedTag>& program) :
    facts(program.get_program().get_predicates<::tyr::formalism::StaticTag>(),
          program.get_program().get_functions<::tyr::formalism::StaticTag>(),
          program.get_domains().static_predicate_domains,
          program.get_domains().static_function_domains,
          program.get_program().get_objects().size(),
          program.get_program().get_atoms<::tyr::formalism::StaticTag>(),
          program.get_program().get_fterm_values<::tyr::formalism::StaticTag>(),
          program.get_program_repository()),
    predicate_rules(),
    function_rules()
{
    initialize_const_rule_workspaces<::tyr::formalism::PredicateTag>(program, facts, predicate_rules);
    initialize_const_rule_workspaces<::tyr::formalism::FunctionTag>(program, facts, function_rules);
}

}
