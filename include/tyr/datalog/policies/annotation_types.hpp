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
#include <mutex>
#include <optional>
#include <span>
#include <tuple>
#include <type_traits>
#include <urd/repr/maps/index_map.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/segmented.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/repr/wrappers/recycling.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <variant>
#include <vector>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>
#include <urd/config.hpp>

namespace tyr::datalog
{

struct NumericSupport : urd::compared_by_identity<NumericSupport>
{
    formalism::datalog::FunctionBindingIndex<formalism::FluentTag> key;
    ygg::ClosedInterval<ygg::float_t> interval;
    Cost cost;

    NumericSupport() = delete;
    NumericSupport(formalism::datalog::FunctionBindingIndex<formalism::FluentTag> key_, ygg::ClosedInterval<ygg::float_t> interval_, Cost cost_) :
        key(key_),
        interval(std::move(interval_)),
        cost(cost_)
    {
    }

    [[nodiscard]] auto get_key() const noexcept { return key; }
    [[nodiscard]] auto get_interval() const noexcept { return interval; }
    [[nodiscard]] auto get_cost() const noexcept { return cost; }

    static constexpr auto identifying_members() noexcept { return urd::Members<&NumericSupport::key, &NumericSupport::interval, &NumericSupport::cost> {}; }
};

template<formalism::RelationKind R = formalism::PredicateTag>
struct WitnessAnnotation : urd::compared_by_identity<WitnessAnnotation<R>>
{
private:
    using NumericSupports = urd::Store<urd::sequence<NumericSupport>>;

public:
    WitnessAnnotation(formalism::datalog::RuleBindingIndex<R> rule_key, Cost cost);
    WitnessAnnotation(formalism::datalog::RuleBindingIndex<R> rule_key, Cost cost, NumericSupports numeric_supports);
    WitnessAnnotation(formalism::datalog::RuleBindingIndex<R> rule_key, Cost cost, std::span<const NumericSupport> numeric_supports);

    [[nodiscard]] auto get_rule_key() const noexcept { return rule_key; }
    [[nodiscard]] auto get_cost() const noexcept { return cost; }
    [[nodiscard]] const auto& get_numeric_supports() const noexcept { return numeric_supports; }

    static constexpr auto identifying_members() noexcept
    { return urd::Members<&WitnessAnnotation::rule_key, &WitnessAnnotation::cost, &WitnessAnnotation::numeric_supports> {}; }

private:
    formalism::datalog::RuleBindingIndex<R> rule_key;
    Cost cost;
    NumericSupports numeric_supports;
};

using PredicateRuleBinding = formalism::datalog::RuleBindingIndex<formalism::PredicateTag>;

struct PredicateAchiever
{
    formalism::datalog::PredicateBindingIndex<formalism::FluentTag> head;
    WitnessAnnotation<formalism::PredicateTag> witness;
};

struct PredicateAchieverKey : urd::compared_by_identity<PredicateAchieverKey>
{
    PredicateRuleBinding rule_key;
    formalism::datalog::PredicateBindingIndex<formalism::FluentTag> head;

    PredicateAchieverKey() = default;
    PredicateAchieverKey(PredicateRuleBinding rule_key_, formalism::datalog::PredicateBindingIndex<formalism::FluentTag> head_) :
        rule_key(rule_key_),
        head(head_)
    {
    }

    static constexpr auto identifying_members() noexcept { return urd::Members<&PredicateAchieverKey::rule_key, &PredicateAchieverKey::head> {}; }
};

/// The achievers by cost, least first; erased buckets keep their vectors for the next costs (retains).
using PendingPredicateAchieverBuckets = urd::Store<urd::map<Cost, urd::Store<urd::sequence<PredicateAchieverKey>>>, urd::sorted, urd::retains, urd::stable_refs>;

/// Owned by the program workspace and reused across solves: clear() keeps the capacity of by_key and of the bucket vectors.
struct PendingPredicateAchievers
{
    PendingPredicateAchieverBuckets buckets;
    urd::Store<urd::map<PredicateAchieverKey, PredicateAchiever>> by_key;  ///< Cheapest unpublished achiever per rule and head; erased once published.

    urd::Store<urd::sequence<PredicateAchieverKey>>& bucket(Cost cost) { return buckets[cost]; }

    /// Erases the buckets before `end`, cheapest first.
    void erase_buckets(PendingPredicateAchieverBuckets::const_iterator end)
    {
        while (buckets.begin() != end)
            buckets.erase(buckets.begin());
    }

    void clear()
    {
        by_key.clear();
        erase_buckets(buckets.end());
    }
};

inline void insert_pending_achiever(PendingPredicateAchievers& pending, PredicateAchiever achiever)
{
    const auto cost = achiever.witness.get_cost();
    const auto key = PredicateAchieverKey(achiever.witness.get_rule_key(), achiever.head);
    const auto it = pending.by_key.find(key);
    if (it == pending.by_key.end())
    {
        pending.by_key.emplace(key, std::move(achiever));
    }
    else
    {
        if (it->second.witness.get_cost() <= cost)
            return;
        it->second = std::move(achiever);
    }
    pending.bucket(cost).push_back(key);
}

template<typename AnnotationPolicy>
void publish_pending_achievers(PendingPredicateAchievers& pending, PendingPredicateAchieverBuckets::const_iterator end, AnnotationPolicy& annotation_policy)
{
    if constexpr (AnnotationPolicy::records_propositional_achievers)
        for (auto bucket = pending.buckets.begin(); bucket != end; ++bucket)
            for (const auto key : bucket->second)
            {
                // An achiever improved into a cheaper bucket appears again in its older buckets; only its current one publishes it.
                // Published entries are erased: record_achiever keeps the cheapest witness per (head, rule) itself.
                const auto it = pending.by_key.find(key);
                if (it == pending.by_key.end() || it->second.witness.get_cost() != bucket->first)
                    continue;
                annotation_policy.record_achiever(it->second.head, std::move(it->second.witness));
                pending.by_key.erase(it);
            }
    pending.erase_buckets(end);
}

struct BaseAnnotation : urd::compared_by_identity<BaseAnnotation>
{
public:
    BaseAnnotation() : m_cost(static_cast<Cost>(0)) {}
    explicit BaseAnnotation(Cost cost) : m_cost(cost) {}

    [[nodiscard]] auto get_cost() const noexcept { return m_cost; }

    static constexpr auto identifying_members() noexcept { return urd::Members<&BaseAnnotation::m_cost> {}; }

private:
    Cost m_cost;
};

template<formalism::RelationKind R = formalism::PredicateTag>
using Annotation = std::variant<BaseAnnotation, WitnessAnnotation<R>>;

template<formalism::RelationKind R>
inline Cost get_cost(const Annotation<R>& annotation) noexcept
{
    // get_if rather than std::visit, which throws for a valueless variant; annotations are never valueless.
    if (const auto* base = std::get_if<BaseAnnotation>(&annotation))
        return base->get_cost();
    const auto* witness = std::get_if<WitnessAnnotation<R>>(&annotation);
    URD_ASSUME(witness != nullptr);
    return witness->get_cost();
}

struct CostUpdate
{
    std::optional<Cost> old_cost;
    Cost new_cost;

    CostUpdate() noexcept : old_cost(std::nullopt), new_cost(static_cast<Cost>(0)) { assert(is_monotone()); }
    CostUpdate(std::optional<Cost> old_cost_, Cost new_cost_) noexcept : old_cost(old_cost_), new_cost(new_cost_) { assert(is_monotone()); }
    CostUpdate(Cost old_cost_, Cost new_cost_) noexcept :
        old_cost(old_cost_ == std::numeric_limits<Cost>::max() ? std::nullopt : std::optional<Cost>(old_cost_)),
        new_cost(new_cost_)
    { assert(is_monotone()); }

    [[nodiscard]] bool is_monotone() const noexcept { return !old_cost || new_cost <= *old_cost; }
    [[nodiscard]] bool is_strict_improvement() const noexcept { return new_cost < old_cost.value_or(std::numeric_limits<Cost>::max()); }
};

/// One urd generation map per relation (clear starts a new generation; slots keep their storage for reuse).
/// ThreadSafe permits concurrent read(), update(), and row growth after relation lanes
/// are initialized. initialize(), clear(), find(), moves, and destruction require quiescence.
template<formalism::RelationKind R, std::default_initializable Value, bool ThreadSafe = false>
class DenseRelationMap
{
public:
    static constexpr bool thread_safe = ThreadSafe;

    using Key = std::conditional_t<std::same_as<R, formalism::PredicateTag>,
                                   formalism::datalog::PredicateBindingIndex<formalism::FluentTag>,
                                   formalism::datalog::FunctionBindingIndex<formalism::FluentTag>>;
    using Relation = decltype(std::declval<Key>().relation);
    using Row = decltype(std::declval<Key>().row);

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
    ~DenseRelationMap() = default;

    void initialize(size_t num_relations) { m_lanes = urd::Store<urd::sequence<Lane>>(num_relations); }

    void clear() noexcept
    {
        for (auto& lane : m_lanes)
            lane.clear();
    }

    template<std::invocable<const Value&> Read>
    std::invoke_result_t<Read, const Value&> read(Key key, Read&& read, std::invoke_result_t<Read, const Value&> missing) const
    {
        const auto relation = ygg::uint_t(key.relation.value);
        return relation < m_lanes.size() ? m_lanes[relation].read(key.row, std::forward<Read>(read), missing) : missing;
    }

    template<std::invocable<Value&, bool> Update>
    decltype(auto) update(Key key, Update&& update)
    { return this->update(key.relation, key.row, std::forward<Update>(update)); }

    template<std::invocable<Value&, bool> Update>
    decltype(auto) update(Relation relation, Row row, Update&& update)
    {
        const auto index = ygg::uint_t(relation.value);
        if constexpr (ThreadSafe)
            assert(index < m_lanes.size());
        else if (index >= m_lanes.size())
            m_lanes.resize(index + 1);
        return m_lanes[index].update(row, std::forward<Update>(update));
    }

    [[nodiscard]] const Value* find(Key key) const noexcept { return find(key.relation, key.row); }

    Value* find(Key key) noexcept
        requires(!ThreadSafe)
    { return const_cast<Value*>(std::as_const(*this).find(key)); }

    [[nodiscard]] const Value* find(Relation relation, Row row) const noexcept
    {
        const auto index = ygg::uint_t(relation.value);
        return index < m_lanes.size() ? m_lanes[index].read(row, [](const Value& value) { return &value; }, static_cast<const Value*>(nullptr)) : nullptr;
    }

    Value* find(Relation relation, Row row) noexcept
        requires(!ThreadSafe)
    { return const_cast<Value*>(std::as_const(*this).find(relation, row)); }

private:
    using Lane = urd::Store<urd::map<Row, Value>, urd::dense, urd::constant_clear, std::conditional_t<ThreadSafe, urd::concurrent, urd::single_threaded>>;

    urd::Store<urd::sequence<Lane>> m_lanes;
};

/// ThreadSafe permits concurrent insertions and cost reads; clear(), find(), moves,
/// and destruction require quiescence.
template<bool ThreadSafe = false>
class PredicateAnnotationMap
{
public:
    static constexpr bool thread_safe = ThreadSafe;

    /// Both engines key predicate annotations by the fluent predicate binding.
    using Key = formalism::datalog::PredicateBindingIndex<formalism::FluentTag>;

    PredicateAnnotationMap()
        requires(!ThreadSafe)
    = default;
    explicit PredicateAnnotationMap(size_t num_relations) : m_annotations(num_relations) {}

    /// Resetting bumps the generation rather than touching the dense row span. Slots of older
    /// generations read as absent and retain their annotation storage for reuse.
    void clear() noexcept { m_annotations.clear(); }

    void insert_or_assign(Key key, Annotation<> annotation)
    {
        m_annotations.update(key, [&](auto& incumbent, bool) { incumbent = std::move(annotation); });
    }

    [[nodiscard]] Cost fetch_cost(Key key) const noexcept
    {
        return m_annotations.read(key, [](const auto& annotation) { return get_cost(annotation); }, std::numeric_limits<Cost>::max());
    }

    std::optional<CostUpdate> insert_if_better(Key key, Annotation<> annotation)
    {
        return m_annotations.update(key,
                                    [&](auto& incumbent, bool initialized)
                                    {
                                        const auto new_cost = get_cost(annotation);
                                        const auto old_cost = initialized ? std::optional<Cost>(get_cost(incumbent)) : std::nullopt;
                                        if (old_cost && *old_cost <= new_cost)
                                            return std::optional<CostUpdate> {};

                                        incumbent = std::move(annotation);
                                        return std::optional<CostUpdate>(std::in_place, old_cost, new_cost);
                                    });
    }

    [[nodiscard]] const Annotation<>* find(Key key) const noexcept { return m_annotations.find(key); }

    Annotation<>* find(Key key) noexcept
        requires(!ThreadSafe)
    { return const_cast<Annotation<>*>(std::as_const(*this).find(key)); }

private:
    DenseRelationMap<formalism::PredicateTag, Annotation<>, ThreadSafe> m_annotations;
};

template<bool ThreadSafe = false>
using PredicateAnnotations = PredicateAnnotationMap<ThreadSafe>;

struct NumericIntervalAnnotation
{
    ygg::ClosedInterval<ygg::float_t> interval;
    Annotation<formalism::FunctionTag> annotation;

    NumericIntervalAnnotation() = default;
    NumericIntervalAnnotation(ygg::ClosedInterval<ygg::float_t> interval_, Annotation<formalism::FunctionTag> annotation_) :
        interval(std::move(interval_)),
        annotation(std::move(annotation_))
    {
    }
};

inline bool numeric_interval_key_less(const NumericIntervalAnnotation& lhs, const NumericIntervalAnnotation& rhs) noexcept
{
    return urd::Less<> {}(std::tuple(get_cost(lhs.annotation), lower(lhs.interval), upper(lhs.interval)),
                          std::tuple(get_cost(rhs.annotation), lower(rhs.interval), upper(rhs.interval)));
}

inline const Annotation<formalism::FunctionTag>* find_numeric_interval_annotation(const urd::Store<urd::sequence<NumericIntervalAnnotation>>& entries,
                                                                                  ygg::ClosedInterval<ygg::float_t> interval) noexcept
{
    const auto it = std::find_if(entries.begin(), entries.end(), [&](const auto& entry) { return entry.interval == interval; });
    return it == entries.end() ? nullptr : &it->annotation;
}

/// Retains the first cheapest witness for each exact interval.
inline bool insert_first_best_numeric_interval_annotation(urd::Store<urd::sequence<NumericIntervalAnnotation>>& entries, NumericIntervalAnnotation entry)
{
    const auto incumbent = std::find_if(entries.begin(), entries.end(), [&](const auto& candidate) { return candidate.interval == entry.interval; });
    if (incumbent != entries.end())
    {
        if (get_cost(incumbent->annotation) <= get_cost(entry.annotation))
            return false;

        const auto pos = std::upper_bound(entries.begin(), incumbent, entry, numeric_interval_key_less);
        std::move_backward(pos, incumbent, incumbent + 1);
        *pos = std::move(entry);
        return true;
    }

    const auto pos = std::upper_bound(entries.begin(), entries.end(), entry, numeric_interval_key_less);
    entries.insert(pos, std::move(entry));
    return true;
}

/// ThreadSafe permits concurrent insertions and cost reads; clear(), find(), moves,
/// and destruction require quiescence.
template<bool ThreadSafe = false>
class NumericIntervalAnnotations
{
public:
    static constexpr bool thread_safe = ThreadSafe;

private:
    using Binding = formalism::datalog::FunctionBindingIndex<formalism::FluentTag>;
    using Entry = NumericIntervalAnnotation;

public:
    using Entries = urd::Store<urd::sequence<Entry>>;

private:
    using Storage = DenseRelationMap<formalism::FunctionTag, Entries, ThreadSafe>;

public:
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

    [[nodiscard]] size_t size() const noexcept
        requires(!ThreadSafe)
    { return m_size; }

    [[nodiscard]] Cost fetch_cost(Binding binding, ygg::ClosedInterval<ygg::float_t> interval) const noexcept
    {
        return m_slots.read(
            binding,
            [&](const auto& entries)
            {
                const auto* annotation = find_numeric_interval_annotation(entries, interval);
                return annotation ? get_cost(*annotation) : std::numeric_limits<Cost>::max();
            },
            std::numeric_limits<Cost>::max());
    }

    [[nodiscard]] const Entries* find_entries(Binding binding) const noexcept { return m_slots.find(binding); }

    [[nodiscard]] const Entries* find_entries(Relation relation, Key key) const noexcept { return m_slots.find(relation, key); }

    [[nodiscard]] const Annotation<formalism::FunctionTag>* find(Binding binding) const noexcept
    {
        const auto* entries = find_entries(binding);
        return (!entries || entries->empty()) ? nullptr : &entries->back().annotation;
    }

    Annotation<formalism::FunctionTag>* find(Binding binding) noexcept
        requires(!ThreadSafe)
    {
        auto* entries = m_slots.find(binding);
        return (!entries || entries->empty()) ? nullptr : &entries->back().annotation;
    }

    [[nodiscard]] const Annotation<formalism::FunctionTag>* find(Binding binding, ygg::ClosedInterval<ygg::float_t> interval) const noexcept
    {
        const auto* entries = find_entries(binding);
        return entries ? find_numeric_interval_annotation(*entries, std::move(interval)) : nullptr;
    }

    bool insert(Binding binding, const ygg::ClosedInterval<ygg::float_t>& interval, Annotation<formalism::FunctionTag> annotation)
    { return insert(binding.relation, binding.row, std::move(interval), std::move(annotation)); }

    bool insert(Relation relation, Key key, const ygg::ClosedInterval<ygg::float_t>& interval, Annotation<formalism::FunctionTag> annotation)
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

template<bool ThreadSafe = false>
using FunctionAnnotations = NumericIntervalAnnotations<ThreadSafe>;

/// Stores of staged predicate annotations: PredicateAnnotations<true> for the parallel lifted engine,
/// StagedPredicateAnnotation for the ground engine.
template<typename T>
concept PredicateAnnotationStore = requires(T& store, const T& const_store, typename T::Key key, Annotation<> annotation) {
    { const_store.fetch_cost(key) } -> std::same_as<Cost>;
    { const_store.find(key) } -> std::same_as<const Annotation<>*>;
    { store.insert_if_better(key, std::move(annotation)) } -> std::same_as<std::optional<CostUpdate>>;
    store.clear();
};

/// Stores of staged function annotations: FunctionAnnotations<true> for the parallel lifted engine,
/// StagedFunctionAnnotation for the ground engine.
template<typename T>
concept FunctionAnnotationStore = requires(T& store,
                                           const T& const_store,
                                           formalism::datalog::FunctionBindingIndex<formalism::FluentTag> binding,
                                           ygg::ClosedInterval<ygg::float_t> interval,
                                           Annotation<formalism::FunctionTag> annotation) {
    { const_store.fetch_cost(binding, interval) } -> std::same_as<Cost>;
    { const_store.find(binding, interval) } -> std::same_as<const Annotation<formalism::FunctionTag>*>;
    { store.insert(binding, interval, std::move(annotation)) } -> std::same_as<bool>;
    store.clear();
};

/// The predicate annotations staged by one ground rule firing; the ground engine fires one rule, i.e. its few heads, at a time.
class StagedPredicateAnnotation
{
public:
    using Key = formalism::datalog::PredicateBindingIndex<formalism::FluentTag>;

    void clear() noexcept { m_entries.clear(); }

    [[nodiscard]] Cost fetch_cost(Key key) const noexcept
    {
        const auto* annotation = find(key);
        return annotation ? get_cost(*annotation) : std::numeric_limits<Cost>::max();
    }

    [[nodiscard]] const Annotation<>* find(Key key) const noexcept
    {
        const auto it = std::find_if(m_entries.begin(), m_entries.end(), [&](const auto& entry) { return entry.first == key; });
        return it != m_entries.end() ? &it->second : nullptr;
    }

    std::optional<CostUpdate> insert_if_better(Key key, Annotation<> annotation)
    {
        const auto new_cost = get_cost(annotation);
        const auto it = std::find_if(m_entries.begin(), m_entries.end(), [&](const auto& entry) { return entry.first == key; });
        if (it == m_entries.end())
        {
            m_entries.emplace_back(key, std::move(annotation));
            return std::optional<CostUpdate>(std::in_place, std::nullopt, new_cost);
        }

        const auto old_cost = get_cost(it->second);
        if (old_cost <= new_cost)
            return std::nullopt;

        it->second = std::move(annotation);
        return std::optional<CostUpdate>(std::in_place, old_cost, new_cost);
    }

private:
    urd::Store<urd::sequence<std::pair<Key, Annotation<>>>> m_entries;
};

/// The function annotation staged by one ground rule firing (one head interval).
class StagedFunctionAnnotation
{
public:
    using Binding = formalism::datalog::FunctionBindingIndex<formalism::FluentTag>;

    void clear() noexcept { m_entry.reset(); }

    [[nodiscard]] Cost fetch_cost(Binding binding, const ygg::ClosedInterval<ygg::float_t>& interval) const noexcept
    {
        const auto* annotation = find(binding, interval);
        return annotation ? get_cost(*annotation) : std::numeric_limits<Cost>::max();
    }

    [[nodiscard]] const Annotation<formalism::FunctionTag>* find(Binding binding, const ygg::ClosedInterval<ygg::float_t>& interval) const noexcept
    { return m_entry && m_entry->binding == binding && m_entry->interval == interval ? &m_entry->annotation : nullptr; }

    /// Same replacement rule as NumericIntervalAnnotations: an equal interval is replaced only by a strictly cheaper annotation.
    bool insert(Binding binding, const ygg::ClosedInterval<ygg::float_t>& interval, Annotation<formalism::FunctionTag> annotation)
    {
        assert((!m_entry || (m_entry->binding == binding && m_entry->interval == interval)) && "A ground rule firing stages a single head interval.");
        if (m_entry && get_cost(m_entry->annotation) <= get_cost(annotation))
            return false;

        m_entry.emplace(Entry { binding, interval, std::move(annotation) });
        return true;
    }

private:
    struct Entry
    {
        Binding binding;
        ygg::ClosedInterval<ygg::float_t> interval;
        Annotation<formalism::FunctionTag> annotation;
    };

    std::optional<Entry> m_entry;
};

static_assert(PredicateAnnotationStore<PredicateAnnotations<true>>);
static_assert(PredicateAnnotationStore<StagedPredicateAnnotation>);
static_assert(FunctionAnnotationStore<FunctionAnnotations<true>>);
static_assert(FunctionAnnotationStore<StagedFunctionAnnotation>);
}

#endif
