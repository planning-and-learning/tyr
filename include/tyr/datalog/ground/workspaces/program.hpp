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

#ifndef TYR_DATALOG_GROUND_WORKSPACES_PROGRAM_HPP_
#define TYR_DATALOG_GROUND_WORKSPACES_PROGRAM_HPP_

#include "tyr/datalog/cost_buckets.hpp"
#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/ground/scheduler.hpp"
#include "tyr/datalog/ground/workspaces/facts.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/policies/annotation_concepts.hpp"
#include "tyr/datalog/policies/cost.hpp"
#include "tyr/datalog/policies/cost_concepts.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/datalog/policies/termination_concepts.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <type_traits>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace tyr::datalog
{

struct GroundRuleDependencies
{
private:
    using Rules = urd::Store<urd::sequence<formalism::datalog::RuleView<GroundTag>>>;

public:
    DenseRelationMap<formalism::PredicateTag, Rules> fluent_precondition_to_rules;

    explicit GroundRuleDependencies(size_t num_predicates) : fluent_precondition_to_rules(num_predicates) {}
};

template<>
struct ConstProgramWorkspace<GroundTag>
{
    formalism::datalog::ProgramView<GroundTag> program;
    ConstFactsWorkspace<GroundTag> facts;
    GroundRuleDependencies dependencies;

    explicit ConstProgramWorkspace(formalism::datalog::ProgramView<GroundTag> program);
};

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
struct ProgramWorkspace<GroundTag, AP, TP, CP>
{
    const ConstProgramWorkspace<GroundTag>* const_workspace;
    FactsWorkspace<GroundTag> facts;
    AP annotation_policy;
    PredicateAnnotations<> annotations;
    TP tp;
    CP cost_policy;
    CostBuckets cost_buckets;
    PendingPredicateAchievers pending_achievers;
    Scheduler<GroundTag> scheduler;
    formalism::datalog::Builder datalog_builder;

    explicit ProgramWorkspace(const ConstProgramWorkspace<GroundTag>& cws, AP annotation_policy_ = AP(), TP tp_ = TP(), CP cost_policy_ = CP()) :
        const_workspace(&cws),
        facts(cws.program.template get_predicates<formalism::FluentTag>(),
              cws.program.template get_functions<formalism::FluentTag>(),
              cws.program.template get_atoms<formalism::FluentTag>(),
              cws.program.template get_fterm_values<formalism::FluentTag>(),
              cws.program.context()),
        annotation_policy(std::move(annotation_policy_)),
        annotations(cws.program.template get_predicates<formalism::FluentTag>().size()),
        tp(std::move(tp_)),
        cost_policy(std::move(cost_policy_)),
        scheduler(cws.program),
        datalog_builder()
    {
        if constexpr (AP::records_propositional_achievers)
            annotation_policy.initialize(cws.program.template get_predicates<formalism::FluentTag>().size());
    }

    explicit ProgramWorkspace(Program<GroundTag>& program, AP annotation_policy_ = AP(), TP tp_ = TP(), CP cost_policy_ = CP());

    void clear_costs() { cost_policy.clear(); }
};

}

#endif
