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

#include "tyr/datalog/ground/solver.hpp"

#include "tyr/datalog/applicability.hpp"
#include "tyr/datalog/cost_buckets.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/ground/policies/numeric_support.hpp"
#include "tyr/datalog/ground/rule_instance.hpp"
#include "tyr/datalog/rule_evaluation.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <limits>
#include <optional>
#include <vector>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{
namespace
{
namespace f = ::tyr::formalism;
namespace fd = ::tyr::formalism::datalog;

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
using GroundCtx = ProgramExecutionContext<GroundTag, AP, TP, CP>;

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
    requires(!AP::stores_annotations)
GroundNumericSupportSelector make_numeric_support_selector(const GroundCtx<AP, TP, CP>& ctx)
{
    return GroundNumericSupportSelector(ctx.in().facts(), ctx.out().facts(), ctx.out().numeric_annotations(), true);
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
    requires(AP::stores_annotations)
GroundNumericSupportSelector make_numeric_support_selector(const GroundCtx<AP, TP, CP>& ctx)
{
    return GroundNumericSupportSelector(ctx.in().facts(), ctx.out().facts(), ctx.out().numeric_annotations());
}

template<f::RelationKind R, AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void enqueue_rule(GroundCtx<AP, TP, CP>& ctx, fd::GroundRuleView<R> rule, Cost queue_label)
{
    auto& out = ctx.out();
    const auto rule_index = rule.get_index();
    auto& states = out.template rule_states<R>();
    if (states[ygg::uint_t(rule_index)].unsatisfied_count != 0 || states[ygg::uint_t(rule_index)].fired)
        return;

    auto& queued_cost = states[ygg::uint_t(rule_index)].queued_cost;
    if (queued_cost && *queued_cost <= queue_label)
        return;
    queued_cost = queue_label;

    auto& queue = out.template queue_storage<R>();
    queue.push_back(GroundQueueEntry<R> { queue_label, rule });
    std::push_heap(queue.begin(), queue.end(), std::greater<> {});

    ++out.statistics().num_queue_pushes;
    out.statistics().max_queue_size =
        std::max(out.statistics().max_queue_size,
                 static_cast<ygg::uint_t>(out.template queue_storage<f::PredicateTag>().size() + out.template queue_storage<f::FunctionTag>().size()));
}

template<f::RelationKind R, AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void push_rule(GroundCtx<AP, TP, CP>& ctx, fd::GroundRuleView<R> rule)
{
    const auto& state = ctx.out().template rule_states<R>()[ygg::uint_t(rule.get_index())];
    if (state.unsatisfied_count != 0 || state.fired)
        return;

    auto instance = RuleInstance<GroundTag, R>(rule);
    auto selector = make_numeric_support_selector(ctx);
    auto& workspace = ctx.out().queue().scratch.rule_evaluation;
    const auto priority = evaluate_rule_priority(instance,
                                                 ctx.out().annotation_policy(),
                                                 ctx.out().cost_policy(),
                                                 RuleEvaluationInput<GroundTag> { selector, ctx.out().annotations() },
                                                 workspace);
    if (priority)
        enqueue_rule(ctx, rule, *priority);
}

template<f::RelationKind R, AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void update_numeric_constraint_satisfaction(GroundCtx<AP, TP, CP>& ctx, fd::GroundRuleView<R> rule)
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

template<f::RelationKind R, AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void initialize_numeric_constraint_satisfaction_for(GroundCtx<AP, TP, CP>& ctx)
{
    for (const auto rule : ctx.in().program().template get_rules<R>())
        update_numeric_constraint_satisfaction(ctx, rule);
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void initialize_numeric_constraint_satisfaction(GroundCtx<AP, TP, CP>& ctx)
{
    initialize_numeric_constraint_satisfaction_for<f::PredicateTag>(ctx);
    initialize_numeric_constraint_satisfaction_for<f::FunctionTag>(ctx);
}

template<f::RelationKind R, AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void seed_queue_for(GroundCtx<AP, TP, CP>& ctx)
{
    for (const auto rule : ctx.in().program().template get_rules<R>())
        push_rule(ctx, rule);
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void seed_queue(GroundCtx<AP, TP, CP>& ctx)
{
    seed_queue_for<f::PredicateTag>(ctx);
    seed_queue_for<f::FunctionTag>(ctx);
}

template<f::RelationKind R, AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
std::optional<GroundQueueEntry<R>> pop_next_entry(GroundCtx<AP, TP, CP>& ctx)
{
    auto& queue = ctx.out().template queue_storage<R>();
    if (queue.empty())
        return std::nullopt;

    std::pop_heap(queue.begin(), queue.end(), std::greater<> {});
    const auto entry = queue.back();
    queue.pop_back();
    ++ctx.out().statistics().num_queue_pops;
    return entry;
}

template<f::RelationKind R, AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
bool is_stale_entry(const GroundCtx<AP, TP, CP>& ctx, const GroundQueueEntry<R>& entry) noexcept
{
    const auto& out = ctx.out();
    const auto& state = out.template rule_states<R>()[ygg::uint_t(entry.rule.get_index())];
    return state.fired || state.unsatisfied_count != 0;
}

template<f::RelationKind R, AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void notify_fact_inserted_for(GroundCtx<AP, TP, CP>& ctx, fd::PredicateBindingView<f::FluentTag> fact)
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

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void notify_fact_inserted(GroundCtx<AP, TP, CP>& ctx, fd::PredicateBindingView<f::FluentTag> fact)
{
    notify_fact_inserted_for<f::PredicateTag>(ctx, fact);
    notify_fact_inserted_for<f::FunctionTag>(ctx, fact);
}

template<f::RelationKind R, AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void notify_fact_annotation_improved_for(GroundCtx<AP, TP, CP>& ctx, fd::PredicateBindingView<f::FluentTag> fact)
{
    auto& out = ctx.out();
    const auto& dependencies = ctx.in().template dependencies<R>().fluent_precondition_to_rules;
    const auto* dependent_rules = dependencies.find(fact);
    if (!dependent_rules)
        return;

    for (const auto dependent_rule : *dependent_rules)
        if (out.template rule_states<R>()[ygg::uint_t(dependent_rule.get_index())].unsatisfied_count == 0)
            push_rule(ctx, dependent_rule);
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void notify_fact_annotation_improved(GroundCtx<AP, TP, CP>& ctx, fd::PredicateBindingView<f::FluentTag> fact)
{
    notify_fact_annotation_improved_for<f::PredicateTag>(ctx, fact);
    notify_fact_annotation_improved_for<f::FunctionTag>(ctx, fact);
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
bool derive_fact(GroundCtx<AP, TP, CP>& ctx, fd::PredicateBindingView<f::FluentTag> fact)
{
    auto& out = ctx.out();
    const auto inserted = out.fact_sets().predicate.insert(fact);
    if (inserted)
    {
        ++out.statistics().num_facts_derived;
    }
    return inserted;
}

template<f::RelationKind R, AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void notify_numeric_interval_changed_for(GroundCtx<AP, TP, CP>& ctx, fd::FunctionBindingView<f::FluentTag> term)
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

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void notify_numeric_interval_changed(GroundCtx<AP, TP, CP>& ctx, fd::FunctionBindingView<f::FluentTag> term)
{
    notify_numeric_interval_changed_for<f::PredicateTag>(ctx, term);
    notify_numeric_interval_changed_for<f::FunctionTag>(ctx, term);
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
bool derive_interval(GroundCtx<AP, TP, CP>& ctx, fd::FunctionBindingView<f::FluentTag> term, ygg::ClosedInterval<ygg::float_t> interval)
{
    if (empty(interval))
        return false;

    return ctx.out().fact_sets().function.insert(term, interval);
}

bool is_annotation_improvement(const std::optional<CostUpdate<GroundTag>>& update) noexcept
{
    return update && (!update->old_cost || update->new_cost < *update->old_cost);
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void fire_rule(GroundCtx<AP, TP, CP>& ctx,
               fd::GroundRuleView<f::PredicateTag> rule,
               RuleInstance<GroundTag, f::PredicateTag>& instance,
               const PredicateCandidate<GroundTag>& candidate,
               CostBuckets& pending_heads)
{
    auto& out = ctx.out();
    out.template rule_states<f::PredicateTag>()[ygg::uint_t(rule.get_index())].fired = true;
    ++out.statistics().num_rules_fired;

    if constexpr (AP::stores_annotations)
    {
        const auto update = publish_candidate(out.annotation_policy(), instance, candidate, out.annotations());
        if (out.fact_sets().predicate.contains(candidate.head))
        {
            if (is_annotation_improvement(update))
                notify_fact_annotation_improved(ctx, candidate.head);
        }
        else if (update)
        {
            pending_heads.update(*update, candidate.head);
        }
    }
    else
    {
        if (!out.fact_sets().predicate.contains(candidate.head))
            pending_heads.insert(candidate.cost, candidate.head);
    }
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void fire_rule(GroundCtx<AP, TP, CP>& ctx,
               fd::GroundRuleView<f::FunctionTag>,
               RuleInstance<GroundTag, f::FunctionTag>& instance,
               const FunctionCandidate<GroundTag>& candidate,
               CostBuckets& pending_heads)
{
    ++ctx.out().statistics().num_rules_fired;
    // FunctionAnnotations retains the cheapest certificate for each exact interval.
    // FactSets determines whether that certificate is available; CostBuckets schedules only hull growth.
    if constexpr (AP::stores_annotations)
        publish_candidate(ctx.out().annotation_policy(), instance, candidate, ctx.out().numeric_annotations());
    if (candidate.grows_fact)
        pending_heads.insert(candidate.cost, candidate.fact_head, candidate.interval);
}

template<f::RelationKind R, AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
Cost next_rule_cost_for(const GroundCtx<AP, TP, CP>& ctx) noexcept
{
    const auto& queue = ctx.out().template queue_storage<R>();
    return queue.empty() ? std::numeric_limits<Cost>::max() : queue.front().cost;
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
Cost next_rule_cost(const GroundCtx<AP, TP, CP>& ctx) noexcept
{
    return std::min(next_rule_cost_for<f::PredicateTag>(ctx), next_rule_cost_for<f::FunctionTag>(ctx));
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
bool commit_head_bucket(GroundCtx<AP, TP, CP>& ctx, CostBuckets& pending_heads, Cost cost)
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

    return (!changed_facts.empty() || !changed_terms.empty())
           && ctx.out().tp().should_terminate(FactSets { ctx.in().facts().fact_sets, ctx.out().facts().fact_sets });
}

template<f::RelationKind R, AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void process_next_rule(GroundCtx<AP, TP, CP>& ctx, CostBuckets& pending_heads)
{
    auto entry = pop_next_entry<R>(ctx);
    assert(entry);

    auto& out = ctx.out();
    const auto rule_index = entry->rule.get_index();
    auto& queued_cost = out.template rule_states<R>()[ygg::uint_t(rule_index)].queued_cost;
    if (!queued_cost || *queued_cost != entry->cost)
    {
        ++out.statistics().num_stale_queue_pops;
        return;
    }
    queued_cost.reset();

    if (is_stale_entry(ctx, *entry))
    {
        ++out.statistics().num_stale_queue_pops;
        return;
    }

    auto instance = RuleInstance<GroundTag, R>(entry->rule);
    auto selector = make_numeric_support_selector(ctx);
    auto& workspace = out.queue().scratch.rule_evaluation;
    const auto input = RuleEvaluationInput<GroundTag> { selector, out.annotations() };
    const auto candidate = [&]
    {
        if constexpr (std::same_as<R, f::PredicateTag>)
            return evaluate_predicate_candidate(instance, out.annotation_policy(), out.cost_policy(), input, workspace);
        else
            return evaluate_function_candidate(instance, out.annotation_policy(), out.cost_policy(), input, workspace);
    }();
    if (!candidate || candidate->queue_label != entry->cost)
    {
        ++out.statistics().num_stale_queue_pops;
        if (candidate)
            enqueue_rule(ctx, entry->rule, candidate->queue_label);
        return;
    }

    fire_rule(ctx, entry->rule, instance, *candidate, pending_heads);
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void process_rule_frontier(GroundCtx<AP, TP, CP>& ctx, CostBuckets& pending_heads, Cost cost)
{
    while (next_rule_cost(ctx) == cost)
    {
        const auto& predicates = ctx.out().template queue_storage<f::PredicateTag>();
        const auto& functions = ctx.out().template queue_storage<f::FunctionTag>();
        // Queue entries are ordered by (cost, relation kind, rule index), with predicates before functions.
        const auto process_predicate = functions.empty() || (!predicates.empty() && predicates.front().cost <= functions.front().cost);
        if (process_predicate)
            process_next_rule<f::PredicateTag>(ctx, pending_heads);
        else
            process_next_rule<f::FunctionTag>(ctx, pending_heads);
    }
}

}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void compute_model(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    if (ctx.out().tp().should_terminate(FactSets { ctx.in().facts().fact_sets, ctx.out().facts().fact_sets }))
        return;

    initialize_numeric_constraint_satisfaction(ctx);
    seed_queue(ctx);

    auto pending_heads = CostBuckets {};
    while (next_rule_cost(ctx) != std::numeric_limits<Cost>::max() || !pending_heads.is_empty())
    {
        const auto rule_cost = next_rule_cost(ctx);
        const auto head_cost = pending_heads.min_cost();
        if (rule_cost <= head_cost)
            process_rule_frontier(ctx, pending_heads, rule_cost);
        else if (commit_head_bucket(ctx, pending_heads, head_cost))
            break;
    }
}

template void compute_model(ProgramExecutionContext<GroundTag, NoAnnotationPolicy<GroundTag>, NoTerminationPolicy<GroundTag>, RuleCostPolicy<GroundTag>>& ctx);
template void compute_model(
    ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<GroundTag, SumAggregation>, NoTerminationPolicy<GroundTag>, RuleCostPolicy<GroundTag>>& ctx);
template void compute_model(ProgramExecutionContext<GroundTag,
                                                    MinCostAnnotationPolicy<GroundTag, SumAggregation>,
                                                    TerminationPolicy<GroundTag, SumAggregation>,
                                                    RuleCostPolicy<GroundTag>>& ctx);
template void compute_model(
    ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<GroundTag, MaxAggregation>, NoTerminationPolicy<GroundTag>, RuleCostPolicy<GroundTag>>& ctx);
template void compute_model(ProgramExecutionContext<GroundTag,
                                                    MinCostAnnotationPolicy<GroundTag, MaxAggregation>,
                                                    TerminationPolicy<GroundTag, MaxAggregation>,
                                                    RuleCostPolicy<GroundTag>>& ctx);
template void compute_model(ProgramExecutionContext<GroundTag,
                                                    MinCostAnnotationWithAchieversPolicy<GroundTag, MaxAggregation>,
                                                    TerminationPolicy<GroundTag, MaxAggregation>,
                                                    RuleCostPolicy<GroundTag>>& ctx);
template void compute_model(
    ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<GroundTag, SumAggregation>, NoTerminationPolicy<GroundTag>, RuleCostOverridePolicy<GroundTag>>&
        ctx);
template void compute_model(ProgramExecutionContext<GroundTag,
                                                    MinCostAnnotationPolicy<GroundTag, SumAggregation>,
                                                    TerminationPolicy<GroundTag, SumAggregation>,
                                                    RuleCostOverridePolicy<GroundTag>>& ctx);
template void compute_model(
    ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<GroundTag, MaxAggregation>, NoTerminationPolicy<GroundTag>, RuleCostOverridePolicy<GroundTag>>&
        ctx);
template void compute_model(ProgramExecutionContext<GroundTag,
                                                    MinCostAnnotationPolicy<GroundTag, MaxAggregation>,
                                                    TerminationPolicy<GroundTag, MaxAggregation>,
                                                    RuleCostOverridePolicy<GroundTag>>& ctx);
template void compute_model(ProgramExecutionContext<GroundTag,
                                                    MinCostAnnotationWithAchieversPolicy<GroundTag, MaxAggregation>,
                                                    TerminationPolicy<GroundTag, MaxAggregation>,
                                                    RuleCostOverridePolicy<GroundTag>>& ctx);
template void compute_model(ProgramExecutionContext<GroundTag,
                                                    MinCostAnnotationWithAchieversPolicy<GroundTag, MaxAggregation>,
                                                    FullModelGoalPolicy<GroundTag, MaxAggregation>,
                                                    RuleCostOverridePolicy<GroundTag>>& ctx);
}
