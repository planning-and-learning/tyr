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

#include "tyr/datalog/ground/scheduler.hpp"

#include "tyr/datalog/applicability.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/ground/contexts/program.hpp"
#include "tyr/datalog/ground/rule_instance.hpp"
#include "tyr/datalog/rule_evaluation.hpp"

#include <algorithm>
#include <cassert>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{
namespace scheduler_impl
{
namespace f = ::tyr::formalism;
namespace fd = ::tyr::formalism::datalog;
}

template<::tyr::formalism::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void Scheduler<GroundTag>::schedule(scheduler_impl::fd::GroundRuleView<R> rule, ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    if (get_states<R>()[ygg::uint_t(rule.get_index())].unsatisfied_count != 0)
        return;

    auto instance = RuleInstance<GroundTag, R>(rule);
    auto selector = ctx.out().numeric_support_selector();
    auto& workspace = m_scratch.rule_evaluation;
    const auto input = RuleEvaluationInput { selector, ctx.out().annotations() };
    const auto priority = evaluate_rule_priority(instance, ctx.out().annotation_policy(), ctx.out().cost_policy(), input, workspace);
    if (priority)
        enqueue(rule, *priority);
}

template<::tyr::formalism::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void Scheduler<GroundTag>::update_numeric_constraint_satisfaction(scheduler_impl::fd::GroundRuleView<R> rule,
                                                                  ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    const auto rule_index = rule.get_index();
    auto& states = get_states<R>();
    auto& satisfied = states[ygg::uint_t(rule_index)].numeric_constraint_satisfied;
    const auto numeric_constraints = rule.get_body().get_numeric_constraints();
    if (satisfied.size() != numeric_constraints.size())
        satisfied.assign(numeric_constraints.size(), false);

    const auto fact_sets = FactSets { ctx.in().facts().fact_sets, ctx.out().facts().fact_sets };
    for (ygg::uint_t i = 0; i < numeric_constraints.size(); ++i)
    {
        if (satisfied[i] || !evaluate(numeric_constraints[i], fact_sets))
            continue;

        satisfied[i] = true;
        auto& unsatisfied_count = states[ygg::uint_t(rule_index)].unsatisfied_count;
        assert(unsatisfied_count > 0);
        --unsatisfied_count;
    }
}

template<::tyr::formalism::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void Scheduler<GroundTag>::on_generate_predicate(scheduler_impl::fd::PredicateBindingView<scheduler_impl::f::FluentTag> fact,
                                                 ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    const auto* dependent_rules = ctx.in().template dependencies<R>().fluent_precondition_to_rules.find(fact);
    if (!dependent_rules)
        return;

    for (const auto dependent_rule : *dependent_rules)
    {
        auto& unsatisfied_count = get_states<R>()[ygg::uint_t(dependent_rule.get_index())].unsatisfied_count;
        if (unsatisfied_count == 0)
            continue;

        --unsatisfied_count;
        schedule(dependent_rule, ctx);
    }
}

template<::tyr::formalism::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void Scheduler<GroundTag>::on_generate_function(scheduler_impl::fd::FunctionBindingView<scheduler_impl::f::FluentTag> term,
                                                ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    const auto* dependent_rules = ctx.in().template dependencies<R>().fluent_function_term_to_rules.find(term);
    if (!dependent_rules)
        return;

    for (const auto dependent_rule : *dependent_rules)
    {
        update_numeric_constraint_satisfaction(dependent_rule, ctx);
        if (get_states<R>()[ygg::uint_t(dependent_rule.get_index())].unsatisfied_count == 0)
            schedule(dependent_rule, ctx);
    }
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void Scheduler<GroundTag>::seed(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    for (const auto rule : m_program.get_rules<scheduler_impl::f::PredicateTag>())
        update_numeric_constraint_satisfaction(rule, ctx);
    for (const auto rule : m_program.get_rules<scheduler_impl::f::FunctionTag>())
        update_numeric_constraint_satisfaction(rule, ctx);

    for (const auto rule : m_program.get_rules<scheduler_impl::f::PredicateTag>())
        schedule(rule, ctx);
    for (const auto rule : m_program.get_rules<scheduler_impl::f::FunctionTag>())
        schedule(rule, ctx);
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void Scheduler<GroundTag>::on_generate(scheduler_impl::fd::PredicateBindingView<scheduler_impl::f::FluentTag> fact,
                                       ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    ++m_statistics.num_facts_derived;
    on_generate_predicate<scheduler_impl::f::PredicateTag>(fact, ctx);
    on_generate_predicate<scheduler_impl::f::FunctionTag>(fact, ctx);
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void Scheduler<GroundTag>::on_generate(scheduler_impl::fd::FunctionBindingView<scheduler_impl::f::FluentTag> term,
                                       ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    on_generate_function<scheduler_impl::f::PredicateTag>(term, ctx);
    on_generate_function<scheduler_impl::f::FunctionTag>(term, ctx);
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void Scheduler<GroundTag>::on_generate(const CostBuckets::Bucket& bucket, ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    m_scratch.changed_facts.assign(bucket.predicate.begin(), bucket.predicate.end());
    m_scratch.changed_terms.clear();
    m_scratch.changed_terms.reserve(bucket.function.size());
    for (const auto& entry : bucket.function)
        m_scratch.changed_terms.push_back(entry.first);

    std::sort(m_scratch.changed_facts.begin(), m_scratch.changed_facts.end());
    std::sort(m_scratch.changed_terms.begin(), m_scratch.changed_terms.end());
    for (const auto fact : m_scratch.changed_facts)
        on_generate(fact, ctx);
    for (const auto term : m_scratch.changed_terms)
        on_generate(term, ctx);
}

}
