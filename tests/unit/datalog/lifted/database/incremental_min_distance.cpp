/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "query_helpers.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <urd/store.hpp>
#include "tyr/datalog/lifted/database/semantics/incremental/min_distance.hpp"
#include "tyr/datalog/lifted/database/semantics/query_evaluation.hpp"
#include "tyr/datalog/lifted/database/semantics/relation_view.hpp"

#include <cstddef>
#include <gtest/gtest.h>
#include <map>
#include <random>

namespace
{
using Column = urd::index<ygg::Column>;
using Relation = urd::builder<ygg::Relation<ygg::Schema>>;
using ygg::uint_t;

using namespace tyr::datalog::tests::distances;

/// An input slot's exact change, read through a layer (SlotChanges).
struct ChangeView
{
    ygg::FreeBuilderRelationView added;
    ygg::FreeBuilderRelationView removed;
};

/// Runs `steps` states; each mutates the inputs and maybe the sources and targets, then compares guided distances
/// (blind and with a consistent heuristic) against tyr::datalog::database::min_distance over the fully evaluated edges.
/// `maximal` are inputs whose edges include those of every state.
template<typename Mutate>
void check_sequence(tyr::datalog::database::QueryView<> edges,
                    const urd::Store<urd::sequence<Relation>>& maximal,
                    urd::Store<urd::sequence<Relation>>& inputs,
                    urd::Store<urd::set<size_t>, urd::dense> static_slots,
                    Relation& sources,
                    Relation& targets,
                    size_t steps,
                    Mutate mutate)
{
    const auto k = sources.columns().size();
    const ygg::FreeRelationRepository layer;
    const auto plan = tyr::datalog::database::DistancePlan(sources.columns(), edges.columns(), targets.columns(), Column(99));
    auto blind = tyr::datalog::database::incremental::MinDistance(layer, edges, plan, static_slots);
    auto guided = tyr::datalog::database::incremental::MinDistance<HalfDistance>(layer, edges, plan, static_slots);
    // Fed the exact changes of its inputs (update()) instead of diffing them (bind()).
    auto exact = tyr::datalog::database::incremental::MinDistance<HalfDistance>(layer, edges, plan, static_slots);
    auto previous = urd::Store<urd::sequence<Relation>>();
    struct Change
    {
        Relation added;
        Relation removed;
    };
    auto changes = urd::Store<urd::sequence<Change>>();
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
        blind.bind(input);
        guided.bind(input);
        if (previous.empty())
            exact.bind(input);
        else
        {
            auto changed = urd::Store<urd::set<size_t>, urd::dense>();
            changes.clear();
            for (size_t slot = 0; slot < inputs.size(); ++slot)
            {
                auto& change = changes.emplace_back(Change { Relation(inputs[slot].columns()), Relation(inputs[slot].columns()) });
                for (const auto row : layer.view(inputs[slot]).rows())
                    if (!previous[slot].contains(row))
                        change.added.insert(row);
                for (const auto row : layer.view(previous[slot]).rows())
                    if (!inputs[slot].contains(row))
                        change.removed.insert(row);
                if (!static_slots.contains(slot) && (!change.added.empty() || !change.removed.empty()))
                    changed.insert(slot);
            }
            exact.update([&](size_t slot) { return ChangeView { layer.view(changes[slot].added), layer.view(changes[slot].removed) }; }, changed);
        }
        previous.clear();
        for (const auto& relation : inputs)
        {
            auto& copy = previous.emplace_back(relation.columns());
            for (const auto row : layer.view(relation).rows())
                copy.insert(row);
        }
        heuristic.distances = distances_to(borrow(layer, supergraph), borrow(layer, targets), k);
        ASSERT_EQ(blind.min_distance(borrow(layer, sources), borrow(layer, targets)), expected) << "step " << step;
        ASSERT_EQ(guided.min_distance(borrow(layer, sources), borrow(layer, targets), heuristic), expected) << "step " << step;
        ASSERT_EQ(exact.min_distance(borrow(layer, sources), borrow(layer, targets), heuristic), expected) << "step " << step;
        // Reuse across calls without changes.
        ASSERT_EQ(guided.min_distance(borrow(layer, sources), borrow(layer, targets), heuristic), expected) << "step " << step;
        checked += expected != std::numeric_limits<ygg::uint_t>::max();
    }
    EXPECT_GT(checked, steps / 8);
    EXPECT_GT(guided.statistics().tree_hits, 0);
    EXPECT_GT(guided.statistics().repaired, 0);
    EXPECT_GT(guided.statistics().backward_vertices, 0);
}

const Column x(0), y(1), a(10), b(11), a2(12), b2(13);

class DatabaseIncrementalMinDistance : public testing::TestWithParam<unsigned>
{
};

TEST_P(DatabaseIncrementalMinDistance, SingleColumnEdgesUnderChanges)
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

TEST_P(DatabaseIncrementalMinDistance, PairEdgesWithJoinsUnionsSelectionsAndDifferences)
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
    const auto second =
        qb::project(repository,
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

TEST_P(DatabaseIncrementalMinDistance, RelationEdgesUnderChanges)
{
    constexpr ygg::uint_t n = 12;
    auto edges = Relation(tyr::datalog::tests::ops::cols({ x, y }));
    auto sources = Relation(tyr::datalog::tests::ops::cols({ x }));
    auto targets = Relation(tyr::datalog::tests::ops::cols({ y }));
    const auto plan = tyr::datalog::database::DistancePlan(sources.columns(), edges.columns(), targets.columns(), Column(99));
    const ygg::FreeRelationRepository layer;
    auto blind = tyr::datalog::database::incremental::MinDistance(layer, plan);
    auto guided = tyr::datalog::database::incremental::MinDistance<HalfDistance>(layer, plan);
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
        const auto expected = tyr::datalog::database::min_distance(borrow(layer, sources), borrow(layer, edges), borrow(layer, targets), plan, result, workspace);
        const auto edge_view = borrow(layer, edges);
        const auto edge_input = [&](size_t) -> const auto& { return edge_view; };
        auto maybe_changed = urd::Store<urd::set<size_t>, urd::dense>();
        if (changed || step == 0)
            maybe_changed.insert(0);
        blind.bind(edge_input, &maybe_changed);
        guided.bind(edge_input, &maybe_changed);
        ASSERT_EQ(blind.min_distance(borrow(layer, sources), borrow(layer, targets)), expected) << "step " << step;
        ASSERT_EQ(guided.min_distance(borrow(layer, sources), borrow(layer, targets), heuristic), expected) << "step " << step;
    }
    EXPECT_GT(guided.statistics().tree_hits, 0);
}

INSTANTIATE_TEST_SUITE_P(Seeds, DatabaseIncrementalMinDistance, testing::Range(1u, 9u));

TEST(DatabaseIncrementalBoundEvaluation, VersionChangesOnlyWhenRowsCanBeRemoved)
{
    namespace qb = tyr::datalog::tests::qb;
    auto repository = qb::repository();
    // E(x, y) without the edges into N(y).
    const auto edges = qb::antijoin(repository, qb::input(repository, 0, { x, y }), qb::rename(repository, qb::input(repository, 1, { x }), { y }));
    auto e = Relation(tyr::datalog::tests::ops::cols({ x, y }));
    auto n = Relation(tyr::datalog::tests::ops::cols({ x }));
    e.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    const ygg::FreeRelationRepository layer;
    auto evaluator = tyr::datalog::database::incremental::bound::QueryEvaluator(layer, edges);
    evaluator.initialize([&](size_t slot) { return borrow(layer, slot == 0 ? e : n); });
    auto e_added = Relation(tyr::datalog::tests::ops::cols({ x, y }));
    auto e_removed = Relation(tyr::datalog::tests::ops::cols({ x, y }));
    auto n_added = Relation(tyr::datalog::tests::ops::cols({ x }));
    auto n_removed = Relation(tyr::datalog::tests::ops::cols({ x }));
    const auto bumps = [&](size_t slot)
    {
        const auto version = evaluator.version();
        if (slot == 0)
            evaluator.update(0, layer.view(e_added), layer.view(e_removed));
        else
            evaluator.update(1, layer.view(n_added), layer.view(n_removed));
        e_added.clear();
        e_removed.clear();
        n_added.clear();
        n_removed.clear();
        return evaluator.version() != version;
    };
    e_added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    EXPECT_FALSE(bumps(0));  // A positive input gains a row: no edge can disappear.
    e_removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    EXPECT_TRUE(bumps(0));  // It loses one.
    n_added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    EXPECT_TRUE(bumps(1));  // The negated input gains a row.
    n_removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    EXPECT_FALSE(bumps(1));  // It loses one.
}

/// The exact change of one input slot, for MinDistance::update().
struct SlotChange
{
    Relation added = Relation(tyr::datalog::tests::ops::cols({ x, y }));
    Relation removed = Relation(tyr::datalog::tests::ops::cols({ x, y }));
};

/// Edges E0(x, y) ∪ E1(x, y) over slots 0 and 1, from source 0 to target 2.
struct TwoSlotEdges
{
    ygg::FreeRelationRepository layer;
    tyr::datalog::database::QueryRepository repository = tyr::datalog::tests::qb::repository();
    tyr::datalog::database::QueryView<> edges =
        tyr::datalog::tests::qb::union_(repository, tyr::datalog::tests::qb::input(repository, 0, { x, y }), tyr::datalog::tests::qb::input(repository, 1, { x, y }));
    Relation sources = Relation(tyr::datalog::tests::ops::cols({ x }));
    Relation targets = Relation(tyr::datalog::tests::ops::cols({ y }));
    tyr::datalog::database::DistancePlan plan = tyr::datalog::database::DistancePlan(sources.columns(), edges.columns(), targets.columns(), Column(99));
    urd::Store<urd::sequence<Relation>> inputs;

    TwoSlotEdges()
    {
        sources.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)) });
        targets.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)) });
        inputs.emplace_back(tyr::datalog::tests::ops::cols({ x, y }));
        inputs.emplace_back(tyr::datalog::tests::ops::cols({ x, y }));
        inputs[0].insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(static_cast<ygg::uint_t>(1)) });
        inputs[0].insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    }
    [[nodiscard]] auto input() const
    {
        return [this](size_t slot) { return borrow(layer, inputs[slot]); };
    }
    /// The changes as views (SlotChanges).
    [[nodiscard]] auto changes_of(const urd::Store<urd::sequence<SlotChange>>& changes) const
    {
        return [this, &changes](size_t slot) { return ChangeView { layer.view(changes[slot].added), layer.view(changes[slot].removed) }; };
    }
};

TEST(DatabaseIncrementalMinDistanceContract, InitializeDiscardsTheSearchWithUnchangedTargets)
{
    auto graph = TwoSlotEdges();
    auto changes = urd::Store<urd::sequence<SlotChange>>(2);
    const auto change = graph.changes_of(changes);
    for (const auto via_update : { false, true })
    {
        SCOPED_TRACE(via_update);
        auto search = tyr::datalog::database::incremental::MinDistance(graph.layer, graph.edges, graph.plan);
        graph.inputs[0].clear();
        graph.inputs[0].insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(static_cast<ygg::uint_t>(1)) });
        graph.inputs[0].insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(2)) });
        graph.inputs[1].clear();
        if (via_update)
        {
            search.initialize(graph.input());
            changes[1].added.clear();
            changes[1].added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(1)) });
            search.update(change, { 1 });
        }
        else
            search.bind(graph.input());
        ASSERT_EQ(search.min_distance(borrow(graph.layer, graph.sources), borrow(graph.layer, graph.targets)), 2);
        // A shortcut in a new baseline: the stored path 0 -> 1 -> 2 still exists but is no longer shortest.
        graph.inputs[0].insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(static_cast<ygg::uint_t>(2)) });
        search.initialize(graph.input());
        EXPECT_EQ(search.min_distance(borrow(graph.layer, graph.sources), borrow(graph.layer, graph.targets)), 1);
    }
}

TEST(DatabaseIncrementalMinDistanceContract, UpdateRejectsStaticUnboundAndOutOfRangeSlots)
{
    auto graph = TwoSlotEdges();
    auto changes = urd::Store<urd::sequence<SlotChange>>(3);
    changes[1].added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    changes[2].added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    const auto change = graph.changes_of(changes);

    auto unbound = tyr::datalog::database::incremental::MinDistance(graph.layer, graph.edges, graph.plan);
    EXPECT_THROW(unbound.update(change, { 1 }), std::invalid_argument);

    auto with_static = tyr::datalog::database::incremental::MinDistance(graph.layer, graph.edges, graph.plan, { 1 });
    with_static.initialize(graph.input());
    EXPECT_THROW(with_static.update(change, { 1 }), std::invalid_argument);

    auto search = tyr::datalog::database::incremental::MinDistance(graph.layer, graph.edges, graph.plan);
    search.initialize(graph.input());
    EXPECT_THROW(search.update(change, { 2 }), std::invalid_argument);
    EXPECT_EQ(search.min_distance(borrow(graph.layer, graph.sources), borrow(graph.layer, graph.targets)), 2);
}

TEST(DatabaseIncrementalMinDistanceContract, DistancesStayExactAfterAThrowingUpdate)
{
    auto graph = TwoSlotEdges();
    auto search = tyr::datalog::database::incremental::MinDistance(graph.layer, graph.edges, graph.plan);
    search.initialize(graph.input());
    ASSERT_EQ(search.min_distance(borrow(graph.layer, graph.sources), borrow(graph.layer, graph.targets)), 2);
    // Slot 0 gains the shortcut 0 -> 2 before slot 1's change throws: its removed row is absent.
    auto changes = urd::Store<urd::sequence<SlotChange>>(2);
    changes[0].added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    changes[1].removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(5)), ygg::encode_value(static_cast<ygg::uint_t>(5)) });
    EXPECT_THROW(search.update(graph.changes_of(changes), { 0, 1 }), std::invalid_argument);
    EXPECT_EQ(search.min_distance(borrow(graph.layer, graph.sources), borrow(graph.layer, graph.targets)), 1);
}

TEST(DatabaseIncrementalBoundEvaluation, FailedChangesLeaveTheInputUnchanged)
{
    namespace qb = tyr::datalog::tests::qb;
    auto repository = qb::repository();
    auto e = Relation(tyr::datalog::tests::ops::cols({ x, y }));
    e.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    e.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    const ygg::FreeRelationRepository layer;
    auto evaluator = tyr::datalog::database::incremental::bound::QueryEvaluator(layer, qb::input(repository, 0, { x, y }));
    evaluator.initialize([&](size_t) { return borrow(layer, e); });
    const auto successors = evaluator.prepare({ 0 });
    const auto count = [&]
    {
        auto rows = static_cast<size_t>(0);
        evaluator.evaluate(successors,
                           std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)) },
                           [&](std::span<const ygg::Value>)
                           {
                               ++rows;
                               return true;
                           });
        return rows;
    };
    ASSERT_EQ(count(), 2);  // Builds the index on x.
    const auto version = evaluator.version();
    auto added = Relation(tyr::datalog::tests::ops::cols({ x, y }));
    auto removed = Relation(tyr::datalog::tests::ops::cols({ x, y }));
    // A present row removed before an absent one.
    removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(3)), ygg::encode_value(static_cast<ygg::uint_t>(3)) });
    EXPECT_THROW(evaluator.update(0, layer.view(added), layer.view(removed)), std::invalid_argument);
    // A new row added before a present one.
    removed.clear();
    added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(static_cast<ygg::uint_t>(3)) });
    added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    EXPECT_THROW(evaluator.update(0, layer.view(added), layer.view(removed)), std::invalid_argument);
    EXPECT_EQ(evaluator.version(), version);
    EXPECT_EQ(evaluator.input(0).size(), 2);
    EXPECT_TRUE(evaluator.input(0).contains(std::array { ygg::encode_value(ygg::uint_t(0)), ygg::encode_value(ygg::uint_t(1)) }));
    EXPECT_FALSE(evaluator.input(0).contains(std::array { ygg::encode_value(ygg::uint_t(0)), ygg::encode_value(ygg::uint_t(3)) }));
    EXPECT_EQ(count(), 2);
    EXPECT_THROW(evaluator.update(1, layer.view(added), layer.view(removed)), std::invalid_argument);
}

}  // namespace
