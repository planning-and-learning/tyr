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
    ::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::FluentTag> head,
    ygg::ClosedInterval<ygg::float_t> interval,
    FunctionAnnotations<GroundTag>& numeric_annotations) const
{
    numeric_annotations.insert(head, interval, BaseAnnotation<GroundTag>(Cost(0)));
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
MinCostAnnotationPolicy<GroundTag, AggregationFunction>::update_annotation(::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> head,
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
using SelectionEntry = GroundNumericSupportSelectorWorkspace::SelectionEntry;

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
            continue;

        metric = aggregate_numeric_selection_metric(metric, context.selection_scratch, context.numeric_support_selector);
        if constexpr (std::same_as<R, f::PredicateTag>)
            append_numeric_supports(context.witness_support_scratch, context.selection_scratch, context.numeric_support_selector);
    }
    return metric;
}

std::optional<Cost>
metric_effect_delta(fd::GroundNumericEffectView<f::FluentTag> effect, const GroundNumericSupportSelector& selector, std::vector<SelectionEntry>& selection)
{
    return tyr::datalog::metric_effect_delta(
        effect.get_operator(),
        [&] { return selector.select_fluent_interval(effect.get_fterm(), selection); },
        [&] { return selector.evaluate_effect_expression(effect.get_fexpr(), selection); });
}

template<f::RelationKind R>
Cost aggregate_metric_effect_cost(const AnnotationContext<GroundTag, R>& context)
{
    context.selection_scratch.clear();

    auto delta = Cost(0);
    for (const auto& metric_effect : context.rule.get_metric_effects())
    {
        const auto effect_delta =
            ygg::visit([&](auto&& effect) { return metric_effect_delta(effect, context.numeric_support_selector, context.selection_scratch); },
                       metric_effect.get_variant());
        if (!effect_delta)
            return std::numeric_limits<Cost>::max();
        delta += *effect_delta;
    }
    return reduce_cost(delta, context.rule_cost);
}

Metric add_metric_delta(Metric metric, Cost delta)
{
    if (delta == Cost(0))
        return metric;
    if (empty(metric))
        return Metric(delta, delta);
    return Metric(lower(metric) + delta, upper(metric) + delta);
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

template<typename AggregationFunction>
bool MinCostAnnotationPolicy<GroundTag, AggregationFunction>::update_annotation(
    ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> head,
    const AnnotationContext<GroundTag, ::tyr::formalism::PredicateTag>& context,
    DeltaPredicateAnnotations<GroundTag>& delta_annotations) const
{
    const auto best_global_cost = fetch_annotation_cost<GroundTag>(head, context.annotations);
    const auto best_local_cost = fetch_annotation_cost<GroundTag>(head, delta_annotations);
    const auto best_cost = std::min(best_global_cost, best_local_cost);
    if (best_cost < context.current_cost)
        return false;

    auto witness = make_witness<AggregationFunction>(context);
    if (best_cost == context.current_cost
        && !witness_wins_tie<GroundTag>(witness, select_incumbent<GroundTag>(head, best_global_cost, best_local_cost, context.annotations, delta_annotations)))
        return false;

    delta_annotations.insert_or_assign(head, Annotation<GroundTag>(std::move(witness)));
    return true;
}

template<typename AggregationFunction>
bool MinCostAnnotationPolicy<GroundTag, AggregationFunction>::update_annotation(
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

template class MinCostAnnotationPolicy<GroundTag, SumAggregation>;
template class MinCostAnnotationPolicy<GroundTag, MaxAggregation>;
template class MinCostAnnotationWithAchieversPolicy<GroundTag, MaxAggregation>;

}
