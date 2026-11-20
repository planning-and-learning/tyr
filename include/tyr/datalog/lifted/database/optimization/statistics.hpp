/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_OPTIMIZATION_STATISTICS_HPP_
#define TYR_DATALOG_DATABASE_OPTIMIZATION_STATISTICS_HPP_

#include "tyr/datalog/lifted/database/semantics/operations.hpp"
#include "tyr/datalog/lifted/database/semantics/relation_view.hpp"
#include "tyr/datalog/lifted/database/syntax/query.hpp"

#include <iterator>
#include <optional>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>

namespace tyr::datalog::database
{
struct RelationStatistics
{
    double rows = 0;
    urd::Store<urd::map<urd::index<ygg::Column>, double>> distinct;
};

/// What is known about the data. Everything is optional; nothing known means unbounded.
struct Statistics
{
    /// Domain size; empty means an unbounded domain.
    std::optional<size_t> objects;
    /// Measured inputs, keyed by input slot.
    urd::Store<urd::map<size_t, RelationStatistics>> inputs;
    /// Observed results of queries in the roots' repository; they override estimates.
    urd::Store<urd::map<urd::index<Query<>>, RelationStatistics>> expressions;
};

/// Row and per-column distinct counts of one relation.
template<ygg::RelationViewConcept<ygg::Schema> Relation>
RelationStatistics measure_relation(const Relation& relation)
{
    RelationStatistics stats;
    stats.rows = static_cast<double>(relation.get_rows().size());
    const auto columns = relation.columns();
    auto seen = urd::Store<urd::set<ygg::Value>>();
    for (size_t c = 0; c < columns.size(); ++c)
    {
        seen.clear();
        for (const auto& held : relation.get_rows())
            ygg::with_values(held, [&](const auto& values) { seen.insert(*std::ranges::next(std::ranges::begin(values), static_cast<std::ptrdiff_t>(c))); });
        stats.distinct[columns[c].label] = static_cast<double>(seen.size());
    }
    return stats;
}
}  // namespace tyr::datalog::database

#endif
