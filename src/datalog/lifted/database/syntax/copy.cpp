/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/syntax/query.hpp"

#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::datalog::database
{
urd::Store<urd::sequence<size_t>> input_slots(std::span<const QueryView<>> roots)
{
    auto slots = urd::Store<urd::sequence<size_t>>();
    for (const auto query : reachable(roots))
        if (is<QueryInputTag>(query))
            slots.push_back(as<QueryInputTag>(query).get_input_slot());
    std::ranges::sort(slots);
    const auto [first, last] = std::ranges::unique(slots);
    slots.erase(first, last);
    return slots;
}
}

namespace tyr::datalog::database
{
QueryPlan compile(std::span<const QueryView<>> roots)
{
    if (roots.empty())
        return QueryPlan {};
    auto repository = std::make_shared<QueryRepository>();
    QueryBuilder builder;
    urd::Store<urd::sequence<urd::index<Query<>>>> outputs;
    for (const auto copied : database::copy(roots, [](size_t slot) { return slot; }, *repository, builder))
        outputs.push_back(copied.index());
    return QueryPlan(std::move(repository), std::move(outputs));
}
}

namespace tyr::datalog::database
{
QueryPlan compile(std::initializer_list<QueryView<>> roots) { return compile(std::span<const QueryView<>>(roots)); }
}
