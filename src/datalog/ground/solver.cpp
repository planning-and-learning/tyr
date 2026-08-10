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

#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/ground/policies/numeric_support.hpp"
#include "tyr/datalog/numeric_utils.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <utility>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/containers/variant.hpp>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{
namespace
{
namespace f = ::tyr::formalism;
namespace fd = ::tyr::formalism::datalog;

template<typename Index>
size_t position(Index index) noexcept
{
    return static_cast<size_t>(index.get_value());
}

template<typename T, typename Index>
decltype(auto) at(std::vector<T>& vector, Index index) noexcept
{
    return vector[position(index)];
}

template<typename T, typename Index>
decltype(auto) at(const std::vector<T>& vector, Index index) noexcept
{
    return vector[position(index)];
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
using GroundCtx = ProgramExecutionContext<GroundTag, AP, TP, CP>;

struct PendingNumericBuckets
{
    using Term = fd::GroundFunctionTermView<f::FluentTag>;
    using Interval = ygg::ClosedInterval<ygg::float_t>;
    using Bucket = ygg::UnorderedMap<Term, Interval>;

    bool is_empty() const noexcept { return buckets.empty(); }

    Cost min_cost() const noexcept { return buckets.empty() ? std::numeric_limits<Cost>::max() : buckets.begin()->first; }

    bool insert(Cost cost, Term term, Interval interval)
    {
        if (empty(interval))
            return false;

        auto& bucket = buckets[cost];
        const auto it = bucket.find(term);
        if (it == bucket.end())
        {
            bucket.emplace(term, interval);
            return true;
        }

        if (subset(interval, it->second))
            return false;

        it->second = hull(it->second, interval);
        return true;
    }

    Bucket take(Cost cost)
    {
        auto it = buckets.find(cost);
        if (it == buckets.end())
            return Bucket {};

        auto bucket = std::move(it->second);
        buckets.erase(it);
        return bucket;
    }

    std::map<Cost, Bucket> buckets;
};

template<typename Facts>
ygg::ClosedInterval<ygg::float_t> find_interval(const Facts& facts, fd::GroundFunctionTermView<f::FluentTag> term) noexcept
{
    return facts.fact_sets.function[term];
}

using NumericSelectionEntry = GroundNumericSupportSelectorWorkspace::SelectionEntry;

template<TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
GroundNumericSupportSelector make_numeric_support_selector(const GroundCtx<NoAnnotationPolicy<GroundTag>, TP, CP>& ctx)
{
    return GroundNumericSupportSelector(ctx.in().facts(), ctx.out().facts(), ctx.out().numeric_annotations(), true);
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
    requires requires { typename AP::Aggregation; }
GroundNumericSupportSelector make_numeric_support_selector(const GroundCtx<AP, TP, CP>& ctx)
{
    return GroundNumericSupportSelector(ctx.in().facts(), ctx.out().facts(), ctx.out().numeric_annotations());
}

Cost aggregate_selection_cost(Cost cost, const std::vector<NumericSelectionEntry>&, const NoAnnotationPolicy<GroundTag>&) noexcept { return cost; }

template<AnnotationPolicyConcept<GroundTag> AP>
    requires requires { typename AP::Aggregation; }
Cost aggregate_selection_cost(Cost cost, const std::vector<NumericSelectionEntry>& selection, const AP&)
{
    const auto agg = typename AP::Aggregation {};
    for (const auto& entry : selection)
        cost = agg(cost, entry.cost);
    return cost;
}

std::optional<Cost> metric_effect_delta(fd::GroundNumericEffectView<f::FluentTag> effect,
                                        const GroundNumericSupportSelector& selector,
                                        std::vector<NumericSelectionEntry>& selection)
{
    return tyr::datalog::metric_effect_delta(
        effect.get_operator(),
        [&] { return selector.select_fluent_interval(effect.get_fterm(), selection); },
        [&] { return selector.evaluate_effect_expression(effect.get_fexpr(), selection); });
}

template<f::RelationKind R, AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
Cost aggregate_metric_effect_cost(fd::GroundRuleView<R> rule, const GroundCtx<AP, TP, CP>& ctx, std::vector<NumericSelectionEntry>& selection)
{
    auto selector = make_numeric_support_selector(ctx);
    selection.clear();

    auto delta = Cost(0);
    for (const auto& metric_effect : rule.get_metric_effects())
    {
        const auto effect_delta = ygg::visit([&](auto&& effect) { return metric_effect_delta(effect, selector, selection); }, metric_effect.get_variant());
        if (!effect_delta)
            return std::numeric_limits<Cost>::max();
        delta += *effect_delta;
    }

    return reduce_cost(delta, ctx.out().cost_policy().get_cost(rule));
}

template<f::RelationKind R, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP, typename EvaluateEffectExpression>
Cost aggregate_numeric_effect_support_cost(fd::GroundRuleView<R> rule,
                                           const GroundCtx<NoAnnotationPolicy<GroundTag>, TP, CP>& ctx,
                                           std::vector<NumericSelectionEntry>& selection,
                                           std::vector<NumericSelectionEntry>& temporary_selection,
                                           EvaluateEffectExpression&& evaluate_effect_expression)
{
    auto selector = make_numeric_support_selector(ctx);
    selection.clear();
    for (const auto numeric_constraint : rule.get_body().get_numeric_constraints())
    {
        if (selector.get_constraint_cost(numeric_constraint, temporary_selection, MaxAggregation {}) == std::numeric_limits<Cost>::max())
            return std::numeric_limits<Cost>::max();
        selection.insert(selection.end(), temporary_selection.begin(), temporary_selection.end());
    }

    temporary_selection.clear();
    if (!evaluate_effect_expression(selector, temporary_selection))
        return std::numeric_limits<Cost>::max();
    selection.insert(selection.end(), temporary_selection.begin(), temporary_selection.end());
    return Cost(0);
}

template<AnnotationPolicyConcept<GroundTag> AP,
         f::RelationKind R,
         TerminationPolicyConcept<GroundTag> TP,
         RuleCostPolicyConcept<GroundTag> CP,
         typename EvaluateEffectExpression>
    requires requires { typename AP::Aggregation; }
Cost aggregate_numeric_effect_support_cost(fd::GroundRuleView<R> rule,
                                           const GroundCtx<AP, TP, CP>& ctx,
                                           std::vector<NumericSelectionEntry>& selection,
                                           std::vector<NumericSelectionEntry>& temporary_selection,
                                           EvaluateEffectExpression&& evaluate_effect_expression)
{
    using Aggregation = typename AP::Aggregation;
    const auto agg = Aggregation {};
    auto cost = Aggregation::identity();
    for (const auto literal : rule.get_body().template get_literals<f::FluentTag>())
    {
        if (!literal.get_polarity())
            continue;
        const auto* annotation = ctx.out().annotations().find(literal.get_atom().get_row());
        assert(annotation && "annotated ground rule has a positive fluent body atom without a cost annotation");
        cost = agg(cost, get_cost(*annotation));
    }

    auto selector = make_numeric_support_selector(ctx);
    selection.clear();
    for (const auto numeric_constraint : rule.get_body().get_numeric_constraints())
    {
        const auto constraint_cost = selector.get_constraint_cost(numeric_constraint, temporary_selection, agg);
        if (constraint_cost == std::numeric_limits<Cost>::max())
            return std::numeric_limits<Cost>::max();
        selection.insert(selection.end(), temporary_selection.begin(), temporary_selection.end());
        cost = agg(cost, constraint_cost);
    }

    temporary_selection.clear();
    if (!evaluate_effect_expression(selector, temporary_selection))
        return std::numeric_limits<Cost>::max();

    selection.insert(selection.end(), temporary_selection.begin(), temporary_selection.end());
    for (const auto& entry : temporary_selection)
        cost = agg(cost, entry.cost);
    return cost;
}

template<AnnotationPolicyConcept<GroundTag> AP, f::RelationKind R, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
Cost aggregate_body_cost(fd::GroundRuleView<R> rule,
                         const GroundCtx<AP, TP, CP>& ctx,
                         std::vector<NumericSelectionEntry>& selection,
                         std::vector<NumericSelectionEntry>& temporary_selection)
{
    return aggregate_numeric_effect_support_cost(rule, ctx, selection, temporary_selection, [](const auto&, auto&) { return true; });
}

template<f::RelationKind R, AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
Cost aggregate_numeric_effect_rule_cost(fd::GroundRuleView<R> rule,
                                        fd::GroundNumericEffectView<f::FluentTag> effect,
                                        const GroundCtx<AP, TP, CP>& ctx,
                                        std::vector<NumericSelectionEntry>& selection,
                                        std::vector<NumericSelectionEntry>& temporary_selection,
                                        std::vector<NumericSelectionEntry>& metric_selection)
{
    const auto support_cost = aggregate_numeric_effect_support_cost(rule,
                                                                    ctx,
                                                                    selection,
                                                                    temporary_selection,
                                                                    [&](const auto& selector, auto& selected)
                                                                    {
                                                                        if (effect.get_operator() != f::NumericEffectOperatorKind::Assign)
                                                                            if (empty(selector.select_fluent_interval(effect.get_fterm(), selected)))
                                                                                return false;
                                                                        if (empty(selector.evaluate_effect_expression(effect.get_fexpr(), selected)))
                                                                            return false;
                                                                        return true;
                                                                    });

    if (support_cost == std::numeric_limits<Cost>::max())
        return support_cost;

    const auto metric_cost = aggregate_metric_effect_cost(rule, ctx, metric_selection);
    if (metric_cost == std::numeric_limits<Cost>::max())
        return metric_cost;

    return aggregate_selection_cost(support_cost, metric_selection, ctx.out().annotation_policy()) + metric_cost;
}

template<TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
Cost aggregate_rule_cost(fd::GroundRuleView<f::PredicateTag> rule, GroundCtx<NoAnnotationPolicy<GroundTag>, TP, CP>& ctx)
{
    auto selector = make_numeric_support_selector(ctx);
    for (const auto numeric_constraint : rule.get_body().get_numeric_constraints())
        if (selector.get_constraint_cost(numeric_constraint, MaxAggregation {}) == std::numeric_limits<Cost>::max())
            return std::numeric_limits<Cost>::max();
    return Cost(0);
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
    requires requires { typename AP::Aggregation; }
Cost aggregate_rule_cost(fd::GroundRuleView<f::PredicateTag> rule, GroundCtx<AP, TP, CP>& ctx)
{
    auto& scratch = ctx.out().queue().scratch;
    const auto body_cost = aggregate_body_cost(rule, ctx, scratch.support_selection, scratch.auxiliary_selection);
    if (body_cost == std::numeric_limits<Cost>::max())
        return body_cost;
    const auto metric_cost = aggregate_metric_effect_cost(rule, ctx, scratch.metric_selection);
    if (metric_cost == std::numeric_limits<Cost>::max())
        return metric_cost;
    return aggregate_selection_cost(body_cost, scratch.metric_selection, ctx.out().annotation_policy()) + metric_cost;
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
Cost aggregate_rule_cost(fd::GroundRuleView<f::FunctionTag> rule, GroundCtx<AP, TP, CP>& ctx)
{
    auto& scratch = ctx.out().queue().scratch;
    auto best_cost = std::numeric_limits<Cost>::max();
    ygg::visit(
        [&](auto&& effect)
        {
            best_cost = std::min(
                best_cost,
                aggregate_numeric_effect_rule_cost(rule, effect, ctx, scratch.support_selection, scratch.auxiliary_selection, scratch.metric_selection));
        },
        rule.get_head().get_variant());
    return best_cost;
}

template<f::RelationKind R, AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void push_rule(GroundCtx<AP, TP, CP>& ctx, fd::GroundRuleView<R> rule)
{
    auto& out = ctx.out();
    const auto rule_index = rule.get_index();
    auto& states = out.template rule_states<R>();
    if (at(states, rule_index).unsatisfied_count != 0 || at(states, rule_index).fired)
        return;

    const auto cost = aggregate_rule_cost(rule, ctx);
    if (cost == std::numeric_limits<Cost>::max())
        return;

    auto& queued_cost = at(states, rule_index).queued_cost;
    if (queued_cost && *queued_cost <= cost)
        return;
    queued_cost = cost;

    auto& queue = out.template queue_storage<R>();
    queue.push_back(GroundQueueEntry<R> { cost, rule });
    std::push_heap(queue.begin(), queue.end(), std::greater<> {});

    ++out.statistics().num_queue_pushes;
    out.statistics().max_queue_size =
        std::max(out.statistics().max_queue_size,
                 static_cast<ygg::uint_t>(out.template queue_storage<f::PredicateTag>().size() + out.template queue_storage<f::FunctionTag>().size()));
}

template<f::RelationKind R, AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void update_numeric_constraint_satisfaction(GroundCtx<AP, TP, CP>& ctx, fd::GroundRuleView<R> rule)
{
    auto& out = ctx.out();
    const auto rule_index = rule.get_index();
    auto& states = out.template rule_states<R>();
    auto& satisfied = at(states, rule_index).numeric_constraint_satisfied;
    const auto numeric_constraints = rule.get_body().get_numeric_constraints();
    if (satisfied.size() != numeric_constraints.size())
        satisfied.assign(numeric_constraints.size(), false);

    auto selector = make_numeric_support_selector(ctx);
    for (ygg::uint_t i = 0; i < numeric_constraints.size(); ++i)
    {
        if (satisfied[i])
            continue;

        if (selector.get_constraint_cost(numeric_constraints[i], MaxAggregation {}) == std::numeric_limits<Cost>::max())
            continue;

        satisfied[i] = true;
        auto& unsatisfied_count = at(states, rule_index).unsatisfied_count;
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
    const auto& state = at(out.template rule_states<R>(), entry.rule.get_index());
    return state.fired || state.unsatisfied_count != 0;
}

template<f::RelationKind R, AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void notify_fact_inserted_for(GroundCtx<AP, TP, CP>& ctx, fd::GroundAtomView<f::FluentTag> fact)
{
    auto& out = ctx.out();
    const auto& dependencies = ctx.in().template dependencies<R>().fluent_precondition_to_rules;
    const auto* dependent_rules = dependencies.find(fact.get_row());
    if (!dependent_rules)
        return;

    for (const auto dependent_rule : *dependent_rules)
    {
        auto& unsatisfied_count = at(out.template rule_states<R>(), dependent_rule.get_index()).unsatisfied_count;
        if (unsatisfied_count == 0)
            continue;

        --unsatisfied_count;
        push_rule(ctx, dependent_rule);
    }
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void notify_fact_inserted(GroundCtx<AP, TP, CP>& ctx, fd::GroundAtomView<f::FluentTag> fact)
{
    notify_fact_inserted_for<f::PredicateTag>(ctx, fact);
    notify_fact_inserted_for<f::FunctionTag>(ctx, fact);
}

template<f::RelationKind R, AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void notify_fact_annotation_improved_for(GroundCtx<AP, TP, CP>& ctx, fd::GroundAtomView<f::FluentTag> fact)
{
    auto& out = ctx.out();
    const auto& dependencies = ctx.in().template dependencies<R>().fluent_precondition_to_rules;
    const auto* dependent_rules = dependencies.find(fact.get_row());
    if (!dependent_rules)
        return;

    for (const auto dependent_rule : *dependent_rules)
        if (at(out.template rule_states<R>(), dependent_rule.get_index()).unsatisfied_count == 0)
            push_rule(ctx, dependent_rule);
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void notify_fact_annotation_improved(GroundCtx<AP, TP, CP>& ctx, fd::GroundAtomView<f::FluentTag> fact)
{
    notify_fact_annotation_improved_for<f::PredicateTag>(ctx, fact);
    notify_fact_annotation_improved_for<f::FunctionTag>(ctx, fact);
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
bool derive_fact(GroundCtx<AP, TP, CP>& ctx, fd::GroundAtomView<f::FluentTag> fact)
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
void notify_numeric_interval_changed_for(GroundCtx<AP, TP, CP>& ctx, fd::GroundFunctionTermView<f::FluentTag> term)
{
    const auto& dependencies = ctx.in().template dependencies<R>().fluent_function_term_to_rules;
    const auto* dependent_rules = dependencies.find(term.get_row());
    if (!dependent_rules)
        return;

    for (const auto dependent_rule : *dependent_rules)
    {
        update_numeric_constraint_satisfaction(ctx, dependent_rule);
        if (at(ctx.out().template rule_states<R>(), dependent_rule.get_index()).unsatisfied_count == 0)
            push_rule(ctx, dependent_rule);
    }
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void notify_numeric_interval_changed(GroundCtx<AP, TP, CP>& ctx, fd::GroundFunctionTermView<f::FluentTag> term)
{
    notify_numeric_interval_changed_for<f::PredicateTag>(ctx, term);
    notify_numeric_interval_changed_for<f::FunctionTag>(ctx, term);
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
bool derive_interval(GroundCtx<AP, TP, CP>& ctx, fd::GroundFunctionTermView<f::FluentTag> term, ygg::ClosedInterval<ygg::float_t> interval)
{
    if (empty(interval))
        return false;

    return ctx.out().fact_sets().function.insert(term, interval);
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void update_numeric_annotation(GroundCtx<AP, TP, CP>& ctx,
                               fd::GroundRuleView<f::FunctionTag> rule,
                               fd::GroundFunctionTermView<f::FluentTag> term,
                               ygg::ClosedInterval<ygg::float_t> interval,
                               Cost cost,
                               std::span<const NumericSelectionEntry> numeric_support_selection)
{
    auto& out = ctx.out();
    auto& scratch = out.queue().scratch;
    scratch.delta_numeric_annotations.clear();
    scratch.numeric_supports.clear();

    const auto selector = make_numeric_support_selector(ctx);
    const auto context = AnnotationContext<GroundTag, f::FunctionTag> { cost,
                                                                        rule,
                                                                        out.cost_policy().get_cost(rule),
                                                                        selector,
                                                                        scratch.auxiliary_selection,
                                                                        numeric_support_selection,
                                                                        scratch.numeric_supports,
                                                                        out.annotations() };
    if (out.annotation_policy().update_annotation(term, interval, context, scratch.delta_numeric_annotations))
    {
        const auto* annotation = scratch.delta_numeric_annotations.find(term, interval);
        assert(annotation);
        out.numeric_annotations().insert(term, interval, *annotation);
    }
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
bool enqueue_numeric_effect(GroundCtx<AP, TP, CP>& ctx,
                            fd::GroundRuleView<f::FunctionTag> rule,
                            fd::GroundNumericEffectView<f::FluentTag> effect,
                            PendingNumericBuckets& pending_numeric)
{
    auto& scratch = ctx.out().queue().scratch;
    auto selector = make_numeric_support_selector(ctx);
    scratch.evaluation_selection.clear();
    const auto lhs = [&]
    {
        if (effect.get_operator() == f::NumericEffectOperatorKind::Assign)
            return ygg::ClosedInterval<ygg::float_t>();
        else
            return selector.select_fluent_interval(effect.get_fterm(), scratch.evaluation_selection);
    }();
    if (effect.get_operator() != f::NumericEffectOperatorKind::Assign)
        if (empty(lhs))
            return false;

    const auto rhs = selector.evaluate_effect_expression(effect.get_fexpr(), scratch.evaluation_selection);
    if (empty(rhs))
        return false;

    auto interval = apply_numeric_effect(effect.get_operator(), lhs, rhs);
    if (empty(interval))
        return false;

    const auto current = find_interval(ctx.out().facts(), effect.get_fterm());
    if (!empty(current) && subset(interval, current))
        return false;

    const auto generated_cost =
        aggregate_numeric_effect_rule_cost(rule, effect, ctx, scratch.support_selection, scratch.auxiliary_selection, scratch.metric_selection);
    if (generated_cost == std::numeric_limits<Cost>::max())
        return false;

    if (!empty(current))
    {
        const auto rem_rule_cost = aggregate_metric_effect_cost(rule, ctx, scratch.metric_selection);
        if (rem_rule_cost == Cost(0))
            interval = widen_free_growth(interval, current);
    }

    const auto transition_cost = reduce_cost(generated_cost, ctx.out().cost_policy().get_cost(rule, effect.get_fterm(), interval));

    if (!pending_numeric.insert(transition_cost, effect.get_fterm(), interval))
        return false;

    update_numeric_annotation(ctx, rule, effect.get_fterm(), interval, transition_cost, scratch.support_selection);
    return true;
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
std::optional<CostUpdate<GroundTag>>
update_fact_annotation(GroundCtx<AP, TP, CP>& ctx, fd::GroundRuleView<f::PredicateTag> rule, fd::GroundAtomView<f::FluentTag> fact, Cost cost)
{
    auto& out = ctx.out();
    auto& scratch = out.queue().scratch;
    const auto head = fact.get_row();
    scratch.delta_annotations.clear();
    scratch.numeric_supports.clear();

    const auto selector = make_numeric_support_selector(ctx);
    const auto context = AnnotationContext<GroundTag, f::PredicateTag> { cost,
                                                                         rule,
                                                                         out.cost_policy().get_cost(rule),
                                                                         selector,
                                                                         scratch.auxiliary_selection,
                                                                         std::span<const NumericSelectionEntry> {},
                                                                         scratch.numeric_supports,
                                                                         out.annotations() };

    out.annotation_policy().record_achiever(head, context);
    if (out.annotation_policy().update_annotation(head, context, scratch.delta_annotations))
        return out.annotation_policy().update_annotation(head, scratch.delta_annotations, out.annotations());

    return std::nullopt;
}

bool is_annotation_improvement(const std::optional<CostUpdate<GroundTag>>& update) noexcept
{
    return update && (!update->old_cost || update->new_cost < *update->old_cost);
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
bool fire_rule(GroundCtx<AP, TP, CP>& ctx, fd::GroundRuleView<f::PredicateTag> rule, Cost cost, PendingNumericBuckets&)
{
    auto& out = ctx.out();
    at(out.template rule_states<f::PredicateTag>(), rule.get_index()).fired = true;
    ++out.statistics().num_rules_fired;

    const auto head = rule.get_head();
    const auto update = update_fact_annotation(ctx, rule, head, cost);
    const auto inserted = derive_fact(ctx, head);
    if (inserted)
        notify_fact_inserted(ctx, head);
    if (is_annotation_improvement(update))
        notify_fact_annotation_improved(ctx, head);
    return ctx.out().tp().should_terminate(FactSets { ctx.in().facts().fact_sets, ctx.out().facts().fact_sets });
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
bool fire_rule(GroundCtx<AP, TP, CP>& ctx, fd::GroundRuleView<f::FunctionTag> rule, Cost, PendingNumericBuckets& pending_numeric)
{
    ++ctx.out().statistics().num_rules_fired;
    ygg::visit([&](auto&& effect) { enqueue_numeric_effect(ctx, rule, effect, pending_numeric); }, rule.get_head().get_variant());
    return false;
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
bool commit_numeric_bucket(GroundCtx<AP, TP, CP>& ctx, PendingNumericBuckets& pending_numeric, Cost cost)
{
    auto bucket = pending_numeric.take(cost);
    auto& changed_terms = ctx.out().queue().scratch.changed_terms;
    changed_terms.clear();
    changed_terms.reserve(bucket.size());

    for (const auto& [term, interval] : bucket)
        if (derive_interval(ctx, term, interval))
            changed_terms.push_back(term);

    // Keep equal-cost rule notifications independent of unordered bucket iteration.
    if (changed_terms.size() > 1)
        std::sort(changed_terms.begin(), changed_terms.end());
    for (const auto term : changed_terms)
        notify_numeric_interval_changed(ctx, term);

    return !changed_terms.empty() && ctx.out().tp().should_terminate(FactSets { ctx.in().facts().fact_sets, ctx.out().facts().fact_sets });
}

template<f::RelationKind R, AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
bool process_next_rule(GroundCtx<AP, TP, CP>& ctx, PendingNumericBuckets& pending_numeric)
{
    auto entry = pop_next_entry<R>(ctx);
    assert(entry);

    auto& out = ctx.out();
    const auto rule_index = entry->rule.get_index();
    auto& queued_cost = at(out.template rule_states<R>(), rule_index).queued_cost;
    if (!queued_cost || *queued_cost != entry->cost)
    {
        ++out.statistics().num_stale_queue_pops;
        return false;
    }
    queued_cost.reset();

    if (is_stale_entry(ctx, *entry))
    {
        ++out.statistics().num_stale_queue_pops;
        return false;
    }

    return fire_rule(ctx, entry->rule, entry->cost, pending_numeric);
}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
bool process_rule_frontier(GroundCtx<AP, TP, CP>& ctx, PendingNumericBuckets& pending_numeric, Cost cost)
{
    while (next_rule_cost(ctx) == cost)
    {
        const auto& predicates = ctx.out().template queue_storage<f::PredicateTag>();
        const auto& functions = ctx.out().template queue_storage<f::FunctionTag>();
        // Queue entries are ordered by (cost, relation kind, rule index), with predicates before functions.
        const auto process_predicate = functions.empty() || (!predicates.empty() && predicates.front().cost <= functions.front().cost);
        if ((process_predicate && process_next_rule<f::PredicateTag>(ctx, pending_numeric))
            || (!process_predicate && process_next_rule<f::FunctionTag>(ctx, pending_numeric)))
            return true;
    }
    return false;
}

}

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
void compute_model(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    initialize_numeric_constraint_satisfaction(ctx);
    seed_queue(ctx);

    auto pending_numeric = PendingNumericBuckets {};
    while (next_rule_cost(ctx) != std::numeric_limits<Cost>::max() || !pending_numeric.is_empty())
    {
        const auto rule_cost = next_rule_cost(ctx);
        const auto numeric_cost = pending_numeric.min_cost();
        const auto cost = std::min(rule_cost, numeric_cost);

        if (numeric_cost == cost && commit_numeric_bucket(ctx, pending_numeric, cost))
            break;

        if (rule_cost == cost && process_rule_frontier(ctx, pending_numeric, cost))
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
}
