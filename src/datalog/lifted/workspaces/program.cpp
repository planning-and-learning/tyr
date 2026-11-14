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

#include "tyr/datalog/formatter.hpp"
#include "tyr/datalog/lifted/program.hpp"
#include "tyr/datalog/policies/aggregation.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/policies/termination.hpp"

#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::datalog
{
template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
ProgramWorkspace<LiftedTag, AP, TP, CP>::ProgramWorkspace(const Program<LiftedTag>& program, AP annotation_policy_, TP tp_, CP cost_policy_) :
    const_workspace(&program.get_const_program_workspace()),
    program_repository(&program.get_program_repository()),
    workspace_repository(std::make_shared<formalism::datalog::Repository>(program.get_program().get_objects().size(), program_repository)),
    facts(program.get_program().get_predicates<formalism::FluentTag>(),
          program.get_program().get_functions<formalism::FluentTag>(),
          program.get_program().get_atoms<formalism::FluentTag>(),
          program.get_program().get_fterm_values<formalism::FluentTag>(),
          *workspace_repository),
    annotation_policy(std::move(annotation_policy_)),
    annotations(program.get_program().get_predicates<formalism::FluentTag>().size()),
    delta_annotations(program.get_program().get_predicates<formalism::FluentTag>().size()),
    tp(std::move(tp_)),
    cost_policy(std::move(cost_policy_)),
    engine(const_workspace->engine, program.get_program(), program.get_domains(), facts.fact_sets),
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
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void ProgramWorkspace<LiftedTag, AP, TP, CP>::reset_evaluation()
{
    engine.clear();
    cost_buckets.clear();
    tp.clear();
    cost_policy.clear();
    annotations.clear();
    delta_annotations.clear();
    annotation_policy.clear_achievers();
    facts.reset();
    workspace_repository->clear_all();
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
          program.get_program().get_atoms<formalism::StaticTag>(),
          program.get_program().get_fterm_values<formalism::StaticTag>(),
          *program.m_program_repository),
    engine(program.get_program(), *program.m_program_repository, program.m_analysis, facts.fact_sets)
{
}

}
