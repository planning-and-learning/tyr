/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/semantics/incremental/search/tree_repair.hpp"
#include "tyr/datalog/lifted/database/semantics/search/a_star.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <gtest/gtest.h>
#include <limits>
#include <random>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace
{
namespace db = tyr::datalog::database;
using ygg::uint_t;
using Handle = db::VertexSet::handle_type;
using Adjacency = urd::Store<urd::sequence<urd::Store<urd::sequence<uint_t>>>>;

constexpr uint_t unreachable = std::numeric_limits<uint_t>::max();

/// Vertices are uint_t values; edges are adjacency lists.
struct ListGraph
{
    using Row = std::array<ygg::Value, 1>;

    Adjacency out;
    Adjacency in;

    explicit ListGraph(uint_t n) : out(n), in(n) {}

    static uint_t id(std::span<const ygg::Value> vertex) { return ygg::decode_value<uint_t>(vertex[0]); }
    static Row row(uint_t value) { return Row { ygg::encode_value(value) }; }
    template<typename OnVertex>
    static void each(const urd::Store<urd::sequence<uint_t>>& vertices, const OnVertex& on_vertex)
    {
        for (const auto w : vertices)
            if (const auto encoded = row(w); !on_vertex(std::span<const ygg::Value>(encoded)))
                return;
    }
    template<typename OnVertex>
    void successors(std::span<const ygg::Value> vertex, const OnVertex& on_vertex)
    { each(out[id(vertex)], on_vertex); }
    template<typename OnVertex>
    void predecessors(std::span<const ygg::Value> vertex, const OnVertex& on_vertex)
    { each(in[id(vertex)], on_vertex); }
    bool edge_exists(std::span<const ygg::Value> from, std::span<const ygg::Value> to)
    { return std::ranges::find(out[id(from)], id(to)) != out[id(from)].end(); }
    [[nodiscard]] bool has(uint_t u, uint_t v) const { return std::ranges::find(out[u], v) != out[u].end(); }
    /// Adds the edge unless it is a loop or present; returns whether it was added.
    bool add(uint_t u, uint_t v)
    {
        if (u == v || has(u, v))
            return false;
        out[u].push_back(v);
        in[v].push_back(u);
        return true;
    }
    void remove(uint_t u, uint_t v)
    {
        out[u].erase(std::ranges::find(out[u], v));
        in[v].erase(std::ranges::find(in[v], u));
    }
};

ListGraph random_graph(std::mt19937& rng, uint_t n, size_t edges)
{
    auto graph = ListGraph(n);
    for (size_t e = 0; e < edges; ++e)
        graph.add(static_cast<uint_t>(rng() % n), static_cast<uint_t>(rng() % n));
    return graph;
}

struct ListVertices
{
    urd::Store<urd::sequence<ListGraph::Row>> rows;

    [[nodiscard]] size_t size() const noexcept { return rows.size(); }
    [[nodiscard]] std::span<const ygg::Value> row(size_t i) const { return rows[i]; }
};

/// The targets of a search: their values, and their vertices in the tree.
struct Targets
{
    db::VertexSet set;
    urd::Store<urd::sequence<uint_t>> ids;
    urd::Store<urd::sequence<Handle>> stored;

    template<typename H>
    void add(uint_t target, db::SearchTree& tree, const H& heuristic)
    {
        const auto row = ListGraph::row(target);
        if (set.insert(std::span<const ygg::Value>(row)).second)
        {
            ids.push_back(target);
            stored.push_back(tree.ensure(row, heuristic));
        }
    }
};

/// The distance of every vertex to the nearest target, by breadth-first search backward; unreachable if none is.
urd::Store<urd::sequence<uint_t>> distances_to(const ListGraph& graph, const Targets& targets)
{
    auto result = urd::Store<urd::sequence<uint_t>>(graph.out.size(), unreachable);
    auto queue = urd::Store<urd::sequence<uint_t>>();
    for (const auto t : targets.ids)
    {
        result[t] = 0;
        queue.push_back(t);
    }
    for (size_t i = 0; i < queue.size(); ++i)
        for (const auto u : graph.in[queue[i]])
            if (result[u] == unreachable)
            {
                result[u] = result[queue[i]] + 1;
                queue.push_back(u);
            }
    return result;
}

/// Checks that AStar::path() visits a simple path of length `distance` from a source to a target: where the search
/// ended back to its source (the only source on the path, as sources have g = 0), then the stored chain from there to
/// a target. Returns whether it ends on a stored chain.
template<typename Search>
bool expect_path(const ListGraph& graph, const ListVertices& sources, const Targets& targets, const Search& search, uint_t distance)
{
    auto path = urd::Store<urd::sequence<uint_t>>();
    search.path([&](std::span<const ygg::Value> vertex) { path.push_back(ListGraph::id(vertex)); });
    const auto is_source = [&](uint_t v) { return std::ranges::any_of(sources.rows, [&](const auto& row) { return ListGraph::id(row) == v; }); };
    const auto source = static_cast<size_t>(std::ranges::find_if(path, is_source) - path.begin());
    EXPECT_LT(source, path.size());
    EXPECT_EQ(path.size(), distance + 1);
    auto distinct = path;
    std::ranges::sort(distinct);
    EXPECT_EQ(std::ranges::unique(distinct).begin(), distinct.end());
    if (source >= path.size() || path.size() != distance + 1)
        return false;
    for (size_t i = 0; i < source; ++i)
        EXPECT_TRUE(graph.has(path[i + 1], path[i]));
    for (size_t i = source + 1; i < path.size(); ++i)
        EXPECT_TRUE(graph.has(i == source + 1 ? path.front() : path[i - 1], path[i]));
    EXPECT_TRUE(targets.set.contains(std::span<const ygg::Value>(ListGraph::row(source + 1 < path.size() ? path.back() : path.front()))));
    return source + 1 < path.size();
}

class DatabaseSearch : public testing::TestWithParam<unsigned>
{
};

/// AStar::path() visits a shortest path from a source to a target, whether the search ends at a target or on a stored
/// chain of the backward frontier.
TEST_P(DatabaseSearch, AStarPathIsAShortestPath)
{
    auto rng = std::mt19937(GetParam());
    const auto n = static_cast<uint_t>(40);
    auto paths = static_cast<size_t>(0);
    auto chains = static_cast<size_t>(0);  // Paths that end on a stored chain.
    auto heuristic = db::BlindDistanceHeuristic {};
    auto tree = db::SearchTree {};
    auto backward = db::BackwardFrontier {};
    auto storage = db::AStarStorage {};
    for (size_t trial = 0; trial < 50; ++trial)
    {
        auto graph = random_graph(rng, n, 70);
        auto sources = ListVertices {};
        auto targets = Targets {};
        tree.clear();
        for (size_t i = 0; i < 2; ++i)
        {
            sources.rows.push_back(ListGraph::row(static_cast<uint_t>(rng() % n)));
            targets.add(static_cast<uint_t>(rng() % n), tree, heuristic);
        }
        backward.start(tree, targets.stored);
        auto search = db::AStar<ListGraph, db::BlindDistanceHeuristic>(graph, tree, backward, heuristic, targets.set, storage, 0);
        const auto distance = search.run(sources);
        if (distance == unreachable)
            continue;
        chains += expect_path(graph, sources, targets, search, distance);
        ++paths;
    }
    EXPECT_GT(paths, 10);
    EXPECT_GT(chains, 0);
}

/// The zero estimate, and with `proves` a proof of unreachability from the current graph's distances (`distances`),
/// tried early (ProvesUnreachable).
struct ProvingHeuristic
{
    const urd::Store<urd::sequence<uint_t>>* distances = nullptr;
    bool proves = false;

    [[nodiscard]] uint_t estimate(std::span<const ygg::Value>) const noexcept { return 0; }
    [[nodiscard]] bool unreachable(std::span<const ygg::Value> vertex) const { return (*distances)[ListGraph::id(vertex)] == ::unreachable; }
    [[nodiscard]] size_t proof_cost() const noexcept { return proves ? 4 : std::numeric_limits<size_t>::max(); }
};

/// A graph of `n` vertices and `edges` random edges, which gets `added` random edges every `period` searches.
struct UpdateShape
{
    uint_t n;
    size_t edges;
    size_t period;
    size_t added;
};

/// With `proves`, the searches prune the vertices that the current graph's distances prove unreachable: the learned
/// values must stay valid in later graphs, also where an added edge leads through a pruned vertex. With any_path, the
/// searches find a path exactly when a target is reachable, a simple one: a path that is not a shortest one can meet its
/// generated vertices on the tree path where it ends, and linking those would make a cycle, on which AStar::path() does
/// not return (larger, denser graphs, where this occurs).
void search_across_updates(unsigned seed, UpdateShape shape, bool any_path, bool proves)
{
    auto rng = std::mt19937(seed);
    const auto n = shape.n;
    auto chains = static_cast<size_t>(0);
    auto expected = urd::Store<urd::sequence<uint_t>>();
    auto heuristic = ProvingHeuristic { &expected, proves };
    auto tree = db::SearchTree {};
    auto backward = db::BackwardFrontier {};
    auto storage = db::AStarStorage {};
    auto repair = db::incremental::TreeRepair {};
    for (size_t trial = 0; trial < 30; ++trial)
    {
        auto graph = random_graph(rng, n, shape.edges);
        auto targets = Targets {};
        tree.clear();
        for (size_t i = 0; i < 3; ++i)
            targets.add(static_cast<uint_t>(rng() % n), tree, heuristic);
        backward.start(tree, targets.stored);
        for (size_t s = 0; s < (any_path ? 200U : 40U); ++s)
        {
            // Removed edges keep the frontier's levels lower bounds; the version expires the chains' edge checks.
            for (size_t r = 0; r < 2; ++r)
                if (const auto u = static_cast<uint_t>(rng() % n); !graph.out[u].empty())
                    graph.remove(u, graph.out[u][rng() % graph.out[u].size()]);
            // Every few searches, added edges: TreeRepair lowers the stored values.
            if (s % shape.period == 0)
            {
                auto added = db::incremental::EdgeList {};
                for (size_t r = 0; r < shape.added; ++r)
                {
                    const auto u = static_cast<uint_t>(rng() % n);
                    const auto v = static_cast<uint_t>(rng() % n);
                    if (graph.add(u, v))
                        added.add(ListGraph::row(u), ListGraph::row(v));
                }
                repair.run(tree, added, graph, heuristic);
                // The frontier starts anew only where an added edge breaks its bounds.
                auto broken = false;
                for (size_t i = 0; i < added.size(); ++i)
                {
                    const auto to = backward.bound(tree.find(added.to(i)));
                    broken = broken || (to != unreachable && backward.bound(tree.find(added.from(i))) > to + 1);
                }
                if (broken)
                    backward.start(tree, targets.stored);
            }
            expected = distances_to(graph, targets);
            auto sources = ListVertices {};
            const auto from = static_cast<uint_t>(rng() % n);
            sources.rows.push_back(ListGraph::row(from));
            auto search = db::AStar<ListGraph, ProvingHeuristic>(graph, tree, backward, heuristic, targets.set, storage, s + 1, any_path);
            const auto distance = search.run(sources);
            if (any_path)
                ASSERT_EQ(distance == unreachable, expected[from] == unreachable) << "trial " << trial << ", search " << s;
            else
                ASSERT_EQ(distance, expected[from]) << "trial " << trial << ", search " << s;
            if (distance != unreachable)
                chains += expect_path(graph, sources, targets, search, distance);
        }
    }
    EXPECT_GT(chains, 0);
}

/// Searches from many sources over one stored tree and backward frontier, under removed and added edges (repaired by
/// TreeRepair, with the frontier started anew), find the distances and visit shortest paths: the frontier advances
/// during later searches and relinks vertices of chains that an earlier search stored, so a chain kept as the incumbent
/// may stop holding, and learned values must stay consistent for TreeRepair to lower every one that added edges lower.
TEST_P(DatabaseSearch, AStarPathsHoldAcrossSearchesOfOneTree)
{
    for (const auto proves : { false, true })
    {
        search_across_updates(GetParam(), UpdateShape { 60, 150, 5, 3 }, false, proves);
        search_across_updates(GetParam(), UpdateShape { 100, 400, 5, 3 }, true, proves);
    }
    // Sparser graphs with more added edges: added edges through vertices that a search pruned.
    search_across_updates(GetParam(), UpdateShape { 60, 80, 3, 10 }, false, true);
}

/// A second search from the same source over the same graph ends on the stored path at the same distance, without
/// expanding: the path's length meets the source's lower bound, also when the backward frontier ended the first search
/// (whose learned bounds can then stay below the path's length).
TEST_P(DatabaseSearch, AStarReusesTheStoredPath)
{
    auto rng = std::mt19937(GetParam());
    const auto n = static_cast<uint_t>(40);
    auto heuristic = db::BlindDistanceHeuristic {};
    auto tree = db::SearchTree {};
    auto backward = db::BackwardFrontier {};
    auto storage = db::AStarStorage {};
    auto searched = static_cast<size_t>(0);
    for (size_t trial = 0; trial < 50; ++trial)
    {
        auto graph = random_graph(rng, n, 70);
        auto sources = ListVertices {};
        sources.rows.push_back(ListGraph::row(static_cast<uint_t>(rng() % n)));
        auto targets = Targets {};
        tree.clear();
        targets.add(static_cast<uint_t>(rng() % n), tree, heuristic);
        backward.start(tree, targets.stored);
        const auto first = db::AStar<ListGraph, db::BlindDistanceHeuristic>(graph, tree, backward, heuristic, targets.set, storage, 0).run(sources);
        if (first == unreachable || first == 0)
            continue;
        auto again = db::AStar<ListGraph, db::BlindDistanceHeuristic>(graph, tree, backward, heuristic, targets.set, storage, 0);
        EXPECT_EQ(again.run(sources), first) << "trial " << trial;
        EXPECT_EQ(again.counters().tree_hits, 1) << "trial " << trial;
        ++searched;
    }
    EXPECT_GT(searched, 10);
}

/// An exact learned distance next to blind estimates is admissible but not consistent: the search first closes c
/// through the longer path s -> b1 -> b2 -> c, then reaches it through s -> a1 -> c; reopening c gives the distance 5.
/// The target's 20 dead-end predecessors keep the backward frontier at its first level.
TEST(DatabaseSearch, AStarReopensUnderAnInconsistentEstimate)
{
    constexpr uint_t s = 0, a1 = 1, b1 = 2, b2 = 3, c = 4, d1 = 5, d2 = 6, t = 7;
    auto graph = ListGraph(28);
    for (const auto& [u, v] :
         std::array<std::pair<uint_t, uint_t>, 8> { { { s, a1 }, { a1, c }, { s, b1 }, { b1, b2 }, { b2, c }, { c, d1 }, { d1, d2 }, { d2, t } } })
        graph.add(u, v);
    for (uint_t dead = 8; dead < 28; ++dead)
        graph.add(dead, t);
    auto heuristic = db::BlindDistanceHeuristic {};
    auto tree = db::SearchTree {};
    auto backward = db::BackwardFrontier {};
    auto storage = db::AStarStorage {};
    auto targets = Targets {};
    targets.add(t, tree, heuristic);
    backward.start(tree, targets.stored);
    tree.learn(tree.ensure(ListGraph::row(a1), heuristic), 4);  // Its distance.
    auto sources = ListVertices {};
    sources.rows.push_back(ListGraph::row(s));
    EXPECT_EQ((db::AStar<ListGraph, db::BlindDistanceHeuristic>(graph, tree, backward, heuristic, targets.set, storage, 0).run(sources)), 5);
}

INSTANTIATE_TEST_SUITE_P(Seeds, DatabaseSearch, testing::Range(1u, 9u));

}  // namespace
