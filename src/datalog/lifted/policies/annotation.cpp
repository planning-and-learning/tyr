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

#include "tyr/datalog/lifted/policies/annotation.hpp"

#include "tyr/datalog/lifted/policies/numeric_support.hpp"
#include "tyr/datalog/policies/aggregation.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/formalism/binding_index.hpp"
#include "tyr/formalism/datalog/builder.hpp"
#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/ground_atom_index.hpp"
#include "tyr/formalism/datalog/ground_conjunctive_condition_index.hpp"
#include "tyr/formalism/datalog/grounder.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_index.hpp"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <fmt/format.h>
#include <limits>
#include <optional>
#include <tuple>
#include <variant>
#include <vector>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/semantics/comparison.hpp>

namespace tyr::datalog
{
/**
 * OrAnnotationPolicy
 */

void OrAnnotationPolicy<LiftedTag>::initialize_annotation(::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> head,
                                                          SelectedPredicateAnnotations<LiftedTag>& program_and_annot) const
{
    program_and_annot.insert_or_assign(head, BaseAnnotation<LiftedTag>(Cost(0)));
}

void OrAnnotationPolicy<LiftedTag>::initialize_annotation(::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> head,
                                                          ygg::ClosedInterval<ygg::float_t> interval,
                                                          SelectedFunctionAnnotations<LiftedTag>& program_numeric_and_annot) const
{
    program_numeric_and_annot.insert(head, interval, BaseAnnotation<LiftedTag>(Cost(0)));
}

CostUpdate<LiftedTag>
OrAnnotationPolicy<LiftedTag>::update_annotation(::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> head,
                                                 const DeltaPredicateAnnotations<LiftedTag>& delta_and_annot,
                                                 SelectedPredicateAnnotations<LiftedTag>& program_and_annot) const
{
    const auto* old_annotation = program_and_annot.find(head);
    const auto old_cost = old_annotation ? get_cost(*old_annotation) : std::numeric_limits<Cost>::max();
    if (old_cost == Cost(0))
        return CostUpdate<LiftedTag>(old_cost, old_cost);

    const auto* delta_annotation = delta_and_annot.find(head);
    const auto* witness = delta_annotation ? std::get_if<WitnessAnnotation<LiftedTag>>(delta_annotation) : nullptr;
    if (!witness)
        return CostUpdate<LiftedTag>(old_cost, old_cost);

    const auto new_cost = witness->get_cost();
    if (new_cost < old_cost)
    {
        program_and_annot.insert_or_assign(head, *delta_annotation);
        return CostUpdate<LiftedTag>(old_cost, new_cost);
    }
    return CostUpdate<LiftedTag>(old_cost, old_cost);  ///< First witness of a cost wins, see try_ground_better_witness.
}

/**
 * AndAnnotationPolicy
 */

namespace
{
template<typename AggregationFunction, ::tyr::formalism::RelationKind R, typename CanWin>
std::optional<WitnessAnnotation<LiftedTag, R>> try_ground_witness(const AndAnnotationContext<LiftedTag, R>& context, CanWin&& can_win)
{
    auto body_metric = ygg::ClosedInterval<ygg::float_t>();
    auto body_cost = AggregationFunction::identity();
    const auto lower_bound = [&] { return std::max(body_cost, context.current_cost) + context.metric_effect_cost; };
    if (!can_win(lower_bound()))
        return std::nullopt;

    auto& numeric_supports = context.witness_support_scratch;
    numeric_supports.clear();

    for (const auto literal : context.witness_condition.template get_literals<::tyr::formalism::FluentTag>())
    {
        assert(literal.get_polarity());

        const auto [program_binding, inserted] = ::tyr::formalism::datalog::ground_binding(literal.get_atom(), context.ground_context);
        assert(!inserted);  ///< must exist in program because the precondition is applicable in program fact set.

        const auto* annotation = context.program_and_annot.find(program_binding);
        assert(annotation && "applicable lifted rule has a positive fluent body atom without an annotation");
        const auto program_binding_cost = get_cost(*annotation);
        assert(program_binding_cost != std::numeric_limits<Cost>::max());

        body_cost = AggregationFunction()(body_cost, program_binding_cost);
        if (!can_win(lower_bound()))
            return std::nullopt;
        body_metric = aggregate_metric_support(body_metric, get_metric(*annotation));
    }

    for (const auto numeric_constraint : context.witness_condition.get_numeric_constraints())
    {
        const auto ground_constraint = ::tyr::formalism::datalog::ground(numeric_constraint, context.ground_context);
        const auto constraint_cost =
            context.numeric_support_selector.get_constraint_cost(ground_constraint, context.numeric_support_selector_workspace, AggregationFunction {});

        if (constraint_cost == std::numeric_limits<Cost>::max())
            return std::nullopt;

        body_cost = AggregationFunction()(body_cost, constraint_cost);
        if (!can_win(lower_bound()))
            return std::nullopt;

        for (const auto& entry : context.numeric_support_selector_workspace.selection)
        {
            if (entry.annotation)
                body_metric = aggregate_metric_support(body_metric, get_metric(entry.annotation->annotation));
            numeric_supports.push_back(NumericSupport<LiftedTag> { entry.key, entry.interval, entry.cost });
        }
    }

    body_cost = std::max(body_cost, context.current_cost);

    if (context.metric_effect_cost != Cost(0))
        body_metric = empty(body_metric) ?
                          ygg::ClosedInterval<ygg::float_t>(context.metric_effect_cost, context.metric_effect_cost) :
                          ygg::ClosedInterval<ygg::float_t>(lower(body_metric) + context.metric_effect_cost, upper(body_metric) + context.metric_effect_cost);

    numeric_supports.insert(numeric_supports.end(), context.numeric_supports.begin(), context.numeric_supports.end());
    const auto rule_binding =
        context.rule_binding ? *context.rule_binding : ::tyr::formalism::datalog::ground_binding(context.rule, context.ground_context).first;
    return WitnessAnnotation<LiftedTag, R>(rule_binding,
                                           body_metric,
                                           body_cost + context.metric_effect_cost,
                                           std::span<const NumericSupport<LiftedTag>>(numeric_supports));
}

template<typename AggregationFunction, ::tyr::formalism::RelationKind R>
std::optional<WitnessAnnotation<LiftedTag, R>> try_ground_better_witness(Cost best_cost, const AndAnnotationContext<LiftedTag, R>& context)
{
    return try_ground_witness<AggregationFunction>(context, [best_cost](Cost lower_bound) { return lower_bound < best_cost; });
}

}

template<typename AggregationFunction>
void AndAnnotationPolicy<LiftedTag, AggregationFunction>::update_annotation(
    ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> head,
    const AndAnnotationContext<LiftedTag, ::tyr::formalism::PredicateTag>& context,
    DeltaPredicateAnnotations<LiftedTag>& delta_and_annot) const
{
    // Use min among global minimum in cost of last iteration and thread local minimum.
    const auto best_global_cost = fetch_annotation_cost<LiftedTag>(head, context.program_and_annot);
    const auto best_local_cost = fetch_annotation_cost<LiftedTag>(head, delta_and_annot);
    const auto best_cost = std::min(best_global_cost, best_local_cost);
    const auto cur_cost_lower_bound = context.current_cost + context.metric_effect_cost;

    /// Only strict improvements update the annotation, so the first witness of a given cost wins.
    /// Which witness that is depends on the clique enumeration order, hence changing that order (or the
    /// rule schedule) changes the recorded witness and moves the search-statistics fixtures. That is
    /// expected, not a defect. Determinism at one worker rests on run_active_rules pinning the sorted
    /// rule order plus the sorted consumers (get_sorted_rows/updates/pending_rule_bindings). Under
    /// inner parallelism it would need deterministic key ownership per stripe, not a tie-break.
    if (best_cost <= cur_cost_lower_bound)
        return;  ///< No local or global improvement

    auto witness = try_ground_better_witness<AggregationFunction>(best_cost, context);
    if (!witness)
        return;  ///< No local or global improvement

    /// Update improved witness and cost annotation
    delta_and_annot.insert_or_assign(head, Annotation<LiftedTag>(std::move(*witness)));
}

template<typename AggregationFunction>
void AndAnnotationPolicy<LiftedTag, AggregationFunction>::update_annotation(
    ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> head,
    ygg::ClosedInterval<ygg::float_t> interval,
    const AndAnnotationContext<LiftedTag, ::tyr::formalism::FunctionTag>& context,
    DeltaFunctionAnnotations<LiftedTag>& delta_numeric_and_annot) const
{
    const auto best_cost = std::numeric_limits<Cost>::max();
    const auto cur_cost_lower_bound = context.current_cost + context.metric_effect_cost;

    if (best_cost <= cur_cost_lower_bound)
        return;

    auto witness = try_ground_better_witness<AggregationFunction>(best_cost, context);
    if (!witness)
        return;

    delta_numeric_and_annot.insert(head, interval, Annotation<LiftedTag, ::tyr::formalism::FunctionTag>(std::move(*witness)));
}

/**
 * AchieverAndAnnotationPolicy
 */

template<typename AggregationFunction>
void AchieverAndAnnotationPolicy<LiftedTag, AggregationFunction>::clear_achievers() noexcept
{
    m_achievers.clear();
}

template<typename AggregationFunction>
const typename AchieverAndAnnotationPolicy<LiftedTag, AggregationFunction>::Achievers*
AchieverAndAnnotationPolicy<LiftedTag, AggregationFunction>::find_achievers(PredicateBinding head) const noexcept
{
    const auto it = m_achievers.find(head.get_index());
    return it == m_achievers.end() ? nullptr : &it->second;
}

template<typename AggregationFunction>
void AchieverAndAnnotationPolicy<LiftedTag, AggregationFunction>::record_achiever(
    PredicateBinding head,
    const AndAnnotationContext<LiftedTag, ::tyr::formalism::PredicateTag>& context) const
{
    auto witness = try_ground_witness<AggregationFunction>(context, [](Cost) { return true; });
    if (witness)
    {
        auto& achievers = m_achievers[head.get_index()];
        if (std::find(achievers.begin(), achievers.end(), *witness) == achievers.end())
            achievers.push_back(std::move(*witness));
    }
}

template class AndAnnotationPolicy<LiftedTag, SumAggregation>;
template class AndAnnotationPolicy<LiftedTag, MaxAggregation>;
template class AchieverAndAnnotationPolicy<LiftedTag, MaxAggregation>;

}
