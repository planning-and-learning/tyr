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
#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/policies/annotation_concepts.hpp"
#include "tyr/datalog/policies/annotation_types.hpp"
#include "tyr/datalog/policies/cost.hpp"
#include "tyr/datalog/policies/cost_concepts.hpp"
#include "tyr/datalog/policies/termination_concepts.hpp"
#include "tyr/datalog/rule_instance.hpp"
#include "tyr/formalism/numeric_expression_utils.hpp"

#include <algorithm>
#include <boost/mp11.hpp>
#include <cassert>
#include <concepts>
#include <cstdint>
#include <limits>
#include <optional>
#include <ranges>
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
template<TaskKind Kind>
class RuleEvaluationInput
{
public:
    RuleEvaluationInput(FactSets fact_sets, const PredicateAnnotations<Kind>& predicate_annotations) noexcept :
        m_fact_sets(fact_sets),
        m_predicate_annotations(&predicate_annotations)
    {
    }

    [[nodiscard]] const FactSets& fact_sets() const noexcept { return m_fact_sets; }

    [[nodiscard]] const PredicateAnnotations<Kind>& predicate_annotations() const noexcept { return *m_predicate_annotations; }

private:
    FactSets m_fact_sets;
    const PredicateAnnotations<Kind>* m_predicate_annotations = nullptr;
};

struct PredicateCandidate
{
    Cost cost;
    Cost queue_label;
};

/// The heads of kind F that rule instances updated in one iteration, each once, and their achievers.
template<TaskKind Kind, formalism::FactKind F>
class PredicateHeadUpdates
{
private:
    using Binding = formalism::PredicateRowKey<F>;
    using Witness = WitnessAnnotation<Kind>;

    /// The rows seen per relation; only membership is queried, so the order of bindings stays that of their first insertion.
    urd::Store<urd::sequence<urd::Store<urd::set<urd::index<ygg::Row<formalism::Predicate<F>>>>, urd::dense, urd::proportional_clear>>> m_seen_rows;
    urd::Store<urd::sequence<Binding>> m_bindings;
    urd::Store<urd::sequence<PredicateAchiever<Kind, F>>> m_achievers;

public:
    [[nodiscard]] const urd::Store<urd::sequence<Binding>>& bindings() const noexcept { return m_bindings; }

    /// Mutable: reducing the updates moves the achievers out.
    urd::Store<urd::sequence<PredicateAchiever<Kind, F>>>& achievers() noexcept { return m_achievers; }

    void clear() noexcept
    {
        for (const auto binding : m_bindings)
            m_seen_rows[static_cast<ygg::uint_t>(binding.template get<formalism::Predicate<F>>().value)].clear();
        m_bindings.clear();
        m_achievers.clear();
    }

    void insert(Binding binding)
    {
        const auto relation = static_cast<size_t>(static_cast<ygg::uint_t>(binding.template get<formalism::Predicate<F>>().value));
        if (relation >= m_seen_rows.size())
            m_seen_rows.resize(relation + 1);
        if (m_seen_rows[relation].insert(binding.template get<ygg::Row<formalism::Predicate<F>>>()).second)
            m_bindings.push_back(binding);
    }

    void insert_achiever(Binding head, Witness witness) { m_achievers.push_back(PredicateAchiever<Kind, F> { head, witness }); }
};

template<TaskKind Kind>
struct PredicateHeadUpdatesOf
{
    template<typename F>
    using type = PredicateHeadUpdates<Kind, F>;
};

/// The head updates of each mutable fact kind.
template<TaskKind Kind>
struct PredicateHeadUpdateFamily : MutableFamily<PredicateHeadUpdatesOf<Kind>::template type>
{
};

template<TaskKind Kind, AnnotationPolicyConcept AP, PredicateAnnotationStore DeltaAnnotations>
void reduce_predicate_head_updates(PredicateHeadUpdateFamily<Kind>& head_update_family,
                                   [[maybe_unused]] AP& annotation_policy,
                                   const FactSets& facts,
                                   [[maybe_unused]] DeltaAnnotations& delta_annotations,
                                   [[maybe_unused]] PredicateAnnotations<Kind>& annotations,
                                   CostBuckets& cost_buckets,
                                   [[maybe_unused]] PendingPredicateAchieverFamily<Kind>& pending_achievers)
{
    boost::mp11::mp_for_each<MutableTags>(
        [&]<typename F>(F)
        {
            auto& head_updates = head_update_family.template get<F>();
            const auto& fact_sets = facts.template get<F>();
            if constexpr (AP::records_propositional_achievers)
                for (auto& achiever : head_updates.achievers())
                    insert_pending_achiever(pending_achievers, achiever);

            for (const auto head : head_updates.bindings())
            {
                if constexpr (AP::stores_annotations)
                {
                    const auto* delta_annotation = delta_annotations.find(head);
                    if (contains(fact_sets, head))
                    {
                        assert((!delta_annotation || get_cost(*delta_annotation) >= annotations.fetch_cost(head))
                               && "A committed predicate label must be final.");
                        continue;
                    }

                    if (!delta_annotation && !annotations.find(head))
                        continue;
                    cost_buckets.update(annotation_policy.commit_annotation(head, delta_annotations, annotations), head);
                }
                else if (!contains(fact_sets, head))
                {
                    cost_buckets.insert(static_cast<Cost>(0), head);
                }
            }
        });
}

template<TaskKind Kind, AnnotationPolicyConcept AP, RuleCostPolicyConcept CP>
struct RuleUpdateInput
{
    RuleInstance<Kind> rule_instance;
    RuleEvaluationInput<Kind> evaluation;
    AP* annotation_policy = nullptr;
    const CP* cost_policy = nullptr;
};

template<TaskKind Kind, AnnotationPolicyConcept AP, RuleCostPolicyConcept CP>
RuleUpdateInput<Kind, AP, CP> make_rule_update_input(RuleInstance<Kind> rule_instance,
                                                     FactSets fact_sets,
                                                     const PredicateAnnotations<Kind>& predicate_annotations,
                                                     AP& annotation_policy,
                                                     const CP& cost_policy)
{ return { std::move(rule_instance), RuleEvaluationInput<Kind> { fact_sets, predicate_annotations }, &annotation_policy, &cost_policy }; }

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
        return policy.get_cost(instance);
}

/// The cost of the instance's body atoms, where they all have one and its numeric constraints hold on the facts.
template<TaskKind Kind, typename AP, typename CanContinue>
std::optional<EvaluationState>
evaluate_annotated_rule(RuleInstance<Kind>& instance, const AP&, const RuleEvaluationInput<Kind>& input, CanContinue can_continue)
{
    using Aggregation = typename AP::Aggregation;
    const auto aggregate = Aggregation {};
    auto state = EvaluationState { Aggregation::identity(), static_cast<Cost>(instance.get_rule().get_cost()) };

    if (!can_continue(state.support_cost, state.raw_edge))
        return std::nullopt;

    auto supported = true;
    boost::mp11::mp_for_each<MutableTags>(
        [&]<typename F>(F)
        {
            for (const auto literal : instance.get_body().template get_literals<F>())
            {
                if (!supported)
                    return;
                if (!literal.get_polarity())
                    continue;

                const auto binding = instance.resolve(literal.get_atom()).index();
                const auto* annotation = input.predicate_annotations().find(binding);
                if (!annotation || get_cost(*annotation) == std::numeric_limits<Cost>::max())
                {
                    supported = false;
                    return;
                }

                state.support_cost = aggregate(state.support_cost, get_cost(*annotation));
                supported = can_continue(state.support_cost, state.raw_edge);
            }
        });
    if (!supported)
        return std::nullopt;

    for (const auto constraint : instance.get_body().get_numeric_constraints())
        if (!evaluate(instance.resolve(constraint), input.fact_sets()))
            return std::nullopt;

    return state;
}

}

template<TaskKind Kind, typename AP, typename CP>
std::optional<Cost>
evaluate_rule_priority(RuleInstance<Kind>& instance, const AP& annotation_policy, const CP& cost_policy, const RuleEvaluationInput<Kind>& input)
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
                                                               const RuleEvaluationInput<Kind>& input,
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
                                      const RuleEvaluationInput<Kind>& input,
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

/// The heads of kind F of a rule instance as the keys of its predicate annotations, a range read more than once.
template<typename Range, typename F>
concept PredicateRowRange = std::ranges::forward_range<Range> && std::same_as<std::ranges::range_value_t<Range>, formalism::PredicateRowKey<F>>;

template<typename R, typename F>
concept PredicateHeadsOf = requires(const R& heads) {
    { heads.template get<F>() } -> PredicateRowRange<F>;
};

template<typename R, typename Tags>
inline constexpr bool is_predicate_heads_for_all = false;

template<typename R, typename... Fs>
inline constexpr bool is_predicate_heads_for_all<R, boost::mp11::mp_list<Fs...>> = (PredicateHeadsOf<R, Fs> && ...);

/// The heads of a rule instance, a range of them for each mutable fact kind (get<F>()).
template<typename R>
concept PredicateHeads = is_predicate_heads_for_all<R, MutableTags>;

/// Whether the rule instance has no head of any kind.
template<PredicateHeads Heads>
bool has_no_heads(const Heads& heads)
{
    auto result = true;
    boost::mp11::mp_for_each<MutableTags>([&]<typename F>(F) { result = result && std::ranges::empty(heads.template get<F>()); });
    return result;
}

/// The cost below which a candidate improves some head of the instance: each head's best cost, committed or staged,
/// the most of them over the heads of every kind. A candidate at or above it improves none.
template<TaskKind Kind, PredicateAnnotationStore DeltaAnnotations, PredicateHeads Heads>
Cost best_head_cost(const Heads& heads, const RuleEvaluationInput<Kind>& input, const DeltaAnnotations& delta_annotations)
{
    auto best_cost = static_cast<Cost>(0);
    boost::mp11::mp_for_each<MutableTags>(
        [&]<typename F>(F)
        {
            for (const auto head : heads.template get<F>())
                best_cost = std::max(best_cost, std::min(input.predicate_annotations().fetch_cost(head), delta_annotations.fetch_cost(head)));
        });
    return best_cost;
}

/// Prunes only once the instance cannot improve any of its heads.
template<TaskKind Kind, typename AP, typename CP, PredicateAnnotationStore DeltaAnnotations, PredicateHeads Heads>
auto evaluate_propositional_candidate(const Heads& heads,
                                      RuleInstance<Kind>& rule_instance,
                                      const AP& annotation_policy,
                                      const CP& cost_policy,
                                      const RuleEvaluationInput<Kind>& input,
                                      const DeltaAnnotations& delta_annotations)
{ return evaluate_propositional_candidate(rule_instance, annotation_policy, cost_policy, input, best_head_cost(heads, input, delta_annotations)); }

template<TaskKind Kind, typename Candidate>
WitnessAnnotation<Kind> materialize_witness(RuleInstance<Kind>& instance, const Candidate& candidate)
{ return WitnessAnnotation<Kind>(instance.witness_key(), candidate.cost); }

/// Updates the heads of kind F of the rule instance with its candidate, the cost every head of the instance shares, and
/// records each one as updated.
template<TaskKind Kind,
         formalism::FactKind F,
         AnnotationPolicyConcept AP,
         RuleCostPolicyConcept CP,
         PredicateAnnotationStore DeltaAnnotations,
         PredicateRowRange<F> Heads>
void apply_propositional_candidate_to(const Heads& heads,
                                      RuleUpdateInput<Kind, AP, CP>& input,
                                      PredicateHeadUpdates<Kind, F>& head_updates,
                                      DeltaAnnotations& delta_annotations,
                                      const PredicateCandidate& candidate)
{
    if constexpr (AP::stores_annotations)
    {
        for (const auto head : heads)
        {
            const auto can_update = input.annotation_policy->can_update(head, candidate.cost, input.evaluation.predicate_annotations(), delta_annotations);
            if constexpr (AP::records_propositional_achievers)
            {
                auto witness = materialize_witness(input.rule_instance, candidate);
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
                input.annotation_policy->try_update_candidate(head, materialize_witness(input.rule_instance, candidate), delta_annotations);
            }
        }
    }
    // A previously staged certificate may still need to become an available fact after a resumed solve.
    for (const auto head : heads)
        head_updates.insert(head);
}

/// Updates each head of the rule instance, of every kind, with its candidate (none where it is pruned or not available),
/// sharing one witness.
template<TaskKind Kind, AnnotationPolicyConcept AP, RuleCostPolicyConcept CP, PredicateAnnotationStore DeltaAnnotations, PredicateHeads Heads>
RuleUpdateResult apply_propositional_candidate(const Heads& heads,
                                               RuleUpdateInput<Kind, AP, CP>& input,
                                               PredicateHeadUpdateFamily<Kind>& head_updates,
                                               DeltaAnnotations& delta_annotations,
                                               const std::optional<PredicateCandidate>& candidate,
                                               bool pruned,
                                               std::optional<Cost> required_queue_label)
{
    if (!candidate)
    {
        if (!pruned)
            return { RuleUpdateStatus::Unavailable, std::nullopt };

        // A staged annotation can still need its head restored to the cost frontier after resume.
        boost::mp11::mp_for_each<MutableTags>(
            [&]<typename F>(F)
            {
                for (const auto head : heads.template get<F>())
                    head_updates.template get<F>().insert(head);
            });
        return { RuleUpdateStatus::Pruned, std::nullopt };
    }

    if (required_queue_label && candidate->queue_label != *required_queue_label)
        return { RuleUpdateStatus::QueueLabelChanged, candidate->queue_label };

    boost::mp11::mp_for_each<MutableTags>(
        [&]<typename F>(F)
        { apply_propositional_candidate_to(heads.template get<F>(), input, head_updates.template get<F>(), delta_annotations, *candidate); });
    return { RuleUpdateStatus::Completed, candidate->queue_label };
}

/// Evaluates the rule instance once and updates each of its heads with the shared witness.
template<TaskKind Kind, AnnotationPolicyConcept AP, RuleCostPolicyConcept CP, PredicateAnnotationStore DeltaAnnotations, PredicateHeads Heads>
RuleUpdateResult insert_propositional_update(const Heads& heads,
                                             RuleUpdateInput<Kind, AP, CP>& input,
                                             PredicateHeadUpdateFamily<Kind>& head_updates,
                                             DeltaAnnotations& delta_annotations,
                                             std::optional<Cost> required_queue_label = std::nullopt)
{
    assert(!has_no_heads(heads));
    const auto [candidate, pruned] =
        evaluate_propositional_candidate(heads, input.rule_instance, *input.annotation_policy, *input.cost_policy, input.evaluation, delta_annotations);
    return apply_propositional_candidate(heads, input, head_updates, delta_annotations, candidate, pruned, required_queue_label);
}

/// Fires a rule instance whose conditions are all committed: its cost is `support`, the aggregate of their costs, plus
/// its edge cost (less its credit), so its body is not read; pruned, as insert_propositional_update prunes, where it
/// improves no head.
template<TaskKind Kind, AnnotationPolicyConcept AP, RuleCostPolicyConcept CP, PredicateAnnotationStore DeltaAnnotations, PredicateHeads Heads>
RuleUpdateResult fire_propositional_update(const Heads& heads,
                                           RuleUpdateInput<Kind, AP, CP>& input,
                                           PredicateHeadUpdateFamily<Kind>& head_updates,
                                           DeltaAnnotations& delta_annotations,
                                           Cost support)
{
    assert(!has_no_heads(heads));
    if constexpr (!AP::stores_annotations)
    {
        return apply_propositional_candidate(heads, input, head_updates, delta_annotations, PredicateCandidate { 0, 0 }, false, std::nullopt);
    }
    else
    {
        const auto edge = static_cast<Cost>(input.rule_instance.get_rule().get_cost());
        auto pruned = false;
        if constexpr (!AP::records_propositional_achievers && std::same_as<CP, RuleCostPolicy>)
            pruned = support + edge >= best_head_cost(heads, input.evaluation, delta_annotations);
        if (pruned)
            return apply_propositional_candidate(heads, input, head_updates, delta_annotations, std::nullopt, true, std::nullopt);
        const auto cost = support + reduce_cost(edge, rule_evaluation_detail::get_rule_credit(*input.cost_policy, input.rule_instance));
        return apply_propositional_candidate(heads, input, head_updates, delta_annotations, PredicateCandidate { cost, cost }, false, std::nullopt);
    }
}

}

#endif
