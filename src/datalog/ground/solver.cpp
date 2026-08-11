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

#include "tyr/datalog/solver.hpp"

#include "tyr/datalog/applicability.hpp"
#include "tyr/datalog/cost_buckets.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/ground/contexts/program.hpp"
#include "tyr/datalog/ground/rule_instance.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/policies/numeric_support.hpp"
#include "tyr/datalog/rule_evaluation.hpp"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <functional>
#include <limits>
#include <optional>
#include <utility>
#include <vector>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{
namespace
{
namespace f = ::tyr::formalism;
namespace fd = ::tyr::formalism::datalog;

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void enqueue_rule(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx, fd::GroundRuleView<R> rule, Cost queue_label)
{
    auto& out = ctx.out();
    const auto rule_index = rule.get_index();
    auto& states = out.template rule_states<R>();
    if (states[ygg::uint_t(rule_index)].unsatisfied_count != 0)
        return;

    auto& queued_cost = states[ygg::uint_t(rule_index)].queued_cost;
    if (queued_cost && *queued_cost <= queue_label)
        return;
    queued_cost = queue_label;

    auto& queue = out.template queue_storage<R>();
    queue.push_back(GroundQueueEntry<R> { queue_label, rule });
    std::push_heap(queue.begin(), queue.end(), std::greater<> {});

    ++out.queue_statistics().num_queue_pushes;
    out.queue_statistics().max_queue_size =
        std::max(out.queue_statistics().max_queue_size,
                 static_cast<ygg::uint_t>(out.template queue_storage<f::PredicateTag>().size() + out.template queue_storage<f::FunctionTag>().size()));
}

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void push_rule(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx, fd::GroundRuleView<R> rule)
{
    const auto& state = ctx.out().template rule_states<R>()[ygg::uint_t(rule.get_index())];
    if (state.unsatisfied_count != 0)
        return;

    auto instance = RuleInstance<GroundTag, R>(rule);
    auto selector = ctx.out().numeric_support_selector();
    auto& workspace = ctx.out().queue().scratch.rule_evaluation;
    const auto input = RuleEvaluationInput { selector, ctx.out().annotations() };
    const auto priority = evaluate_rule_priority(instance, ctx.out().annotation_policy(), ctx.out().cost_policy(), input, workspace);
    if (priority)
        enqueue_rule(ctx, rule, *priority);
}

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void update_numeric_constraint_satisfaction(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx, fd::GroundRuleView<R> rule)
{
    auto& out = ctx.out();
    const auto rule_index = rule.get_index();
    auto& states = out.template rule_states<R>();
    auto& satisfied = states[ygg::uint_t(rule_index)].numeric_constraint_satisfied;
    const auto numeric_constraints = rule.get_body().get_numeric_constraints();
    if (satisfied.size() != numeric_constraints.size())
        satisfied.assign(numeric_constraints.size(), false);

    const auto fact_sets = FactSets { ctx.in().facts().fact_sets, ctx.out().facts().fact_sets };
    for (ygg::uint_t i = 0; i < numeric_constraints.size(); ++i)
    {
        if (satisfied[i])
            continue;

        if (!evaluate(numeric_constraints[i], fact_sets))
            continue;

        satisfied[i] = true;
        auto& unsatisfied_count = states[ygg::uint_t(rule_index)].unsatisfied_count;
        assert(unsatisfied_count > 0);
        --unsatisfied_count;
    }
}

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void initialize_numeric_constraint_satisfaction_for(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    for (const auto rule : ctx.in().program().template get_rules<R>())
        update_numeric_constraint_satisfaction(ctx, rule);
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void initialize_numeric_constraint_satisfaction(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    initialize_numeric_constraint_satisfaction_for<f::PredicateTag>(ctx);
    initialize_numeric_constraint_satisfaction_for<f::FunctionTag>(ctx);
}

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void seed_queue_for(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    for (const auto rule : ctx.in().program().template get_rules<R>())
        push_rule(ctx, rule);
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void seed_queue(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    seed_queue_for<f::PredicateTag>(ctx);
    seed_queue_for<f::FunctionTag>(ctx);
}

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
std::optional<GroundQueueEntry<R>> pop_next_entry(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    auto& queue = ctx.out().template queue_storage<R>();
    if (queue.empty())
        return std::nullopt;

    std::pop_heap(queue.begin(), queue.end(), std::greater<> {});
    const auto entry = queue.back();
    queue.pop_back();
    ++ctx.out().queue_statistics().num_queue_pops;
    return entry;
}

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void notify_fact_inserted_for(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx, fd::PredicateBindingView<f::FluentTag> fact)
{
    auto& out = ctx.out();
    const auto& dependencies = ctx.in().template dependencies<R>().fluent_precondition_to_rules;
    const auto* dependent_rules = dependencies.find(fact);
    if (!dependent_rules)
        return;

    for (const auto dependent_rule : *dependent_rules)
    {
        auto& unsatisfied_count = out.template rule_states<R>()[ygg::uint_t(dependent_rule.get_index())].unsatisfied_count;
        if (unsatisfied_count == 0)
            continue;

        --unsatisfied_count;
        push_rule(ctx, dependent_rule);
    }
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void notify_fact_inserted(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx, fd::PredicateBindingView<f::FluentTag> fact)
{
    notify_fact_inserted_for<f::PredicateTag>(ctx, fact);
    notify_fact_inserted_for<f::FunctionTag>(ctx, fact);
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
bool derive_fact(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx, fd::PredicateBindingView<f::FluentTag> fact)
{
    auto& out = ctx.out();
    const auto inserted = out.facts().fact_sets.predicate.insert(fact);
    if (inserted)
    {
        ++out.queue_statistics().num_facts_derived;
    }
    return inserted;
}

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void notify_numeric_interval_changed_for(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx, fd::FunctionBindingView<f::FluentTag> term)
{
    const auto& dependencies = ctx.in().template dependencies<R>().fluent_function_term_to_rules;
    const auto* dependent_rules = dependencies.find(term);
    if (!dependent_rules)
        return;

    for (const auto dependent_rule : *dependent_rules)
    {
        update_numeric_constraint_satisfaction(ctx, dependent_rule);
        if (ctx.out().template rule_states<R>()[ygg::uint_t(dependent_rule.get_index())].unsatisfied_count == 0)
            push_rule(ctx, dependent_rule);
    }
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void notify_numeric_interval_changed(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx, fd::FunctionBindingView<f::FluentTag> term)
{
    notify_numeric_interval_changed_for<f::PredicateTag>(ctx, term);
    notify_numeric_interval_changed_for<f::FunctionTag>(ctx, term);
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
bool derive_interval(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx,
                     fd::FunctionBindingView<f::FluentTag> term,
                     ygg::ClosedInterval<ygg::float_t> interval)
{
    if (empty(interval))
        return false;

    return ctx.out().facts().fact_sets.function.insert(term, interval);
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void stage_rule(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx,
                RuleInstance<GroundTag, f::PredicateTag>& instance,
                const PredicateCandidate& candidate,
                CostBuckets& pending_heads,
                PendingPredicateAchievers& pending_achievers)
{
    auto& out = ctx.out();
    ++out.queue_statistics().num_rules_fired;

    if constexpr (AP::stores_annotations)
    {
        const auto factual = out.facts().fact_sets.predicate.contains(candidate.head);
        if (factual)
        {
            assert(candidate.cost >= out.annotations().fetch_cost(candidate.head) && "A committed predicate label must be final.");
            if constexpr (AP::records_propositional_achievers)
                insert_pending_achiever(pending_achievers, PredicateAchiever { candidate.head, materialize_witness(instance, candidate) });
            return;
        }

        auto witness = materialize_witness(instance, candidate);
        if constexpr (AP::records_propositional_achievers)
            insert_pending_achiever(pending_achievers, PredicateAchiever { candidate.head, witness });

        const auto update = out.annotation_policy().publish_annotation(candidate.head, std::move(witness), out.annotations());
        if (update)
        {
            pending_heads.update(*update, candidate.head);
        }
        else
        {
            if (const auto* annotation = out.annotations().find(candidate.head))
                pending_heads.insert(get_cost(*annotation), candidate.head);
        }
    }
    else
    {
        if (!out.facts().fact_sets.predicate.contains(candidate.head))
            pending_heads.insert(candidate.cost, candidate.head);
    }
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void fire_rule(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx,
               RuleInstance<GroundTag, f::FunctionTag>& instance,
               const FunctionCandidate& candidate,
               CostBuckets& pending_heads)
{
    ++ctx.out().queue_statistics().num_rules_fired;
    // FunctionAnnotations retains the cheapest certificate for each exact interval.
    // FactSets determines whether that certificate is available; CostBuckets schedules only hull growth.
    auto annotation_improved = false;
    if constexpr (AP::stores_annotations)
        annotation_improved = ctx.out().annotation_policy().try_update_candidate(candidate.head,
                                                                                 candidate.interval,
                                                                                 materialize_witness(instance, candidate),
                                                                                 ctx.out().numeric_annotations());
    if (candidate.grows_fact)
        pending_heads.insert(candidate.cost, candidate.head, candidate.interval);
    else if (annotation_improved)
        notify_numeric_interval_changed(ctx, candidate.head);
}

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
Cost next_rule_cost_for(const ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx) noexcept
{
    const auto& queue = ctx.out().template queue_storage<R>();
    return queue.empty() ? std::numeric_limits<Cost>::max() : queue.front().cost;
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
Cost next_rule_cost(const ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx) noexcept
{
    return std::min(next_rule_cost_for<f::PredicateTag>(ctx), next_rule_cost_for<f::FunctionTag>(ctx));
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void commit_head_bucket(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx, CostBuckets& pending_heads, Cost cost)
{
    auto bucket = pending_heads.take(cost);
    auto changed_facts = std::vector<fd::PredicateBindingView<f::FluentTag>> {};
    changed_facts.reserve(bucket.predicate.size());
    auto& changed_terms = ctx.out().queue().scratch.changed_terms;
    changed_terms.clear();
    changed_terms.reserve(bucket.function.size());

    for (const auto fact : bucket.predicate)
        if (derive_fact(ctx, fact))
            changed_facts.push_back(fact);
    for (const auto& [term, interval] : bucket.function)
        if (derive_interval(ctx, term, interval))
            changed_terms.push_back(term);

    // Install the entire bucket before notifying rules, and keep notification order independent of unordered bucket iteration.
    if (changed_facts.size() > 1)
        std::sort(changed_facts.begin(), changed_facts.end());
    if (changed_terms.size() > 1)
        std::sort(changed_terms.begin(), changed_terms.end());
    for (const auto fact : changed_facts)
        notify_fact_inserted(ctx, fact);
    for (const auto term : changed_terms)
        notify_numeric_interval_changed(ctx, term);
}

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void process_next_rule(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx, CostBuckets& pending_heads, PendingPredicateAchievers& pending_achievers)
{
    auto entry = pop_next_entry<R>(ctx);
    assert(entry);

    auto& out = ctx.out();
    const auto rule_index = entry->rule.get_index();
    auto& state = out.template rule_states<R>()[ygg::uint_t(rule_index)];
    auto& queued_cost = state.queued_cost;
    if (!queued_cost || *queued_cost != entry->cost)
    {
        ++out.queue_statistics().num_stale_queue_pops;
        return;
    }
    queued_cost.reset();

    auto instance = RuleInstance<GroundTag, R>(entry->rule);
    auto selector = ctx.out().numeric_support_selector();
    auto& workspace = out.queue().scratch.rule_evaluation;
    const auto input = RuleEvaluationInput { selector, out.annotations() };
    const auto candidate = [&]
    {
        if constexpr (std::same_as<R, f::PredicateTag>)
            return evaluate_predicate_candidate(instance, out.annotation_policy(), out.cost_policy(), input, workspace);
        else
            return evaluate_function_candidate(instance, out.annotation_policy(), out.cost_policy(), input, workspace);
    }();
    if (!candidate || candidate->queue_label != entry->cost)
    {
        ++out.queue_statistics().num_stale_queue_pops;
        if (candidate)
            enqueue_rule(ctx, entry->rule, candidate->queue_label);
        return;
    }

    if constexpr (std::same_as<R, f::PredicateTag>)
        stage_rule(ctx, instance, *candidate, pending_heads, pending_achievers);
    else
        fire_rule(ctx, instance, *candidate, pending_heads);
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void process_rule_frontier(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx,
                           CostBuckets& pending_heads,
                           PendingPredicateAchievers& pending_achievers,
                           Cost cost)
{
    while (next_rule_cost(ctx) == cost)
    {
        const auto predicate_cost = next_rule_cost_for<f::PredicateTag>(ctx);
        const auto function_cost = next_rule_cost_for<f::FunctionTag>(ctx);
        if (predicate_cost <= function_cost)
            process_next_rule<f::PredicateTag>(ctx, pending_heads, pending_achievers);
        else
            process_next_rule<f::FunctionTag>(ctx, pending_heads, pending_achievers);
    }
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void process_rule_wave(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx, CostBuckets& pending_heads, PendingPredicateAchievers& pending_achievers)
{
    while (next_rule_cost(ctx) != std::numeric_limits<Cost>::max())
        process_rule_frontier(ctx, pending_heads, pending_achievers, next_rule_cost(ctx));
}

}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void compute_model(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    initialize_numeric_constraint_satisfaction(ctx);
    seed_queue(ctx);

    auto pending_heads = CostBuckets {};
    auto pending_achievers = PendingPredicateAchievers {};
    while (true)
    {
        if (ctx.out().tp().should_terminate(FactSets { ctx.in().facts().fact_sets, ctx.out().facts().fact_sets }))
            return;

        if (next_rule_cost(ctx) != std::numeric_limits<Cost>::max())
        {
            // Every candidate in a ready wave observes the same committed fact snapshot.
            process_rule_wave(ctx, pending_heads, pending_achievers);
            continue;
        }

        if (pending_heads.is_empty())
        {
            publish_pending_achievers(pending_achievers, pending_achievers.buckets.end(), ctx.out().annotation_policy());
            return;
        }

        const auto cost = pending_heads.min_cost();
        commit_head_bucket(ctx, pending_heads, cost);
        publish_pending_achievers(pending_achievers, pending_achievers.buckets.upper_bound(cost), ctx.out().annotation_policy());
    }
}

template void compute_model(ProgramExecutionContext<GroundTag, NoAnnotationPolicy, NoTerminationPolicy, RuleCostPolicy>& ctx);
template void compute_model(ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<SumAggregation>, NoTerminationPolicy, RuleCostPolicy>& ctx);
template void
compute_model(ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<SumAggregation>, TerminationPolicy<SumAggregation>, RuleCostPolicy>& ctx);
template void compute_model(ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<MaxAggregation>, NoTerminationPolicy, RuleCostPolicy>& ctx);
template void
compute_model(ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<MaxAggregation>, TerminationPolicy<MaxAggregation>, RuleCostPolicy>& ctx);
template void
compute_model(ProgramExecutionContext<GroundTag, MinCostAnnotationWithAchieversPolicy<MaxAggregation>, TerminationPolicy<MaxAggregation>, RuleCostPolicy>& ctx);
template void
compute_model(ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<SumAggregation>, NoTerminationPolicy, RuleCostOverridePolicy<GroundTag>>& ctx);
template void compute_model(
    ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<SumAggregation>, TerminationPolicy<SumAggregation>, RuleCostOverridePolicy<GroundTag>>& ctx);
template void
compute_model(ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<MaxAggregation>, NoTerminationPolicy, RuleCostOverridePolicy<GroundTag>>& ctx);
template void compute_model(
    ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<MaxAggregation>, TerminationPolicy<MaxAggregation>, RuleCostOverridePolicy<GroundTag>>& ctx);
template void compute_model(ProgramExecutionContext<GroundTag,
                                                    MinCostAnnotationWithAchieversPolicy<MaxAggregation>,
                                                    TerminationPolicy<MaxAggregation>,
                                                    RuleCostOverridePolicy<GroundTag>>& ctx);
}
