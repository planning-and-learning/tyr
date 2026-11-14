/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_OPTIMIZATION_DETAILS_OPTIMIZATION_HPP_
#define TYR_DATALOG_DATABASE_OPTIMIZATION_DETAILS_OPTIMIZATION_HPP_

#include "tyr/datalog/lifted/database/optimization/details/cost.hpp"
#include "tyr/datalog/lifted/database/optimization/details/estimation.hpp"
#include "tyr/datalog/lifted/database/optimization/details/join_block.hpp"
#include "tyr/datalog/lifted/database/optimization/details/normalization.hpp"
#include "tyr/datalog/lifted/database/optimization/details/structural.hpp"
#include "tyr/datalog/lifted/database/optimization/optimization.hpp"

#include <cassert>
#include <concepts>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <urd/repr/maps/index_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/std_set.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace tyr::datalog::database::detail
{
/// Plans normalized queries block by block into a plan repository.
class Planner
{
    using QueryIndex = urd::index<Query<>>;

    const Statistics* m_statistics;
    const urd::Store<urd::map<QueryIndex, QueryIndex>, urd::dense>* m_normalized_sources;
    OperatorBuilder m_build;
    // Memos by query; computed values are stored after recursing, because recursion may grow the maps.
    urd::Store<urd::map<QueryIndex, QueryView<>>, urd::dense> m_planned;
    /// Equivalent caller query of a plan query, for observed statistics.
    urd::Store<urd::map<QueryIndex, QueryIndex>, urd::dense> m_sources;
    urd::Store<urd::map<QueryIndex, Estimate>, urd::dense> m_estimates;
    urd::Store<urd::map<QueryIndex, bool>, urd::dense> m_known;

    static bool joins(QueryView<> query) { return is<QueryJoinTag>(query) || is<QueryGenericJoinTag>(query); }

    [[nodiscard]] std::optional<QueryIndex> source(QueryView<> plan) const
    {
        const auto it = m_sources.find(plan.index());
        return it != m_sources.end() ? std::optional(it->second) : std::nullopt;
    }
    /// Whether the plan query's rows are measured or derivable from measured inputs.
    bool known(QueryView<> plan);
    Estimate estimate(QueryView<> plan)
    {
        if (const auto it = m_estimates.find(plan.index()); it != m_estimates.end())
            return it->second;
        auto result = detail::estimate(plan, source(plan), [&](QueryView<> child) { return estimate(child); }, *m_statistics);
        m_estimates.insert_or_assign(plan.index(), result);
        return result;
    }

    /// The planned operands of the joins below a normalized join, each once.
    void collect(QueryView<> normalized, urd::Store<urd::sequence<QueryView<>>>& atoms, urd::Store<urd::set<QueryIndex>>& seen)
    {
        if (!joins(normalized))
        {
            if (const auto atom = plan(normalized); seen.insert(atom.index()).second)
                atoms.push_back(atom);
            return;
        }
        for_each_child(normalized, [&](QueryView<> child) { collect(child, atoms, seen); });
    }
    /// Plans π_output(⋈ atoms): cost-based when every atom is known, otherwise from structure.
    QueryView<> plan_block(QueryView<> join, std::span<const ygg::ColumnLayout> output);

public:
    Planner(const Statistics& statistics,
            const urd::Store<urd::map<QueryIndex, QueryIndex>, urd::dense>& normalized_sources,
            QueryRepository& plans,
            QueryBuilder& builder) :
        m_statistics(&statistics),
        m_normalized_sources(&normalized_sources),
        m_build(plans, builder)
    {
    }

    /// The plan of a normalized query, with the same ordered schema.
    QueryView<> plan(QueryView<> normalized);
};

void validate(std::span<const QueryView<>> roots, const Statistics& statistics);

}  // namespace tyr::datalog::database::detail

#endif
