/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/optimization/details/estimation.hpp"

#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::datalog::database::detail
{
Estimate factor(const RelationStatistics& stats, std::span<const ygg::ColumnLayout> columns, FactorIdentity identity)
{
    Estimate result;
    result.rows = stats.rows;
    Factor single { identity, stats.rows, {} };
    for (size_t i = 0; i < columns.size(); ++i)
    {
        const auto label = columns[i].label;
        const auto found = stats.distinct.find(label);
        const auto ndv = std::min(stats.rows, found == stats.distinct.end() ? stats.rows : found->second);
        result.distinct[label] = ndv;
        single.attributes.push_back({ i, label, ndv });
    }
    result.factors.push_back(std::move(single));
    return result;
}
}

namespace tyr::datalog::database::detail
{
Estimate conjunction(Estimate result, std::span<const urd::index<ygg::Column>> labels)
{
    // ponytail: hand-rolled union-find over a few columns; boost::disjoint_sets needs property maps for no gain here.
    urd::Store<urd::map<urd::index<ygg::Column>, urd::index<ygg::Column>>> parent;
    for (const auto& single : result.factors)
        for (const auto& attribute : single.attributes)
            parent[attribute.column] = attribute.column;
    const auto find = [&](urd::index<ygg::Column> column)
    {
        while (parent.at(column) != column)
            column = parent.at(column);
        return column;
    };
    for (const auto& [lhs, rhs] : result.equalities)
    {
        const auto left = find(lhs), right = find(rhs);
        parent[std::max(left, right)] = std::min(left, right);
    }
    urd::Store<urd::map<urd::index<ygg::Column>, ygg::Value>> bound;
    bool zero = false;
    for (const auto& [column, value] : result.constants)
    {
        const auto [it, inserted] = bound.emplace(find(column), value);
        if (!inserted && it->second != value)
            zero = true;
    }
    urd::Store<urd::map<urd::index<ygg::Column>, urd::Store<urd::sequence<double>>>> domains;
    urd::Store<urd::set<std::pair<FactorIdentity, urd::Store<urd::sequence<std::pair<size_t, urd::index<ygg::Column>>>>>>, urd::sorted> unique;
    double logarithm = 0;
    for (const auto& single : result.factors)
    {
        urd::Store<urd::sequence<std::pair<size_t, urd::index<ygg::Column>>>> bindings;
        bindings.reserve(single.attributes.size());
        for (const auto& attribute : single.attributes)
            bindings.emplace_back(attribute.position, find(attribute.column));
        if (!unique.emplace(single.identity, std::move(bindings)).second)
            continue;
        if (single.rows == 0)
            zero = true;
        else
            logarithm += std::log(single.rows);
        for (const auto& attribute : single.attributes)
            domains[find(attribute.column)].push_back(std::max(1.0, attribute.distinct));
    }
    for (const auto& [column, counts] : domains)
    {
        for (const auto count : counts)
            logarithm -= std::log(count);
        if (!bound.contains(column))
            logarithm += std::log(*std::ranges::min_element(counts));
    }
    result.rows = zero ? 0 : std::exp(std::clamp(logarithm, -max_log_estimate, max_log_estimate));
    result.distinct.clear();
    for (const auto label : labels)
    {
        const auto representative = find(label);
        const auto& counts = domains.at(representative);
        result.distinct[label] = std::min(result.rows, bound.contains(representative) ? 1.0 : *std::ranges::min_element(counts));
    }
    return result;
}
}

namespace tyr::datalog::database::detail
{
Estimate combine(Estimate lhs, const Estimate& rhs)
{
    for (const auto& single : rhs.factors)
        if (std::ranges::none_of(lhs.factors,
                                 [&](const auto& existing) { return existing.identity == single.identity && existing.attributes == single.attributes; }))
            lhs.factors.push_back(single);
    lhs.equalities.insert(rhs.equalities.begin(), rhs.equalities.end());
    lhs.constants.insert(rhs.constants.begin(), rhs.constants.end());
    return lhs;
}
}

namespace tyr::datalog::database::detail
{
Estimate conjunction(Estimate result, std::span<const ygg::ColumnLayout> columns)
{
    auto labels = urd::Store<urd::sequence<urd::index<ygg::Column>>>();
    for (const auto& column : columns)
        labels.push_back(column.label);
    return conjunction(std::move(result), labels);
}
}

namespace tyr::datalog::database::detail
{
void validate_statistics(const RelationStatistics& stats, std::span<const ygg::ColumnLayout> columns)
{
    if (!std::isfinite(stats.rows) || stats.rows < 0 || (columns.empty() && stats.rows > 1))
        throw std::invalid_argument("Optimizer: invalid row count.");
    for (const auto& [label, count] : stats.distinct)
        if (std::ranges::find(columns, label, &ygg::ColumnLayout::label) == columns.end() || !std::isfinite(count) || count < 0 || count > stats.rows
            || (stats.rows > 0 && count < 1))
            throw std::invalid_argument("Optimizer: invalid distinct count or column.");
}
}

namespace tyr::datalog::database::detail
{
RelationStatistics with_default_distinct(RelationStatistics stats, std::span<const ygg::ColumnLayout> columns, std::optional<size_t> objects)
{
    const auto domain = objects ? static_cast<double>(*objects) : max_estimate;
    for (const auto& column : columns)
        stats.distinct.try_emplace(column.label, std::min(stats.rows, domain));
    return stats;
}
}

namespace tyr::datalog::database::detail
{
RelationStatistics capped(RelationStatistics stats)
{
    for (const auto& [column, count] : stats.distinct)
        stats.distinct.at(column) = std::min(count, stats.rows);
    return stats;
}
}
