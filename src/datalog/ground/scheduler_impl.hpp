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

#ifndef TYR_SRC_DATALOG_GROUND_SCHEDULER_IMPL_HPP_
#define TYR_SRC_DATALOG_GROUND_SCHEDULER_IMPL_HPP_

#include "tyr/datalog/applicability.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/ground/contexts/program.hpp"
#include "tyr/datalog/ground/rule_instance.hpp"
#include "tyr/datalog/ground/scheduler.hpp"
#include "tyr/datalog/rule_evaluation.hpp"

#include <algorithm>
#include <cassert>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{
template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void Scheduler<GroundTag>::schedule(formalism::datalog::RuleView<GroundTag> rule, ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    if (m_states[static_cast<ygg::uint_t>(rule.index().value)].unsatisfied_count != 0)
        return;

    auto instance = RuleInstance<GroundTag>(rule);
    const auto input = RuleEvaluationInput { FactSets { ctx.in().facts().fact_sets, ctx.out().facts().fact_sets }, ctx.out().annotations() };
    const auto priority = evaluate_rule_priority(instance, ctx.out().annotation_policy(), ctx.out().cost_policy(), input);
    if (priority)
        enqueue(rule, *priority);
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void Scheduler<GroundTag>::update_numeric_constraint_satisfaction(formalism::datalog::RuleView<GroundTag> rule,
                                                                  ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    const auto rule_index = rule.index();
    auto& states = m_states;
    auto& satisfied = states[static_cast<ygg::uint_t>(rule_index.value)].numeric_constraint_satisfied;
    const auto numeric_constraints = rule.get_body().get_numeric_constraints();

    const auto fact_sets = FactSets { ctx.in().facts().fact_sets, ctx.out().facts().fact_sets };
    for (ygg::uint_t i = 0; i < numeric_constraints.size(); ++i)
    {
        if (satisfied.contains(i) || !evaluate(numeric_constraints[i], fact_sets))
            continue;

        satisfied.insert(i);
        auto& unsatisfied_count = states[static_cast<ygg::uint_t>(rule_index.value)].unsatisfied_count;
        assert(unsatisfied_count > 0);
        --unsatisfied_count;
    }
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void Scheduler<GroundTag>::notify_predicate_generated_for(formalism::datalog::PredicateRowIndex<formalism::FluentTag> fact,
                                                          ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    const auto* dependent_rules = ctx.in().dependencies().fluent_precondition_to_rules.find(fact);
    if (!dependent_rules)
        return;

    for (const auto dependent_rule : *dependent_rules)
    {
        auto& unsatisfied_count = m_states[ygg::uint_t(dependent_rule.index().value)].unsatisfied_count;
        if (unsatisfied_count == 0)
            continue;

        --unsatisfied_count;
        schedule(dependent_rule, ctx);
    }
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void Scheduler<GroundTag>::begin_stratum(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    for (const auto rule : m_program.get_rules())
        update_numeric_constraint_satisfaction(rule, ctx);

    for (const auto rule : m_program.get_rules())
        schedule(rule, ctx);
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void Scheduler<GroundTag>::notify_predicate_generated(formalism::datalog::PredicateRowIndex<formalism::FluentTag> fact,
                                                      ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    ++m_statistics.num_facts_derived;
    notify_predicate_generated_for(fact, ctx);
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void Scheduler<GroundTag>::notify_generated(const CostBuckets::Bucket& bucket, ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    m_scratch.changed_facts.assign(bucket.predicate.begin(), bucket.predicate.end());
    std::sort(m_scratch.changed_facts.begin(), m_scratch.changed_facts.end());
    for (const auto fact : m_scratch.changed_facts)
        notify_predicate_generated(fact, ctx);
}

}

#endif
