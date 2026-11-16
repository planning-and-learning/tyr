/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/optimization/details/cost.hpp"

#include <algorithm>
#include <urd/bits.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::datalog::database::detail
{
const CostBasedPlanner::Refined& CostBasedPlanner::refine(size_t node)
{
    if (const auto& refined = m_refined[node])
        return *refined;
    Refined result;
    const auto subset = m_tree[node].subset;
    if (const auto children = m_tree[node].children)
    {
        const auto [left_child, right_child] = *children;
        const auto& left = refine(left_child);
        const auto& right = refine(right_child);
        const auto largest_input = std::max(rows(m_tree[left_child].subset), rows(m_tree[right_child].subset));
        const bool growing = rows(subset) - largest_input > ygg::FloatTolerance<ygg::float_t>::tolerance(rows(subset), largest_input);
        if (growing || left.multiway || right.multiway)
        {
            urd::Store<urd::sequence<size_t>> inputs;
            for (const auto& [child, refined] : { std::pair(left_child, &left), std::pair(right_child, &right) })
                if (refined->multiway)
                    inputs.insert(inputs.end(), refined->inputs.begin(), refined->inputs.end());
                else
                    inputs.push_back(child);
            result = { true, std::move(inputs) };
        }
    }
    return m_refined[node].emplace(std::move(result));
}
}

namespace tyr::datalog::database::detail
{
QueryView<> CostBasedPlanner::build(size_t node)
{
    const auto subset = m_tree[node].subset;
    const auto children = m_tree[node].children;
    const auto keep = needed(subset);
    if (!children)
        return m_build->restrict(m_block->atoms[std::countr_zero(subset)], keep);
    const auto [left, right] = *children;
    // Multi-way joins with only two inputs are built as binary joins (Freitag et al.).
    const auto& refined = refine(node);
    if (!refined.multiway || refined.inputs.size() == 2)
        return m_build->restrict(m_build->join(build(left), build(right)), keep);
    urd::Store<urd::sequence<QueryView<>>> inputs;
    inputs.reserve(refined.inputs.size());
    for (const auto input : refined.inputs)
        inputs.push_back(build(input));
    return m_build->restrict(m_build->generic_join(inputs, variable_order(inputs)), keep);
}
}

namespace tyr::datalog::database::detail
{
QueryView<> CostBasedPlanner::plan() &&
{
    const auto count = m_block->atoms.size();
    urd::Store<urd::sequence<std::uint64_t>> graph(count);
    for (size_t left = 0; left < count; ++left)
        for (size_t right = left + 1; right < count; ++right)
            if (std::ranges::find_first_of(m_variables[left], m_variables[right]) != m_variables[left].end())
            {
                graph[left] |= std::uint64_t { 1 } << right;
                graph[right] |= std::uint64_t { 1 } << left;
            }
    enumerate_connected_pairs(graph,
                              [&](std::uint64_t left, std::uint64_t right)
                              {
                                  const auto subset = left | right;
                                  const auto cost = m_best.at(left).cost + m_best.at(right).cost + rows(subset);
                                  if (const auto it = m_best.find(subset); it == m_best.end() || cost < it->second.cost)
                                      m_best.insert_or_assign(subset, Split { cost, left });
                                  return true;
                              });
    urd::Store<urd::sequence<std::uint64_t>> components;
    for (auto unseen = urd::bits::lo_set<std::uint64_t>[count]; unseen;)
    {
        auto component = unseen & -unseen;
        for (std::uint64_t previous = 0; component != previous;)
        {
            previous = component;
            urd::bits::for_each_set_bit(previous, [&](size_t atom) { component |= graph[atom]; });
        }
        components.push_back(component);
        unseen &= ~component;
    }
    // Ascending sizes minimize C_out over left-deep sequences of cross products.
    std::ranges::stable_sort(components, {}, [&](std::uint64_t component) { return rows(component); });
    auto root = tree(components.front());
    auto joined = components.front();
    for (const auto component : std::span(components).subspan(1))
    {
        joined |= component;
        const auto right = tree(component);
        root = add({ joined, std::pair(root, right) });
    }
    m_refined.resize(m_tree.size());
    return build(root);
}
}
