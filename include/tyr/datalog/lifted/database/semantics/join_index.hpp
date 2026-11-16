/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_JOIN_INDEX_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_JOIN_INDEX_HPP_

#include "tyr/datalog/lifted/database/semantics/plans.hpp"
#include "tyr/datalog/lifted/database/semantics/relation_view.hpp"

#include <cstddef>
#include <ranges>
#include <span>
#include <urd/elements/hash.hpp>
#include <urd/memory.hpp>
#include <urd/repr/maps/postings.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>

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
bool join_keys_equal(const L& left, std::span<const ColumnSlice> left_keys, const R& right, std::span<const ColumnSlice> right_keys) noexcept
{
    for (size_t i = 0; i < left_keys.size(); ++i)
        if (field_value(left, left_keys[i].position) != field_value(right, right_keys[i].position))
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

}  // namespace tyr::datalog::database

#endif
