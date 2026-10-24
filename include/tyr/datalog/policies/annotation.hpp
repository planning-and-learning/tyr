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
#include <utility>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>

namespace tyr::datalog
{

/// Zero-cost conservative reachability: solvers ignore annotation-only metric effects and cost credits.
class NoAnnotationPolicy
{
    using PredicateHead = formalism::datalog::PredicateBindingIndex<formalism::FluentTag>;
    using FunctionBinding = formalism::datalog::FunctionBindingIndex<formalism::FluentTag>;

public:
    static constexpr bool stores_annotations = false;
    static constexpr bool records_propositional_achievers = false;

    bool is_widening_label_preserving(Cost, Cost) const noexcept { return true; }

    void initialize_annotation(PredicateHead, PredicateAnnotations<>&) const noexcept {}
    void initialize_annotation(FunctionBinding, ygg::ClosedInterval<ygg::float_t>, FunctionAnnotations<>&) const noexcept {}

    void clear_achievers() noexcept {}
};

template<typename AggregationFunction>
class MinCostAnnotationPolicy
{
public:
    using Aggregation = AggregationFunction;

private:
    using PredicateHead = formalism::datalog::PredicateBindingIndex<formalism::FluentTag>;
    using FunctionBinding = formalism::datalog::FunctionBindingIndex<formalism::FluentTag>;
    using PredicateWitness = WitnessAnnotation<formalism::PredicateTag>;
    using FunctionWitness = WitnessAnnotation<formalism::FunctionTag>;

public:
    static constexpr bool stores_annotations = true;
    static constexpr bool records_propositional_achievers = false;

    bool is_widening_label_preserving(Cost candidate_label, Cost current_target_label) const noexcept
    {
        if constexpr (std::same_as<AggregationFunction, MaxAggregation>)
            return true;
        return candidate_label == current_target_label;
    }

    void initialize_annotation(PredicateHead head, PredicateAnnotations<>& annotations) const;

    void initialize_annotation(FunctionBinding head, ygg::ClosedInterval<ygg::float_t> interval, FunctionAnnotations<>& numeric_annotations) const;

    void clear_achievers() noexcept {}

    void record_achiever(PredicateHead, PredicateWitness) const noexcept {}

    template<PredicateAnnotationStore Annotations>
    std::optional<CostUpdate> publish_annotation(PredicateHead head, PredicateWitness witness, Annotations& annotations) const
    { return annotations.insert_if_better(head, Annotation<>(std::move(witness))); }

    template<PredicateAnnotationStore Annotations>
    bool try_update_candidate(PredicateHead head, PredicateWitness&& witness, Annotations& annotations) const
    { return publish_annotation(head, std::move(witness), annotations).has_value(); }

    template<FunctionAnnotationStore Annotations>
    bool
    try_update_candidate(FunctionBinding head, ygg::ClosedInterval<ygg::float_t> interval, FunctionWitness&& witness, Annotations& numeric_annotations) const
    { return numeric_annotations.insert(head, interval, Annotation<formalism::FunctionTag>(std::move(witness))); }

    template<PredicateAnnotationStore DeltaAnnotations>
    bool can_update(PredicateHead head, Cost cost, const PredicateAnnotations<>& annotations, const DeltaAnnotations& delta_annotations) const noexcept
    { return cost < std::min(annotations.fetch_cost(head), delta_annotations.fetch_cost(head)); }

    template<FunctionAnnotationStore DeltaAnnotations>
    bool can_update(FunctionBinding head,
                    ygg::ClosedInterval<ygg::float_t> interval,
                    Cost cost,
                    const FunctionAnnotations<>& numeric_annotations,
                    const DeltaAnnotations& delta_numeric_annotations) const noexcept
    { return cost < std::min(numeric_annotations.fetch_cost(head, interval), delta_numeric_annotations.fetch_cost(head, interval)); }

    template<PredicateAnnotationStore DeltaAnnotations>
    CostUpdate commit_annotation(PredicateHead head, const DeltaAnnotations& delta_annotations, PredicateAnnotations<>& annotations) const
    {
        const auto old_cost = annotations.fetch_cost(head);
        if (const auto* delta_annotation = delta_annotations.find(head))
            if (auto update = annotations.insert_if_better(head, *delta_annotation))
                return *update;
        return CostUpdate(old_cost, old_cost);
    }

    template<FunctionAnnotationStore DeltaAnnotations>
    CostUpdate commit_annotation(FunctionBinding head,
                                 ygg::ClosedInterval<ygg::float_t> interval,
                                 const DeltaAnnotations& delta_numeric_annotations,
                                 FunctionAnnotations<>& numeric_annotations) const
    {
        const auto old_cost = numeric_annotations.fetch_cost(head, interval);
        const auto* delta_annotation = delta_numeric_annotations.find(head, interval);
        if (delta_annotation && numeric_annotations.insert(head, interval, *delta_annotation))
            return CostUpdate(old_cost, get_cost(*delta_annotation));
        return CostUpdate(old_cost, old_cost);
    }
};

template<typename AggregationFunction>
class MinCostAnnotationWithAchieversPolicy : public MinCostAnnotationPolicy<AggregationFunction>
{
    using PredicateHead = formalism::datalog::PredicateBindingIndex<formalism::FluentTag>;
    using PredicateWitness = WitnessAnnotation<formalism::PredicateTag>;

public:
    using Achievers = std::vector<PredicateWitness>;

    static constexpr bool records_propositional_achievers = true;

    void initialize(size_t num_predicates) { m_achievers.initialize(num_predicates); }

    void clear_achievers() noexcept;

    const Achievers* find_achievers(PredicateHead head) const noexcept;

    void record_achiever(PredicateHead head, PredicateWitness witness);

private:
    using RuleBinding = formalism::datalog::RuleBindingIndex<formalism::PredicateTag>;

    struct IndexedAchievers
    {
        Achievers achievers;
        ygg::UnorderedMap<RuleBinding, size_t> indices;
    };

    DenseRelationMap<formalism::PredicateTag, IndexedAchievers> m_achievers;
};
}

#endif
