/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/semantics/details/min_distance.hpp"

#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::datalog::database::detail
{
urd::Store<urd::sequence<size_t>> column_range(size_t first, size_t last)
{
    auto positions = urd::Store<urd::sequence<size_t>>();
    for (auto c = first; c < last; ++c)
        positions.push_back(c);
    return positions;
}
}

namespace tyr::datalog::database::detail
{
QueryView<> checked_edges(QueryView<> edges, const DistancePlan& plan)
{
    if (!std::ranges::equal(edges.columns(), plan.edge_columns()))
        throw std::invalid_argument("Relational operation: input schema does not match prepared plan.");
    return edges;
}
}
