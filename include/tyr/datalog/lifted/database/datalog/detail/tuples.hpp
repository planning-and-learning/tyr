/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_DATALOG_DETAIL_TUPLES_HPP_
#define TYR_DATALOG_DATABASE_DATALOG_DETAIL_TUPLES_HPP_

#include "yggdrasil/core/config.hpp"

#include <cstddef>
#include <limits>
#include <optional>
#include <ranges>
#include <span>
#include <urd/elements/hash.hpp>
#include <urd/memory.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/pools/pools.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>

/// Row storage of the evaluator (evaluator.hpp): fixed-arity tuples of object ids.
namespace tyr::datalog::database::datalog::detail
{
/// Rows of one arity, interned in a urd pool; a row is its handle.
using Rows = urd::Store<urd::pool<urd::Store<urd::sequence<ygg::uint_t>, urd::frozen>>>;
using RowHandle = Rows::handle_type;
/// What is kept per row of a pool, addressed by its handle.
template<typename T>
using PerRow = urd::Store<urd::sequence<T, RowHandle>>;

/// Whether what is kept per row has an entry for the row.
template<typename T>
[[nodiscard]] bool has_entry(const PerRow<T>& entries, RowHandle handle) noexcept
{ return handle < entries.position(entries.end()); }

/// The handles of the rows of a urd pool: they are dense, from 0 to size() - 1, as erasing moves the last row into the
/// erased row's handle.
template<typename Pool>
auto row_handles(const Pool& rows)
{
    using Handle = typename Pool::handle_type;
    return std::views::iota(ygg::uint_t { 0 }, ygg::to_uint_t(rows.size())) | std::views::transform([](ygg::uint_t i) { return Handle(i); });
}

/// Erases a row and what is kept for it: the pool's last row takes the erased row's handle, and so does its entry.
template<typename... T>
void erase_row(Rows& rows, RowHandle handle, PerRow<T>&... per_row)
{
    const auto moved = rows.erase(handle);
    (
        [&](auto& entries)
        {
            if (!moved.is_none())
                entries[handle] = std::move(entries[moved]);
            entries.pop_back();
        }(per_row),
        ...);
}

/// The rows of a tuple set by the hash of some columns (an access pattern): per key hash a doubly linked list of rows
/// and their number, so inserting and removing a row are O(1) however many rows share its key.
class TupleIndex
{
    struct Head
    {
        RowHandle first;
        ygg::uint_t count = 0;
    };

    urd::Store<urd::sequence<size_t>> m_columns;
    urd::Store<urd::map<ygg::hash_t, Head>> m_heads;
    PerRow<RowHandle> m_next;
    PerRow<RowHandle> m_previous;
    PerRow<ygg::hash_t> m_hashes;

    void link(RowHandle row, RowHandle previous, RowHandle next, ygg::hash_t hash)
    {
        if (previous.is_none())
            m_heads[hash].first = row;
        else
            m_next[previous] = row;
        if (!next.is_none())
            m_previous[next] = row;
    }

public:
    explicit TupleIndex(urd::Store<urd::sequence<size_t>> columns) : m_columns(std::move(columns)) {}

    [[nodiscard]] const urd::Store<urd::sequence<size_t>>& columns() const noexcept { return m_columns; }
    [[nodiscard]] ygg::hash_t hash(std::span<const ygg::uint_t> row) const noexcept
    {
        return urd::hash_range(m_columns | std::views::transform([&](size_t column) { return row[column]; }));
    }
    /// Indexes a row new to the pool: its handle is the pool's last.
    void insert(RowHandle handle, std::span<const ygg::uint_t> row)
    {
        const auto key = hash(row);
        const auto [it, inserted] = m_heads.try_emplace(key, Head { handle, 0 });
        const auto next = inserted ? RowHandle() : it->second.first;
        if (!inserted)
        {
            m_previous[next] = handle;
            it->second.first = handle;
        }
        ++it->second.count;
        m_next.push_back(next);
        m_previous.push_back(RowHandle());
        m_hashes.push_back(key);
    }
    /// Removes a row as the pool erases it: `moved`, the handle the pool's last row had (none if the erased row was the
    /// last), now names the erased row's handle.
    void erase(RowHandle erased, RowHandle moved)
    {
        const auto previous = m_previous[erased], next = m_next[erased];
        const auto head = m_heads.find(m_hashes[erased]);
        if (--head->second.count == 0)
            m_heads.erase(head);
        else if (previous.is_none())
            head->second.first = next;
        if (!previous.is_none())
            m_next[previous] = next;
        if (!next.is_none())
            m_previous[next] = previous;
        if (!moved.is_none())
        {
            // The moved row's links may name the erased row's neighbours; read them after the unlinking above.
            m_previous[erased] = m_previous[moved];
            m_next[erased] = m_next[moved];
            m_hashes[erased] = m_hashes[moved];
            link(erased, m_previous[erased], m_next[erased], m_hashes[erased]);
        }
        m_next.pop_back();
        m_previous.pop_back();
        m_hashes.pop_back();
    }
    void clear() noexcept
    {
        m_heads.clear();
        m_next.clear();
        m_previous.clear();
        m_hashes.clear();
    }
    /// Calls f(handle) for the rows whose key hashes like `key`'s values.
    template<typename F>
    void for_each_candidate(std::span<const ygg::uint_t> key, const F& f) const
    {
        const auto it = m_heads.find(urd::hash_range(key));
        if (it == m_heads.end())
            return;
        for (auto handle = it->second.first; !handle.is_none(); handle = m_next[handle])
            f(handle);
    }
    /// The number of rows whose key hashes like `key`'s values.
    [[nodiscard]] size_t count(std::span<const ygg::uint_t> key) const
    {
        const auto it = m_heads.find(urd::hash_range(key));
        return it == m_heads.end() ? 0 : it->second.count;
    }
    [[nodiscard]] size_t memory_usage() const noexcept
    {
        return urd::memory_usage(m_columns) + urd::memory_usage(m_heads) + urd::memory_usage(m_next) + urd::memory_usage(m_previous)
               + urd::memory_usage(m_hashes);
    }
};

/// A tuple set with indexes on column subsets (access patterns), kept valid under erase.
class IndexedTuples
{
    Rows m_rows;
    urd::Store<urd::sequence<TupleIndex>> m_indexes;

public:
    [[nodiscard]] const Rows& rows() const noexcept { return m_rows; }
    [[nodiscard]] size_t size() const noexcept { return m_rows.size(); }
    [[nodiscard]] bool empty() const noexcept { return m_rows.size() == 0; }
    [[nodiscard]] bool contains(std::span<const ygg::uint_t> row) const { return !m_rows.find(row).is_none(); }
    [[nodiscard]] std::span<const ygg::uint_t> at(RowHandle handle) const { return m_rows.at(handle); }
    [[nodiscard]] const urd::Store<urd::sequence<size_t>>& columns(size_t index) const noexcept { return m_indexes[index].columns(); }

    /// The index on these columns, made if new; existing rows are indexed.
    size_t add_index(const urd::Store<urd::sequence<size_t>>& columns)
    {
        for (size_t i = 0; i < m_indexes.size(); ++i)
            if (m_indexes[i].columns() == columns)
                return i;
        auto& index = m_indexes.emplace_back(columns);
        for (const auto handle : row_handles(m_rows))
            index.insert(handle, m_rows.at(handle));
        return m_indexes.size() - 1;
    }
    /// Whether the row is new.
    bool insert(std::span<const ygg::uint_t> row)
    {
        const auto [handle, inserted] = m_rows.insert(row);
        if (inserted)
            for (auto& index : m_indexes)
                index.insert(handle, row);
        return inserted;
    }
    /// Whether the row was present.
    bool erase(std::span<const ygg::uint_t> row)
    {
        const auto handle = m_rows.find(row);
        if (handle.is_none())
            return false;
        const auto moved = m_rows.erase(handle);
        for (auto& index : m_indexes)
            index.erase(handle, moved);
        return true;
    }
    void clear() noexcept
    {
        m_rows.clear();
        for (auto& index : m_indexes)
            index.clear();
    }
    /// About the number of rows for_each_match visits: all rows without an index.
    [[nodiscard]] size_t count(size_t index, std::span<const ygg::uint_t> key) const
    { return index == std::numeric_limits<size_t>::max() ? m_rows.size() : m_indexes[index].count(key); }
    /// Calls f(row) for every row whose index columns equal `key`.
    template<typename F>
    void for_each_match(size_t index, std::span<const ygg::uint_t> key, const F& f) const
    {
        const auto& columns = m_indexes[index].columns();
        m_indexes[index].for_each_candidate(key,
                                            [&](RowHandle handle)
                                            {
                                                const auto row = m_rows.at(handle);
                                                for (size_t i = 0; i < columns.size(); ++i)
                                                    if (row[columns[i]] != key[i])
                                                        return;
                                                f(row);
                                            });
    }
    template<typename F>
    void for_each(const F& f) const
    {
        for (const auto handle : row_handles(m_rows))
            f(m_rows.at(handle));
    }
    [[nodiscard]] size_t memory_usage() const noexcept
    {
        size_t result = urd::memory_usage(m_rows);
        for (const auto& index : m_indexes)
            result += index.memory_usage();
        return result;
    }
};
}  // namespace tyr::datalog::database::datalog::detail

#endif
