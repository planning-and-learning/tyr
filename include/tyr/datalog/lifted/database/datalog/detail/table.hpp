/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_DATALOG_DETAIL_TABLE_HPP_
#define TYR_DATALOG_DATABASE_DATALOG_DETAIL_TABLE_HPP_

#include "tyr/datalog/lifted/database/datalog/detail/tuples.hpp"
#include "yggdrasil/core/config.hpp"

#include <cassert>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <urd/memory.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::datalog::database::datalog::detail
{
constexpr size_t npos = std::numeric_limits<size_t>::max();

/// Which rows of a table a literal reads: one of the table's row sets, or OLD, the rows before the update (the current
/// ones without the added, with the removed).
enum class Version : uint8_t
{
    CURRENT,
    ADDED,
    REMOVED,
    /// The rows found last by a semi-naive iteration (insertion or over-deletion).
    FRONTIER,
    OLD,
};

/// A row set of a table: rows of object ids in a pool with an index per access pattern (urd::multi_indexed).
using Relation = Rows;
/// A row stored in a relation, read in place.
using StoredRow = decltype(std::declval<const Rows&>().at(RowHandle()));

/// The rows of a relation (an input, a derived relation, or an aggregate's body or groups) in each version a literal
/// can read: a relation per row set, all with the same columns (object ids) and indexes (access patterns). The state
/// of how a stratum maintains them is the stratum's.
struct Table
{
    size_t arity;
    std::optional<size_t> slot;
    bool is_static = false;
    /// The last column is a weight, least per row of the other columns.
    bool weighted = false;
    bool loaded = false;
    /// An aggregate's value of an absent group (COUNT and SUM: 0); its last column holds the value.
    std::optional<ygg::uint_t> fallback;
    Relation current, added, removed, frontier;

    explicit Table(size_t arity_) : arity(arity_) {}

    /// The index on these columns, in every version; made where new.
    size_t add_index(std::span<const size_t> columns)
    {
        const auto index = current.add_index(columns);
        for (auto* set : { &added, &removed, &frontier })
            set->add_index(columns);
        return index;
    }

    [[nodiscard]] bool changed() const noexcept { return !added.empty() || !removed.empty(); }

    void clear_changes() noexcept
    {
        added.clear();
        removed.clear();
    }

    void clear() noexcept
    {
        for (auto* set : { &current, &added, &removed, &frontier })
            set->clear();
    }

    /// The relation of a version other than OLD.
    [[nodiscard]] const Relation& rows(Version version) const noexcept
    {
        assert(version != Version::OLD);
        return version == Version::ADDED ? added : version == Version::REMOVED ? removed : version == Version::FRONTIER ? frontier : current;
    }

    [[nodiscard]] Relation& rows(Version version) noexcept
    {
        assert(version != Version::OLD);
        return version == Version::ADDED ? added : version == Version::REMOVED ? removed : version == Version::FRONTIER ? frontier : current;
    }

    [[nodiscard]] size_t size(Version version) const noexcept { return rows(version).size(); }

    [[nodiscard]] bool empty(Version version) const noexcept { return rows(version).empty(); }

    void clear(Version version) noexcept { rows(version).clear(); }

    /// Whether the row is new to the version (other than OLD).
    bool insert(Version version, std::span<const ygg::uint_t> row)
    {
        auto& set = rows(version);
        return set.insert(row).second;
    }

    /// Whether the row was in the version (other than OLD).
    bool erase(Version version, std::span<const ygg::uint_t> row)
    {
        auto& set = rows(version);
        const auto at = set.find(row);
        if (at.is_none())
            return false;
        set.erase(at);
        return true;
    }

    [[nodiscard]] bool contains(Version version, std::span<const ygg::uint_t> row) const
    {
        const auto in = [&](const Relation& set) { return !set.find(row).is_none(); };
        if (version != Version::OLD)
            return in(rows(version));
        return (in(current) && !in(added)) || in(removed);
    }

    /// Calls f(row) for the rows of a version other than OLD, as object ids.
    template<typename F>
    void for_each(Version version, const F& f) const
    {
        const auto& set = rows(version);
        for (const auto handle : set)
            f(std::span<const ygg::uint_t>(set.at(handle)));
    }

    /// Calls f(row) for the stored rows of the version whose index columns hold `key` (all rows without an index).
    template<typename F>
    void lookup(Version version, size_t index, std::span<const ygg::uint_t> key, const F& f) const
    {
        const auto each = [&](const Relation& set, auto&& g)
        {
            if (index == npos)
            {
                for (const auto handle : set)
                    g(set.at(handle));
                return;
            }
            const auto [first, last] = set.index(index).equal_range(key);
            for (auto it = first; it != last; ++it)
                g(set.at(*it));
        };
        if (version != Version::OLD)
            return each(rows(version), f);
        each(current,
             [&](const StoredRow& row)
             {
                 if (added.find(row).is_none())
                     f(row);
             });
        each(removed, f);
    }

    /// About the number of rows lookup() visits: exact but for OLD, which counts the added rows too.
    [[nodiscard]] size_t count(Version version, size_t index, std::span<const ygg::uint_t> key) const
    {
        const auto in = [&](const Relation& set) { return index == npos ? set.size() : set.index(index).count(key); };
        if (version != Version::OLD)
            return in(rows(version));
        return in(current) + in(removed);
    }

    [[nodiscard]] size_t memory_usage() const noexcept
    { return urd::memory_usage(current) + urd::memory_usage(added) + urd::memory_usage(removed) + urd::memory_usage(frontier); }
};
}  // namespace tyr::datalog::database::datalog::detail

#endif
