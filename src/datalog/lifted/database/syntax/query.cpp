/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/syntax/query.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <urd/store.hpp>

namespace tyr::datalog::database::detail
{
QueryView<> clone_query(QueryView<> source, std::span<const QueryView<>> children, QueryRepository& destination, QueryBuilder& builder)
{
    urd::Store<urd::sequence<QueryView<>>> original;
    for_each_child(source, [&](QueryView<> child) { original.push_back(child); });
    if (original.size() != children.size())
        throw std::invalid_argument("Query clone: wrong child count.");
    for (size_t i = 0; i < children.size(); ++i)
    {
        if (&get_repository(children[i].context()) != &destination)
            throw std::invalid_argument("Query clone: children must belong to the destination.");
        if (!std::ranges::equal(children[i].columns(), original[i].columns()))
            throw std::invalid_argument("Query clone: a child's columns differ from the original's.");
    }
    return std::visit(
        [&]<typename Tag>(QueryView<Tag> concrete)
        {
            size_t child = 0;
            auto data = urd::map_children<boost::mp11::mp_list<Query<>>, Query<Tag>>(concrete.data(), concrete.context(),
                                                                               [&](QueryView<>) { return children[child++].index(); });
            return insert_query(destination, builder, data);
        },
        source.get_variant());
}
}

namespace tyr::datalog::database
{
urd::Store<urd::sequence<QueryView<>>> reachable(std::span<const QueryView<>> roots)
{
    if (roots.empty())
        return {};
    const auto& repository = get_repository(roots.front().context());
    urd::Store<urd::set<urd::index<Query<>>>, urd::dense> seen;
    urd::Store<urd::sequence<QueryView<>>> pending;
    for (const auto root : roots)
    {
        if (&get_repository(root.context()) != &repository || root.index().value >= repository.size())
            throw std::invalid_argument("Query: roots must belong to one repository.");
        pending.push_back(root);
    }
    while (!pending.empty())
    {
        const auto query = pending.back();
        pending.pop_back();
        if (!seen.insert(query.index()).second)
            continue;
        for_each_child(query, [&](QueryView<> child) { pending.push_back(child); });
    }
    urd::Store<urd::sequence<QueryView<>>> result;
    for (const auto index : seen)  // in index order
        result.push_back(repository.view(index));
    return result;
}
}
