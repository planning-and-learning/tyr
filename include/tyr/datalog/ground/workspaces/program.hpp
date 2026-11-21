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
#include "tyr/formalism/repository.hpp"

#include <boost/mp11.hpp>
#include <type_traits>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace tyr::datalog
{

/// The rules that wait on each fact of a mutable kind, a positive literal of their body.
struct GroundRuleDependencies
{
private:
    using Rules = urd::Store<urd::sequence<formalism::RuleView<GroundTag>>>;

    template<typename F>
    using PreconditionToRules = DenseRelationMap<formalism::PredicateTag, F, Rules>;

public:
    MutableFamily<PreconditionToRules> precondition_to_rules;

    /// Sized for the predicates of each mutable kind of the program.
    explicit GroundRuleDependencies(formalism::ProgramView<GroundTag> program)
    {
        boost::mp11::mp_for_each<MutableTags>([&]<typename F>(F)
                                              { precondition_to_rules.template get<F>().initialize(program.template get_predicates<F>().size()); });
    }

    /// The rules that wait on the fact, if any.
    template<formalism::FactKind F>
    [[nodiscard]] const Rules* find(formalism::PredicateRowIndex<F> fact) const noexcept
    { return precondition_to_rules.template get<F>().find(fact); }
};

template<>
struct ConstProgramWorkspace<GroundTag>
{
    formalism::ProgramView<GroundTag> program;
    ConstFactsWorkspace<GroundTag> facts;
    GroundRuleDependencies dependencies;

    explicit ConstProgramWorkspace(formalism::ProgramView<GroundTag> program);
};

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
struct ProgramWorkspace<GroundTag, AP, TP, CP>
{
    const ConstProgramWorkspace<GroundTag>* const_workspace;
    FactsWorkspace<GroundTag> facts;
    AP annotation_policy;
    PredicateAnnotations<GroundTag> annotations;
    TP tp;
    CP cost_policy;
    CostBuckets cost_buckets;
    PendingPredicateAchieverFamily<GroundTag> pending_achievers;
    Scheduler<GroundTag> scheduler;
    formalism::Builder datalog_builder;

    explicit ProgramWorkspace(const ConstProgramWorkspace<GroundTag>& cws, AP annotation_policy_ = AP(), TP tp_ = TP(), CP cost_policy_ = CP()) :
        const_workspace(&cws),
        facts(cws.program, cws.program.context()),
        annotation_policy(std::move(annotation_policy_)),
        annotations(cws.program),
        tp(std::move(tp_)),
        cost_policy(std::move(cost_policy_)),
        scheduler(cws.program),
        datalog_builder()
    {
        if constexpr (AP::records_propositional_achievers)
            annotation_policy.initialize(cws.program);
    }

    explicit ProgramWorkspace(Program<GroundTag>& program, AP annotation_policy_ = AP(), TP tp_ = TP(), CP cost_policy_ = CP());

    void clear_costs() { cost_policy.clear(); }
};

}

#endif
