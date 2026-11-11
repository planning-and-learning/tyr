/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_JOIN_INDEX_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_JOIN_INDEX_HPP_

#include "tyr/datalog/lifted/database/semantics/plans.hpp"
#include "tyr/datalog/lifted/database/semantics/relation_view.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <ranges>
#include <span>
#include <stdexcept>
#include <tuple>
#include <urd/elements/hash.hpp>
#include <urd/memory.hpp>
#include <urd/repr/maps/postings.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/std_set.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <vector>

namespace tyr::datalog::database
{

/// Reusable hash index for one immutable relation and ordered key columns. Indexed inputs are told apart by their
/// identity (an interned relation's layer and index, a builder's address); rows stay unchanged until the index is
/// destroyed. No row pointers or column labels are retained.
class JoinIndex
{
private:
    ygg::RelationIdentity m_relation;
    urd::Store<urd::sequence<ColumnSlice>> m_keys;
    urd::Store<urd::map<ygg::hash_t, size_t>, urd::multi, urd::retains> m_index;

public:
    template<RelationViewConcept V>
    JoinIndex(const V& build, std::span<const ColumnSlice> keys);

    [[nodiscard]] const urd::Store<urd::map<ygg::hash_t, size_t>, urd::multi, urd::retains>& index() const noexcept { return m_index; }

    [[nodiscard]] ygg::RelationIdentity relation() const noexcept { return m_relation; }
    /// The key slices as a span: lookups probe with a std::tuple of the relation identity and one.
    [[nodiscard]] std::span<const ColumnSlice> key_span() const noexcept { return m_keys; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&JoinIndex::relation, &JoinIndex::key_span> {}; }
};

/// Shares indexes across plans using the same ordered key slices, keyed by the indexed input's identity. Indexed rows
/// must remain stationary and unchanged: clear before any source is mutated, destroyed, moved, or reused. No schemas or
/// views are retained.
class JoinIndexCache
{
private:
    urd::Store<urd::set<JoinIndex>> m_indexes;

public:
    /// Hits borrow the lookup key and allocate nothing. Returned references
    /// remain valid until an insertion or clear().
    template<RelationViewConcept V>
    const JoinIndex& get_or_create(const V& relation, std::span<const ColumnSlice> keys);

    [[nodiscard]] size_t size() const noexcept { return m_indexes.size(); }
    void clear() noexcept { m_indexes.clear(); }
};

}  // namespace tyr::datalog::database

namespace tyr::datalog::database
{

namespace detail
{
/// The value of a row's field: a stored row's, or a row of values' (a query's frame of variables).
inline ygg::Value field_value(const ygg::RowPool::row_type& row, size_t position) noexcept { return row.get(position); }
inline ygg::Value field_value(std::span<const ygg::Value> values, size_t position) noexcept { return values[position]; }

/// Hash the key fields' canonical values in logical key order, without materializing a key: as urd::hash_range of them.
template<typename R>
ygg::hash_t join_key_hash(const R& row, std::span<const ColumnSlice> keys) noexcept
{
    return urd::hash_range(keys | std::views::transform([&](const ColumnSlice& key) { return field_value(row, key.position); }));
}

template<typename L, typename R>
bool join_keys_equal(const L& lhs, std::span<const ColumnSlice> lhs_keys, const R& rhs, std::span<const ColumnSlice> rhs_keys) noexcept
{
    for (size_t i = 0; i < lhs_keys.size(); ++i)
        if (field_value(lhs, lhs_keys[i].position) != field_value(rhs, rhs_keys[i].position))
            return false;
    return true;
}

template<RelationViewConcept V>
void build_join_index(const V& build, std::span<const ColumnSlice> keys, urd::Store<urd::map<ygg::hash_t, size_t>, urd::multi, urd::retains>& index)
{
    index.clear();
    index.reserve(build.size());
    for (size_t i = 0; i < build.size(); ++i)
        index.insert({ join_key_hash(build.row(i).get_values(), keys), i });
}
}  // namespace detail

template<RelationViewConcept V>
JoinIndex::JoinIndex(const V& build, std::span<const ColumnSlice> keys) : m_relation(build.relation_identity()), m_keys(keys.begin(), keys.end())
{
    assert(std::ranges::all_of(keys, [&build](ColumnSlice key) { return key.position < build.columns().size(); }));
    if (!keys.empty())
        detail::build_join_index(build, keys, m_index);
}

template<RelationViewConcept V>
const JoinIndex& JoinIndexCache::get_or_create(const V& relation, std::span<const ColumnSlice> keys)
{
    const auto found = m_indexes.find(std::make_tuple(relation.relation_identity(), keys));
    if (found != m_indexes.end())
        return *found;
    return *m_indexes.emplace(relation, keys).first;
}

}  // namespace tyr::datalog::database

#endif
