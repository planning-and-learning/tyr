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

namespace tyr::datalog
{

/// Zero-cost conservative reachability: solvers ignore annotation-only metric effects and cost credits.
class NoAnnotationPolicy
{
public:
    using PredicateHead = ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag>;
    using FunctionBinding = ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag>;

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
    using PredicateHead = ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag>;
    using FunctionBinding = ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag>;
    using PredicateWitness = WitnessAnnotation<::tyr::formalism::PredicateTag>;
    using FunctionWitness = WitnessAnnotation<::tyr::formalism::FunctionTag>;

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

    void record_achiever(PredicateHead, const PredicateWitness&) const noexcept {}

    template<bool ThreadSafe>
    std::optional<CostUpdate> publish_annotation(PredicateHead head, PredicateWitness witness, PredicateAnnotations<ThreadSafe>& annotations) const
    {
        return annotations.insert_if_better(head, Annotation<>(std::move(witness)));
    }

    template<bool ThreadSafe>
    bool try_update_candidate(PredicateHead head, PredicateWitness&& witness, PredicateAnnotations<ThreadSafe>& annotations) const
    {
        return publish_annotation(head, std::move(witness), annotations).has_value();
    }

    template<bool ThreadSafe>
    bool try_update_candidate(FunctionBinding head,
                              ygg::ClosedInterval<ygg::float_t> interval,
                              FunctionWitness&& witness,
                              FunctionAnnotations<ThreadSafe>& numeric_annotations) const
    {
        return numeric_annotations.insert(head, interval, Annotation<::tyr::formalism::FunctionTag>(std::move(witness)));
    }

    bool
    can_update(PredicateHead head, Cost cost, const PredicateAnnotations<>& annotations, const PredicateAnnotations<true>& delta_annotations) const noexcept;

    bool can_update(FunctionBinding head,
                    ygg::ClosedInterval<ygg::float_t> interval,
                    Cost cost,
                    const FunctionAnnotations<>& numeric_annotations,
                    const FunctionAnnotations<true>& delta_numeric_annotations) const noexcept;

    CostUpdate commit_annotation(PredicateHead head, const PredicateAnnotations<true>& delta_annotations, PredicateAnnotations<>& annotations) const;
};

template<typename AggregationFunction>
class MinCostAnnotationWithAchieversPolicy : public MinCostAnnotationPolicy<AggregationFunction>
{
public:
    using PredicateHead = ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag>;
    using PredicateWitness = WitnessAnnotation<::tyr::formalism::PredicateTag>;
    using Achievers = std::vector<PredicateWitness>;

    static constexpr bool records_propositional_achievers = true;

    void initialize(size_t num_predicates) { m_achievers.initialize(num_predicates); }

    void clear_achievers() noexcept;

    const Achievers* find_achievers(PredicateHead head) const noexcept;

    void record_achiever(PredicateHead head, const PredicateWitness& witness);

private:
    DenseRelationMap<::tyr::formalism::PredicateTag, Achievers> m_achievers;
};

}

#endif
