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

/// The rows of a relation (an input, a derived relation, or an aggregate's body or groups) in each version a literal
/// can read, all indexed on the same access patterns. The state of how a stratum maintains them is the stratum's.
struct Table
{
    size_t arity;
    std::optional<size_t> slot;
    bool is_static = false;
    /// The last column is a weight, least per row of the other columns.
    bool weighted = false;
    bool loaded = false;
    IndexedTuples current, added, removed, frontier;
    /// An aggregate's value of an absent group (COUNT and SUM: 0); its last column holds the value.
    std::optional<ygg::uint_t> fallback;

    explicit Table(size_t arity_) : arity(arity_) {}

    size_t add_index(const urd::Store<urd::sequence<size_t>>& columns)
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

    /// The row set of a version other than OLD.
    [[nodiscard]] const IndexedTuples& rows(Version version) const noexcept
    {
        assert(version != Version::OLD);
        return version == Version::ADDED ? added : version == Version::REMOVED ? removed : version == Version::FRONTIER ? frontier : current;
    }
    /// Calls f(row) for the rows of the version that match the key on the index (all rows without an index).
    template<typename F>
    void lookup(Version version, size_t index, std::span<const ygg::uint_t> key, const F& f) const
    {
        const auto each = [&](const IndexedTuples& set, auto&& g)
        {
            if (index == npos)
                set.for_each(g);
            else
                set.for_each_match(index, key, g);
        };
        if (version != Version::OLD)
            return each(rows(version), f);
        each(current,
             [&](std::span<const ygg::uint_t> row)
             {
                 if (!added.contains(row))
                     f(row);
             });
        each(removed, f);
    }
    /// About the number of rows lookup() visits.
    [[nodiscard]] size_t count(Version version, size_t index, std::span<const ygg::uint_t> key) const
    {
        if (version != Version::OLD)
            return rows(version).count(index, key);
        return current.count(index, key) + removed.count(index, key);
    }
    [[nodiscard]] bool contains(Version version, std::span<const ygg::uint_t> row) const
    {
        if (version != Version::OLD)
            return rows(version).contains(row);
        return (current.contains(row) && !added.contains(row)) || removed.contains(row);
    }
    [[nodiscard]] size_t memory_usage() const noexcept
    {
        size_t result = 0;
        for (const auto* set : { &current, &added, &removed, &frontier })
            result += set->memory_usage();
        return result;
    }
};
}  // namespace tyr::datalog::database::datalog::detail

#endif
