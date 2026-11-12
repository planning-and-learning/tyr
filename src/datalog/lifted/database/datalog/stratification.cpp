/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/datalog/detail/stratification.hpp"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::datalog::database::datalog::detail
{
urd::Store<urd::sequence<Component>> stratify(size_t relations, std::span<const Dependency> dependencies)
{
    // Tarjan's algorithm emits components in reverse topological order of the "reads" edges, i.e. dependencies first.
    auto edges = urd::Store<urd::sequence<urd::Store<urd::sequence<size_t>>>>(relations);
    for (const auto& dependency : dependencies)
        if (dependency.head < relations && dependency.body < relations)
            edges[dependency.head].push_back(dependency.body);
    auto index = urd::Store<urd::sequence<std::optional<size_t>>>(relations);
    auto low = urd::Store<urd::sequence<size_t>>(relations);
    auto on_stack = urd::Store<urd::sequence<bool>>(relations);
    auto stack = urd::Store<urd::sequence<size_t>>();
    auto component = urd::Store<urd::sequence<size_t>>(relations);
    auto result = urd::Store<urd::sequence<Component>>();
    size_t counter = 0;
    // Iterative DFS: (node, next edge).
    auto frames = urd::Store<urd::sequence<std::pair<size_t, size_t>>>();
    for (size_t root = 0; root < relations; ++root)
    {
        if (index[root])
            continue;
        frames.emplace_back(root, 0);
        while (!frames.empty())
        {
            auto& [node, edge] = frames.back();
            if (edge == 0 && !index[node])
            {
                index[node] = low[node] = counter++;
                stack.push_back(node);
                on_stack[node] = true;
            }
            if (edge < edges[node].size())
            {
                const auto next = edges[node][edge++];
                if (!index[next])
                    frames.emplace_back(next, 0);
                else if (on_stack[next])
                    low[node] = std::min(low[node], *index[next]);
                continue;
            }
            if (low[node] == *index[node])
            {
                auto& stratum = result.emplace_back();
                for (size_t member = relations; member != node;)
                {
                    member = stack.back();
                    stack.pop_back();
                    on_stack[member] = false;
                    component[member] = result.size() - 1;
                    stratum.relations.push_back(member);
                }
                std::ranges::sort(stratum.relations);
            }
            const auto finished = node;
            frames.pop_back();
            if (!frames.empty())
                low[frames.back().first] = std::min(low[frames.back().first], low[finished]);
        }
    }
    for (const auto& dependency : dependencies)
    {
        if (dependency.head >= relations || dependency.body >= relations || component[dependency.head] != component[dependency.body])
            continue;
        if (dependency.negative)
            throw std::invalid_argument("Datalog: the program is not stratified; a relation depends on itself through negation or aggregation.");
        result[component[dependency.head]].recursive = true;
    }
    return result;
}
}  // namespace tyr::datalog::database::datalog::detail
