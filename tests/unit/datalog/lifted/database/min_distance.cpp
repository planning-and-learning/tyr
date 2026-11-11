/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/semantics/min_distance.hpp"

#include "query_helpers.hpp"
#include "tyr/datalog/lifted/database/semantics/query_evaluation.hpp"
#include "tyr/datalog/lifted/database/semantics/relation_view.hpp"

#include <array>
#include <cstddef>
#include <cstring>
#include <gtest/gtest.h>
#include <map>
#include <random>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <urd/store.hpp>

namespace
{
using Column = urd::index<ygg::Column>;
using Relation = urd::builder<ygg::Relation<ygg::Schema>>;
using ygg::uint_t;

using namespace tyr::datalog::tests::distances;

/// Runs `steps` states; each mutates the inputs and maybe the sources and targets, then compares guided distances
/// (blind and with a consistent heuristic) against tyr::datalog::database::min_distance over the fully evaluated edges.
/// `maximal` are inputs whose edges include those of every state.
template<typename Mutate>
void check_sequence(tyr::datalog::database::QueryView<> edges,
                    const urd::Store<urd::sequence<Relation>>& maximal,
                    urd::Store<urd::sequence<Relation>>& inputs,
                    const urd::Store<urd::set<size_t>, urd::dense>& static_slots,
                    Relation& sources,
                    Relation& targets,
                    size_t steps,
                    Mutate mutate)
{
    const auto k = sources.columns().size();
    const ygg::FreeRelationRepository layer;
    const auto plan = tyr::datalog::database::DistancePlan(sources.columns(), edges.columns(), targets.columns(), Column(99));
    auto blind = tyr::datalog::database::MinDistance(layer, edges, plan, static_slots);
    auto guided = tyr::datalog::database::MinDistance<HalfDistance>(layer, edges, plan, static_slots);
    auto evaluator = tyr::datalog::database::QueryEvaluator(layer, tyr::datalog::database::compile({ edges }));
    evaluator.evaluate(borrow_all(layer, maximal));
    auto supergraph = Relation(evaluator.get_result().columns());
    for (size_t i = 0; i < evaluator.get_result().size(); ++i)
        supergraph.insert(evaluator.get_result().row(i).get_values());
    auto heuristic = HalfDistance {};
    heuristic.columns = sources.columns();
    auto workspace = tyr::datalog::database::DistanceWorkspace(plan);
    auto result = Relation(plan.output_columns());
    auto checked = static_cast<size_t>(0);
    for (size_t step = 0; step < steps; ++step)
    {
        mutate(step);
        const auto views = borrow_all(layer, inputs);
        evaluator.evaluate(views);
        const auto materialized = evaluator.get_result();
        auto edge_relation = Relation(materialized.columns());
        for (size_t i = 0; i < materialized.size(); ++i)
            edge_relation.insert(materialized.row(i).get_values());
        const auto expected =
            tyr::datalog::database::min_distance(borrow(layer, sources), borrow(layer, edge_relation), borrow(layer, targets), plan, result, workspace);
        const auto input = [&](size_t slot) { return views[slot]; };
        heuristic.distances = distances_to(borrow(layer, supergraph), borrow(layer, targets), k);
        ASSERT_EQ(blind.min_distance(input, borrow(layer, sources), borrow(layer, targets)), expected) << "step " << step;
        ASSERT_EQ(guided.min_distance(input, borrow(layer, sources), borrow(layer, targets), heuristic), expected) << "step " << step;
        checked += expected != std::numeric_limits<ygg::uint_t>::max();
    }
    EXPECT_GT(checked, steps / 8);
    EXPECT_GT(guided.statistics().tree_hits, 0);
    EXPECT_GT(guided.statistics().backward_vertices, 0);
}

const Column x(0), y(1), a(10), b(11), a2(12), b2(13);

class DatabaseMinDistance : public testing::TestWithParam<unsigned>
{
};

TEST_P(DatabaseMinDistance, SingleColumnEdgesUnderChanges)
{
    auto repository = tyr::datalog::tests::qb::repository();
    const auto edges = tyr::datalog::tests::qb::input(repository, 0, { x, y });
    auto inputs = urd::Store<urd::sequence<Relation>>();
    inputs.emplace_back(tyr::datalog::tests::ops::cols({ x, y }));
    auto sources = Relation(tyr::datalog::tests::ops::cols({ x }));
    auto targets = Relation(tyr::datalog::tests::ops::cols({ y }));
    auto random = std::mt19937(GetParam());
    const auto vertex = [&] { return static_cast<ygg::uint_t>(random() % 12); };
    auto maximal = urd::Store<urd::sequence<Relation>>();
    maximal.emplace_back(tyr::datalog::tests::ops::cols({ x, y }));
    for (ygg::uint_t v = 0; v < 12; ++v)
        for (ygg::uint_t w = 0; w < 12; ++w)
            maximal[0].insert(std::array { ygg::encode_value(v), ygg::encode_value(w) });
    check_sequence(edges,
                   maximal,
                   inputs,
                   {},
                   sources,
                   targets,
                   300,
                   [&](size_t step)
                   {
                       if (step % 25 == 0)
                       {
                           targets.clear();
                           for (int i = 0; i < 2; ++i)
                               targets.insert(std::array { ygg::encode_value(vertex()) });
                       }
                       sources.clear();
                       sources.insert(std::array { ygg::encode_value(vertex()) });
                       if (step % 7 == 0)
                           sources.insert(std::array { ygg::encode_value(vertex()) });
                       for (int i = 0; i < 3; ++i)
                       {
                           const auto edge = std::array { ygg::encode_value(vertex()), ygg::encode_value(vertex()) };
                           if (inputs[0].contains(edge))
                               inputs[0].erase(*inputs[0].find(edge));
                           else
                               inputs[0].insert(edge);
                       }
                   });
}

TEST_P(DatabaseMinDistance, PairEdgesWithJoinsUnionsSelectionsAndDifferences)
{
    auto repository = tyr::datalog::tests::qb::repository();
    namespace qb = tyr::datalog::tests::qb;
    // Slots: 0 moves (x, y), 1 free cells (x), 2 blocked pairs (x, y), 3 static steps (x, y), 4 static identity (x, y).
    const auto moves = qb::input(repository, 0, { x, y });
    const auto free = qb::input(repository, 1, { x });
    const auto blocked = qb::input(repository, 2, { x, y });
    const auto steps = qb::input(repository, 3, { x, y });
    const auto identity = qb::input(repository, 4, { x, y });
    const auto first = qb::project(repository,
                                   qb::join(repository,
                                            qb::join(repository, qb::rename(repository, moves, { a, a2 }), qb::rename(repository, free, { a2 })),
                                            qb::rename(repository, identity, { b, b2 })),
                                   { a, b, a2, b2 });
    const auto variable_order = std::array { a, b, a2, b2 };
    const auto inputs_of_second = std::array { qb::rename(repository, steps, { b, b2 }), qb::rename(repository, identity, { a, a2 }) };
    const auto second = qb::project(
        repository,
        qb::generic_join(repository, std::span<const tyr::datalog::database::QueryView<>>(inputs_of_second), std::span<const Column>(variable_order)),
        { a, b, a2, b2 });
    const auto teleport = qb::project(repository,
                                      qb::join(repository,
                                               qb::select_value(repository, qb::rename(repository, moves, { a, a2 }), a, static_cast<ygg::uint_t>(0)),
                                               qb::rename(repository, identity, { b, b2 })),
                                      { a, b, a2, b2 });
    const auto all = qb::union_(repository, qb::union_(repository, first, second), teleport);
    // Safe-range negation: no edge may enter a blocked pair, nor end on equal coordinates.
    const auto diagonal = qb::select_equal(repository, all, a2, b2);
    const auto edges = qb::difference(repository, qb::antijoin(repository, all, qb::rename(repository, blocked, { a2, b2 })), diagonal);

    auto inputs = urd::Store<urd::sequence<Relation>>();
    for (const auto& columns : urd::Store<urd::sequence<urd::Store<urd::sequence<Column>>>> { { x, y }, { x }, { x, y }, { x, y }, { x, y } })
        inputs.emplace_back(tyr::datalog::tests::qb::columns_of(std::span<const Column>(columns)));
    constexpr ygg::uint_t n = 6;
    for (ygg::uint_t v = 0; v < n; ++v)
    {
        inputs[1].insert(std::array { ygg::encode_value(v) });
        for (ygg::uint_t w = 0; w < n; w += 2)
            inputs[0].insert(std::array { ygg::encode_value(v), ygg::encode_value(((v + w + 1) % n)) });
        inputs[4].insert(std::array { ygg::encode_value(v), ygg::encode_value(v) });
        inputs[3].insert(std::array { ygg::encode_value(v), ygg::encode_value(((v + 1) % n)) });
    }
    auto sources = Relation(tyr::datalog::tests::ops::cols({ a, b }));
    auto targets = Relation(tyr::datalog::tests::ops::cols({ a2, b2 }));
    auto random = std::mt19937(GetParam());
    const auto value = [&] { return static_cast<ygg::uint_t>(random() % n); };
    const auto toggle = [](Relation& relation, auto row)
    {
        if (const auto position = relation.find(row))
            relation.erase(*position);
        else
            relation.insert(row);
    };
    auto maximal = urd::Store<urd::sequence<Relation>>();
    for (const auto& input : inputs)
        maximal.emplace_back(input.columns());
    for (ygg::uint_t v = 0; v < n; ++v)
    {
        maximal[1].insert(std::array { ygg::encode_value(v) });
        for (ygg::uint_t w = 0; w < n; ++w)
            maximal[0].insert(std::array { ygg::encode_value(v), ygg::encode_value(w) });
        maximal[3].insert(std::array { ygg::encode_value(v), ygg::encode_value(((v + 1) % n)) });
        maximal[4].insert(std::array { ygg::encode_value(v), ygg::encode_value(v) });
    }
    check_sequence(edges,
                   maximal,
                   inputs,
                   { 3, 4 },
                   sources,
                   targets,
                   300,
                   [&](size_t step)
                   {
                       if (step % 30 == 0)
                       {
                           targets.clear();
                           targets.insert(std::array { ygg::encode_value(value()), ygg::encode_value(value()) });
                           targets.insert(std::array { ygg::encode_value(value()), ygg::encode_value(value()) });
                       }
                       sources.clear();
                       sources.insert(std::array { ygg::encode_value(value()), ygg::encode_value(value()) });
                       toggle(inputs[0], std::array { ygg::encode_value(value()), ygg::encode_value(value()) });
                       toggle(inputs[1], std::array { ygg::encode_value(value()) });
                       if (step % 3 == 0)
                           toggle(inputs[2], std::array { ygg::encode_value(value()), ygg::encode_value(value()) });
                   });
}

TEST_P(DatabaseMinDistance, RelationEdgesUnderChanges)
{
    constexpr ygg::uint_t n = 12;
    auto edges = Relation(tyr::datalog::tests::ops::cols({ x, y }));
    auto sources = Relation(tyr::datalog::tests::ops::cols({ x }));
    auto targets = Relation(tyr::datalog::tests::ops::cols({ y }));
    const auto plan = tyr::datalog::database::DistancePlan(sources.columns(), edges.columns(), targets.columns(), Column(99));
    const ygg::FreeRelationRepository layer;
    auto blind = tyr::datalog::database::MinDistance(layer, plan);
    auto guided = tyr::datalog::database::MinDistance<HalfDistance>(layer, plan);
    auto complete = Relation(tyr::datalog::tests::ops::cols({ x, y }));
    for (ygg::uint_t v = 0; v < n; ++v)
        for (ygg::uint_t w = 0; w < n; ++w)
            complete.insert(std::array { ygg::encode_value(v), ygg::encode_value(w) });
    auto heuristic = HalfDistance {};
    heuristic.columns = sources.columns();
    auto workspace = tyr::datalog::database::DistanceWorkspace(plan);
    auto result = Relation(plan.output_columns());
    auto random = std::mt19937(GetParam());
    const auto vertex = [&] { return static_cast<ygg::uint_t>(random() % n); };
    for (size_t step = 0; step < 300; ++step)
    {
        if (step % 25 == 0)
        {
            targets.clear();
            targets.insert(std::array { ygg::encode_value(vertex()) });
            targets.insert(std::array { ygg::encode_value(vertex()) });
            heuristic.distances = distances_to(borrow(layer, complete), borrow(layer, targets), 1);
        }
        sources.clear();
        sources.insert(std::array { ygg::encode_value(vertex()) });
        const auto changed = step % 3 == 0;
        if (changed)
            for (int i = 0; i < 3; ++i)
            {
                const auto edge = std::array { ygg::encode_value(vertex()), ygg::encode_value(vertex()) };
                if (edges.contains(edge))
                    edges.erase(edges.find(edge).value());
                else
                    edges.insert(edge);
            }
        const auto expected =
            tyr::datalog::database::min_distance(borrow(layer, sources), borrow(layer, edges), borrow(layer, targets), plan, result, workspace);
        const auto edge_view = borrow(layer, edges);
        const auto edge_input = [&](size_t) -> const auto& { return edge_view; };
        ASSERT_EQ(blind.min_distance(edge_input, borrow(layer, sources), borrow(layer, targets)), expected) << "step " << step;
        ASSERT_EQ(guided.min_distance(edge_input, borrow(layer, sources), borrow(layer, targets), heuristic), expected) << "step " << step;
    }
    EXPECT_GT(guided.statistics().tree_hits, 0);
}

/// Vertices are uint_t values; edges are adjacency lists.
struct ListGraph
{
    using Bytes = std::array<ygg::Value, 1>;

    urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::uint_t>>>> out;
    urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::uint_t>>>> in;

    static ygg::uint_t id(std::span<const ygg::Value> vertex) { return ygg::decode_value<ygg::uint_t>(vertex[0]); }
    static Bytes bytes(ygg::uint_t value) { return Bytes { ygg::encode_value(value) }; }
    template<typename OnVertex>
    static void each(const urd::Store<urd::sequence<ygg::uint_t>>& vertices, const OnVertex& on_vertex)
    {
        for (const auto w : vertices)
            if (const auto encoded = bytes(w); !on_vertex(std::span<const ygg::Value>(encoded)))
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
};

struct ListVertices
{
    urd::Store<urd::sequence<ListGraph::Bytes>> rows;

    [[nodiscard]] size_t size() const noexcept { return rows.size(); }
    [[nodiscard]] std::span<const ygg::Value> row(size_t i) const { return rows[i]; }
};

/// AStar::path() visits a shortest path from a source to a target, whether the search ends at a target or on a stored
/// chain of the backward frontier.
TEST_P(DatabaseMinDistance, AStarPathIsAShortestPath)
{
    auto rng = std::mt19937(GetParam());
    const auto n = static_cast<ygg::uint_t>(40);
    auto paths = static_cast<size_t>(0);
    auto chains = static_cast<size_t>(0);  // Paths that end on a stored chain.
    auto heuristic = tyr::datalog::database::BlindDistanceHeuristic {};
    auto tree = tyr::datalog::database::SearchTree {};
    auto backward = tyr::datalog::database::BackwardFrontier {};
    auto storage = tyr::datalog::database::AStarStorage {};
    for (size_t trial = 0; trial < 50; ++trial)
    {
        auto graph = ListGraph { urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::uint_t>>>>(n),
                                 urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::uint_t>>>>(n) };
        for (size_t e = 0; e < 70; ++e)
        {
            const auto u = static_cast<ygg::uint_t>(rng() % n);
            const auto v = static_cast<ygg::uint_t>(rng() % n);
            if (u != v && std::ranges::find(graph.out[u], v) == graph.out[u].end())
            {
                graph.out[u].push_back(v);
                graph.in[v].push_back(u);
            }
        }
        auto sources = ListVertices {};
        auto targets = tyr::datalog::database::VertexSet();
        tree.clear();
        auto ids = urd::Store<urd::sequence<tyr::datalog::database::VertexSet::handle_type>>();
        for (size_t i = 0; i < 2; ++i)
        {
            sources.rows.push_back(ListGraph::bytes(static_cast<ygg::uint_t>(rng() % n)));
            const auto target = ListGraph::bytes(static_cast<ygg::uint_t>(rng() % n));
            if (targets.insert(std::span<const ygg::Value>(target)).second)
                ids.push_back(tree.ensure(target, heuristic));
        }
        backward.start(ids);
        auto search =
            tyr::datalog::database::AStar<ListGraph, tyr::datalog::database::BlindDistanceHeuristic>(graph, tree, backward, heuristic, targets, storage, 0);
        const auto distance = search.run(sources);
        if (distance == std::numeric_limits<ygg::uint_t>::max())
            continue;
        auto path = urd::Store<urd::sequence<ygg::uint_t>>();
        search.path([&](std::span<const ygg::Value> vertex) { path.push_back(ListGraph::id(vertex)); });
        // Visited: where the search ended back to its source (the only source on the path, as sources have g = 0),
        // then the stored chain from where it ended to a target.
        const auto is_source = [&](ygg::uint_t v) { return std::ranges::any_of(sources.rows, [&](const auto& row) { return ListGraph::id(row) == v; }); };
        const auto edge = [&](ygg::uint_t u, ygg::uint_t v) { return graph.edge_exists(ListGraph::bytes(u), ListGraph::bytes(v)); };
        const auto source = static_cast<size_t>(std::ranges::find_if(path, is_source) - path.begin());
        ASSERT_LT(source, path.size()) << "trial " << trial;
        ASSERT_EQ(path.size(), distance + 1) << "trial " << trial;
        for (size_t i = 0; i < source; ++i)
            EXPECT_TRUE(edge(path[i + 1], path[i])) << "trial " << trial;
        for (size_t i = source + 1; i < path.size(); ++i)
            EXPECT_TRUE(edge(i == source + 1 ? path.front() : path[i - 1], path[i])) << "trial " << trial;
        EXPECT_TRUE(targets.contains(std::span<const ygg::Value>(ListGraph::bytes(source + 1 < path.size() ? path.back() : path.front()))));
        ++paths;
        chains += source + 1 < path.size();
    }
    EXPECT_GT(paths, 10);
    EXPECT_GT(chains, 0);
}

INSTANTIATE_TEST_SUITE_P(Seeds, DatabaseMinDistance, testing::Range(1u, 9u));

}  // namespace
