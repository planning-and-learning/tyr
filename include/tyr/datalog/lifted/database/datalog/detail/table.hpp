/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_DATALOG_DETAIL_TABLE_HPP_
#define TYR_DATALOG_DATABASE_DATALOG_DETAIL_TABLE_HPP_

#include "yggdrasil/core/config.hpp"
#include "yggdrasil/formalism/relation_builder.hpp"
#include "yggdrasil/formalism/values.hpp"

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

using Relation = urd::builder<ygg::Relation<ygg::Schema>>;
/// A row stored in a relation, read in place.
using StoredRow = ygg::RowPool::row_type;

/// A relation of `arity` object ids: its columns labeled by position.
inline Relation object_relation(size_t arity)
{
    auto columns = urd::Store<urd::sequence<ygg::ColumnLayout>>();
    for (size_t c = 0; c < arity; ++c)
        columns.push_back(ygg::ColumnLayout { urd::index<ygg::Column>(ygg::to_uint_t(c)), ygg::type_id<ygg::uint_t> });
    return Relation(std::span<const ygg::ColumnLayout>(columns));
}
/// The values of object ids, valid until the next call on the thread: a row or key being looked up.
[[nodiscard]] inline std::span<const ygg::Value> encoded(std::span<const ygg::uint_t> ids)
{
    thread_local auto values = urd::Store<urd::sequence<ygg::Value>>();
    values.resize(ids.size());
    for (size_t c = 0; c < ids.size(); ++c)
        values[c] = ygg::encode_value(ids[c]);
    return values;
}

/// The rows of a relation (an input, a derived relation, or an aggregate's body or groups) in each version a literal
/// can read: a relation per row set, all with the same columns (object ids) and indexes (access patterns). The state
/// of how a stratum maintains them is the stratum's. Rows come and go as object ids; the relations store them as
/// values.
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

    explicit Table(size_t arity_) :
        arity(arity_),
        current(object_relation(arity_)),
        added(object_relation(arity_)),
        removed(object_relation(arity_)),
        frontier(object_relation(arity_))
    {
    }

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
        const auto before = set.size();
        set.insert(encoded(row));
        return set.size() != before;
    }
    /// Whether the row was in the version (other than OLD).
    bool erase(Version version, std::span<const ygg::uint_t> row)
    {
        auto& set = rows(version);
        const auto at = set.find(encoded(row));
        if (!at)
            return false;
        set.erase(*at);
        return true;
    }
    [[nodiscard]] bool contains(Version version, std::span<const ygg::uint_t> row) const { return contains(version, encoded(row)); }
    /// By the row's values.
    [[nodiscard]] bool contains(Version version, std::span<const ygg::Value> values) const
    {
        if (version != Version::OLD)
            return rows(version).contains(values);
        return (current.contains(values) && !added.contains(values)) || removed.contains(values);
    }
    /// Calls f(row) for the rows of a version other than OLD, as object ids.
    template<typename F>
    void for_each(Version version, const F& f) const
    {
        const auto& set = rows(version);
        auto row = urd::Store<urd::sequence<ygg::uint_t>>(arity);
        for (const auto handle : set.storage())
        {
            const auto stored = set.storage().at(handle);
            for (size_t c = 0; c < arity; ++c)
                row[c] = static_cast<ygg::uint_t>(stored.get(c));
            f(std::span<const ygg::uint_t>(row));
        }
    }
    /// Calls f(row) for the stored rows of the version whose index columns hold `key`, as values (all rows without an
    /// index).
    template<typename F>
    void lookup(Version version, size_t index, std::span<const ygg::Value> key, const F& f) const
    {
        const auto each = [&](const Relation& set, auto&& g)
        {
            if (index == npos)
            {
                for (const auto handle : set.storage())
                    g(set.storage().at(handle));
                return;
            }
            const auto [first, last] = set.index(index).equal_range(key);
            for (auto it = first; it != last; ++it)
                g(set.storage().at(*it));
        };
        if (version != Version::OLD)
            return each(rows(version), f);
        each(current,
             [&](const StoredRow& row)
             {
                 if (!added.contains(row))
                     f(row);
             });
        each(removed, f);
    }
    /// About the number of rows lookup() visits: exact but for OLD, which counts the added rows too.
    [[nodiscard]] size_t count(Version version, size_t index, std::span<const ygg::Value> key) const
    {
        const auto in = [&](const Relation& set) { return index == npos ? set.size() : set.index(index).count(key); };
        if (version != Version::OLD)
            return in(rows(version));
        return in(current) + in(removed);
    }
    [[nodiscard]] size_t memory_usage() const noexcept
    { return current.memory_usage() + added.memory_usage() + removed.memory_usage() + frontier.memory_usage(); }
};
}  // namespace tyr::datalog::database::datalog::detail

#endif
