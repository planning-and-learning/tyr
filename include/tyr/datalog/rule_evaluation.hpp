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

#ifndef TYR_DATALOG_RULE_EVALUATION_HPP_
#define TYR_DATALOG_RULE_EVALUATION_HPP_

#include "tyr/datalog/numeric_utils.hpp"
#include "tyr/datalog/policies/annotation_types.hpp"
#include "tyr/datalog/policies/cost.hpp"
#include "tyr/datalog/policies/numeric_support.hpp"
#include "tyr/datalog/rule_instance.hpp"

#include <cassert>
#include <limits>
#include <optional>
#include <span>
#include <type_traits>
#include <vector>
#include <yggdrasil/containers/variant.hpp>

namespace tyr::datalog
{

template<TaskKind Kind>
struct RuleEvaluationInput
{
    const NumericSupportSelector<Kind>& selector;
    const PredicateAnnotations<Kind>& predicate_annotations;
};

template<TaskKind Kind>
struct RuleEvaluationWorkspace
{
    NumericSupportSelectorWorkspace<Kind> selector;
    std::vector<NumericSupport<Kind>> exact_supports;
};

template<TaskKind Kind>
struct CandidateEvidence
{
    ygg::ClosedInterval<ygg::float_t> metric;
    std::span<const NumericSupport<Kind>> numeric_supports;
};

template<TaskKind Kind>
struct PredicateCandidate
{
    ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> head;
    Cost cost;
    Cost queue_label;
    std::optional<CandidateEvidence<Kind>> evidence;
};

template<TaskKind Kind>
struct FunctionCandidate
{
    ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> head;
    ygg::ClosedInterval<ygg::float_t> interval;
    Cost cost;
    Cost queue_label;
    bool grows_fact;
    std::optional<CandidateEvidence<Kind>> evidence;
};

namespace rule_evaluation_detail
{

template<TaskKind Kind>
using Metric = ygg::ClosedInterval<ygg::float_t>;

template<TaskKind Kind>
struct EvaluationState
{
    Cost support_cost;
    Metric<Kind> support_metric;
    Cost raw_edge;
    std::optional<ResolvedNumericEffect> numeric_effect;
    ygg::ClosedInterval<ygg::float_t> raw_interval;
    ygg::ClosedInterval<ygg::float_t> current_interval;
};

template<typename Aggregation, TaskKind Kind>
void aggregate_selection_cost(Cost& cost, const NumericSupportSelectorWorkspace<Kind>& workspace)
{
    const auto aggregate = Aggregation {};
    for (const auto& entry : workspace.selection)
        cost = aggregate(cost, entry.cost);
}

template<bool CollectEvidence, TaskKind Kind>
bool append_selection_evidence(const RuleEvaluationInput<Kind>& input, RuleEvaluationWorkspace<Kind>& workspace, Metric<Kind>& metric)
{
    if constexpr (!CollectEvidence)
    {
        return true;
    }
    else
    {
        for (const auto& entry : workspace.selector.selection)
        {
            const auto reported = input.selector.for_each_entry_support(entry,
                                                                        [&](const auto key, const auto interval, const auto& annotation)
                                                                        {
                                                                            metric = aggregate_metric_support(metric, get_metric(annotation));
                                                                            workspace.exact_supports.emplace_back(key, interval, get_cost(annotation));
                                                                        });
            if (!reported)
            {
                assert(false && "annotated rule support has no exact backing certificate");
                return false;
            }
        }
        return true;
    }
}

template<TaskKind Kind, ::tyr::formalism::RelationKind R, typename CP>
Cost get_rule_credit(const CP& policy, RuleInstance<Kind, R>& instance)
{
    if constexpr (std::same_as<std::remove_cvref_t<CP>, RuleCostPolicy<Kind>>)
        return Cost(0);
    else
        return policy.get_cost(instance.witness_key());
}

template<TaskKind Kind, typename CP>
Cost get_transition_credit(const CP& policy,
                           RuleInstance<Kind, ::tyr::formalism::FunctionTag>& instance,
                           ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> head,
                           ygg::ClosedInterval<ygg::float_t> interval)
{
    if constexpr (std::same_as<std::remove_cvref_t<CP>, RuleCostPolicy<Kind>>)
        return Cost(0);
    else
        return policy.get_cost(instance.witness_key(), head, interval);
}

template<bool CollectEvidence, TaskKind Kind, ::tyr::formalism::RelationKind R, typename AP, typename CP>
std::optional<EvaluationState<Kind>>
evaluate_annotated_rule(RuleInstance<Kind, R>& instance, const AP&, const CP&, const RuleEvaluationInput<Kind>& input, RuleEvaluationWorkspace<Kind>& workspace)
{
    using Aggregation = typename AP::Aggregation;
    const auto aggregate = Aggregation {};
    auto state = EvaluationState<Kind> { Aggregation::identity(), {}, Cost(0), std::nullopt, {}, {} };

    workspace.exact_supports.clear();

    for (const auto literal : instance.get_body().template get_literals<::tyr::formalism::FluentTag>())
    {
        if (!literal.get_polarity())
            continue;

        const auto binding = instance.resolve(literal.get_atom());
        const auto* annotation = input.predicate_annotations.find(binding);
        if (!annotation || get_cost(*annotation) == std::numeric_limits<Cost>::max())
            return std::nullopt;

        state.support_cost = aggregate(state.support_cost, get_cost(*annotation));
        if constexpr (CollectEvidence)
            state.support_metric = aggregate_metric_support(state.support_metric, get_metric(*annotation));
    }

    for (const auto constraint : instance.get_body().get_numeric_constraints())
    {
        const auto resolved = instance.resolve(constraint);
        const auto constraint_cost = input.selector.get_constraint_cost(resolved, workspace.selector.selection, Aggregation {});
        if (constraint_cost == std::numeric_limits<Cost>::max())
            return std::nullopt;

        state.support_cost = aggregate(state.support_cost, constraint_cost);
        if (!append_selection_evidence<CollectEvidence>(input, workspace, state.support_metric))
            return std::nullopt;
    }

    if constexpr (std::same_as<R, ::tyr::formalism::FunctionTag>)
    {
        state.numeric_effect = ygg::visit([&](const auto effect) { return instance.resolve(effect); }, instance.get_head().get_variant());
        const auto& effect = *state.numeric_effect;
        workspace.selector.clear();

        auto lhs = ygg::ClosedInterval<ygg::float_t> {};
        if (effect.operator_kind != ::tyr::formalism::NumericEffectOperatorKind::Assign)
        {
            lhs = input.selector.select_fluent_interval(effect.head, workspace.selector.selection);
            if (empty(lhs))
                return std::nullopt;
        }

        const auto rhs = input.selector.evaluate_effect_expression(effect.rhs, workspace.selector.selection);
        if (empty(rhs))
            return std::nullopt;

        state.raw_interval = apply_numeric_effect(effect.operator_kind, lhs, rhs);
        if (empty(state.raw_interval))
            return std::nullopt;

        state.current_interval = input.selector.current_interval(effect.head);
        aggregate_selection_cost<Aggregation>(state.support_cost, workspace.selector);
        if (!append_selection_evidence<CollectEvidence>(input, workspace, state.support_metric))
            return std::nullopt;
    }

    for (const auto& metric_operator : instance.get_metric_effects())
    {
        const auto delta = ygg::visit(
            [&](const auto metric_effect) -> std::optional<Cost>
            {
                const auto effect = instance.resolve(metric_effect);
                workspace.selector.clear();
                const auto result = metric_effect_delta(
                    effect.operator_kind,
                    [&] { return input.selector.select_fluent_interval(effect.head, workspace.selector.selection); },
                    [&] { return input.selector.evaluate_effect_expression(effect.rhs, workspace.selector.selection); });
                if (!result)
                    return std::nullopt;

                aggregate_selection_cost<Aggregation>(state.support_cost, workspace.selector);
                if (!append_selection_evidence<CollectEvidence>(input, workspace, state.support_metric))
                    return std::nullopt;
                return result;
            },
            metric_operator.get_variant());
        if (!delta)
            return std::nullopt;
        state.raw_edge += *delta;
    }

    return state;
}

template<TaskKind Kind, typename AP>
std::optional<FunctionCandidate<Kind>> evaluate_unannotated_function(RuleInstance<Kind, ::tyr::formalism::FunctionTag>& instance,
                                                                     const AP& policy,
                                                                     const RuleEvaluationInput<Kind>& input,
                                                                     RuleEvaluationWorkspace<Kind>& workspace)
{
    const auto effect = ygg::visit([&](const auto source) { return instance.resolve(source); }, instance.get_head().get_variant());
    workspace.selector.clear();

    auto lhs = ygg::ClosedInterval<ygg::float_t> {};
    if (effect.operator_kind != ::tyr::formalism::NumericEffectOperatorKind::Assign)
    {
        lhs = input.selector.select_fluent_interval(effect.head, workspace.selector.selection);
        if (empty(lhs))
            return std::nullopt;
    }

    const auto rhs = input.selector.evaluate_effect_expression(effect.rhs, workspace.selector.selection);
    if (empty(rhs))
        return std::nullopt;

    const auto raw_interval = apply_numeric_effect(effect.operator_kind, lhs, rhs);
    if (empty(raw_interval))
        return std::nullopt;

    const auto current = input.selector.current_interval(effect.head);
    auto interval = raw_interval;
    if (!empty(current) && policy.is_widening_label_preserving(Cost(0), Cost(0)))
        interval = widen_free_growth(raw_interval, current);

    return FunctionCandidate<Kind> { effect.head, interval, Cost(0), Cost(0), empty(current) || !subset(interval, current), std::nullopt };
}

}

template<TaskKind Kind, ::tyr::formalism::RelationKind R, typename AP, typename CP>
std::optional<Cost> evaluate_rule_priority(RuleInstance<Kind, R>& instance,
                                           const AP& annotation_policy,
                                           const CP& cost_policy,
                                           const RuleEvaluationInput<Kind>& input,
                                           RuleEvaluationWorkspace<Kind>& workspace)
{
    if constexpr (!AP::stores_annotations)
    {
        if constexpr (std::same_as<R, ::tyr::formalism::PredicateTag>)
            return Cost(0);
        else
        {
            const auto candidate = rule_evaluation_detail::evaluate_unannotated_function(instance, annotation_policy, input, workspace);
            return candidate ? std::optional(candidate->queue_label) : std::nullopt;
        }
    }
    else
    {
        const auto state = rule_evaluation_detail::evaluate_annotated_rule<false>(instance, annotation_policy, cost_policy, input, workspace);
        if (!state)
            return std::nullopt;
        if constexpr (std::same_as<R, ::tyr::formalism::PredicateTag>)
        {
            const auto edge = reduce_cost(state->raw_edge, rule_evaluation_detail::get_rule_credit<Kind, R>(cost_policy, instance));
            return state->support_cost + edge;
        }
        else
        {
            return state->support_cost;
        }
    }
}

template<TaskKind Kind, typename AP, typename CP>
std::optional<PredicateCandidate<Kind>> evaluate_predicate_candidate(RuleInstance<Kind, ::tyr::formalism::PredicateTag>& instance,
                                                                     const AP& annotation_policy,
                                                                     const CP& cost_policy,
                                                                     const RuleEvaluationInput<Kind>& input,
                                                                     RuleEvaluationWorkspace<Kind>& workspace)
{
    const auto head = instance.resolve(instance.get_head());
    if constexpr (!AP::stores_annotations)
    {
        return PredicateCandidate<Kind> { head, Cost(0), Cost(0), std::nullopt };
    }
    else
    {
        const auto state = rule_evaluation_detail::evaluate_annotated_rule<true>(instance, annotation_policy, cost_policy, input, workspace);
        if (!state)
            return std::nullopt;

        const auto edge = reduce_cost(state->raw_edge, rule_evaluation_detail::get_rule_credit<Kind, ::tyr::formalism::PredicateTag>(cost_policy, instance));
        const auto cost = state->support_cost + edge;
        return PredicateCandidate<Kind> { head,
                                          cost,
                                          cost,
                                          CandidateEvidence<Kind> { add_metric_delta(state->support_metric, edge), workspace.exact_supports } };
    }
}

template<TaskKind Kind, typename AP, typename CP>
std::optional<FunctionCandidate<Kind>> evaluate_function_candidate(RuleInstance<Kind, ::tyr::formalism::FunctionTag>& instance,
                                                                   const AP& annotation_policy,
                                                                   const CP& cost_policy,
                                                                   const RuleEvaluationInput<Kind>& input,
                                                                   RuleEvaluationWorkspace<Kind>& workspace)
{
    if constexpr (!AP::stores_annotations)
    {
        return rule_evaluation_detail::evaluate_unannotated_function(instance, annotation_policy, input, workspace);
    }
    else
    {
        const auto state = rule_evaluation_detail::evaluate_annotated_rule<true>(instance, annotation_policy, cost_policy, input, workspace);
        if (!state || !state->numeric_effect)
            return std::nullopt;

        const auto& effect = *state->numeric_effect;
        auto edge = reduce_cost(state->raw_edge, rule_evaluation_detail::get_rule_credit<Kind, ::tyr::formalism::FunctionTag>(cost_policy, instance));
        const auto pre_transition_label = state->support_cost + edge;
        auto interval = state->raw_interval;
        if (!empty(state->current_interval) && edge == Cost(0))
        {
            const auto current_label = input.selector.get_current_interval_cost(effect.head, state->current_interval);
            if (annotation_policy.is_widening_label_preserving(pre_transition_label, current_label))
                interval = widen_free_growth(state->raw_interval, state->current_interval);
        }

        if (interval == state->raw_interval)
            edge = reduce_cost(edge, rule_evaluation_detail::get_transition_credit<Kind>(cost_policy, instance, effect.head, interval));

        const auto cost = state->support_cost + edge;
        return FunctionCandidate<Kind> { effect.head,
                                         interval,
                                         cost,
                                         state->support_cost,
                                         empty(state->current_interval) || !subset(interval, state->current_interval),
                                         CandidateEvidence<Kind> { add_metric_delta(state->support_metric, edge), workspace.exact_supports } };
    }
}

template<TaskKind Kind, ::tyr::formalism::RelationKind R, typename Candidate>
WitnessAnnotation<Kind, R> materialize_witness(RuleInstance<Kind, R>& instance, const Candidate& candidate)
{
    assert(candidate.evidence);
    return WitnessAnnotation<Kind, R>(instance.witness_key(), candidate.evidence->metric, candidate.cost, candidate.evidence->numeric_supports);
}

}

#endif
