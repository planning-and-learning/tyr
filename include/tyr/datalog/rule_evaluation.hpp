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

#include "tyr/datalog/cost_buckets.hpp"
#include "tyr/formalism/numeric_expression_utils.hpp"
#include "tyr/datalog/policies/annotation_concepts.hpp"
#include "tyr/datalog/policies/annotation_types.hpp"
#include "tyr/datalog/policies/cost.hpp"
#include "tyr/datalog/policies/cost_concepts.hpp"
#include "tyr/datalog/policies/numeric_support.hpp"
#include "tyr/datalog/policies/termination_concepts.hpp"
#include "tyr/datalog/rule_instance.hpp"

#include <algorithm>
#include <boost/dynamic_bitset.hpp>
#include <cassert>
#include <limits>
#include <optional>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/containers/dynamic_bitset.hpp>
#include <yggdrasil/containers/variant.hpp>
#include <yggdrasil/semantics/comparison.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::datalog
{

/// Conservative closure acceleration for detected label-preserving zero-edge bound growth.
/// This does not imply exact numeric closure or global termination.
inline ygg::ClosedInterval<ygg::float_t> widen_free_growth(ygg::ClosedInterval<ygg::float_t> next, ygg::ClosedInterval<ygg::float_t> current)
{
    if (empty(current))
        return next;

    constexpr auto inf = std::numeric_limits<ygg::float_t>::infinity();
    return ygg::ClosedInterval<ygg::float_t>(lower(next) < lower(current) ? -inf : lower(next), upper(next) > upper(current) ? inf : upper(next));
}


struct RuleEvaluationInput
{
    const NumericSupportSelector& selector;
    const PredicateAnnotations<>& predicate_annotations;
};

/// The costs and supports of the ground numeric constraints evaluated in the current execution of a lifted rule, by the
/// index of the constraint's operator: the selector does not change during an execution, and bindings that differ only
/// in variables outside a constraint ground it to the same operator.
struct NumericConstraintCache
{
    struct Entry
    {
        size_t epoch = 0;
        Cost cost = 0;
        size_t selection_begin = 0;
        size_t selection_size = 0;
    };

    /// Called at the start of every rule execution.
    void invalidate() noexcept
    {
        ++epoch;
        selections.clear();
    }

    size_t epoch = 1;
    std::vector<Entry> entries;
    std::vector<NumericSupportSelectorWorkspace::SelectionEntry> selections;
};

struct RuleEvaluationWorkspace
{
    NumericSupportSelectorWorkspace selector;
    std::vector<NumericSupport> exact_supports;
    NumericConstraintCache constraint_cache;
};

struct CandidateEvidence
{
    std::span<const NumericSupport> numeric_supports;
};

struct PredicateCandidate
{
    Cost cost;
    Cost queue_label;
    std::optional<CandidateEvidence> evidence;
};

struct FunctionCandidate
{
    formalism::datalog::FunctionBindingIndex<formalism::FluentTag> head;
    ygg::ClosedInterval<ygg::float_t> interval;
    Cost cost;
    Cost queue_label;
    bool grows_fact;
    std::optional<CandidateEvidence> evidence;
};

struct PredicateHeadUpdates
{
private:
    using Binding = formalism::datalog::PredicateBindingIndex<formalism::FluentTag>;
    using Witness = WitnessAnnotation<formalism::PredicateTag>;

    /// The rows seen per relation; only membership is queried, so the order of bindings stays that of their first insertion.
    std::vector<boost::dynamic_bitset<>> m_seen_rows;

public:
    std::vector<Binding> bindings;
    std::vector<PredicateAchiever> achievers;

    void clear() noexcept
    {
        for (const auto binding : bindings)
            m_seen_rows[ygg::uint_t(binding.relation)].reset(ygg::uint_t(binding.row));
        bindings.clear();
        achievers.clear();
    }

    void insert(Binding binding)
    {
        const auto relation = size_t(ygg::uint_t(binding.relation));
        const auto row = size_t(ygg::uint_t(binding.row));
        if (relation >= m_seen_rows.size())
            m_seen_rows.resize(relation + 1);
        auto& seen = m_seen_rows[relation];
        if (ygg::test(row, seen))
            return;
        ygg::set(row, true, seen);
        bindings.push_back(binding);
    }

    void insert_achiever(Binding head, Witness witness) { achievers.push_back(PredicateAchiever { head, std::move(witness) }); }
};

struct FunctionHeadUpdate : ygg::comparison::Mixin<FunctionHeadUpdate>
{
    formalism::datalog::FunctionBindingIndex<formalism::FluentTag> binding;
    ygg::ClosedInterval<ygg::float_t> interval;
    bool grows_fact;

    FunctionHeadUpdate(formalism::datalog::FunctionBindingIndex<formalism::FluentTag> binding,
                       ygg::ClosedInterval<ygg::float_t> interval,
                       bool grows_fact) :
        binding(binding),
        interval(interval),
        grows_fact(grows_fact)
    {
    }

    static constexpr auto identifying_members() noexcept { return ygg::Members<&FunctionHeadUpdate::binding, &FunctionHeadUpdate::interval> {}; }
};

struct FunctionHeadUpdates
{
    ygg::UnorderedSet<FunctionHeadUpdate> seen_updates;
    std::vector<FunctionHeadUpdate> updates;

    void clear() noexcept
    {
        seen_updates.clear();
        updates.clear();
    }

    void insert(FunctionHeadUpdate update)
    {
        if (seen_updates.emplace(update).second)
            updates.push_back(std::move(update));
    }
};

template<AnnotationPolicyConcept AP, PredicateAnnotationStore DeltaAnnotations>
void reduce_predicate_head_updates(PredicateHeadUpdates& head_updates,
                                   [[maybe_unused]] AP& annotation_policy,
                                   [[maybe_unused]] const PredicateFactSets<formalism::FluentTag>& facts,
                                   [[maybe_unused]] DeltaAnnotations& delta_annotations,
                                   [[maybe_unused]] PredicateAnnotations<>& annotations,
                                   CostBuckets& cost_buckets,
                                   [[maybe_unused]] PendingPredicateAchievers& pending_achievers)
{
    if constexpr (AP::records_propositional_achievers)
        for (auto& achiever : head_updates.achievers)
            insert_pending_achiever(pending_achievers, std::move(achiever));

    for (const auto head : head_updates.bindings)
    {
        if constexpr (AP::stores_annotations)
        {
            const auto* delta_annotation = delta_annotations.find(head);
            if (facts.contains(head))
            {
                assert((!delta_annotation || get_cost(*delta_annotation) >= annotations.fetch_cost(head)) && "A committed predicate label must be final.");
                continue;
            }

            if (!delta_annotation && !annotations.find(head))
                continue;
            cost_buckets.update(annotation_policy.commit_annotation(head, delta_annotations, annotations), head);
        }
        else if (!facts.contains(head))
        {
            cost_buckets.insert(Cost(0), head);
        }
    }
}

template<AnnotationPolicyConcept AP, FunctionAnnotationStore DeltaAnnotations>
bool reduce_function_head_update(const FunctionHeadUpdate& update,
                                 [[maybe_unused]] AP& annotation_policy,
                                 [[maybe_unused]] DeltaAnnotations& delta_numeric_annotations,
                                 [[maybe_unused]] FunctionAnnotations<>& numeric_annotations,
                                 CostBuckets& cost_buckets)
{
    if constexpr (AP::stores_annotations)
    {
        const auto* annotation = delta_numeric_annotations.find(update.binding, update.interval);
        auto improved = false;
        if (annotation)
        {
            improved =
                annotation_policy.commit_annotation(update.binding, update.interval, delta_numeric_annotations, numeric_annotations).is_strict_improvement();
            annotation = numeric_annotations.find(update.binding, update.interval);
        }
        else
            annotation = numeric_annotations.find(update.binding, update.interval);

        // FunctionAnnotations retains the cheapest certificate for each exact interval.
        // FactSets determines whether that certificate is available; CostBuckets schedules only hull growth.
        if (annotation && update.grows_fact)
        {
            cost_buckets.insert(get_cost(*annotation), update.binding, update.interval);
            return false;
        }
        return improved;
    }
    else
    {
        if (update.grows_fact)
            cost_buckets.insert(Cost(0), update.binding, update.interval);
        return false;
    }
}

template<TaskKind Kind, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP, FunctionAnnotationStore DeltaAnnotations>
bool reduce_function_head_updates(FunctionHeadUpdates& head_updates,
                                  AP& annotation_policy,
                                  DeltaAnnotations& delta_numeric_annotations,
                                  FunctionAnnotations<>& numeric_annotations,
                                  CostBuckets& cost_buckets,
                                  Scheduler<Kind>& scheduler,
                                  ProgramExecutionContext<Kind, AP, TP, CP>& ctx)
{
    auto annotation_improved = false;
    for (const auto& update : head_updates.updates)
    {
        if (reduce_function_head_update(update, annotation_policy, delta_numeric_annotations, numeric_annotations, cost_buckets))
        {
            annotation_improved = true;
            scheduler.notify_numeric_changed(update.binding, ctx);
        }
    }
    return annotation_improved;
}

template<formalism::RelationKind R>
using RuleHeadUpdatesT = std::conditional_t<std::same_as<R, formalism::PredicateTag>, PredicateHeadUpdates, FunctionHeadUpdates>;

template<TaskKind Kind, formalism::RelationKind R, AnnotationPolicyConcept AP, RuleCostPolicyConcept CP>
struct RuleUpdateInput
{
    RuleInstance<Kind, R> rule_instance;
    RuleEvaluationInput evaluation;
    RuleEvaluationWorkspace& workspace;
    const FunctionAnnotations<>& numeric_annotations;
    AP& annotation_policy;
    const CP& cost_policy;
};

template<TaskKind Kind, formalism::RelationKind R, AnnotationPolicyConcept AP, RuleCostPolicyConcept CP>
RuleUpdateInput<Kind, R, AP, CP> make_rule_update_input(RuleInstance<Kind, R> rule_instance,
                                                        const NumericSupportSelector& numeric_support_selector,
                                                        const PredicateAnnotations<>& predicate_annotations,
                                                        RuleEvaluationWorkspace& workspace,
                                                        const FunctionAnnotations<>& numeric_annotations,
                                                        AP& annotation_policy,
                                                        const CP& cost_policy)
{
    return { std::move(rule_instance),
             RuleEvaluationInput { numeric_support_selector, predicate_annotations },
             workspace,
             numeric_annotations,
             annotation_policy,
             cost_policy };
}

enum class RuleUpdateStatus
{
    Unavailable,
    Pruned,
    QueueLabelChanged,
    Completed,
};

struct RuleUpdateResult
{
    RuleUpdateStatus status;
    std::optional<Cost> queue_label;

    bool is_handled() const noexcept { return status == RuleUpdateStatus::Pruned || status == RuleUpdateStatus::Completed; }
};

namespace rule_evaluation_detail
{

struct AlwaysContinue
{
    constexpr bool operator()(Cost, Cost) const noexcept { return true; }
};

struct EvaluationState
{
    Cost support_cost;
    Cost raw_edge;
    std::optional<ResolvedNumericEffect> numeric_effect;
    ygg::ClosedInterval<ygg::float_t> raw_interval;
    ygg::ClosedInterval<ygg::float_t> current_interval;
};

template<typename Aggregation>
void aggregate_selection_cost(Cost& cost, const NumericSupportSelectorWorkspace& workspace)
{
    const auto aggregate = Aggregation {};
    for (const auto& entry : workspace.selection)
        cost = aggregate(cost, entry.cost);
}

template<bool CollectEvidence>
bool append_selection_evidence(const RuleEvaluationInput& input, RuleEvaluationWorkspace& workspace)
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

template<TaskKind Kind, formalism::RelationKind R, typename CP>
Cost get_rule_credit(const CP& policy, RuleInstance<Kind, R>& instance)
{
    if constexpr (std::same_as<std::remove_cvref_t<CP>, RuleCostPolicy>)
        return Cost(0);
    else
        return policy.get_cost(instance.witness_key());
}

template<TaskKind Kind, formalism::RelationKind R, typename CP>
Cost get_transition_credit(const CP& policy,
                           RuleInstance<Kind, R>& instance,
                           formalism::datalog::FunctionBindingIndex<formalism::FluentTag> head,
                           ygg::ClosedInterval<ygg::float_t> interval)
{
    if constexpr (std::same_as<std::remove_cvref_t<CP>, RuleCostPolicy>)
        return Cost(0);
    else
        return policy.get_cost(instance.witness_key().get_index(), head, interval);
}

/// The constraint's cost and supports (in workspace.selector.selection), from the cache for lifted rules, which invalidate
/// it at every execution.
template<TaskKind Kind, typename Aggregation>
Cost get_constraint_cost(formalism::datalog::BooleanOperatorView<GroundTag> constraint, const RuleEvaluationInput& input, RuleEvaluationWorkspace& workspace)
{
    auto& selection = workspace.selector.selection;
    if constexpr (!std::same_as<Kind, LiftedTag>)
    {
        return input.selector.get_constraint_cost(constraint, selection, Aggregation {});
    }
    else
    {
        auto& cache = workspace.constraint_cache;
        const auto key = size_t(ygg::visit([](const auto op) { return op.get_index().value; }, constraint.get_variant()));
        if (key >= cache.entries.size())
            cache.entries.resize(key + 1);
        auto& entry = cache.entries[key];
        if (entry.epoch == cache.epoch)
        {
            const auto begin = cache.selections.begin() + long(entry.selection_begin);
            selection.assign(begin, begin + long(entry.selection_size));
            return entry.cost;
        }
        const auto cost = input.selector.get_constraint_cost(constraint, selection, Aggregation {});
        entry = { cache.epoch, cost, cache.selections.size(), selection.size() };
        cache.selections.insert(cache.selections.end(), selection.begin(), selection.end());
        return cost;
    }
}

template<bool CollectEvidence, TaskKind Kind, formalism::RelationKind R, typename AP, typename CP, typename CanContinue>
std::optional<EvaluationState> evaluate_annotated_rule(RuleInstance<Kind, R>& instance,
                                                       const AP&,
                                                       const CP&,
                                                       const RuleEvaluationInput& input,
                                                       RuleEvaluationWorkspace& workspace,
                                                       CanContinue&& can_continue)
{
    using Aggregation = typename AP::Aggregation;
    const auto aggregate = Aggregation {};
    auto state = EvaluationState { Aggregation::identity(), Cost(instance.get_rule().get_cost()), std::nullopt, {}, {} };

    workspace.exact_supports.clear();

    if (!can_continue(state.support_cost, state.raw_edge))
        return std::nullopt;

    for (const auto literal : instance.get_body().template get_literals<formalism::FluentTag>())
    {
        if (!literal.get_polarity())
            continue;

        const auto binding = instance.resolve(literal.get_atom());
        const auto* annotation = input.predicate_annotations.find(binding.get_index());
        if (!annotation || get_cost(*annotation) == std::numeric_limits<Cost>::max())
            return std::nullopt;

        state.support_cost = aggregate(state.support_cost, get_cost(*annotation));
        if (!can_continue(state.support_cost, state.raw_edge))
            return std::nullopt;
    }

    for (const auto constraint : instance.get_body().get_numeric_constraints())
    {
        const auto resolved = instance.resolve(constraint);
        const auto constraint_cost = rule_evaluation_detail::get_constraint_cost<Kind, Aggregation>(resolved, input, workspace);
        if (constraint_cost == std::numeric_limits<Cost>::max())
            return std::nullopt;

        state.support_cost = aggregate(state.support_cost, constraint_cost);
        if (!can_continue(state.support_cost, state.raw_edge))
            return std::nullopt;
        if (!append_selection_evidence<CollectEvidence>(input, workspace))
            return std::nullopt;
    }

    if constexpr (std::same_as<R, formalism::FunctionTag>)
    {
        state.numeric_effect = ygg::visit([&](const auto effect) { return instance.resolve(effect); }, instance.get_head().get_variant());
        const auto& effect = *state.numeric_effect;
        workspace.selector.clear();

        auto lhs = ygg::ClosedInterval<ygg::float_t> {};
        if (effect.operator_kind != formalism::NumericEffectOperatorKind::Assign)
        {
            lhs = input.selector.select_fluent_interval(effect.head, workspace.selector.selection);
            if (empty(lhs))
                return std::nullopt;
        }

        const auto rhs = input.selector.evaluate_effect_expression(effect.rhs, workspace.selector.selection);
        if (empty(rhs))
            return std::nullopt;

        state.raw_interval = formalism::apply(effect.operator_kind, lhs, rhs);
        if (empty(state.raw_interval))
            return std::nullopt;

        state.current_interval = input.selector.current_interval(effect.head);
        aggregate_selection_cost<Aggregation>(state.support_cost, workspace.selector);
        if (!can_continue(state.support_cost, state.raw_edge))
            return std::nullopt;
        if (!append_selection_evidence<CollectEvidence>(input, workspace))
            return std::nullopt;
    }

    return state;
}

template<TaskKind Kind, typename AP>
std::optional<FunctionCandidate> evaluate_unannotated_function(RuleInstance<Kind, formalism::FunctionTag>& instance,
                                                               const AP& policy,
                                                               const RuleEvaluationInput& input,
                                                               RuleEvaluationWorkspace& workspace)
{
    const auto effect = ygg::visit([&](const auto source) { return instance.resolve(source); }, instance.get_head().get_variant());
    workspace.selector.clear();

    auto lhs = ygg::ClosedInterval<ygg::float_t> {};
    if (effect.operator_kind != formalism::NumericEffectOperatorKind::Assign)
    {
        lhs = input.selector.select_fluent_interval(effect.head, workspace.selector.selection);
        if (empty(lhs))
            return std::nullopt;
    }

    const auto rhs = input.selector.evaluate_effect_expression(effect.rhs, workspace.selector.selection);
    if (empty(rhs))
        return std::nullopt;

    const auto raw_interval = formalism::apply(effect.operator_kind, lhs, rhs);
    if (empty(raw_interval))
        return std::nullopt;

    const auto current = input.selector.current_interval(effect.head);
    auto interval = raw_interval;
    if (!empty(current) && policy.is_widening_label_preserving(Cost(0), Cost(0)))
        interval = widen_free_growth(raw_interval, current);

    return FunctionCandidate { effect.head, interval, Cost(0), Cost(0), empty(current) || !subset(interval, current), std::nullopt };
}

}

template<TaskKind Kind, formalism::RelationKind R, typename AP, typename CP>
std::optional<Cost> evaluate_rule_priority(RuleInstance<Kind, R>& instance,
                                           const AP& annotation_policy,
                                           const CP& cost_policy,
                                           const RuleEvaluationInput& input,
                                           RuleEvaluationWorkspace& workspace)
{
    if constexpr (!AP::stores_annotations)
    {
        if constexpr (std::same_as<R, formalism::PredicateTag>)
            return Cost(0);
        else
        {
            const auto candidate = rule_evaluation_detail::evaluate_unannotated_function(instance, annotation_policy, input, workspace);
            return candidate ? std::optional(candidate->queue_label) : std::nullopt;
        }
    }
    else
    {
        const auto state = rule_evaluation_detail::evaluate_annotated_rule<false>(instance,
                                                                                  annotation_policy,
                                                                                  cost_policy,
                                                                                  input,
                                                                                  workspace,
                                                                                  rule_evaluation_detail::AlwaysContinue {});
        if (!state)
            return std::nullopt;
        if constexpr (std::same_as<R, formalism::PredicateTag>)
        {
            const auto edge = reduce_cost(state->raw_edge, rule_evaluation_detail::get_rule_credit(cost_policy, instance));
            return state->support_cost + edge;
        }
        else
        {
            return state->support_cost;
        }
    }
}

template<TaskKind Kind, typename AP, typename CP, typename CanContinue = rule_evaluation_detail::AlwaysContinue>
std::optional<PredicateCandidate> evaluate_predicate_candidate(RuleInstance<Kind, formalism::PredicateTag>& instance,
                                                               const AP& annotation_policy,
                                                               const CP& cost_policy,
                                                               const RuleEvaluationInput& input,
                                                               RuleEvaluationWorkspace& workspace,
                                                               CanContinue can_continue = {})
{
    if constexpr (!AP::stores_annotations)
    {
        return PredicateCandidate { Cost(0), Cost(0), std::nullopt };
    }
    else
    {
        const auto state = rule_evaluation_detail::evaluate_annotated_rule<true>(instance, annotation_policy, cost_policy, input, workspace, can_continue);
        if (!state)
            return std::nullopt;

        const auto edge = reduce_cost(state->raw_edge, rule_evaluation_detail::get_rule_credit(cost_policy, instance));
        const auto cost = state->support_cost + edge;
        return PredicateCandidate { cost, cost, CandidateEvidence { workspace.exact_supports } };
    }
}

template<TaskKind Kind, typename AP, typename CP>
auto evaluate_propositional_candidate(RuleInstance<Kind, formalism::PredicateTag>& rule_instance,
                                      const AP& annotation_policy,
                                      const CP& cost_policy,
                                      const RuleEvaluationInput& input,
                                      RuleEvaluationWorkspace& workspace,
                                      Cost best_cost)
{
    auto pruned = false;
    if constexpr (AP::stores_annotations && !AP::records_propositional_achievers && std::same_as<CP, RuleCostPolicy>)
    {
        auto candidate = evaluate_predicate_candidate(rule_instance,
                                                      annotation_policy,
                                                      cost_policy,
                                                      input,
                                                      workspace,
                                                      [&](const Cost support_cost, const Cost edge)
                                                      {
                                                          pruned = support_cost + edge >= best_cost;
                                                          return !pruned;
                                                      });
        return std::pair(std::move(candidate), pruned);
    }
    else
    {
        return std::pair(evaluate_predicate_candidate(rule_instance, annotation_policy, cost_policy, input, workspace), false);
    }
}

/// Prunes only once the instance cannot improve any of its heads.
template<TaskKind Kind, typename AP, typename CP, PredicateAnnotationStore DeltaAnnotations>
auto evaluate_propositional_candidate(std::span<const formalism::datalog::PredicateBindingIndex<formalism::FluentTag>> heads,
                                      RuleInstance<Kind, formalism::PredicateTag>& rule_instance,
                                      const AP& annotation_policy,
                                      const CP& cost_policy,
                                      const RuleEvaluationInput& input,
                                      RuleEvaluationWorkspace& workspace,
                                      const DeltaAnnotations& delta_annotations)
{
    auto best_cost = Cost(0);
    for (const auto head : heads)
        best_cost = std::max(best_cost, std::min(input.predicate_annotations.fetch_cost(head), delta_annotations.fetch_cost(head)));
    return evaluate_propositional_candidate(rule_instance, annotation_policy, cost_policy, input, workspace, best_cost);
}

template<TaskKind Kind, typename AP, typename CP>
std::optional<FunctionCandidate> evaluate_function_candidate(RuleInstance<Kind, formalism::FunctionTag>& instance,
                                                             const AP& annotation_policy,
                                                             const CP& cost_policy,
                                                             const RuleEvaluationInput& input,
                                                             RuleEvaluationWorkspace& workspace)
{
    if constexpr (!AP::stores_annotations)
    {
        return rule_evaluation_detail::evaluate_unannotated_function(instance, annotation_policy, input, workspace);
    }
    else
    {
        const auto state = rule_evaluation_detail::evaluate_annotated_rule<true>(instance,
                                                                                 annotation_policy,
                                                                                 cost_policy,
                                                                                 input,
                                                                                 workspace,
                                                                                 rule_evaluation_detail::AlwaysContinue {});
        if (!state || !state->numeric_effect)
            return std::nullopt;

        const auto& effect = *state->numeric_effect;
        auto edge = reduce_cost(state->raw_edge, rule_evaluation_detail::get_rule_credit(cost_policy, instance));
        const auto pre_transition_label = state->support_cost + edge;
        auto interval = state->raw_interval;
        if (!empty(state->current_interval) && edge == Cost(0))
        {
            const auto current_label = input.selector.get_current_interval_cost(effect.head, state->current_interval);
            if (annotation_policy.is_widening_label_preserving(pre_transition_label, current_label))
                interval = widen_free_growth(state->raw_interval, state->current_interval);
        }

        if (interval == state->raw_interval)
            edge = reduce_cost(edge, rule_evaluation_detail::get_transition_credit(cost_policy, instance, effect.head, interval));

        const auto cost = state->support_cost + edge;
        return FunctionCandidate { effect.head,
                                   interval,
                                   cost,
                                   state->support_cost,
                                   empty(state->current_interval) || !subset(interval, state->current_interval),
                                   CandidateEvidence { workspace.exact_supports } };
    }
}

template<TaskKind Kind, formalism::RelationKind R, typename Candidate>
WitnessAnnotation<R> materialize_witness(RuleInstance<Kind, R>& instance, const Candidate& candidate)
{
    assert(candidate.evidence);
    return WitnessAnnotation<R>(instance.witness_key().get_index(), candidate.cost, candidate.evidence->numeric_supports);
}

/// Evaluates the rule instance once and updates each of its heads with the shared witness.
template<TaskKind Kind, AnnotationPolicyConcept AP, RuleCostPolicyConcept CP, PredicateAnnotationStore DeltaAnnotations>
RuleUpdateResult insert_propositional_update(std::span<const formalism::datalog::PredicateBindingIndex<formalism::FluentTag>> heads,
                                             RuleUpdateInput<Kind, formalism::PredicateTag, AP, CP>& input,
                                             PredicateHeadUpdates& head_updates,
                                             DeltaAnnotations& delta_annotations,
                                             std::optional<Cost> required_queue_label = std::nullopt)
{
    assert(!heads.empty());
    auto [candidate, pruned] = evaluate_propositional_candidate(heads,
                                                                input.rule_instance,
                                                                input.annotation_policy,
                                                                input.cost_policy,
                                                                input.evaluation,
                                                                input.workspace,
                                                                delta_annotations);
    if (!candidate)
    {
        if (!pruned)
            return { RuleUpdateStatus::Unavailable, std::nullopt };

        // A staged annotation can still need its head restored to the cost frontier after resume.
        for (const auto head : heads)
            head_updates.insert(head);
        return { RuleUpdateStatus::Pruned, std::nullopt };
    }

    if (required_queue_label && candidate->queue_label != *required_queue_label)
        return { RuleUpdateStatus::QueueLabelChanged, candidate->queue_label };

    if constexpr (AP::stores_annotations)
    {
        for (const auto head : heads)
        {
            const auto can_update = input.annotation_policy.can_update(head, candidate->cost, input.evaluation.predicate_annotations, delta_annotations);
            if constexpr (AP::records_propositional_achievers)
            {
                auto witness = materialize_witness(input.rule_instance, *candidate);
                if (can_update)
                {
                    head_updates.insert_achiever(head, witness);
                    input.annotation_policy.try_update_candidate(head, std::move(witness), delta_annotations);
                }
                else
                {
                    head_updates.insert_achiever(head, std::move(witness));
                }
            }
            else if (can_update)
            {
                input.annotation_policy.try_update_candidate(head, materialize_witness(input.rule_instance, *candidate), delta_annotations);
            }
        }
    }
    // A previously staged certificate may still need to become an available fact after a resumed solve.
    for (const auto head : heads)
        head_updates.insert(head);
    return { RuleUpdateStatus::Completed, candidate->queue_label };
}

template<TaskKind Kind, AnnotationPolicyConcept AP, RuleCostPolicyConcept CP, FunctionAnnotationStore DeltaAnnotations>
RuleUpdateResult insert_numeric_update(RuleUpdateInput<Kind, formalism::FunctionTag, AP, CP>& input,
                                       FunctionHeadUpdates& head_updates,
                                       [[maybe_unused]] DeltaAnnotations& delta_numeric_annotations,
                                       std::optional<Cost> required_queue_label = std::nullopt)
{
    auto candidate = evaluate_function_candidate(input.rule_instance, input.annotation_policy, input.cost_policy, input.evaluation, input.workspace);
    if (!candidate)
        return { RuleUpdateStatus::Unavailable, std::nullopt };

    if (required_queue_label && candidate->queue_label != *required_queue_label)
        return { RuleUpdateStatus::QueueLabelChanged, candidate->queue_label };

    if constexpr (!AP::stores_annotations)
    {
        if (candidate->grows_fact)
            head_updates.insert(FunctionHeadUpdate(candidate->head, candidate->interval, true));
    }
    else
    {
        auto staged = false;
        if (input.annotation_policy.can_update(candidate->head, candidate->interval, candidate->cost, input.numeric_annotations, delta_numeric_annotations))
        {
            auto witness = materialize_witness(input.rule_instance, *candidate);
            staged = input.annotation_policy.try_update_candidate(candidate->head, candidate->interval, std::move(witness), delta_numeric_annotations);
        }
        if (staged || candidate->grows_fact)
            head_updates.insert(FunctionHeadUpdate(candidate->head, candidate->interval, candidate->grows_fact));
    }
    return { RuleUpdateStatus::Completed, candidate->queue_label };
}

}

#endif
