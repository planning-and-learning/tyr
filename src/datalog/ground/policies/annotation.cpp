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

#include "tyr/datalog/ground/policies/annotation.hpp"

#include "tyr/datalog/numeric_utils.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/policies/annotation_concept.hpp"

#include <algorithm>
#include <cassert>
#include <limits>
#include <optional>
#include <span>
#include <utility>
#include <variant>
#include <yggdrasil/containers/variant.hpp>

namespace tyr::datalog
{

template<typename AggregationFunction>
void MinCostAnnotationPolicy<GroundTag, AggregationFunction>::initialize_annotation(
    ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> head,
    PredicateAnnotations<GroundTag>& annotations) const
{
    annotations.insert_or_assign(head, BaseAnnotation<GroundTag>(Cost(0)));
}

template<typename AggregationFunction>
void MinCostAnnotationPolicy<GroundTag, AggregationFunction>::initialize_annotation(
    ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> head,
    ygg::ClosedInterval<ygg::float_t> interval,
    FunctionAnnotations<GroundTag>& numeric_annotations) const
{
    numeric_annotations.insert(head.get_relation().get_index(), head.get_index().row, interval, BaseAnnotation<GroundTag>(Cost(0)));
}

template<typename AggregationFunction>
CostUpdate<GroundTag>
MinCostAnnotationPolicy<GroundTag, AggregationFunction>::commit_annotation(::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> head,
                                                                           const DeltaPredicateAnnotations<GroundTag>& delta_annotations,
                                                                           PredicateAnnotations<GroundTag>& annotations) const
{
    const auto* delta_annotation = delta_annotations.find(head);
    if (!delta_annotation)
        return {};

    const auto new_cost = get_cost(*delta_annotation);
    if (const auto* old_annotation = annotations.find(head))
    {
        const auto old_cost = get_cost(*old_annotation);
        if (new_cost < old_cost)
        {
            annotations.insert_or_assign(head, *delta_annotation);
            return CostUpdate<GroundTag>(old_cost, new_cost);
        }
        if (new_cost == old_cost)
            if (const auto* witness = std::get_if<WitnessAnnotation<GroundTag>>(delta_annotation);
                witness && witness_wins_tie<GroundTag>(*witness, old_annotation))
                annotations.insert_or_assign(head, *delta_annotation);
        return CostUpdate<GroundTag>(old_cost, old_cost);
    }

    annotations.insert_or_assign(head, *delta_annotation);
    return CostUpdate<GroundTag>(std::nullopt, new_cost);
}

namespace
{
namespace f = ::tyr::formalism;
namespace fd = ::tyr::formalism::datalog;

using Metric = ygg::ClosedInterval<ygg::float_t>;

template<typename AggregationFunction, f::RelationKind R, typename EvaluateEffectExpression>
Cost aggregate_rule_support_cost(const GroundAnnotationCostContext<R>& context, EvaluateEffectExpression&& evaluate_effect_expression)
{
    const auto agg = AggregationFunction {};
    auto cost = AggregationFunction::identity();
    for (const auto literal : context.rule.get_body().template get_literals<f::FluentTag>())
    {
        if (!literal.get_polarity())
            continue;

        const auto* annotation = context.annotations.find(literal.get_atom().get_row());
        assert(annotation && "annotated ground rule has a positive fluent body atom without a cost annotation");
        cost = agg(cost, get_cost(*annotation));
    }

    context.support_selection.clear();
    for (const auto numeric_constraint : context.rule.get_body().get_numeric_constraints())
    {
        const auto constraint_cost = context.numeric_support_selector.get_constraint_cost(numeric_constraint, context.auxiliary_selection, agg);
        if (constraint_cost == std::numeric_limits<Cost>::max())
            return std::numeric_limits<Cost>::max();
        context.support_selection.insert(context.support_selection.end(), context.auxiliary_selection.begin(), context.auxiliary_selection.end());
        cost = agg(cost, constraint_cost);
    }

    context.auxiliary_selection.clear();
    if (!evaluate_effect_expression(context.numeric_support_selector, context.auxiliary_selection))
        return std::numeric_limits<Cost>::max();

    context.support_selection.insert(context.support_selection.end(), context.auxiliary_selection.begin(), context.auxiliary_selection.end());
    for (const auto& entry : context.auxiliary_selection)
        cost = agg(cost, entry.cost);
    return cost;
}

Cost evaluate_numeric_effect_support(const GroundAnnotationCostContext<f::FunctionTag>& context, fd::GroundNumericEffectView<f::FluentTag> effect)
{
    context.support_selection.clear();
    for (const auto numeric_constraint : context.rule.get_body().get_numeric_constraints())
    {
        if (context.numeric_support_selector.get_constraint_cost(numeric_constraint, context.auxiliary_selection, MaxAggregation {})
            == std::numeric_limits<Cost>::max())
            return std::numeric_limits<Cost>::max();
        context.support_selection.insert(context.support_selection.end(), context.auxiliary_selection.begin(), context.auxiliary_selection.end());
    }

    context.auxiliary_selection.clear();
    if (effect.get_operator() != f::NumericEffectOperatorKind::Assign
        && empty(context.numeric_support_selector.select_fluent_interval(effect.get_fterm(), context.auxiliary_selection)))
        return std::numeric_limits<Cost>::max();
    if (empty(context.numeric_support_selector.evaluate_effect_expression(effect.get_fexpr(), context.auxiliary_selection)))
        return std::numeric_limits<Cost>::max();
    context.support_selection.insert(context.support_selection.end(), context.auxiliary_selection.begin(), context.auxiliary_selection.end());
    return Cost(0);
}

template<f::RelationKind R>
Cost aggregate_metric_effect_cost(const GroundAnnotationCostContext<R>& context)
{
    context.metric_selection.clear();
    const auto delta = sum_metric_effect_deltas(
        Cost(0),
        context.rule.get_metric_effects(),
        [&](const auto& metric_effect)
        {
            return ygg::visit([&](auto&& effect) { return metric_effect_delta(effect, context.numeric_support_selector, context.metric_selection); },
                              metric_effect.get_variant());
        });
    return delta ? reduce_cost(*delta, context.rule_cost) : std::numeric_limits<Cost>::max();
}

template<typename AggregationFunction>
Cost aggregate_selection_cost(Cost cost, std::span<const GroundNumericSupportSelectorWorkspace::SelectionEntry> selection)
{
    const auto agg = AggregationFunction {};
    for (const auto& entry : selection)
        cost = agg(cost, entry.cost);
    return cost;
}

Cost fetch_annotation_cost(fd::PredicateBindingView<f::FluentTag> head, const DeltaPredicateAnnotations<GroundTag>& annotations)
{
    if (const auto* annotation = annotations.find(head))
        return get_cost(*annotation);
    return std::numeric_limits<Cost>::max();
}

const Annotation<GroundTag>* select_incumbent(fd::PredicateBindingView<f::FluentTag> head,
                                              Cost best_global_cost,
                                              Cost best_local_cost,
                                              const PredicateAnnotations<GroundTag>& annotations,
                                              const DeltaPredicateAnnotations<GroundTag>& delta_annotations)
{
    return best_local_cost <= best_global_cost ? delta_annotations.find(head) : annotations.find(head);
}

template<typename Selection>
void append_numeric_supports(std::vector<NumericSupport<GroundTag>>& supports, const Selection& selection, const GroundNumericSupportSelector& selector)
{
    for (const auto& entry : selection)
    {
        const auto reported = selector.for_each_entry_support(entry,
                                                              [&](auto term, auto interval, const auto& annotation)
                                                              { supports.emplace_back(term, interval, get_cost(annotation)); });

        if (!reported)
            supports.emplace_back(entry.key, entry.interval, entry.cost);
    }
}

template<typename Selection>
Metric aggregate_numeric_selection_metric(Metric metric, const Selection& selection, const GroundNumericSupportSelector& selector)
{
    for (const auto& entry : selection)
        selector.for_each_entry_support(entry, [&](auto, auto, const auto& annotation) { metric = aggregate_metric_support(metric, get_metric(annotation)); });
    return metric;
}

template<typename AggregationFunction, f::RelationKind R>
Metric aggregate_body_metric(const AnnotationContext<GroundTag, R>& context)
{
    auto metric = Metric {};
    for (const auto literal : context.rule.get_body().template get_literals<f::FluentTag>())
    {
        if (!literal.get_polarity())
            continue;

        const auto* annotation = context.annotations.find(literal.get_atom().get_row());
        assert(annotation && "applicable ground rule has a positive fluent body atom without a cost annotation");
        metric = aggregate_metric_support(metric, get_metric(*annotation));
    }

    for (const auto numeric_constraint : context.rule.get_body().get_numeric_constraints())
    {
        if (context.numeric_support_selector.get_constraint_cost(numeric_constraint, context.selection_scratch, AggregationFunction {})
            == std::numeric_limits<Cost>::max())
        {
            assert(false && "applicable ground rule has a numeric constraint without numeric support");
            continue;
        }

        metric = aggregate_numeric_selection_metric(metric, context.selection_scratch, context.numeric_support_selector);
        if constexpr (std::same_as<R, f::PredicateTag>)
            append_numeric_supports(context.witness_support_scratch, context.selection_scratch, context.numeric_support_selector);
    }
    return metric;
}

template<f::RelationKind R>
Cost aggregate_metric_effect_cost(const AnnotationContext<GroundTag, R>& context)
{
    context.selection_scratch.clear();

    const auto delta = sum_metric_effect_deltas(
        Cost(0),
        context.rule.get_metric_effects(),
        [&](const auto& metric_effect)
        {
            return ygg::visit([&](auto&& effect) { return metric_effect_delta(effect, context.numeric_support_selector, context.selection_scratch); },
                              metric_effect.get_variant());
        });
    return delta ? reduce_cost(*delta, context.rule_cost) : std::numeric_limits<Cost>::max();
}

template<typename AggregationFunction, f::RelationKind R>
WitnessAnnotation<GroundTag, R> make_witness(const AnnotationContext<GroundTag, R>& context)
{
    context.witness_support_scratch.clear();
    auto metric = aggregate_body_metric<AggregationFunction>(context);

    if constexpr (std::same_as<R, f::FunctionTag>)
    {
        metric = aggregate_numeric_selection_metric(metric, context.numeric_support_selection, context.numeric_support_selector);
        append_numeric_supports(context.witness_support_scratch, context.numeric_support_selection, context.numeric_support_selector);
    }

    const auto metric_cost = aggregate_metric_effect_cost(context);
    if constexpr (std::same_as<R, f::PredicateTag>)
    {
        if (metric_cost != std::numeric_limits<Cost>::max())
        {
            metric = add_metric_delta(metric, metric_cost);
            metric = aggregate_numeric_selection_metric(metric, context.selection_scratch, context.numeric_support_selector);
            append_numeric_supports(context.witness_support_scratch, context.selection_scratch, context.numeric_support_selector);
        }
    }
    else
    {
        metric = add_metric_delta(metric, metric_cost);
        metric = aggregate_numeric_selection_metric(metric, context.selection_scratch, context.numeric_support_selector);
        append_numeric_supports(context.witness_support_scratch, context.selection_scratch, context.numeric_support_selector);
    }

    return WitnessAnnotation<GroundTag, R>(context.rule,
                                           metric,
                                           context.current_cost,
                                           std::span<const NumericSupport<GroundTag>>(context.witness_support_scratch));
}

}

Cost evaluate_cost(const NoAnnotationPolicy<GroundTag>&, const GroundAnnotationCostContext<::tyr::formalism::PredicateTag>&) noexcept { return Cost(0); }

Cost evaluate_cost(const NoAnnotationPolicy<GroundTag>& policy, const GroundAnnotationCostContext<::tyr::formalism::FunctionTag>& context)
{
    auto best_cost = std::numeric_limits<Cost>::max();
    ygg::visit([&](auto&& effect) { best_cost = std::min(best_cost, evaluate_cost(policy, context, effect).total_cost); },
               context.rule.get_head().get_variant());
    return best_cost;
}

GroundNumericEffectCost evaluate_cost(const NoAnnotationPolicy<GroundTag>&,
                                      const GroundAnnotationCostContext<::tyr::formalism::FunctionTag>& context,
                                      ::tyr::formalism::datalog::GroundNumericEffectView<::tyr::formalism::FluentTag> effect)
{
    const auto support_cost = evaluate_numeric_effect_support(context, effect);
    if (support_cost == std::numeric_limits<Cost>::max())
        return { support_cost, support_cost };

    const auto metric_cost = aggregate_metric_effect_cost(context);
    if (metric_cost == std::numeric_limits<Cost>::max())
        return { metric_cost, metric_cost };

    return { support_cost + metric_cost, metric_cost };
}

template<typename AggregationFunction>
Cost MinCostAnnotationPolicy<GroundTag, AggregationFunction>::evaluate_cost(const GroundAnnotationCostContext<::tyr::formalism::PredicateTag>& context) const
{
    const auto support_cost = aggregate_rule_support_cost<AggregationFunction>(context, [](const auto&, auto&) { return true; });
    if (support_cost == std::numeric_limits<Cost>::max())
        return support_cost;

    const auto metric_cost = aggregate_metric_effect_cost(context);
    if (metric_cost == std::numeric_limits<Cost>::max())
        return metric_cost;

    return aggregate_selection_cost<AggregationFunction>(support_cost, context.metric_selection) + metric_cost;
}

template<typename AggregationFunction>
Cost MinCostAnnotationPolicy<GroundTag, AggregationFunction>::evaluate_cost(const GroundAnnotationCostContext<::tyr::formalism::FunctionTag>& context) const
{
    auto best_cost = std::numeric_limits<Cost>::max();
    ygg::visit([&](auto&& effect) { best_cost = std::min(best_cost, evaluate_cost(context, effect).total_cost); }, context.rule.get_head().get_variant());
    return best_cost;
}

template<typename AggregationFunction>
GroundNumericEffectCost MinCostAnnotationPolicy<GroundTag, AggregationFunction>::evaluate_cost(
    const GroundAnnotationCostContext<::tyr::formalism::FunctionTag>& context,
    ::tyr::formalism::datalog::GroundNumericEffectView<::tyr::formalism::FluentTag> effect) const
{
    const auto support_cost =
        aggregate_rule_support_cost<AggregationFunction>(context,
                                                         [&](const auto& support_selector, auto& selected)
                                                         {
                                                             if (effect.get_operator() != ::tyr::formalism::NumericEffectOperatorKind::Assign
                                                                 && empty(support_selector.select_fluent_interval(effect.get_fterm(), selected)))
                                                                 return false;
                                                             return !empty(support_selector.evaluate_effect_expression(effect.get_fexpr(), selected));
                                                         });
    if (support_cost == std::numeric_limits<Cost>::max())
        return { support_cost, support_cost };

    const auto metric_cost = aggregate_metric_effect_cost(context);
    if (metric_cost == std::numeric_limits<Cost>::max())
        return { metric_cost, metric_cost };

    return { aggregate_selection_cost<AggregationFunction>(support_cost, context.metric_selection) + metric_cost, metric_cost };
}

template<typename AggregationFunction>
bool MinCostAnnotationPolicy<GroundTag, AggregationFunction>::try_update_candidate(
    ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> head,
    const AnnotationContext<GroundTag, ::tyr::formalism::PredicateTag>& context,
    DeltaPredicateAnnotations<GroundTag>& delta_annotations) const
{
    const auto best_global_cost = fetch_annotation_cost<GroundTag>(head, context.annotations);
    const auto best_local_cost = fetch_annotation_cost(head, delta_annotations);
    const auto best_cost = std::min(best_global_cost, best_local_cost);
    if (best_cost < context.current_cost)
        return false;

    auto witness = make_witness<AggregationFunction>(context);
    if (best_cost == context.current_cost
        && !witness_wins_tie<GroundTag>(witness, select_incumbent(head, best_global_cost, best_local_cost, context.annotations, delta_annotations)))
        return false;

    delta_annotations.insert_or_assign(head, Annotation<GroundTag>(std::move(witness)));
    return true;
}

template<typename AggregationFunction>
bool MinCostAnnotationPolicy<GroundTag, AggregationFunction>::try_update_candidate(
    ::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::FluentTag> head,
    ygg::ClosedInterval<ygg::float_t> interval,
    const AnnotationContext<GroundTag, ::tyr::formalism::FunctionTag>& context,
    DeltaFunctionAnnotations<GroundTag>& delta_numeric_annotations) const
{
    if (empty(interval))
        return false;

    delta_numeric_annotations.insert(head, interval, make_witness<AggregationFunction>(context));
    return true;
}

template<typename AggregationFunction>
void MinCostAnnotationWithAchieversPolicy<GroundTag, AggregationFunction>::clear_achievers() noexcept
{
    m_achievers.clear();
}

template<typename AggregationFunction>
const typename MinCostAnnotationWithAchieversPolicy<GroundTag, AggregationFunction>::Achievers*
MinCostAnnotationWithAchieversPolicy<GroundTag, AggregationFunction>::find_achievers(PredicateHead head) const noexcept
{
    return m_achievers.find(head);
}

template<typename AggregationFunction>
void MinCostAnnotationWithAchieversPolicy<GroundTag, AggregationFunction>::record_achiever(
    PredicateHead head,
    const AnnotationContext<GroundTag, ::tyr::formalism::PredicateTag>& context)
{
    m_achievers.update(head,
                       [&](auto& achievers, bool initialized)
                       {
                           if (!initialized)
                               achievers.clear();
                           achievers.push_back(make_witness<AggregationFunction>(context));
                       });
}

static_assert(AnnotationPolicyConcept<NoAnnotationPolicy<GroundTag>, GroundTag>);
static_assert(AnnotationPolicyConcept<MinCostAnnotationPolicy<GroundTag, SumAggregation>, GroundTag>);
static_assert(AnnotationPolicyConcept<MinCostAnnotationPolicy<GroundTag, MaxAggregation>, GroundTag>);
static_assert(AnnotationPolicyConcept<MinCostAnnotationWithAchieversPolicy<GroundTag, MaxAggregation>, GroundTag>);

template class MinCostAnnotationPolicy<GroundTag, SumAggregation>;
template class MinCostAnnotationPolicy<GroundTag, MaxAggregation>;
template class MinCostAnnotationWithAchieversPolicy<GroundTag, MaxAggregation>;

}
