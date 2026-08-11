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

#ifndef TYR_DATALOG_LIFTED_POLICIES_ANNOTATION_TYPES_HPP_
#define TYR_DATALOG_LIFTED_POLICIES_ANNOTATION_TYPES_HPP_

#include "tyr/datalog/policies/annotation_types.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <oneapi/tbb/spin_mutex.h>
#include <type_traits>
#include <utility>
#include <vector>
#include <yggdrasil/containers/segmented_vector.hpp>

namespace tyr::datalog
{

template<::tyr::formalism::RelationKind R>
struct WitnessRuleKey<LiftedTag, R>
{
    using type = ::tyr::formalism::datalog::RuleBindingView<R>;
};

template<::tyr::formalism::RelationKind R, std::default_initializable Value>
class ConcurrentRelationMap
{
public:
    using Key = std::conditional_t<std::same_as<R, ::tyr::formalism::PredicateTag>,
                                   ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag>,
                                   ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag>>;
    using Relation = decltype(std::declval<Key>().get_index().relation);
    using Row = decltype(std::declval<Key>().get_index().row);

    explicit ConcurrentRelationMap(size_t num_relations = 0) : m_lanes(num_relations) {}
    ConcurrentRelationMap(const ConcurrentRelationMap&) = delete;
    ConcurrentRelationMap& operator=(const ConcurrentRelationMap&) = delete;
    ConcurrentRelationMap(ConcurrentRelationMap&&) noexcept = default;
    ConcurrentRelationMap& operator=(ConcurrentRelationMap&&) noexcept = default;

    void initialize(size_t num_relations) { m_lanes = std::vector<Lane>(num_relations); }

    /// Evaluation is quiescent here; old slots remain allocated for reuse.
    void clear() noexcept { ++m_generation; }

    template<std::invocable<const Value&> Read>
    std::invoke_result_t<Read, const Value&> read(Key key, Read&& read, std::invoke_result_t<Read, const Value&> missing) const
    {
        const auto* slot = find_slot(key);
        if (!slot)
            return missing;

        const auto lock = oneapi::tbb::spin_mutex::scoped_lock(slot->mutex);
        return slot->generation == m_generation ? std::forward<Read>(read)(slot->value) : missing;
    }

    template<std::invocable<Value&, bool> Update>
    decltype(auto) update(Key key, Update&& update)
    {
        auto& slot = get_or_create_slot(key);
        const auto lock = oneapi::tbb::spin_mutex::scoped_lock(slot.mutex);
        const auto initialized = slot.generation == m_generation;
        slot.generation = m_generation;
        return std::forward<Update>(update)(slot.value, initialized);
    }

    /// Concurrent writers must have completed before calling find.
    const Value* find(Key key) const noexcept
    {
        const auto* slot = find_slot(key);
        return slot && slot->generation == m_generation ? &slot->value : nullptr;
    }

    const Value* find(Relation relation, Row row) const noexcept
    {
        const auto* slot = find_slot(relation, row);
        return slot && slot->generation == m_generation ? &slot->value : nullptr;
    }

private:
    struct Slot
    {
        mutable oneapi::tbb::spin_mutex mutex;
        uint64_t generation { 0 };
        Value value {};
    };

    using Lane = ygg::SegmentedVector<Slot, 32, true>;

    Slot& get_or_create_slot(Key key)
    {
        const auto index = key.get_index();
        const auto relation = ygg::uint_t(index.relation);
        const auto row = ygg::uint_t(index.row);
        assert(relation < m_lanes.size());

        auto& lane = m_lanes[relation];
        while (lane.size() <= row)
            lane.emplace_back();
        return lane[row];
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

class ConcurrentPredicateAnnotations
{
public:
    using Key = PredicateAnnotationHead<LiftedTag>;

    explicit ConcurrentPredicateAnnotations(size_t num_relations) : m_annotations(num_relations) {}

    void clear() noexcept { m_annotations.clear(); }

    Cost fetch_cost(Key key) const noexcept
    {
        return m_annotations.read(key, [](const auto& annotation) { return get_cost(annotation); }, std::numeric_limits<Cost>::max());
    }

    bool insert_if_better(Key key, Annotation<LiftedTag> annotation)
    {
        return m_annotations.update(key,
                                    [&](auto& incumbent, bool initialized)
                                    {
                                        if (initialized && get_cost(incumbent) <= get_cost(annotation))
                                            return false;

                                        incumbent = std::move(annotation);
                                        return true;
                                    });
    }

    const Annotation<LiftedTag>* find(Key key) const noexcept { return m_annotations.find(key); }

private:
    ConcurrentRelationMap<::tyr::formalism::PredicateTag, Annotation<LiftedTag>> m_annotations;
};

class ConcurrentFunctionAnnotations
{
public:
    using Binding = NumericSupportKeyT<LiftedTag>;
    using Relation = typename NumericIntervalBindingParts<LiftedTag>::Relation;
    using Key = typename NumericIntervalBindingParts<LiftedTag>::Key;
    using Entry = NumericIntervalAnnotation<LiftedTag>;
    using Entries = std::vector<Entry>;

    explicit ConcurrentFunctionAnnotations(size_t num_relations) : m_annotations(num_relations) {}

    /// Evaluation is quiescent here; old slots and entry buffers remain allocated for reuse.
    void clear() noexcept { m_annotations.clear(); }

    Cost fetch_cost(Binding binding, ygg::ClosedInterval<ygg::float_t> interval) const noexcept
    {
        return m_annotations.read(
            binding,
            [&](const auto& entries)
            {
                const auto* annotation = find_numeric_interval_annotation<LiftedTag>(entries, interval);
                return annotation ? get_cost(*annotation) : std::numeric_limits<Cost>::max();
            },
            std::numeric_limits<Cost>::max());
    }

    bool insert(Binding binding, ygg::ClosedInterval<ygg::float_t> interval, Annotation<LiftedTag, ::tyr::formalism::FunctionTag> annotation)
    {
        if (empty(interval))
            return false;

        return m_annotations.update(binding,
                                    [&](auto& entries, bool initialized)
                                    {
                                        if (!initialized)
                                            entries.clear();
                                        return insert_first_best_numeric_interval_annotation(entries, Entry { interval, std::move(annotation) });
                                    });
    }

    /// Concurrent writers must have completed before calling find.
    const Annotation<LiftedTag, ::tyr::formalism::FunctionTag>* find(Binding binding, ygg::ClosedInterval<ygg::float_t> interval) const noexcept
    {
        const auto* entries = find_entries(binding);
        return entries ? find_numeric_interval_annotation<LiftedTag>(*entries, interval) : nullptr;
    }

    /// Concurrent writers must have completed before calling find_entries.
    const Entries* find_entries(Binding binding) const noexcept { return m_annotations.find(binding); }

    const Entries* find_entries(Relation relation, Key key) const noexcept { return m_annotations.find(relation, key); }

private:
    ConcurrentRelationMap<::tyr::formalism::FunctionTag, Entries> m_annotations;
};

}

#endif
