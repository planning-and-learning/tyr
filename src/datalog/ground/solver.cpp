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
#include <tuple>
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

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
using GroundCtx = ProgramExecutionContext<GroundTag, AP, TP, CP>;

struct PendingPredicateFinalization
{
    Cost cost;
    fd::GroundRuleView<f::PredicateTag> rule;
};

struct PendingPredicateFinalizationGreater
{
    bool operator()(const PendingPredicateFinalization& lhs, const PendingPredicateFinalization& rhs) const noexcept
    {
        return std::tie(lhs.cost, lhs.rule) > std::tie(rhs.cost, rhs.rule);
    }
};

using PendingPredicateFinalizations = std::vector<PendingPredicateFinalization>;
using PendingPredicateWitnesses = std::vector<WitnessAnnotation<f::PredicateTag>>;

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    requires(!AP::stores_annotations)
NumericSupportSelector make_numeric_support_selector(const GroundCtx<AP, TP, CP>& ctx)
{
    return NumericSupportSelector(FactSets { ctx.in().facts().fact_sets, ctx.out().facts().fact_sets }, ctx.out().numeric_annotations(), true);
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    requires(AP::stores_annotations)
NumericSupportSelector make_numeric_support_selector(const GroundCtx<AP, TP, CP>& ctx)
{
    return NumericSupportSelector(FactSets { ctx.in().facts().fact_sets, ctx.out().facts().fact_sets }, ctx.out().numeric_annotations());
}

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
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

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void push_rule(GroundCtx<AP, TP, CP>& ctx, fd::GroundRuleView<R> rule)
{
    const auto& state = ctx.out().template rule_states<R>()[ygg::uint_t(rule.get_index())];
    if (state.unsatisfied_count != 0 || state.fired)
        return;

    auto instance = RuleInstance<GroundTag, R>(rule);
    auto selector = make_numeric_support_selector(ctx);
    auto& workspace = ctx.out().queue().scratch.rule_evaluation;
    const auto input = RuleEvaluationInput { selector, ctx.out().annotations() };
    const auto priority = evaluate_rule_priority(instance, ctx.out().annotation_policy(), ctx.out().cost_policy(), input, workspace);
    if (priority)
        enqueue_rule(ctx, rule, *priority);
}

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
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

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void initialize_numeric_constraint_satisfaction_for(GroundCtx<AP, TP, CP>& ctx)
{
    for (const auto rule : ctx.in().program().template get_rules<R>())
        update_numeric_constraint_satisfaction(ctx, rule);
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void initialize_numeric_constraint_satisfaction(GroundCtx<AP, TP, CP>& ctx)
{
    initialize_numeric_constraint_satisfaction_for<f::PredicateTag>(ctx);
    initialize_numeric_constraint_satisfaction_for<f::FunctionTag>(ctx);
}

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void seed_queue_for(GroundCtx<AP, TP, CP>& ctx)
{
    for (const auto rule : ctx.in().program().template get_rules<R>())
        push_rule(ctx, rule);
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void seed_queue(GroundCtx<AP, TP, CP>& ctx)
{
    seed_queue_for<f::PredicateTag>(ctx);
    seed_queue_for<f::FunctionTag>(ctx);
}

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
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

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
bool is_stale_entry(const GroundCtx<AP, TP, CP>& ctx, const GroundQueueEntry<R>& entry) noexcept
{
    const auto& out = ctx.out();
    const auto& state = out.template rule_states<R>()[ygg::uint_t(entry.rule.get_index())];
    return state.fired || state.unsatisfied_count != 0;
}

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
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

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void notify_fact_inserted(GroundCtx<AP, TP, CP>& ctx, fd::PredicateBindingView<f::FluentTag> fact)
{
    notify_fact_inserted_for<f::PredicateTag>(ctx, fact);
    notify_fact_inserted_for<f::FunctionTag>(ctx, fact);
}

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
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

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void notify_fact_annotation_improved(GroundCtx<AP, TP, CP>& ctx, fd::PredicateBindingView<f::FluentTag> fact)
{
    notify_fact_annotation_improved_for<f::PredicateTag>(ctx, fact);
    notify_fact_annotation_improved_for<f::FunctionTag>(ctx, fact);
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
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

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
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

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void notify_numeric_interval_changed(GroundCtx<AP, TP, CP>& ctx, fd::FunctionBindingView<f::FluentTag> term)
{
    notify_numeric_interval_changed_for<f::PredicateTag>(ctx, term);
    notify_numeric_interval_changed_for<f::FunctionTag>(ctx, term);
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
bool derive_interval(GroundCtx<AP, TP, CP>& ctx, fd::FunctionBindingView<f::FluentTag> term, ygg::ClosedInterval<ygg::float_t> interval)
{
    if (empty(interval))
        return false;

    return ctx.out().fact_sets().function.insert(term, interval);
}

bool is_annotation_improvement(const std::optional<CostUpdate>& update) noexcept
{
    return update && (!update->old_cost || update->new_cost < *update->old_cost);
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void stage_rule(GroundCtx<AP, TP, CP>& ctx,
                fd::GroundRuleView<f::PredicateTag> rule,
                RuleInstance<GroundTag, f::PredicateTag>& instance,
                const PredicateCandidate& candidate,
                CostBuckets& pending_heads,
                PendingPredicateFinalizations& pending_finalizations,
                PendingPredicateWitnesses& pending_witnesses)
{
    auto& out = ctx.out();
    auto& state = out.template rule_states<f::PredicateTag>()[ygg::uint_t(rule.get_index())];
    if (state.pending_cost && *state.pending_cost <= candidate.cost)
        return;

    if constexpr (AP::stores_annotations)
    {
        auto witness = materialize_witness(instance, candidate);
        if constexpr (AP::records_propositional_achievers)
        {
            if (state.pending_cost)
            {
                assert(state.pending_witness_index < pending_witnesses.size());
                pending_witnesses[state.pending_witness_index] = witness;
            }
            else
            {
                state.pending_witness_index = pending_witnesses.size();
                pending_witnesses.push_back(witness);
            }
        }

        const auto update = out.annotation_policy().publish_annotation(candidate.head, std::move(witness), out.annotations());
        if (out.fact_sets().predicate.contains(candidate.head))
        {
            if (is_annotation_improvement(update))
                notify_fact_annotation_improved(ctx, candidate.head);
        }
        else if (update)
        {
            pending_heads.update(*update, candidate.head);
        }
        else if (const auto* annotation = out.annotations().find(candidate.head))
        {
            pending_heads.insert(get_cost(*annotation), candidate.head);
        }
    }
    else
    {
        if (!out.fact_sets().predicate.contains(candidate.head))
            pending_heads.insert(candidate.cost, candidate.head);
    }

    state.pending_cost = candidate.cost;
    pending_finalizations.push_back({ candidate.cost, rule });
    std::push_heap(pending_finalizations.begin(), pending_finalizations.end(), PendingPredicateFinalizationGreater {});
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void fire_rule(GroundCtx<AP, TP, CP>& ctx,
               fd::GroundRuleView<f::FunctionTag>,
               RuleInstance<GroundTag, f::FunctionTag>& instance,
               const FunctionCandidate& candidate,
               CostBuckets& pending_heads)
{
    ++ctx.out().statistics().num_rules_fired;
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

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
bool is_current_finalization(const GroundCtx<AP, TP, CP>& ctx, const PendingPredicateFinalization& entry)
{
    const auto& state = ctx.out().template rule_states<f::PredicateTag>()[ygg::uint_t(entry.rule.get_index())];
    return !state.fired && state.pending_cost && *state.pending_cost == entry.cost;
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void discard_stale_finalizations(GroundCtx<AP, TP, CP>& ctx, PendingPredicateFinalizations& pending)
{
    const auto greater = PendingPredicateFinalizationGreater {};
    while (!pending.empty() && !is_current_finalization(ctx, pending.front()))
    {
        std::pop_heap(pending.begin(), pending.end(), greater);
        pending.pop_back();
    }
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
Cost next_finalization_cost(GroundCtx<AP, TP, CP>& ctx, PendingPredicateFinalizations& pending)
{
    discard_stale_finalizations(ctx, pending);
    return pending.empty() ? std::numeric_limits<Cost>::max() : pending.front().cost;
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void finalize_next_rule(GroundCtx<AP, TP, CP>& ctx, PendingPredicateFinalizations& pending, PendingPredicateWitnesses& pending_witnesses)
{
    discard_stale_finalizations(ctx, pending);
    assert(!pending.empty());

    const auto greater = PendingPredicateFinalizationGreater {};
    std::pop_heap(pending.begin(), pending.end(), greater);
    auto entry = std::move(pending.back());
    pending.pop_back();

    auto& out = ctx.out();
    auto& state = out.template rule_states<f::PredicateTag>()[ygg::uint_t(entry.rule.get_index())];
    assert(state.pending_cost && *state.pending_cost == entry.cost);
    state.pending_cost.reset();
    state.fired = true;
    ++out.statistics().num_rules_fired;
    if constexpr (AP::records_propositional_achievers)
    {
        assert(state.pending_witness_index < pending_witnesses.size());
        out.annotation_policy().record_achiever(entry.rule.get_head().get_row(), std::move(pending_witnesses[state.pending_witness_index]));
    }
    state.pending_witness_index = 0;
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void reset_pending_finalizations(GroundCtx<AP, TP, CP>& ctx, const PendingPredicateFinalizations& pending)
{
    for (const auto& entry : pending)
    {
        auto& state = ctx.out().template rule_states<f::PredicateTag>()[ygg::uint_t(entry.rule.get_index())];
        if (state.pending_cost && *state.pending_cost == entry.cost)
        {
            state.pending_cost.reset();
            state.pending_witness_index = 0;
        }
    }
}

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
Cost next_rule_cost_for(const GroundCtx<AP, TP, CP>& ctx) noexcept
{
    const auto& queue = ctx.out().template queue_storage<R>();
    return queue.empty() ? std::numeric_limits<Cost>::max() : queue.front().cost;
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
Cost next_rule_cost(const GroundCtx<AP, TP, CP>& ctx) noexcept
{
    return std::min(next_rule_cost_for<f::PredicateTag>(ctx), next_rule_cost_for<f::FunctionTag>(ctx));
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void commit_head_bucket(GroundCtx<AP, TP, CP>& ctx, CostBuckets& pending_heads, Cost cost)
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
void process_next_rule(GroundCtx<AP, TP, CP>& ctx,
                       CostBuckets& pending_heads,
                       PendingPredicateFinalizations& pending_finalizations,
                       PendingPredicateWitnesses& pending_witnesses)
{
    auto entry = pop_next_entry<R>(ctx);
    assert(entry);

    auto& out = ctx.out();
    const auto rule_index = entry->rule.get_index();
    auto& state = out.template rule_states<R>()[ygg::uint_t(rule_index)];
    auto& queued_cost = state.queued_cost;
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
        ++out.statistics().num_stale_queue_pops;
        if (candidate)
            enqueue_rule(ctx, entry->rule, candidate->queue_label);
        return;
    }

    if constexpr (std::same_as<R, f::PredicateTag>)
        stage_rule(ctx, entry->rule, instance, *candidate, pending_heads, pending_finalizations, pending_witnesses);
    else
        fire_rule(ctx, entry->rule, instance, *candidate, pending_heads);
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void process_rule_frontier(GroundCtx<AP, TP, CP>& ctx,
                           CostBuckets& pending_heads,
                           PendingPredicateFinalizations& pending_finalizations,
                           PendingPredicateWitnesses& pending_witnesses,
                           Cost cost)
{
    while (next_rule_cost(ctx) == cost)
    {
        const auto predicate_cost = next_rule_cost_for<f::PredicateTag>(ctx);
        const auto function_cost = next_rule_cost_for<f::FunctionTag>(ctx);
        if (predicate_cost <= function_cost)
            process_next_rule<f::PredicateTag>(ctx, pending_heads, pending_finalizations, pending_witnesses);
        else
            process_next_rule<f::FunctionTag>(ctx, pending_heads, pending_finalizations, pending_witnesses);
    }
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void process_rule_wave(GroundCtx<AP, TP, CP>& ctx,
                       CostBuckets& pending_heads,
                       PendingPredicateFinalizations& pending_finalizations,
                       PendingPredicateWitnesses& pending_witnesses)
{
    while (next_rule_cost(ctx) != std::numeric_limits<Cost>::max())
        process_rule_frontier(ctx, pending_heads, pending_finalizations, pending_witnesses, next_rule_cost(ctx));
}

}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void compute_model(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    initialize_numeric_constraint_satisfaction(ctx);
    seed_queue(ctx);

    auto pending_heads = CostBuckets {};
    auto pending_finalizations = PendingPredicateFinalizations {};
    auto pending_witnesses = PendingPredicateWitnesses {};
    while (true)
    {
        if (ctx.out().tp().should_terminate(FactSets { ctx.in().facts().fact_sets, ctx.out().facts().fact_sets }))
        {
            reset_pending_finalizations(ctx, pending_finalizations);
            return;
        }

        if (next_rule_cost(ctx) != std::numeric_limits<Cost>::max())
        {
            // Every candidate in a ready wave observes the same committed fact snapshot.
            process_rule_wave(ctx, pending_heads, pending_finalizations, pending_witnesses);
            continue;
        }

        const auto head_cost = pending_heads.min_cost();
        const auto finalization_cost = next_finalization_cost(ctx, pending_finalizations);
        if (head_cost == std::numeric_limits<Cost>::max() && finalization_cost == std::numeric_limits<Cost>::max())
            return;

        if (head_cost <= finalization_cost)
        {
            commit_head_bucket(ctx, pending_heads, head_cost);
            while (next_finalization_cost(ctx, pending_finalizations) <= head_cost)
                finalize_next_rule(ctx, pending_finalizations, pending_witnesses);
        }
        else
        {
            while (next_finalization_cost(ctx, pending_finalizations) == finalization_cost)
                finalize_next_rule(ctx, pending_finalizations, pending_witnesses);
        }
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
