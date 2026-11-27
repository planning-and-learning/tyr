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

#include <boost/mp11.hpp>
#include <cassert>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{
template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void Scheduler<GroundTag>::update_numeric_constraint_satisfaction(formalism::RuleView<GroundTag> rule, ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
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
        if (--unsatisfied_count == 0)
            m_ready.insert(rule.index());
    }
}

template<formalism::FactKind F, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void Scheduler<GroundTag>::notify_predicate_generated_for(formalism::PredicateRowKey<F> fact, ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    const auto* dependent_rules = ctx.in().dependencies().find(fact);
    if (!dependent_rules)
        return;

    // The fact is committed, so its cost is final: each rule waiting on it takes it into its support.
    const auto cost = ctx.out().annotations().fetch_cost(fact);
    const auto aggregate = support_aggregation<AP>();
    for (const auto dependent_rule : *dependent_rules)
        if (m_states[ygg::uint_t(dependent_rule.index().value)].unsatisfied_count != 0)
            satisfy(dependent_rule, cost, aggregate);
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void Scheduler<GroundTag>::begin_stratum(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    for (const auto rule : m_numeric_rules)
        update_numeric_constraint_satisfaction(rule, ctx);

    // A negative literal holds of no cost: it takes nothing into the support.
    const auto fact_sets = FactSets { ctx.in().facts().fact_sets, ctx.out().facts().fact_sets };
    const auto aggregate = support_aggregation<AP>();
    for (const auto rule : m_negative_rules[m_stratum])
        boost::mp11::mp_for_each<MutableTags>(
            [&]<typename F>(F)
            {
                for (const auto literal : rule.get_body().template get_literals<F>())
                    if (!literal.get_polarity() && is_applicable(literal, fact_sets))
                        satisfy(rule, support_start<AP>(), aggregate);
            });
}

template<formalism::FactKind F, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void Scheduler<GroundTag>::notify_predicate_generated(formalism::PredicateRowKey<F> fact, ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    ++m_statistics.num_facts_derived;
    notify_predicate_generated_for(fact, ctx);
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void Scheduler<GroundTag>::notify_generated(const CostBuckets::Bucket& bucket, ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    // In any order: waking a rule only counts down its conditions and aggregates its support.
    boost::mp11::mp_for_each<MutableTags>(
        [&]<typename F>(F)
        {
            for (const auto fact : bucket.predicate.template get<F>())
                notify_predicate_generated(fact, ctx);
        });
}

}

#endif
