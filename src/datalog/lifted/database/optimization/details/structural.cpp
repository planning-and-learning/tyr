/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/optimization/details/structural.hpp"

#include <urd/config.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <urd/store.hpp>

namespace tyr::datalog::database::detail
{
std::optional<urd::Store<urd::sequence<std::optional<size_t>>>> join_tree(urd::Store<urd::sequence<urd::Store<urd::sequence<urd::index<ygg::Column>>>>> edges)
{
    const auto count = edges.size();
    urd::Store<urd::sequence<std::optional<size_t>>> parent(count);
    urd::Store<urd::set<size_t>, urd::dense> alive;
    for (size_t edge = 0; edge < count; ++edge)
        alive.insert(edge);
    for (bool changed = true; changed && alive.size() > 1;)
    {
        changed = false;
        for (size_t edge = 0; edge < count; ++edge)
        {
            if (!alive.contains(edge))
                continue;
            const auto removed = erase_if(edges[edge],
                                          [&](urd::index<ygg::Column> variable)
                                          {
                                              for (size_t other = 0; other < count; ++other)
                                                  if (other != edge && alive.contains(other) && std::ranges::binary_search(edges[other], variable))
                                                      return false;
                                              return true;
                                          });
            changed |= removed != 0;
        }
        for (size_t edge = 0; edge < count && alive.size() > 1; ++edge)
        {
            if (!alive.contains(edge))
                continue;
            for (size_t other = 0; other < count; ++other)
                if (other != edge && alive.contains(other) && std::ranges::includes(edges[other], edges[edge]))
                {
                    parent[edge] = other;
                    alive.erase(edge);
                    changed = true;
                    break;
                }
        }
    }
    if (alive.size() > 1)
        return std::nullopt;
    return parent;
}
}

namespace tyr::datalog::database::detail
{
QueryView<> yannakakis(const JoinBlock& block, std::span<const std::optional<size_t>> parent, OperatorBuilder& build)
{
    const auto count = block.atoms.size();
    urd::Store<urd::sequence<urd::Store<urd::sequence<size_t>>>> children(count);
    size_t root = 0;
    for (size_t atom = 0; atom < count; ++atom)
        if (const auto& above = parent[atom])
            children[*above].push_back(atom);
        else
            root = atom;
    urd::Store<urd::sequence<size_t>> preorder { root };
    for (size_t position = 0; position < preorder.size(); ++position)
        for (const auto child : children[preorder[position]])
            preorder.push_back(child);

    auto relations = block.atoms;
    for (auto atom = preorder.rbegin(); atom != preorder.rend(); ++atom)
        if (const auto& above = parent[*atom])
            relations[*above] = semijoin(build, relations[*above], relations[*atom]);
    for (const auto atom : preorder)
        if (const auto& above = parent[atom])
            relations[atom] = semijoin(build, relations[atom], relations[*above]);

    urd::Store<urd::sequence<std::optional<QueryView<>>>> results(count);
    for (auto atom = preorder.rbegin(); atom != preorder.rend(); ++atom)
    {
        auto joined = relations[*atom];
        for (const auto child : children[*atom])
        {
            const auto& result = results[child];
            URD_ASSUME(result.has_value());  // the reverse preorder visits children first
            joined = build.join(joined, *result);
        }
        auto keep = block.required;
        if (const auto& above = parent[*atom])
        {
            const auto shared = variables(block.atoms[*above]);
            keep.insert(keep.end(), shared.begin(), shared.end());
            keep = ygg::canonicalized(std::move(keep));
        }
        results[*atom] = build.restrict(joined, keep);
    }
    const auto& result = results[root];
    URD_ASSUME(result.has_value());  // the reverse preorder ends at the root
    return *result;
}
}

namespace tyr::datalog::database::detail
{
QueryView<> structural_plan(const JoinBlock& block, OperatorBuilder& build)
{
    urd::Store<urd::sequence<urd::Store<urd::sequence<urd::index<ygg::Column>>>>> edges;
    edges.reserve(block.atoms.size());
    for (const auto atom : block.atoms)
        edges.push_back(variables(atom));
    if (const auto parent = join_tree(std::move(edges)))
        return yannakakis(block, *parent, build);
    return build.generic_join(block.atoms, variable_order(block.atoms));
}
}
