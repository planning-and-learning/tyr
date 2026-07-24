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
namespace
{
template<TaskKind Kind, typename Binding>
std::optional<WitnessAnnotation<Kind>> fetch_witness(Binding binding, const SelectedPredicateAnnotations<Kind>& annotations)
{
    if (const auto* annotation = annotations.find(binding))
        if (const auto* witness = std::get_if<WitnessAnnotation<Kind>>(annotation))
            return *witness;
    return std::nullopt;
}

template<TaskKind Kind>
CostUpdate<Kind> update_min_cost(Cost& cost, Cost candidate)
{
    const auto old_cost = cost;
    if (candidate < old_cost)
        cost = candidate;
    return CostUpdate<Kind>(old_cost, cost);
}

}

/**
 * OrAnnotationPolicy
 */

void OrAnnotationPolicy<LiftedTag>::initialize_annotation(::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> program_head,
                                                          SelectedPredicateAnnotations<LiftedTag>& program_and_annot) const
{
    program_and_annot.insert_or_assign(program_head, BaseAnnotation<LiftedTag>(Cost(0)));
}

void OrAnnotationPolicy<LiftedTag>::initialize_annotation(::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> program_head,
                                                          ygg::ClosedInterval<ygg::float_t> interval,
                                                          SelectedFunctionAnnotations<LiftedTag>& program_numeric_and_annot) const
{
    program_numeric_and_annot.insert(program_head, interval, BaseAnnotation<LiftedTag>(Cost(0)));
}

CostUpdate<LiftedTag>
OrAnnotationPolicy<LiftedTag>::update_annotation(::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> program_head,
                                                 ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> delta_head,
                                                 const SelectedPredicateAnnotations<LiftedTag>& delta_and_annot,
                                                 SelectedPredicateAnnotations<LiftedTag>& program_and_annot) const
{
    // Fast path 1: already optimal
    auto old_cost = fetch_annotation_cost<LiftedTag>(program_head, program_and_annot);
    auto or_cost = old_cost;
    if (or_cost == Cost(0))
        return CostUpdate<LiftedTag>(or_cost, or_cost);

    const auto result = fetch_witness<LiftedTag>(delta_head, delta_and_annot);

    // Fast path 2: no witness available => no update
    if (!result)
        return CostUpdate<LiftedTag>(or_cost, or_cost);

    const auto witness = result.value();

    const auto cost_update = update_min_cost<LiftedTag>(or_cost, witness.get_cost());

    if (or_cost < old_cost)
    {
        program_and_annot.insert_or_assign(program_head, Annotation<LiftedTag>(witness));
    }
    else if (witness.get_cost() == old_cost)
    {
        // Binding identity contains the worker repository; compare logical rule and objects so the
        // winner is independent of worker merge order. A BaseAnnotation incumbent always wins.
        const auto* incumbent = program_and_annot.find(program_head);
        const auto* incumbent_witness = incumbent ? std::get_if<WitnessAnnotation<LiftedTag>>(incumbent) : nullptr;
        if (incumbent_witness && canonical_rule_binding_less(witness.get_rule_key(), incumbent_witness->get_rule_key()))
            program_and_annot.insert_or_assign(program_head, Annotation<LiftedTag>(witness));
    }

    return cost_update;
}

/**
 * AndAnnotationPolicy
 */

namespace
{

Cost fetch_current_best_cost(::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> delta_head,
                             const SelectedPredicateAnnotations<LiftedTag>& delta_and_annot)
{
    return fetch_annotation_cost<LiftedTag>(delta_head, delta_and_annot);
}

template<typename AggregationFunction, typename CanWin>
std::optional<WitnessAnnotation<LiftedTag>> try_ground_witness(const AndAnnotationContext<LiftedTag>& context, CanWin&& can_win)
{
    auto body_metric = ygg::ClosedInterval<ygg::float_t>();
    auto body_cost = AggregationFunction::identity();
    const auto lower_bound = [&] { return std::max(body_cost, context.current_cost) + context.metric_effect_cost; };
    if (!can_win(lower_bound()))
        return std::nullopt;

    auto& numeric_supports = context.witness_support_scratch;
    numeric_supports.clear();

    for (const auto literal : context.witness_condition.get_literals<::tyr::formalism::FluentTag>())
    {
        assert(literal.get_polarity());

        const auto [program_binding, inserted] = ::tyr::formalism::datalog::ground_binding(literal.get_atom(), context.iteration_context);
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
        const auto ground_constraint_data = ::tyr::formalism::datalog::ground(numeric_constraint, context.iteration_context);
        const auto ground_constraint = ygg::make_view(ground_constraint_data, context.iteration_context.destination);
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
    return WitnessAnnotation<LiftedTag>(context.rule_binding,
                                        body_metric,
                                        body_cost + context.metric_effect_cost,
                                        std::span<const NumericSupport<LiftedTag>>(numeric_supports));
}

template<typename AggregationFunction>
std::optional<WitnessAnnotation<LiftedTag>> try_ground_better_witness(Cost best_cost, const AndAnnotationContext<LiftedTag>& context)
{
    return try_ground_witness<AggregationFunction>(context, [best_cost](Cost lower_bound) { return lower_bound < best_cost; });
}

/// Like try_ground_better_witness but admits equal-cost witnesses, whose canonical tie-break the caller decides.
template<typename AggregationFunction>
std::optional<WitnessAnnotation<LiftedTag>>
try_ground_witness_leq(Cost best_cost, const Annotation<LiftedTag>* incumbent, const AndAnnotationContext<LiftedTag>& context)
{
    return try_ground_witness<AggregationFunction>(context,
                                                   [best_cost, incumbent, rule_key = context.rule_binding](Cost lower_bound)
                                                   {
                                                       if (lower_bound != best_cost)
                                                           return lower_bound < best_cost;
                                                       if (!incumbent)
                                                           return true;
                                                       const auto* incumbent_witness = std::get_if<WitnessAnnotation<LiftedTag>>(incumbent);
                                                       // Rule keys are the first canonical witness field, so a
                                                       // larger or identical key cannot recover a tie.
                                                       return incumbent_witness && canonical_rule_binding_less(rule_key, incumbent_witness->get_rule_key());
                                                   });
}
}

template<typename AggregationFunction>
void AndAnnotationPolicy<LiftedTag, AggregationFunction>::update_annotation(
    ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> program_head,
    ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> delta_head,
    const AndAnnotationContext<LiftedTag>& context,
    SelectedPredicateAnnotations<LiftedTag>& delta_and_annot) const
{
    // Use min among global minimum in cost of last iteration and thread local minimum.
    const auto best_global_cost = fetch_annotation_cost<LiftedTag>(program_head, context.program_and_annot);
    const auto best_local_cost = fetch_current_best_cost(delta_head, delta_and_annot);
    const auto best_cost = std::min(best_global_cost, best_local_cost);
    const auto cur_cost_lower_bound = context.current_cost + context.metric_effect_cost;

    if (best_cost < cur_cost_lower_bound)
        return;  ///< No local or global improvement or tie

    const auto* incumbent =
        select_incumbent<LiftedTag>(program_head, delta_head, best_global_cost, best_local_cost, context.program_and_annot, delta_and_annot);
    auto witness = try_ground_witness_leq<AggregationFunction>(best_cost, incumbent, context);
    if (!witness)
        return;  ///< No local or global improvement or tie

    const auto* incumbent_witness = incumbent ? std::get_if<WitnessAnnotation<LiftedTag>>(incumbent) : nullptr;
    if (witness->get_cost() == best_cost && (!incumbent_witness || !canonical_rule_binding_less(witness->get_rule_key(), incumbent_witness->get_rule_key())))
        return;  ///< Grounded into a tie that loses canonically

    /// Update improved or canonically tie-winning witness and cost annotation
    delta_and_annot.insert_or_assign(delta_head, Annotation<LiftedTag>(std::move(*witness)));
}

template<typename AggregationFunction>
void AndAnnotationPolicy<LiftedTag, AggregationFunction>::update_annotation(
    ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> program_head,
    ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> delta_head,
    ygg::ClosedInterval<ygg::float_t> interval,
    const AndAnnotationContext<LiftedTag>& context,
    SelectedFunctionAnnotations<LiftedTag>& delta_numeric_and_annot) const
{
    const auto best_cost = std::numeric_limits<Cost>::max();
    const auto cur_cost_lower_bound = context.current_cost + context.metric_effect_cost;

    if (best_cost <= cur_cost_lower_bound)
        return;

    auto witness = try_ground_better_witness<AggregationFunction>(best_cost, context);
    if (!witness)
        return;

    delta_numeric_and_annot.insert(delta_head, interval, Annotation<LiftedTag>(std::move(*witness)));
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
AchieverAndAnnotationPolicy<LiftedTag, AggregationFunction>::find_achievers(PredicateBinding program_head) const noexcept
{
    const auto it = m_achievers.find(program_head.get_index());
    return it == m_achievers.end() ? nullptr : &it->second;
}

template<typename AggregationFunction>
void AchieverAndAnnotationPolicy<LiftedTag, AggregationFunction>::record_achiever(PredicateBinding program_head,
                                                                                  const AndAnnotationContext<LiftedTag>& context) const
{
    auto witness = try_ground_witness<AggregationFunction>(context, [](Cost) { return true; });
    if (witness)
    {
        auto& achievers = m_achievers[program_head.get_index()];
        if (std::find(achievers.begin(), achievers.end(), *witness) == achievers.end())
            achievers.push_back(std::move(*witness));
    }
}

template class AndAnnotationPolicy<LiftedTag, SumAggregation>;
template class AndAnnotationPolicy<LiftedTag, MaxAggregation>;
template class AchieverAndAnnotationPolicy<LiftedTag, MaxAggregation>;

}
