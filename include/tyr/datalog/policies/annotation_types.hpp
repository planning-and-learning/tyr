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

template<TaskKind Kind, ::tyr::formalism::RelationKind R>
struct WitnessRuleKey;

template<TaskKind Kind, ::tyr::formalism::RelationKind R>
using WitnessRuleKeyT = typename WitnessRuleKey<Kind, R>::type;

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

template<TaskKind Kind, ::tyr::formalism::RelationKind R = ::tyr::formalism::PredicateTag>
struct WitnessAnnotation : ygg::comparison::Mixin<WitnessAnnotation<Kind, R>>
{
    using Relation = R;
    using Metric = ygg::ClosedInterval<ygg::float_t>;
    using NumericSupports = std::vector<NumericSupport<Kind>>;

    WitnessAnnotation(WitnessRuleKeyT<Kind, R> rule_key, Cost cost);
    WitnessAnnotation(WitnessRuleKeyT<Kind, R> rule_key, Metric metric, Cost cost);
    WitnessAnnotation(WitnessRuleKeyT<Kind, R> rule_key, Metric metric, Cost cost, NumericSupports numeric_supports);
    WitnessAnnotation(WitnessRuleKeyT<Kind, R> rule_key, Metric metric, Cost cost, std::span<const NumericSupport<Kind>> numeric_supports);

    auto get_rule_key() const noexcept { return rule_key; }
    auto get_metric() const noexcept { return metric; }
    auto get_cost() const noexcept { return cost; }
    const auto& get_numeric_supports() const noexcept { return numeric_supports; }

    auto identifying_members() const noexcept { return std::tie(rule_key, metric, cost, numeric_supports); }

private:
    WitnessRuleKeyT<Kind, R> rule_key;
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

template<TaskKind Kind, ::tyr::formalism::RelationKind R = ::tyr::formalism::PredicateTag>
using Annotation = std::variant<BaseAnnotation<Kind>, WitnessAnnotation<Kind, R>>;

template<TaskKind Kind, ::tyr::formalism::RelationKind R>
inline auto get_metric(const Annotation<Kind, R>& annotation) noexcept
{
    return std::visit([](const auto& value) { return value.get_metric(); }, annotation);
}

template<TaskKind Kind, ::tyr::formalism::RelationKind R>
inline Cost get_cost(const Annotation<Kind, R>& annotation) noexcept
{
    return std::visit([](const auto& value) { return value.get_cost(); }, annotation);
}

template<TaskKind Kind>
class PredicateAnnotationMap
{
public:
    /// Both engines key predicate annotations by the fluent predicate binding; the ground engine's atom
    /// heads convert with get_row() at the call site.
    using Key = ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag>;

    /// Resetting bumps the generation rather than touching the slots: the delta map is reset once per
    /// rule per iteration, while its dense row span grows with the number of derived atoms, so an
    /// O(span) reset would dominate on large tasks. Slots of older generations read as absent and keep
    /// their annotation storage, so a later write into the same row reuses the numeric-support buffer
    /// instead of reallocating it.
    void clear() noexcept { ++m_generation; }

    void insert_or_assign(Key key, Annotation<Kind> annotation)
    {
        const auto index = key.get_index();
        const auto relation = ygg::uint_t(index.relation);
        const auto row = ygg::uint_t(index.row);

        if (relation >= m_annotations.size())
            m_annotations.resize(relation + 1);

        auto& relation_annotations = m_annotations[relation];
        if (row >= relation_annotations.size())
            relation_annotations.resize(row + 1);

        auto& slot = relation_annotations[row];
        slot.generation = m_generation;
        slot.annotation = std::move(annotation);
    }

    const Annotation<Kind>* find(Key key) const noexcept
    {
        const auto index = key.get_index();
        const auto relation = ygg::uint_t(index.relation);
        const auto row = ygg::uint_t(index.row);

        if (relation >= m_annotations.size())
            return nullptr;

        const auto& relation_annotations = m_annotations[relation];
        if (row >= relation_annotations.size())
            return nullptr;

        const auto& slot = relation_annotations[row];
        return slot.generation == m_generation ? &slot.annotation : nullptr;
    }

    Annotation<Kind>* find(Key key) noexcept { return const_cast<Annotation<Kind>*>(std::as_const(*this).find(key)); }

private:
    /// Generation 0 marks a slot that was never written, so the live generation starts at 1.
    struct Slot
    {
        uint64_t generation { 0 };
        Annotation<Kind> annotation {};
    };

    std::vector<std::vector<Slot>> m_annotations;
    uint64_t m_generation { 1 };
};

/// Sparse counterpart of PredicateAnnotationMap for the rule-level delta annotations. Those hold a
/// handful of entries and are reset per rule per iteration, so the dense row span - which grows with
/// the number of derived atoms - would dominate. Resetting clears the table without destroying it, so
/// its buckets stay allocated for the next iteration.
template<TaskKind Kind>
class DeltaPredicateAnnotationMap
{
public:
    using Key = ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag>;

    /// Clears the table without releasing it, so its buckets stay allocated for the next iteration.
    void clear() noexcept { m_annotations.clear(); }

    void insert_or_assign(Key key, Annotation<Kind> annotation) { m_annotations.insert_or_assign(key, std::move(annotation)); }

    const Annotation<Kind>* find(Key key) const noexcept
    {
        const auto it = m_annotations.find(key);
        return it == m_annotations.end() ? nullptr : &it->second;
    }

    Annotation<Kind>* find(Key key) noexcept
    {
        const auto it = m_annotations.find(key);
        return it == m_annotations.end() ? nullptr : &it->second;
    }

private:
    ygg::UnorderedMap<Key, Annotation<Kind>> m_annotations;
};

/// Solve-level annotations: dense indexing, reset once per solve.
template<TaskKind Kind>
using SelectedPredicateAnnotations = PredicateAnnotationMap<Kind>;

/// Rule-level delta annotations: sparse, reset per rule per iteration.
template<TaskKind Kind>
using DeltaPredicateAnnotations = DeltaPredicateAnnotationMap<Kind>;

template<TaskKind Kind>
struct NumericIntervalAnnotation : ygg::comparison::Mixin<NumericIntervalAnnotation<Kind>>
{
    ygg::ClosedInterval<ygg::float_t> interval;
    Annotation<Kind, ::tyr::formalism::FunctionTag> annotation;

    NumericIntervalAnnotation() = default;
    NumericIntervalAnnotation(ygg::ClosedInterval<ygg::float_t> interval, Annotation<Kind, ::tyr::formalism::FunctionTag> annotation) :
        interval(interval),
        annotation(std::move(annotation))
    {
    }

    auto identifying_members() const noexcept { return std::make_tuple(get_cost(annotation), interval, annotation); }
};

template<TaskKind Kind>
struct NumericIntervalBindingParts;

template<TaskKind Kind>
class DeltaNumericIntervalAnnotations
{
public:
    using Binding = typename NumericIntervalBindingParts<Kind>::Binding;
    using Relation = typename NumericIntervalBindingParts<Kind>::Relation;
    using Key = typename NumericIntervalBindingParts<Kind>::Key;
    using Entry = NumericIntervalAnnotation<Kind>;
    using Entries = std::vector<Entry>;
    using KeyPartitions = ygg::UnorderedMap<Key, Entries>;
    using Partitions = ygg::UnorderedMap<Relation, KeyPartitions>;

    /// Clears the nested entry vectors in place: releasing them would hand the buffers back only to
    /// reallocate them next iteration.
    void clear() noexcept
    {
        m_size = 0;
        for (auto& [_, key_partitions] : m_partitions)
            for (auto& [_, entries] : key_partitions)
                entries.clear();
    }

    size_t size() const noexcept { return m_size; }

    const Entries* find_entries(Relation relation, Key key) const noexcept
    {
        const auto relation_it = m_partitions.find(relation);
        if (relation_it == m_partitions.end())
            return nullptr;

        const auto key_it = relation_it->second.find(key);
        return key_it == relation_it->second.end() ? nullptr : &key_it->second;
    }

    const Annotation<Kind, ::tyr::formalism::FunctionTag>* find(Binding binding) const noexcept
    {
        const auto* entries = find_entries(binding);
        return (!entries || entries->empty()) ? nullptr : &entries->back().annotation;
    }

    Annotation<Kind, ::tyr::formalism::FunctionTag>* find(Binding binding) noexcept
    {
        auto* entries = find_entries(binding);
        return (!entries || entries->empty()) ? nullptr : &entries->back().annotation;
    }

    const Annotation<Kind, ::tyr::formalism::FunctionTag>* find(Binding binding, ygg::ClosedInterval<ygg::float_t> interval) const noexcept
    {
        const auto* entries = find_entries(binding);
        if (!entries)
            return nullptr;

        for (const auto& entry : *entries)
            if (entry.interval == interval)
                return &entry.annotation;

        return nullptr;
    }

    void insert(Binding binding, ygg::ClosedInterval<ygg::float_t> interval, Annotation<Kind, ::tyr::formalism::FunctionTag> annotation)
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

/// Solve-level counterpart of DeltaNumericIntervalAnnotations: dense in (relation, key) because it is
/// queried per candidate, and reset once per solve by bumping a generation so no nested container is
/// released. Mirrors the split between PredicateAnnotationMap and DeltaPredicateAnnotationMap.
template<TaskKind Kind>
class NumericIntervalAnnotations
{
public:
    using Binding = typename NumericIntervalBindingParts<Kind>::Binding;
    using Relation = typename NumericIntervalBindingParts<Kind>::Relation;
    using Key = typename NumericIntervalBindingParts<Kind>::Key;
    using Entry = NumericIntervalAnnotation<Kind>;
    using Entries = std::vector<Entry>;

    void clear() noexcept
    {
        ++m_generation;
        m_size = 0;
    }

    size_t size() const noexcept { return m_size; }

    const Entries* find_entries(Relation relation, Key key) const noexcept
    {
        const auto relation_index = ygg::uint_t(relation);
        if (relation_index >= m_slots.size())
            return nullptr;

        const auto& key_slots = m_slots[relation_index];
        const auto key_index = ygg::uint_t(key);
        if (key_index >= key_slots.size())
            return nullptr;

        const auto& slot = key_slots[key_index];
        return slot.generation == m_generation ? &slot.entries : nullptr;
    }

    const Annotation<Kind, ::tyr::formalism::FunctionTag>* find(Binding binding) const noexcept
    {
        const auto* entries = find_entries(binding);
        return (!entries || entries->empty()) ? nullptr : &entries->back().annotation;
    }

    Annotation<Kind, ::tyr::formalism::FunctionTag>* find(Binding binding) noexcept
    {
        auto* entries = const_cast<Entries*>(find_entries(binding));
        return (!entries || entries->empty()) ? nullptr : &entries->back().annotation;
    }

    const Annotation<Kind, ::tyr::formalism::FunctionTag>* find(Binding binding, ygg::ClosedInterval<ygg::float_t> interval) const noexcept
    {
        const auto* entries = find_entries(binding);
        if (!entries)
            return nullptr;

        for (const auto& entry : *entries)
            if (entry.interval == interval)
                return &entry.annotation;

        return nullptr;
    }

    void insert(Binding binding, ygg::ClosedInterval<ygg::float_t> interval, Annotation<Kind, ::tyr::formalism::FunctionTag> annotation)
    {
        insert(NumericIntervalBindingParts<Kind>::get_relation(binding),
               NumericIntervalBindingParts<Kind>::get_key(binding),
               interval,
               std::move(annotation));
    }

    void insert(Relation relation, Key key, ygg::ClosedInterval<ygg::float_t> interval, Annotation<Kind, ::tyr::formalism::FunctionTag> annotation)
    {
        if (empty(interval))
            return;

        auto entry = Entry { interval, std::move(annotation) };
        auto& entries = entries_for_write(relation, key);
        const auto pos = std::upper_bound(entries.begin(), entries.end(), entry);
        if (pos != entries.begin() && *(pos - 1) == entry)
            return;
        entries.insert(pos, std::move(entry));
        ++m_size;
    }

private:
    /// Generation 0 marks a slot that was never written, so the live generation starts at 1.
    struct Slot
    {
        uint64_t generation { 0 };
        Entries entries;
    };

    const Entries* find_entries(Binding binding) const noexcept
    {
        return find_entries(NumericIntervalBindingParts<Kind>::get_relation(binding), NumericIntervalBindingParts<Kind>::get_key(binding));
    }

    Entries& entries_for_write(Relation relation, Key key)
    {
        const auto relation_index = ygg::uint_t(relation);
        if (relation_index >= m_slots.size())
            m_slots.resize(relation_index + 1);

        auto& key_slots = m_slots[relation_index];
        const auto key_index = ygg::uint_t(key);
        if (key_index >= key_slots.size())
            key_slots.resize(key_index + 1);

        auto& slot = key_slots[key_index];
        if (slot.generation != m_generation)
        {
            slot.entries.clear();  ///< clear, never release: the buffer is reused next generation
            slot.generation = m_generation;
        }
        return slot.entries;
    }

    std::vector<std::vector<Slot>> m_slots;
    uint64_t m_generation { 1 };
    size_t m_size { 0 };
};

/// Solve-level numeric annotations: dense, reset once per solve.
template<TaskKind Kind>
using SelectedFunctionAnnotations = NumericIntervalAnnotations<Kind>;

/// Rule-level delta numeric annotations: sparse, reset per rule per iteration.
template<TaskKind Kind>
using DeltaFunctionAnnotations = DeltaNumericIntervalAnnotations<Kind>;

template<TaskKind Kind, ::tyr::formalism::RelationKind R>
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
