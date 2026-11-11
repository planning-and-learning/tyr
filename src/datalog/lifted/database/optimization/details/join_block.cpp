/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/optimization/details/join_block.hpp"

#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::datalog::database::detail
{
urd::Store<urd::sequence<urd::index<ygg::Column>>> variables(QueryView<> query)
{
    auto labels = urd::Store<urd::sequence<urd::index<ygg::Column>>>();
    for (const auto& column : query.columns())
        labels.push_back(column.label);
    return ygg::canonicalized(std::move(labels));
}
}

namespace tyr::datalog::database::detail
{
QueryView<> semijoin(OperatorBuilder& build, QueryView<> lhs, QueryView<> rhs) { return build.join(lhs, build.restrict(rhs, variables(lhs))); }
}

namespace tyr::datalog::database::detail
{
urd::Store<urd::sequence<urd::index<ygg::Column>>> variable_order(std::span<const QueryView<>> atoms)
{
    urd::Store<urd::sequence<urd::index<ygg::Column>>> result;
    for (const auto atom : atoms)
        for (const auto label : variables(atom))
            result.push_back(label);
    ygg::canonicalize(result);
    return result;
}
}
