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

#ifndef TYR_DATALOG_POLICIES_ANNOTATION_TYPES_HPP_
#define TYR_DATALOG_POLICIES_ANNOTATION_TYPES_HPP_

#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/policies/aggregation.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <algorithm>
#include <cassert>
#include <limits>
#include <optional>
#include <span>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/semantics/comparison.hpp>

namespace tyr::datalog
{

template<TaskKind Kind>
struct NumericSupportKey;

template<TaskKind Kind>
using NumericSupportKeyT = typename NumericSupportKey<Kind>::type;

template<TaskKind Kind>
struct WitnessRuleKey;

template<TaskKind Kind>
using WitnessRuleKeyT = typename WitnessRuleKey<Kind>::type;

template<TaskKind Kind>
struct AnnotationPolicyTypes;

template<TaskKind Kind>
using PredicateAnnotationHeadT = typename AnnotationPolicyTypes<Kind>::PredicateHead;

template<TaskKind Kind>
using FunctionAnnotationHeadT = typename AnnotationPolicyTypes<Kind>::FunctionHead;

template<TaskKind Kind>
struct NumericSupport : ygg::comparison::Mixin<NumericSupport<Kind>>
{
    NumericSupportKeyT<Kind> key;
    ygg::ClosedInterval<ygg::float_t> interval;
    Cost cost;

    NumericSupport() = default;
    NumericSupport(NumericSupportKeyT<Kind> key, ygg::ClosedInterval<ygg::float_t> interval, Cost cost) : key(key), interval(interval), cost(cost) {}

    auto get_key() const noexcept { return key; }
    auto get_interval() const noexcept { return interval; }
    auto get_cost() const noexcept { return cost; }

    auto identifying_members() const noexcept { return std::tie(key, interval, cost); }
};

template<TaskKind Kind>
struct WitnessAnnotation : ygg::comparison::Mixin<WitnessAnnotation<Kind>>
{
    using Metric = ygg::ClosedInterval<ygg::float_t>;
    using NumericSupports = std::vector<NumericSupport<Kind>>;

    WitnessAnnotation(WitnessRuleKeyT<Kind> rule_key, Cost cost);
    WitnessAnnotation(WitnessRuleKeyT<Kind> rule_key, Metric metric, Cost cost);
    WitnessAnnotation(WitnessRuleKeyT<Kind> rule_key, Metric metric, Cost cost, NumericSupports numeric_supports);
    WitnessAnnotation(WitnessRuleKeyT<Kind> rule_key, Metric metric, Cost cost, std::span<const NumericSupport<Kind>> numeric_supports);

    auto get_rule_key() const noexcept { return rule_key; }
    auto get_metric() const noexcept { return metric; }
    auto get_cost() const noexcept { return cost; }
    const auto& get_numeric_supports() const noexcept { return numeric_supports; }

    auto identifying_members() const noexcept { return std::tie(rule_key, metric, cost, numeric_supports); }

private:
    WitnessRuleKeyT<Kind> rule_key;
    Metric metric;
    Cost cost;
    NumericSupports numeric_supports;
};

template<TaskKind Kind>
struct BaseAnnotation : ygg::comparison::Mixin<BaseAnnotation<Kind>>
{
public:
    using Metric = ygg::ClosedInterval<ygg::float_t>;

    BaseAnnotation() : m_metric(), m_cost(Cost(0)) {}
    explicit BaseAnnotation(Cost cost) : m_metric(), m_cost(cost) {}
    BaseAnnotation(Metric metric, Cost cost) : m_metric(metric), m_cost(cost) {}

    auto get_metric() const noexcept { return m_metric; }
    auto get_cost() const noexcept { return m_cost; }

    auto identifying_members() const noexcept { return std::tie(m_metric, m_cost); }

private:
    Metric m_metric;
    Cost m_cost;
};

template<TaskKind Kind>
using Annotation = std::variant<BaseAnnotation<Kind>, WitnessAnnotation<Kind>>;

template<TaskKind Kind>
inline auto get_metric(const Annotation<Kind>& annotation) noexcept
{
    return std::visit([](const auto& value) { return value.get_metric(); }, annotation);
}

template<TaskKind Kind>
inline Cost get_cost(const Annotation<Kind>& annotation) noexcept
{
    return std::visit([](const auto& value) { return value.get_cost(); }, annotation);
}

template<TaskKind Kind>
struct PredicateAnnotationKey
{
    using type = ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag>;
};

template<TaskKind Kind>
using PredicateAnnotationKeyT = typename PredicateAnnotationKey<Kind>::type;

template<TaskKind Kind>
inline PredicateAnnotationKeyT<Kind> get_predicate_annotation_key(PredicateAnnotationKeyT<Kind> binding) noexcept
{
    return binding;
}

template<TaskKind Kind>
inline PredicateAnnotationKeyT<Kind> get_predicate_annotation_key(::tyr::formalism::datalog::GroundAtomView<::tyr::formalism::FluentTag> atom) noexcept
{
    return atom.get_row();
}

template<TaskKind Kind>
class PredicateAnnotationMap
{
public:
    using Key = PredicateAnnotationKeyT<Kind>;

    void clear() noexcept { annotations.clear(); }

    template<typename Binding>
    void insert_or_assign(Binding binding, Annotation<Kind> annotation)
    {
        annotations.insert_or_assign(get_predicate_annotation_key<Kind>(binding), std::move(annotation));
    }

    template<typename Binding>
    const Annotation<Kind>* find(Binding binding) const noexcept
    {
        const auto it = annotations.find(get_predicate_annotation_key<Kind>(binding));
        return it == annotations.end() ? nullptr : &it->second;
    }

    template<typename Binding>
    Annotation<Kind>* find(Binding binding) noexcept
    {
        const auto it = annotations.find(get_predicate_annotation_key<Kind>(binding));
        return it == annotations.end() ? nullptr : &it->second;
    }

private:
    ygg::UnorderedMap<Key, Annotation<Kind>> annotations;
};

template<TaskKind Kind>
using SelectedPredicateAnnotations = PredicateAnnotationMap<Kind>;

template<TaskKind Kind>
struct NumericIntervalAnnotation : ygg::comparison::Mixin<NumericIntervalAnnotation<Kind>>
{
    ygg::ClosedInterval<ygg::float_t> interval;
    Annotation<Kind> annotation;

    NumericIntervalAnnotation() = default;
    NumericIntervalAnnotation(ygg::ClosedInterval<ygg::float_t> interval, Annotation<Kind> annotation) : interval(interval), annotation(std::move(annotation))
    {
    }

    auto identifying_members() const noexcept { return std::make_tuple(get_cost(annotation), interval, annotation); }
};

template<TaskKind Kind>
struct NumericIntervalBindingParts;

template<TaskKind Kind>
class NumericIntervalAnnotations
{
public:
    using Binding = typename NumericIntervalBindingParts<Kind>::Binding;
    using Relation = typename NumericIntervalBindingParts<Kind>::Relation;
    using Key = typename NumericIntervalBindingParts<Kind>::Key;
    using Entry = NumericIntervalAnnotation<Kind>;
    using Entries = std::vector<Entry>;
    using KeyPartitions = ygg::UnorderedMap<Key, Entries>;
    using Partitions = ygg::UnorderedMap<Relation, KeyPartitions>;

    void clear() noexcept
    {
        m_size = 0;
        for (auto& [_, key_partitions] : m_partitions)
            for (auto& [_, entries] : key_partitions)
                entries.clear();
    }

    size_t size() const noexcept { return m_size; }

    const Partitions& partitions() const noexcept { return m_partitions; }

    const Annotation<Kind>* find(Binding binding) const noexcept
    {
        const auto* entries = find_entries(binding);
        return (!entries || entries->empty()) ? nullptr : &entries->back().annotation;
    }

    Annotation<Kind>* find(Binding binding) noexcept
    {
        auto* entries = find_entries(binding);
        return (!entries || entries->empty()) ? nullptr : &entries->back().annotation;
    }

    const Annotation<Kind>* find(Binding binding, ygg::ClosedInterval<ygg::float_t> interval) const noexcept
    {
        const auto* entries = find_entries(binding);
        if (!entries)
            return nullptr;

        for (const auto& entry : *entries)
            if (entry.interval == interval)
                return &entry.annotation;

        return nullptr;
    }

    void insert(Binding binding, ygg::ClosedInterval<ygg::float_t> interval, Annotation<Kind> annotation)
    {
        if (empty(interval))
            return;

        auto entry = Entry { interval, std::move(annotation) };
        auto& entries = m_partitions[NumericIntervalBindingParts<Kind>::get_relation(binding)][NumericIntervalBindingParts<Kind>::get_key(binding)];
        const auto pos = std::upper_bound(entries.begin(), entries.end(), entry);
        if (pos != entries.begin() && *(pos - 1) == entry)
            return;
        entries.insert(pos, std::move(entry));
        ++m_size;
    }

private:
    const Entries* find_entries(Binding binding) const noexcept
    {
        const auto relation_it = m_partitions.find(NumericIntervalBindingParts<Kind>::get_relation(binding));
        if (relation_it == m_partitions.end())
            return nullptr;

        const auto key_it = relation_it->second.find(NumericIntervalBindingParts<Kind>::get_key(binding));
        return key_it == relation_it->second.end() ? nullptr : &key_it->second;
    }

    Entries* find_entries(Binding binding) noexcept
    {
        const auto relation_it = m_partitions.find(NumericIntervalBindingParts<Kind>::get_relation(binding));
        if (relation_it == m_partitions.end())
            return nullptr;

        const auto key_it = relation_it->second.find(NumericIntervalBindingParts<Kind>::get_key(binding));
        return key_it == relation_it->second.end() ? nullptr : &key_it->second;
    }

    Partitions m_partitions;
    size_t m_size = 0;
};

template<TaskKind Kind>
using SelectedFunctionAnnotations = NumericIntervalAnnotations<Kind>;

template<TaskKind Kind>
struct AndAnnotationContext;

inline ygg::ClosedInterval<ygg::float_t> aggregate_metric_support(ygg::ClosedInterval<ygg::float_t> lhs, ygg::ClosedInterval<ygg::float_t> rhs) noexcept
{
    if (empty(lhs))
        return rhs;
    if (empty(rhs))
        return lhs;
    return ygg::ClosedInterval<ygg::float_t>(std::max(lower(lhs), lower(rhs)), std::max(upper(lhs), upper(rhs)));
}

template<TaskKind Kind>
struct CostUpdate
{
    std::optional<Cost> old_cost;
    Cost new_cost;

    CostUpdate() noexcept : old_cost(std::nullopt), new_cost(Cost(0)) { assert(is_monoton()); }
    CostUpdate(std::optional<Cost> old_cost, Cost new_cost) noexcept : old_cost(old_cost), new_cost(new_cost) { assert(is_monoton()); }
    CostUpdate(Cost old_cost, Cost new_cost) noexcept :
        old_cost(old_cost == std::numeric_limits<Cost>::max() ? std::nullopt : std::optional<Cost>(old_cost)),
        new_cost(new_cost)
    {
        assert(is_monoton());
    }

    bool is_monoton() const noexcept { return !old_cost || new_cost <= old_cost.value(); }
};

}

#endif
