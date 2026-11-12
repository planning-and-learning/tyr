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
/// A set of rows of one arity, at positions 0, ..., size() - 1: erasing a row moves the last one into its position.
class RowSet
{
    using Pool = urd::Store<urd::pool<urd::Store<urd::sequence<ygg::uint_t>, urd::frozen>>>;
    using Handle = Pool::handle_type;

    Pool m_rows;

public:
    explicit RowSet(size_t /*arity*/) {}

    [[nodiscard]] size_t size() const noexcept { return m_rows.size(); }
    [[nodiscard]] bool empty() const noexcept { return m_rows.size() == 0; }
    [[nodiscard]] bool contains(std::span<const ygg::uint_t> row) const { return !m_rows.find(row).is_none(); }
    [[nodiscard]] std::optional<ygg::uint_t> find(std::span<const ygg::uint_t> row) const
    {
        const auto handle = m_rows.find(row);
        return handle.is_none() ? std::nullopt : std::optional<ygg::uint_t>(handle.value);
    }
    /// The row's position; a present row keeps its own.
    ygg::uint_t insert(std::span<const ygg::uint_t> row) { return m_rows.insert(row).first.value; }
    [[nodiscard]] std::span<const ygg::uint_t> operator[](ygg::uint_t position) const { return m_rows.at(Handle(position)); }
    /// Erases the row at `position`; the last row moves into it.
    void erase(ygg::uint_t position) { m_rows.erase(Handle(position)); }
    void clear() { m_rows.clear(); }
    [[nodiscard]] size_t memory_usage() const noexcept { return urd::memory_usage(m_rows); }
};

/// The rows of a tuple set by the hash of some columns (an access pattern): per key hash a doubly linked list of row
/// positions and their number, so inserting and removing a row are O(1) however many rows share its key.
class TupleIndex
{
    static constexpr ygg::uint_t none = std::numeric_limits<ygg::uint_t>::max();

    struct Head
    {
        ygg::uint_t first;
        ygg::uint_t count;
    };

    urd::Store<urd::sequence<size_t>> m_columns;
    urd::Store<urd::map<ygg::hash_t, Head>> m_heads;
    urd::Store<urd::sequence<ygg::uint_t>> m_next;
    urd::Store<urd::sequence<ygg::uint_t>> m_previous;
    urd::Store<urd::sequence<ygg::hash_t>> m_hashes;

    void link(ygg::uint_t position, ygg::uint_t previous, ygg::uint_t next, ygg::hash_t hash)
    {
        if (previous == none)
            m_heads[hash].first = position;
        else
            m_next[previous] = position;
        if (next != none)
            m_previous[next] = position;
    }

public:
    explicit TupleIndex(urd::Store<urd::sequence<size_t>> columns) : m_columns(std::move(columns)) {}

    [[nodiscard]] const urd::Store<urd::sequence<size_t>>& columns() const noexcept { return m_columns; }
    [[nodiscard]] ygg::hash_t hash(std::span<const ygg::uint_t> row) const noexcept
    {
        return urd::hash_range(m_columns | std::views::transform([&](size_t column) { return row[column]; }));
    }
    /// Indexes the row at `position`, the next position of the set.
    void insert(ygg::uint_t position, std::span<const ygg::uint_t> row)
    {
        const auto key = hash(row);
        const auto [it, inserted] = m_heads.try_emplace(key, Head { position, 0 });
        const auto next = inserted ? none : it->second.first;
        if (!inserted)
        {
            m_previous[next] = position;
            it->second.first = position;
        }
        ++it->second.count;
        m_next.push_back(next);
        m_previous.push_back(none);
        m_hashes.push_back(key);
    }
    /// Removes the row at `position`, then moves the set's last row (`last`) into it, as RowSet::erase does.
    void erase(ygg::uint_t position, ygg::uint_t last)
    {
        const auto previous = m_previous[position], next = m_next[position];
        const auto head = m_heads.find(m_hashes[position]);
        if (--head->second.count == 0)
            m_heads.erase(head);
        else if (previous == none)
            head->second.first = next;
        if (previous != none)
            m_next[previous] = next;
        if (next != none)
            m_previous[next] = previous;
        if (position != last)
        {
            m_previous[position] = m_previous[last];
            m_next[position] = m_next[last];
            m_hashes[position] = m_hashes[last];
            link(position, m_previous[position], m_next[position], m_hashes[position]);
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
    /// Calls f(position) for the rows whose key hashes like `key`'s values.
    template<typename F>
    void for_each_candidate(std::span<const ygg::uint_t> key, const F& f) const
    {
        const auto it = m_heads.find(urd::hash_range(key));
        if (it == m_heads.end())
            return;
        for (auto position = it->second.first; position != none; position = m_next[position])
            f(position);
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
    RowSet m_rows;
    urd::Store<urd::sequence<TupleIndex>> m_indexes;

public:
    explicit IndexedTuples(size_t arity) : m_rows(arity) {}

    [[nodiscard]] const RowSet& rows() const noexcept { return m_rows; }
    [[nodiscard]] size_t size() const noexcept { return m_rows.size(); }
    [[nodiscard]] bool empty() const noexcept { return m_rows.empty(); }
    [[nodiscard]] bool contains(std::span<const ygg::uint_t> row) const { return m_rows.contains(row); }
    std::span<const ygg::uint_t> operator[](ygg::uint_t index) const noexcept { return m_rows[index]; }
    [[nodiscard]] const urd::Store<urd::sequence<size_t>>& columns(size_t index) const noexcept { return m_indexes[index].columns(); }

    /// The index on these columns, made if new; existing rows are indexed.
    size_t add_index(const urd::Store<urd::sequence<size_t>>& columns)
    {
        for (size_t i = 0; i < m_indexes.size(); ++i)
            if (m_indexes[i].columns() == columns)
                return i;
        auto& index = m_indexes.emplace_back(columns);
        for (ygg::uint_t row = 0; row < m_rows.size(); ++row)
            index.insert(row, m_rows[row]);
        return m_indexes.size() - 1;
    }
    bool insert(std::span<const ygg::uint_t> row)
    {
        const auto size = m_rows.size();
        const auto position = m_rows.insert(row);
        if (m_rows.size() == size)
            return false;
        for (auto& index : m_indexes)
            index.insert(position, row);
        return true;
    }
    bool erase(std::span<const ygg::uint_t> row)
    {
        const auto position = m_rows.find(row);
        if (!position)
            return false;
        const auto last = ygg::to_uint_t(m_rows.size() - 1);
        for (auto& index : m_indexes)
            index.erase(*position, last);
        m_rows.erase(*position);
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
                                            [&](ygg::uint_t position)
                                            {
                                                const auto row = m_rows[position];
                                                for (size_t i = 0; i < columns.size(); ++i)
                                                    if (row[columns[i]] != key[i])
                                                        return;
                                                f(row);
                                            });
    }
    template<typename F>
    void for_each(const F& f) const
    {
        for (ygg::uint_t position = 0; position < m_rows.size(); ++position)
            f(m_rows[position]);
    }
    [[nodiscard]] size_t memory_usage() const noexcept
    {
        size_t result = m_rows.memory_usage();
        for (const auto& index : m_indexes)
            result += index.memory_usage();
        return result;
    }
};
}  // namespace tyr::datalog::database::datalog::detail

#endif
