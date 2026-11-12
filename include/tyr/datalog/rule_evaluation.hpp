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

#include "tyr/datalog/applicability.hpp"
#include "tyr/datalog/cost_buckets.hpp"
#include "tyr/datalog/policies/annotation_concepts.hpp"
#include "tyr/datalog/policies/annotation_types.hpp"
#include "tyr/datalog/policies/cost.hpp"
#include "tyr/datalog/policies/cost_concepts.hpp"
#include "tyr/datalog/policies/termination_concepts.hpp"
#include "tyr/datalog/rule_instance.hpp"
#include "tyr/formalism/numeric_expression_utils.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <tuple>
#include <type_traits>
#include <urd/elements/flat.hpp>
#include <urd/elements/hash.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <urd/repr/values/indexed.hpp>
#include <urd/repr/values/std_set.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{

/// What a rule instance is evaluated against: the facts, for its numeric constraints, and the costs of the atoms.
class RuleEvaluationInput
{
public:
    RuleEvaluationInput(FactSets fact_sets, const PredicateAnnotations<>& predicate_annotations) noexcept :
        m_fact_sets(fact_sets),
        m_predicate_annotations(&predicate_annotations)
    {
    }

    [[nodiscard]] const FactSets& fact_sets() const noexcept { return m_fact_sets; }
    [[nodiscard]] const PredicateAnnotations<>& predicate_annotations() const noexcept { return *m_predicate_annotations; }

private:
    FactSets m_fact_sets;
    const PredicateAnnotations<>* m_predicate_annotations = nullptr;
};

struct PredicateCandidate
{
    Cost cost;
    Cost queue_label;
};

class PredicateHeadUpdates
{
private:
    using Binding = formalism::datalog::PredicateRowIndex<formalism::FluentTag>;
    using Witness = WitnessAnnotation;

    /// The rows seen per relation; only membership is queried, so the order of bindings stays that of their first insertion.
    urd::Store<urd::sequence<urd::Store<urd::set<urd::index<ygg::Row<formalism::Predicate<formalism::FluentTag>>>>, urd::dense, urd::proportional_clear>>>
        m_seen_rows;
    urd::Store<urd::sequence<Binding>> m_bindings;
    urd::Store<urd::sequence<PredicateAchiever>> m_achievers;

public:
    [[nodiscard]] const urd::Store<urd::sequence<Binding>>& bindings() const noexcept { return m_bindings; }
    /// Mutable: reducing the updates moves the achievers out.
    urd::Store<urd::sequence<PredicateAchiever>>& achievers() noexcept { return m_achievers; }

    void clear() noexcept
    {
        for (const auto binding : m_bindings)
            m_seen_rows[static_cast<ygg::uint_t>(binding.relation.value)].clear();
        m_bindings.clear();
        m_achievers.clear();
    }

    void insert(Binding binding)
    {
        const auto relation = static_cast<size_t>(static_cast<ygg::uint_t>(binding.relation.value));
        if (relation >= m_seen_rows.size())
            m_seen_rows.resize(relation + 1);
        if (m_seen_rows[relation].insert(binding.row).second)
            m_bindings.push_back(binding);
    }

    void insert_achiever(Binding head, Witness witness) { m_achievers.push_back(PredicateAchiever { head, witness }); }
};

template<AnnotationPolicyConcept AP, PredicateAnnotationStore DeltaAnnotations>
void reduce_predicate_head_updates(PredicateHeadUpdates& head_updates,
                                   [[maybe_unused]] AP& annotation_policy,
                                   [[maybe_unused]] const TaggedFactSets<formalism::FluentTag>& facts,
                                   [[maybe_unused]] DeltaAnnotations& delta_annotations,
                                   [[maybe_unused]] PredicateAnnotations<>& annotations,
                                   CostBuckets& cost_buckets,
                                   [[maybe_unused]] PendingPredicateAchievers& pending_achievers)
{
    if constexpr (AP::records_propositional_achievers)
        for (auto& achiever : head_updates.achievers())
            insert_pending_achiever(pending_achievers, achiever);

    for (const auto head : head_updates.bindings())
    {
        if constexpr (AP::stores_annotations)
        {
            const auto* delta_annotation = delta_annotations.find(head);
            if (contains(facts, head))
            {
                assert((!delta_annotation || get_cost(*delta_annotation) >= annotations.fetch_cost(head)) && "A committed predicate label must be final.");
                continue;
            }

            if (!delta_annotation && !annotations.find(head))
                continue;
            cost_buckets.update(annotation_policy.commit_annotation(head, delta_annotations, annotations), head);
        }
        else if (!contains(facts, head))
        {
            cost_buckets.insert(static_cast<Cost>(0), head);
        }
    }
}

template<TaskKind Kind, AnnotationPolicyConcept AP, RuleCostPolicyConcept CP>
struct RuleUpdateInput
{
    RuleInstance<Kind> rule_instance;
    RuleEvaluationInput evaluation;
    AP* annotation_policy = nullptr;
    const CP* cost_policy = nullptr;
};

template<TaskKind Kind, AnnotationPolicyConcept AP, RuleCostPolicyConcept CP>
RuleUpdateInput<Kind, AP, CP> make_rule_update_input(RuleInstance<Kind> rule_instance,
                                                     FactSets fact_sets,
                                                     const PredicateAnnotations<>& predicate_annotations,
                                                     AP& annotation_policy,
                                                     const CP& cost_policy)
{ return { std::move(rule_instance), RuleEvaluationInput { fact_sets, predicate_annotations }, &annotation_policy, &cost_policy }; }

enum class RuleUpdateStatus : std::uint8_t
{
    Unavailable,
    Pruned,
    QueueLabelChanged,
    Completed,
};

struct RuleUpdateResult
{
    RuleUpdateStatus status = RuleUpdateStatus::Unavailable;
    std::optional<Cost> queue_label;

    [[nodiscard]] bool is_handled() const noexcept { return status == RuleUpdateStatus::Pruned || status == RuleUpdateStatus::Completed; }
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
};

template<TaskKind Kind, typename CP>
Cost get_rule_credit(const CP& policy, RuleInstance<Kind>& instance)
{
    if constexpr (std::same_as<std::remove_cvref_t<CP>, RuleCostPolicy>)
        return static_cast<Cost>(0);
    else
        return policy.get_cost(instance.witness_key());
}

/// The cost of the instance's body atoms, where they all have one and its numeric constraints hold on the facts.
template<TaskKind Kind, typename AP, typename CanContinue>
std::optional<EvaluationState> evaluate_annotated_rule(RuleInstance<Kind>& instance, const AP&, const RuleEvaluationInput& input, CanContinue can_continue)
{
    using Aggregation = typename AP::Aggregation;
    const auto aggregate = Aggregation {};
    auto state = EvaluationState { Aggregation::identity(), static_cast<Cost>(instance.get_rule().get_cost()) };

    if (!can_continue(state.support_cost, state.raw_edge))
        return std::nullopt;

    for (const auto literal : instance.get_body().template get_literals<formalism::FluentTag>())
    {
        if (!literal.get_polarity())
            continue;

        const auto binding = instance.resolve_row(literal.get_atom());
        const auto* annotation = input.predicate_annotations().find(binding);
        if (!annotation || get_cost(*annotation) == std::numeric_limits<Cost>::max())
            return std::nullopt;

        state.support_cost = aggregate(state.support_cost, get_cost(*annotation));
        if (!can_continue(state.support_cost, state.raw_edge))
            return std::nullopt;
    }

    for (const auto constraint : instance.get_body().get_numeric_constraints())
        if (!evaluate(instance.resolve(constraint), input.fact_sets()))
            return std::nullopt;

    return state;
}

}

template<TaskKind Kind, typename AP, typename CP>
std::optional<Cost> evaluate_rule_priority(RuleInstance<Kind>& instance, const AP& annotation_policy, const CP& cost_policy, const RuleEvaluationInput& input)
{
    if constexpr (!AP::stores_annotations)
    {
        return static_cast<Cost>(0);
    }
    else
    {
        const auto state = rule_evaluation_detail::evaluate_annotated_rule(instance, annotation_policy, input, rule_evaluation_detail::AlwaysContinue {});
        if (!state)
            return std::nullopt;
        return state->support_cost + reduce_cost(state->raw_edge, rule_evaluation_detail::get_rule_credit(cost_policy, instance));
    }
}

template<TaskKind Kind, typename AP, typename CP, typename CanContinue = rule_evaluation_detail::AlwaysContinue>
std::optional<PredicateCandidate> evaluate_predicate_candidate(RuleInstance<Kind>& instance,
                                                               const AP& annotation_policy,
                                                               const CP& cost_policy,
                                                               const RuleEvaluationInput& input,
                                                               CanContinue can_continue = {})
{
    if constexpr (!AP::stores_annotations)
    {
        return PredicateCandidate { static_cast<Cost>(0), static_cast<Cost>(0) };
    }
    else
    {
        const auto state = rule_evaluation_detail::evaluate_annotated_rule(instance, annotation_policy, input, can_continue);
        if (!state)
            return std::nullopt;

        const auto cost = state->support_cost + reduce_cost(state->raw_edge, rule_evaluation_detail::get_rule_credit(cost_policy, instance));
        return PredicateCandidate { cost, cost };
    }
}

template<TaskKind Kind, typename AP, typename CP>
auto evaluate_propositional_candidate(RuleInstance<Kind>& rule_instance,
                                      const AP& annotation_policy,
                                      const CP& cost_policy,
                                      const RuleEvaluationInput& input,
                                      Cost best_cost)
{
    auto pruned = false;
    if constexpr (AP::stores_annotations && !AP::records_propositional_achievers && std::same_as<CP, RuleCostPolicy>)
    {
        auto candidate = evaluate_predicate_candidate(rule_instance,
                                                      annotation_policy,
                                                      cost_policy,
                                                      input,
                                                      [&](const Cost support_cost, const Cost edge)
                                                      {
                                                          pruned = support_cost + edge >= best_cost;
                                                          return !pruned;
                                                      });
        return std::pair(std::move(candidate), pruned);
    }
    else
    {
        return std::pair(evaluate_predicate_candidate(rule_instance, annotation_policy, cost_policy, input), false);
    }
}

/// Prunes only once the instance cannot improve any of its heads.
template<TaskKind Kind, typename AP, typename CP, PredicateAnnotationStore DeltaAnnotations>
auto evaluate_propositional_candidate(std::span<const formalism::datalog::PredicateRowIndex<formalism::FluentTag>> heads,
                                      RuleInstance<Kind>& rule_instance,
                                      const AP& annotation_policy,
                                      const CP& cost_policy,
                                      const RuleEvaluationInput& input,
                                      const DeltaAnnotations& delta_annotations)
{
    auto best_cost = static_cast<Cost>(0);
    for (const auto head : heads)
        best_cost = std::max(best_cost, std::min(input.predicate_annotations().fetch_cost(head), delta_annotations.fetch_cost(head)));
    return evaluate_propositional_candidate(rule_instance, annotation_policy, cost_policy, input, best_cost);
}

template<TaskKind Kind, typename Candidate>
WitnessAnnotation materialize_witness(RuleInstance<Kind>& instance, const Candidate& candidate)
{ return WitnessAnnotation(instance.witness_key().data(), candidate.cost); }

/// Evaluates the rule instance once and updates each of its heads with the shared witness.
template<TaskKind Kind, AnnotationPolicyConcept AP, RuleCostPolicyConcept CP, PredicateAnnotationStore DeltaAnnotations>
RuleUpdateResult insert_propositional_update(std::span<const formalism::datalog::PredicateRowIndex<formalism::FluentTag>> heads,
                                             RuleUpdateInput<Kind, AP, CP>& input,
                                             PredicateHeadUpdates& head_updates,
                                             DeltaAnnotations& delta_annotations,
                                             std::optional<Cost> required_queue_label = std::nullopt)
{
    assert(!heads.empty());
    auto [candidate, pruned] =
        evaluate_propositional_candidate(heads, input.rule_instance, *input.annotation_policy, *input.cost_policy, input.evaluation, delta_annotations);
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
            const auto can_update = input.annotation_policy->can_update(head, candidate->cost, input.evaluation.predicate_annotations(), delta_annotations);
            if constexpr (AP::records_propositional_achievers)
            {
                auto witness = materialize_witness(input.rule_instance, *candidate);
                if (can_update)
                {
                    head_updates.insert_achiever(head, witness);
                    input.annotation_policy->try_update_candidate(head, std::move(witness), delta_annotations);
                }
                else
                {
                    head_updates.insert_achiever(head, std::move(witness));
                }
            }
            else if (can_update)
            {
                input.annotation_policy->try_update_candidate(head, materialize_witness(input.rule_instance, *candidate), delta_annotations);
            }
        }
    }
    // A previously staged certificate may still need to become an available fact after a resumed solve.
    for (const auto head : heads)
        head_updates.insert(head);
    return { RuleUpdateStatus::Completed, candidate->queue_label };
}

}

#endif
