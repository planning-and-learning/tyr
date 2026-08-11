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
#include <concepts>
#include <cstdint>
#include <limits>
#include <oneapi/tbb/spin_mutex.h>
#include <optional>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include <yggdrasil/containers/segmented_vector.hpp>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/semantics/comparison.hpp>

namespace tyr::datalog
{

template<TaskKind Kind>
struct NumericSupport : ygg::comparison::Mixin<NumericSupport<Kind>>
{
    ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> key;
    ygg::ClosedInterval<ygg::float_t> interval;
    Cost cost;

    NumericSupport() = default;
    NumericSupport(::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> key, ygg::ClosedInterval<ygg::float_t> interval, Cost cost) :
        key(key),
        interval(interval),
        cost(cost)
    {
    }

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

    WitnessAnnotation(::tyr::formalism::datalog::RuleBindingView<R> rule_key, Cost cost);
    WitnessAnnotation(::tyr::formalism::datalog::RuleBindingView<R> rule_key, Metric metric, Cost cost);
    WitnessAnnotation(::tyr::formalism::datalog::RuleBindingView<R> rule_key, Metric metric, Cost cost, NumericSupports numeric_supports);
    WitnessAnnotation(::tyr::formalism::datalog::RuleBindingView<R> rule_key, Metric metric, Cost cost, std::span<const NumericSupport<Kind>> numeric_supports);

    auto get_rule_key() const noexcept { return rule_key; }
    auto get_metric() const noexcept { return metric; }
    auto get_cost() const noexcept { return cost; }
    const auto& get_numeric_supports() const noexcept { return numeric_supports; }

    auto identifying_members() const noexcept { return std::tie(rule_key, metric, cost, numeric_supports); }

private:
    ::tyr::formalism::datalog::RuleBindingView<R> rule_key;
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
struct CostUpdate
{
    std::optional<Cost> old_cost;
    Cost new_cost;

    CostUpdate() noexcept : old_cost(std::nullopt), new_cost(Cost(0)) { assert(is_monotone()); }
    CostUpdate(std::optional<Cost> old_cost, Cost new_cost) noexcept : old_cost(old_cost), new_cost(new_cost) { assert(is_monotone()); }
    CostUpdate(Cost old_cost, Cost new_cost) noexcept :
        old_cost(old_cost == std::numeric_limits<Cost>::max() ? std::nullopt : std::optional<Cost>(old_cost)),
        new_cost(new_cost)
    {
        assert(is_monotone());
    }

    bool is_monotone() const noexcept { return !old_cost || new_cost <= old_cost.value(); }
};

/// ThreadSafe permits concurrent read(), update(), and row growth after relation lanes
/// are initialized. initialize(), clear(), find(), moves, and destruction require quiescence.
template<::tyr::formalism::RelationKind R, std::default_initializable Value, bool ThreadSafe = false>
class DenseRelationMap
{
public:
    static constexpr bool thread_safe = ThreadSafe;

    using Key = std::conditional_t<std::same_as<R, ::tyr::formalism::PredicateTag>,
                                   ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag>,
                                   ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag>>;
    using Relation = decltype(std::declval<Key>().get_index().relation);
    using Row = decltype(std::declval<Key>().get_index().row);

    explicit DenseRelationMap(size_t num_relations = 0) : m_lanes(num_relations) {}
    DenseRelationMap(const DenseRelationMap&)
        requires(!ThreadSafe)
    = default;
    DenseRelationMap& operator=(const DenseRelationMap&)
        requires(!ThreadSafe)
    = default;
    DenseRelationMap(const DenseRelationMap&)
        requires(ThreadSafe)
    = delete;
    DenseRelationMap& operator=(const DenseRelationMap&)
        requires(ThreadSafe)
    = delete;
    DenseRelationMap(DenseRelationMap&&) noexcept = default;
    DenseRelationMap& operator=(DenseRelationMap&&) noexcept = default;

    void initialize(size_t num_relations) { m_lanes = std::vector<Lane>(num_relations); }

    void clear() noexcept { ++m_generation; }

    template<std::invocable<const Value&> Read>
    std::invoke_result_t<Read, const Value&> read(Key key, Read&& read, std::invoke_result_t<Read, const Value&> missing) const
    {
        const auto* slot = find_slot(key);
        if (!slot)
            return missing;

        return with_slot_lock(*slot, [&] { return slot->generation == m_generation ? std::forward<Read>(read)(slot->value) : missing; });
    }

    template<std::invocable<Value&, bool> Update>
    decltype(auto) update(Key key, Update&& update)
    {
        const auto index = key.get_index();
        return this->update(index.relation, index.row, std::forward<Update>(update));
    }

    template<std::invocable<Value&, bool> Update>
    decltype(auto) update(Relation relation, Row row, Update&& update)
    {
        auto& slot = get_or_create_slot(relation, row);
        return with_slot_lock(slot,
                              [&]() -> decltype(auto)
                              {
                                  const auto initialized = slot.generation == m_generation;
                                  slot.generation = m_generation;
                                  return std::forward<Update>(update)(slot.value, initialized);
                              });
    }

    const Value* find(Key key) const noexcept
    {
        const auto index = key.get_index();
        return find(index.relation, index.row);
    }

    Value* find(Key key) noexcept
        requires(!ThreadSafe)
    {
        return const_cast<Value*>(std::as_const(*this).find(key));
    }

    const Value* find(Relation relation, Row row) const noexcept
    {
        const auto* slot = find_slot(relation, row);
        return slot && slot->generation == m_generation ? &slot->value : nullptr;
    }

    Value* find(Relation relation, Row row) noexcept
        requires(!ThreadSafe)
    {
        return const_cast<Value*>(std::as_const(*this).find(relation, row));
    }

private:
    struct NoMutex
    {
    };

    using Mutex = std::conditional_t<ThreadSafe, oneapi::tbb::spin_mutex, NoMutex>;

    struct Slot
    {
        [[no_unique_address]] mutable Mutex mutex;
        uint64_t generation { 0 };
        Value value {};
    };

    using Lane = std::conditional_t<ThreadSafe, ygg::SegmentedVector<Slot, 32, true>, std::vector<Slot>>;

    template<typename SlotT, typename Callback>
    static decltype(auto) with_slot_lock(SlotT& slot, Callback&& callback)
    {
        if constexpr (ThreadSafe)
        {
            const auto lock = oneapi::tbb::spin_mutex::scoped_lock(slot.mutex);
            return std::forward<Callback>(callback)();
        }
        else
        {
            return std::forward<Callback>(callback)();
        }
    }

    Slot& get_or_create_slot(Relation relation, Row row)
    {
        const auto relation_index = ygg::uint_t(relation);
        if constexpr (ThreadSafe)
            assert(relation_index < m_lanes.size());
        else if (relation_index >= m_lanes.size())
            m_lanes.resize(relation_index + 1);

        auto& lane = m_lanes[relation_index];
        const auto row_index = ygg::uint_t(row);
        if constexpr (ThreadSafe)
        {
            while (lane.size() <= row_index)
                lane.emplace_back();
        }
        else if (row_index >= lane.size())
            lane.resize(row_index + 1);
        return lane[row_index];
    }

    const Slot* find_slot(Key key) const noexcept
    {
        const auto index = key.get_index();
        return find_slot(index.relation, index.row);
    }

    const Slot* find_slot(Relation relation, Row row) const noexcept
    {
        const auto relation_index = ygg::uint_t(relation);
        const auto row_index = ygg::uint_t(row);
        if (relation_index >= m_lanes.size())
            return nullptr;

        const auto& lane = m_lanes[relation_index];
        return row_index < lane.size() ? &lane[row_index] : nullptr;
    }

    std::vector<Lane> m_lanes;
    uint64_t m_generation { 1 };
};

/// ThreadSafe permits concurrent insertions and cost reads; clear(), find(), moves,
/// and destruction require quiescence.
template<TaskKind Kind, bool ThreadSafe = false>
class PredicateAnnotationMap
{
public:
    static constexpr bool thread_safe = ThreadSafe;

    /// Both engines key predicate annotations by the fluent predicate binding.
    using Key = ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag>;

    PredicateAnnotationMap()
        requires(!ThreadSafe)
    = default;
    explicit PredicateAnnotationMap(size_t num_relations) : m_annotations(num_relations) {}

    /// Resetting bumps the generation rather than touching the dense row span. Slots of older
    /// generations read as absent and retain their annotation storage for reuse.
    void clear() noexcept { m_annotations.clear(); }

    void insert_or_assign(Key key, Annotation<Kind> annotation)
    {
        m_annotations.update(key, [&](auto& incumbent, bool) { incumbent = std::move(annotation); });
    }

    Cost fetch_cost(Key key) const noexcept
    {
        return m_annotations.read(key, [](const auto& annotation) { return get_cost(annotation); }, std::numeric_limits<Cost>::max());
    }

    std::optional<CostUpdate<Kind>> insert_if_better(Key key, Annotation<Kind> annotation)
    {
        return m_annotations.update(key,
                                    [&](auto& incumbent, bool initialized)
                                    {
                                        const auto new_cost = get_cost(annotation);
                                        const auto old_cost = initialized ? std::optional<Cost>(get_cost(incumbent)) : std::nullopt;
                                        if (old_cost && *old_cost <= new_cost)
                                            return std::optional<CostUpdate<Kind>> {};

                                        incumbent = std::move(annotation);
                                        return std::optional<CostUpdate<Kind>>(std::in_place, old_cost, new_cost);
                                    });
    }

    const Annotation<Kind>* find(Key key) const noexcept { return m_annotations.find(key); }

    Annotation<Kind>* find(Key key) noexcept
        requires(!ThreadSafe)
    {
        return const_cast<Annotation<Kind>*>(std::as_const(*this).find(key));
    }

private:
    DenseRelationMap<::tyr::formalism::PredicateTag, Annotation<Kind>, ThreadSafe> m_annotations;
};

template<TaskKind Kind, bool ThreadSafe = false>
using PredicateAnnotations = PredicateAnnotationMap<Kind, ThreadSafe>;

template<TaskKind Kind>
struct NumericIntervalAnnotation
{
    ygg::ClosedInterval<ygg::float_t> interval;
    Annotation<Kind, ::tyr::formalism::FunctionTag> annotation;

    NumericIntervalAnnotation() = default;
    NumericIntervalAnnotation(ygg::ClosedInterval<ygg::float_t> interval, Annotation<Kind, ::tyr::formalism::FunctionTag> annotation) :
        interval(interval),
        annotation(std::move(annotation))
    {
    }
};

template<TaskKind Kind>
bool numeric_interval_key_less(const NumericIntervalAnnotation<Kind>& lhs, const NumericIntervalAnnotation<Kind>& rhs) noexcept
{
    return ygg::Less<> {}(std::tuple(get_cost(lhs.annotation), lower(lhs.interval), upper(lhs.interval)),
                          std::tuple(get_cost(rhs.annotation), lower(rhs.interval), upper(rhs.interval)));
}

template<TaskKind Kind>
const Annotation<Kind, ::tyr::formalism::FunctionTag>* find_numeric_interval_annotation(const std::vector<NumericIntervalAnnotation<Kind>>& entries,
                                                                                        ygg::ClosedInterval<ygg::float_t> interval) noexcept
{
    const auto it = std::find_if(entries.begin(), entries.end(), [&](const auto& entry) { return entry.interval == interval; });
    return it == entries.end() ? nullptr : &it->annotation;
}

/// Retains the first cheapest witness for each exact interval.
template<TaskKind Kind>
bool insert_first_best_numeric_interval_annotation(std::vector<NumericIntervalAnnotation<Kind>>& entries, NumericIntervalAnnotation<Kind> entry)
{
    const auto incumbent = std::find_if(entries.begin(), entries.end(), [&](const auto& candidate) { return candidate.interval == entry.interval; });
    if (incumbent != entries.end())
    {
        if (get_cost(incumbent->annotation) <= get_cost(entry.annotation))
            return false;

        const auto pos = std::upper_bound(entries.begin(), incumbent, entry, numeric_interval_key_less<Kind>);
        std::move_backward(pos, incumbent, incumbent + 1);
        *pos = std::move(entry);
        return true;
    }

    const auto pos = std::upper_bound(entries.begin(), entries.end(), entry, numeric_interval_key_less<Kind>);
    entries.insert(pos, std::move(entry));
    return true;
}

/// ThreadSafe permits concurrent insertions and cost reads; clear(), find(), moves,
/// and destruction require quiescence.
template<TaskKind Kind, bool ThreadSafe = false>
class NumericIntervalAnnotations
{
public:
    static constexpr bool thread_safe = ThreadSafe;

    using Binding = ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag>;
    using Entry = NumericIntervalAnnotation<Kind>;
    using Entries = std::vector<Entry>;
    using Storage = DenseRelationMap<::tyr::formalism::FunctionTag, Entries, ThreadSafe>;
    using Relation = typename Storage::Relation;
    using Key = typename Storage::Row;

    NumericIntervalAnnotations()
        requires(!ThreadSafe)
    = default;
    explicit NumericIntervalAnnotations(size_t num_relations) : m_slots(num_relations) {}

    void clear() noexcept
    {
        m_slots.clear();
        m_size = 0;
    }

    size_t size() const noexcept
        requires(!ThreadSafe)
    {
        return m_size;
    }

    Cost fetch_cost(Binding binding, ygg::ClosedInterval<ygg::float_t> interval) const noexcept
    {
        return m_slots.read(
            binding,
            [&](const auto& entries)
            {
                const auto* annotation = find_numeric_interval_annotation<Kind>(entries, interval);
                return annotation ? get_cost(*annotation) : std::numeric_limits<Cost>::max();
            },
            std::numeric_limits<Cost>::max());
    }

    const Entries* find_entries(Binding binding) const noexcept { return m_slots.find(binding); }

    const Entries* find_entries(Relation relation, Key key) const noexcept { return m_slots.find(relation, key); }

    const Annotation<Kind, ::tyr::formalism::FunctionTag>* find(Binding binding) const noexcept
    {
        const auto* entries = find_entries(binding);
        return (!entries || entries->empty()) ? nullptr : &entries->back().annotation;
    }

    Annotation<Kind, ::tyr::formalism::FunctionTag>* find(Binding binding) noexcept
        requires(!ThreadSafe)
    {
        auto* entries = m_slots.find(binding);
        return (!entries || entries->empty()) ? nullptr : &entries->back().annotation;
    }

    const Annotation<Kind, ::tyr::formalism::FunctionTag>* find(Binding binding, ygg::ClosedInterval<ygg::float_t> interval) const noexcept
    {
        const auto* entries = find_entries(binding);
        return entries ? find_numeric_interval_annotation<Kind>(*entries, interval) : nullptr;
    }

    bool insert(Binding binding, ygg::ClosedInterval<ygg::float_t> interval, Annotation<Kind, ::tyr::formalism::FunctionTag> annotation)
    {
        const auto index = binding.get_index();
        return insert(index.relation, index.row, interval, std::move(annotation));
    }

    bool insert(Relation relation, Key key, ygg::ClosedInterval<ygg::float_t> interval, Annotation<Kind, ::tyr::formalism::FunctionTag> annotation)
    {
        if (empty(interval))
            return false;

        auto entry = Entry { interval, std::move(annotation) };
        return m_slots.update(relation,
                              key,
                              [&](Entries& entries, bool initialized)
                              {
                                  if (!initialized)
                                      entries.clear();
                                  const auto old_size = entries.size();
                                  const auto changed = insert_first_best_numeric_interval_annotation(entries, std::move(entry));
                                  if constexpr (!ThreadSafe)
                                      m_size += entries.size() - old_size;
                                  return changed;
                              });
    }

private:
    Storage m_slots;
    size_t m_size { 0 };
};

template<TaskKind Kind, bool ThreadSafe = false>
using FunctionAnnotations = NumericIntervalAnnotations<Kind, ThreadSafe>;

inline ygg::ClosedInterval<ygg::float_t> aggregate_metric_support(ygg::ClosedInterval<ygg::float_t> lhs, ygg::ClosedInterval<ygg::float_t> rhs) noexcept
{
    if (empty(lhs))
        return rhs;
    if (empty(rhs))
        return lhs;
    return ygg::ClosedInterval<ygg::float_t>(std::max(lower(lhs), lower(rhs)), std::max(upper(lhs), upper(rhs)));
}

inline ygg::ClosedInterval<ygg::float_t> add_metric_delta(ygg::ClosedInterval<ygg::float_t> metric, Cost delta) noexcept
{
    if (delta == Cost(0))
        return metric;
    if (empty(metric))
        return ygg::ClosedInterval<ygg::float_t>(delta, delta);
    return ygg::ClosedInterval<ygg::float_t>(lower(metric) + delta, upper(metric) + delta);
}

}

#endif
