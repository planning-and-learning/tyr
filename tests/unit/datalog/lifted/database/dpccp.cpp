/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/optimization/details/dpccp.hpp"

#include <array>
#include <gtest/gtest.h>
#include <map>
#include <random>
#include <set>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>

namespace tyr::datalog::tests
{
namespace
{
using Mask = std::uint64_t;
using Pair = std::pair<Mask, Mask>;

bool connected(std::span<const Mask> graph, Mask vertices)
{
    if (!vertices)
        return false;
    Mask reached = (-vertices) & vertices;
    for (;;)
    {
        auto next = reached;
        for (auto rest = reached; rest; rest &= rest - 1)
            next |= graph[std::countr_zero(rest)] & vertices;
        if (next == reached)
            return reached == vertices;
        reached = next;
    }
}

std::set<Pair> brute_force(std::span<const Mask> graph)
{
    std::set<Pair> result;
    const auto limit = Mask { 1 } << graph.size();
    for (Mask left = 1; left < limit; ++left)
        if (connected(graph, left))
            for (Mask right = 1; right < limit; ++right)
                if (!(left & right) && std::countr_zero(left) < std::countr_zero(right) && connected(graph, right) && connected(graph, left | right))
                    result.emplace(left, right);
    return result;
}

void check_graph(std::span<const Mask> graph)
{
    const auto expected = brute_force(graph);
    std::map<Mask, size_t> totals;
    for (const auto& [left, right] : expected)
        ++totals[left | right];
    std::map<Mask, size_t> seen_counts;
    std::set<Pair> seen;
    urd::Store<urd::sequence<Pair>> sequence;
    EXPECT_TRUE(database::detail::enumerate_connected_pairs(graph,
                                                            [&](Mask left, Mask right)
                                                            {
                                                                EXPECT_TRUE(expected.contains({ left, right }));
                                                                EXPECT_TRUE(seen.emplace(left, right).second);
                                                                // A child must have its optimal cost finalized, not merely have one plan.
                                                                EXPECT_EQ(seen_counts[left], totals[left]);
                                                                EXPECT_EQ(seen_counts[right], totals[right]);
                                                                ++seen_counts[left | right];
                                                                sequence.emplace_back(left, right);
                                                                return true;
                                                            }));
    EXPECT_EQ(seen, expected);
    urd::Store<urd::sequence<Pair>> repeated;
    EXPECT_TRUE(database::detail::enumerate_connected_pairs(graph,
                                                            [&](Mask left, Mask right)
                                                            {
                                                                repeated.emplace_back(left, right);
                                                                return true;
                                                            }));
    EXPECT_EQ(sequence, repeated);
}
}  // namespace

TEST(YggdrasilTests, DatabaseDpccpMatchesEveryGraphThroughFiveVertices)
{
    for (size_t vertices = 0; vertices <= 5; ++vertices)
    {
        const auto edge_count = vertices > 1 ? vertices * (vertices - 1) / 2 : 0;
        for (Mask edges = 0; edges < (Mask { 1 } << edge_count); ++edges)
        {
            urd::Store<urd::sequence<Mask>> graph(vertices);
            size_t bit = 0;
            for (size_t left = 0; left < vertices; ++left)
                for (size_t right = left + 1; right < vertices; ++right, ++bit)
                    if (edges & (Mask { 1 } << bit))
                    {
                        graph[left] |= Mask { 1 } << right;
                        graph[right] |= Mask { 1 } << left;
                    }
            SCOPED_TRACE(vertices);
            SCOPED_TRACE(edges);
            check_graph(graph);
        }
    }
}

TEST(YggdrasilTests, DatabaseDpccpHandlesLargerChainStarCliqueAndArbitraryNumbering)
{
    constexpr size_t count = 8;
    urd::Store<urd::sequence<Mask>> graph(count);
    for (size_t i = 0; i < count - 1; ++i)
    {
        graph[i] |= Mask { 1 } << (i + 1);
        graph[i + 1] |= Mask { 1 } << i;
    }
    check_graph(graph);
    graph.back() |= 1;
    graph.front() |= Mask { 1 } << (count - 1);
    check_graph(graph);
    graph.assign(count, 0);
    for (size_t i = 0; i < count; ++i)
        if (i != 5)
        {
            graph[5] |= Mask { 1 } << i;
            graph[i] |= Mask { 1 } << 5;
        }
    check_graph(graph);
    for (size_t i = 0; i < count; ++i)
        graph[i] = ((Mask { 1 } << count) - 1) ^ (Mask { 1 } << i);
    check_graph(graph);
    // NOLINTNEXTLINE(bugprone-random-generator-seed): a fixed seed keeps the test reproducible
    std::mt19937 random(11);
    for (size_t trial = 0; trial < 12; ++trial)
    {
        graph.assign(count, 0);
        for (size_t left = 0; left < count; ++left)
            for (size_t right = left + 1; right < count; ++right)
                if (random() % 3 == 0)
                {
                    graph[left] |= Mask { 1 } << right;
                    graph[right] |= Mask { 1 } << left;
                }
        check_graph(graph);
    }
}

TEST(YggdrasilTests, DatabaseDpccpStopsAtBudgetAndChecksMaskBounds)
{
    const std::array<Mask, 4> clique { 14, 13, 11, 7 };
    size_t emitted = 0;
    EXPECT_FALSE(database::detail::enumerate_connected_pairs(clique, [&](Mask, Mask) { return ++emitted < 7; }));
    EXPECT_EQ(emitted, 7);
    const auto keep_going = [](Mask, Mask) { return true; };
    urd::Store<urd::sequence<Mask>> graph(64);
    EXPECT_THROW(database::detail::enumerate_connected_pairs(graph, keep_going), std::invalid_argument);
    graph.resize(63);
    graph[0] = Mask { 1 } << 62;
    graph[62] = 1;
    urd::Store<urd::sequence<Pair>> pairs;
    EXPECT_TRUE(database::detail::enumerate_connected_pairs(graph,
                                                            [&](Mask left, Mask right)
                                                            {
                                                                pairs.emplace_back(left, right);
                                                                return true;
                                                            }));
    ASSERT_EQ(pairs.size(), 1);
    EXPECT_EQ(pairs.front(), (Pair { 1, Mask { 1 } << 62 }));
    const std::array<Mask, 1> outside { 2 };
    const std::array<Mask, 1> self { 1 };
    const std::array<Mask, 2> asymmetric { 2, 0 };
    EXPECT_THROW(database::detail::enumerate_connected_pairs(outside, keep_going), std::invalid_argument);
    EXPECT_THROW(database::detail::enumerate_connected_pairs(self, keep_going), std::invalid_argument);
    EXPECT_THROW(database::detail::enumerate_connected_pairs(asymmetric, keep_going), std::invalid_argument);
}
}  // namespace tyr::datalog::tests
