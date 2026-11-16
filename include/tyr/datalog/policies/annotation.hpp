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

#ifndef TYR_DATALOG_POLICIES_ANNOTATION_HPP_
#define TYR_DATALOG_POLICIES_ANNOTATION_HPP_

#include "tyr/datalog/policies/aggregation.hpp"
#include "tyr/datalog/policies/annotation_types.hpp"

#include <concepts>
#include <optional>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace tyr::datalog
{

/// Zero-cost conservative reachability: solvers ignore annotation-only metric effects and cost credits.
class NoAnnotationPolicy
{
    using PredicateHead = formalism::datalog::PredicateRowIndex<formalism::FluentTag>;

public:
    static constexpr bool stores_annotations = false;
    static constexpr bool records_propositional_achievers = false;

    template<TaskKind Kind>
    void initialize_annotation(PredicateHead, PredicateAnnotations<Kind>&) const noexcept
    {
    }

    void clear_achievers() noexcept {}
};

/// Keyless: its member templates take the witnesses and annotations of either engine (Kind), so one policy type serves
/// both; only MinCostAnnotationWithAchieversPolicy stores rule keys and so names its engine.
template<typename AggregationFunction>
class MinCostAnnotationPolicy
{
public:
    using Aggregation = AggregationFunction;

private:
    using PredicateHead = formalism::datalog::PredicateRowIndex<formalism::FluentTag>;

public:
    static constexpr bool stores_annotations = true;
    static constexpr bool records_propositional_achievers = false;

    template<TaskKind Kind>
    void initialize_annotation(PredicateHead head, PredicateAnnotations<Kind>& annotations) const
    { annotations.insert_or_assign(head, BaseAnnotation(static_cast<Cost>(0))); }

    void clear_achievers() noexcept {}

    template<TaskKind Kind>
    void record_achiever(PredicateHead, const WitnessAnnotation<Kind>&) const noexcept
    {
    }

    template<TaskKind Kind, PredicateAnnotationStore Annotations>
    std::optional<CostUpdate> publish_annotation(PredicateHead head, WitnessAnnotation<Kind> witness, Annotations& annotations) const
    { return annotations.insert_if_better(head, Annotation<Kind>(witness)); }

    template<TaskKind Kind, PredicateAnnotationStore Annotations>
    bool try_update_candidate(PredicateHead head, WitnessAnnotation<Kind>&& witness, Annotations& annotations) const
    { return publish_annotation(head, std::move(witness), annotations).has_value(); }

    template<TaskKind Kind, PredicateAnnotationStore DeltaAnnotations>
    [[nodiscard]] bool
    can_update(PredicateHead head, Cost cost, const PredicateAnnotations<Kind>& annotations, const DeltaAnnotations& delta_annotations) const noexcept
    { return cost < std::min(annotations.fetch_cost(head), delta_annotations.fetch_cost(head)); }

    template<TaskKind Kind, PredicateAnnotationStore DeltaAnnotations>
    CostUpdate commit_annotation(PredicateHead head, const DeltaAnnotations& delta_annotations, PredicateAnnotations<Kind>& annotations) const
    {
        const auto old_cost = annotations.fetch_cost(head);
        if (const auto* delta_annotation = delta_annotations.find(head))
            if (auto update = annotations.insert_if_better(head, *delta_annotation))
                return *update;
        return CostUpdate(old_cost, old_cost);
    }
};

template<TaskKind Kind, typename AggregationFunction>
class MinCostAnnotationWithAchieversPolicy : public MinCostAnnotationPolicy<AggregationFunction>
{
    using PredicateHead = formalism::datalog::PredicateRowIndex<formalism::FluentTag>;
    using PredicateWitness = WitnessAnnotation<Kind>;

public:
    using Achievers = urd::Store<urd::sequence<PredicateWitness>>;

    static constexpr bool records_propositional_achievers = true;

    void initialize(size_t num_predicates) { m_achievers.initialize(num_predicates); }

    void clear_achievers() noexcept;

    [[nodiscard]] const Achievers* find_achievers(PredicateHead head) const noexcept;

    void record_achiever(PredicateHead head, PredicateWitness witness);

private:
    struct IndexedAchievers
    {
        Achievers achievers;
        urd::Store<urd::map<RuleKey<Kind>, size_t>> indices;
    };

    DenseRelationMap<formalism::PredicateTag, IndexedAchievers> m_achievers;
};
}

#endif
