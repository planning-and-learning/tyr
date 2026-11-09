/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "query_helpers.hpp"
#include "tyr/datalog/lifted/database/semantics/incremental/distance.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

#include "tyr/datalog/lifted/database/semantics/relation_view.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include <random>
#include <set>
#include <span>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace tyr::datalog::tests
{
namespace
{
using Values = boost::mp11::mp_list<ygg::uint_t>;
using RelationBuilder = urd::builder<ygg::Relation<ygg::Schema>>;
using ColumnIndex = urd::index<ygg::Column>;
using Vertices = std::set<ygg::uint_t>;
using Edges = std::set<std::pair<ygg::uint_t, ygg::uint_t>>;
using Results = std::set<std::array<ygg::uint_t, 3>>;

/// The layer the tests read their builders through.
const ygg::FreeRelationRepository& layer()
{
    static const ygg::FreeRelationRepository repository;
    return repository;
}

template<database::RelationViewConcept V>
Results rows(const V& input)
{
    Results result;
    for (size_t i = 0; i < input.size(); ++i)
    {
        const auto row = input.row(i);
        result.insert({ ygg::decode_value<ygg::uint_t>(row.value(static_cast<size_t>(0))), ygg::decode_value<ygg::uint_t>(row.value(static_cast<size_t>(1))), ygg::decode_value<ygg::uint_t>(row.value(static_cast<size_t>(2))) });
    }
    return result;
}

void fill(RelationBuilder& input, const Vertices& vertices)
{
    input.clear();
    for (const auto vertex : vertices)
        input.insert(std::array { ygg::encode_value(vertex) });
}

void fill(RelationBuilder& input, const Edges& edges)
{
    input.clear();
    for (const auto& [from, to] : edges)
        input.insert(std::array { ygg::encode_value(from), ygg::encode_value(to) });
}

// Independent pair-by-pair BFS, deliberately unrelated to the maintained graph.
Results reference(const Vertices& sources, const Edges& edges, const Vertices& targets, size_t universe)
{
    Results result;
    for (const auto source : sources)
        for (const auto target : targets)
        {
            urd::Store<urd::sequence<int>> distances(universe, -1);
            urd::Store<urd::sequence<ygg::uint_t>> queue { source };
            distances[source] = 0;
            for (size_t head = 0; head < queue.size() && distances[target] < 0; ++head)
                for (const auto& [from, to] : edges)
                    if (from == queue[head] && distances[to] < 0)
                    {
                        distances[to] = distances[from] + 1;
                        queue.push_back(to);
                    }
            if (distances[target] >= 0)
                result.insert({ source, target, static_cast<ygg::uint_t>(distances[target]) });
        }
    return result;
}

void expect_delta(const database::incremental::DistanceEvaluator& evaluator, const Results& before, const Results& after)
{
    Results added, removed;
    for (const auto& row : after)
        if (!before.contains(row))
            added.insert(row);
    for (const auto& row : before)
        if (!after.contains(row))
            removed.insert(row);
    EXPECT_EQ(rows(evaluator.get_result()), after);
    EXPECT_EQ(rows(evaluator.get_delta().change().first), added);
    EXPECT_EQ(rows(evaluator.get_delta().change().second), removed);
}

template<typename Set>
void changes(database::incremental::Delta& delta, const Set& before, const Set& after)
{
    Set added, removed;
    for (const auto& value : after)
        if (!before.contains(value))
            added.insert(value);
    for (const auto& value : before)
        if (!after.contains(value))
            removed.insert(value);
    fill(delta.added, added);
    fill(delta.removed, removed);
}
}  // namespace

TEST(YggdrasilTests, DatabaseDistanceIncludesZeroPathsAndKeepsInputTupleLabelsPositional)
{
    RelationBuilder sources(tyr::datalog::tests::ops::cols({ ColumnIndex(11) })), edges(tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) })), targets(tyr::datalog::tests::ops::cols({ ColumnIndex(99) }));
    fill(sources, Vertices { 0, 5, 8 });
    fill(edges, Edges { { 0, 1 }, { 0, 2 }, { 1, 3 }, { 2, 3 }, { 3, 4 }, { 4, 1 }, { 3, 3 }, { 6, 7 } });
    fill(targets, Vertices { 0, 3, 4, 5, 7, 8 });
    const database::DistancePlan plan(sources.columns(), edges.columns(), targets.columns(), ColumnIndex(500));
    EXPECT_EQ(plan.arity(), 1);
    EXPECT_EQ(plan.tuple_size(), 1);
    ASSERT_EQ(plan.output_columns().size(), 3);
    EXPECT_EQ(plan.output_columns()[0].label, ColumnIndex(1));
    EXPECT_EQ(plan.output_columns()[1].label, ColumnIndex(2));
    EXPECT_EQ(plan.output_columns()[2].label, ColumnIndex(500));
    const Results expected { { 0, 0, 0 }, { 0, 3, 2 }, { 0, 4, 3 }, { 5, 5, 0 }, { 8, 8, 0 } };
    auto result = tyr::datalog::tests::ops::distances(layer().view(sources), layer().view(edges), layer().view(targets), plan);
    EXPECT_EQ(rows(layer().view(result)), expected);

    database::DistanceWorkspace workspace(plan);
    sources.clear();
    database::distance(layer().view(sources), layer().view(edges), layer().view(targets), plan, result, workspace);
    EXPECT_TRUE(result.empty());
    fill(sources, Vertices { 5 });
    edges.clear();
    database::distance(layer().view(sources), layer().view(edges), layer().view(targets), plan, result, workspace);
    EXPECT_EQ(rows(layer().view(result)), (Results { { 5, 5, 0 } }));
    targets.clear();
    database::distance(layer().view(sources), layer().view(edges), layer().view(targets), plan, result, workspace);
    EXPECT_TRUE(result.empty());
}

TEST(YggdrasilTests, DatabaseDistanceAcceptsInternedViewsWithoutRetainingInputOwners)
{
    RelationBuilder sources(tyr::datalog::tests::ops::cols({ ColumnIndex(10) })), edges(tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) })), targets(tyr::datalog::tests::ops::cols({ ColumnIndex(20) }));
    fill(sources, Vertices { 1 });
    fill(edges, Edges { { 1, 2 }, { 2, 3 } });
    fill(targets, Vertices { 3 });
    const database::DistancePlan plan(sources.columns(), edges.columns(), targets.columns(), ColumnIndex(3));
    ygg::FreeRelationRepository repository;
    const auto source_view = tyr::datalog::tests::ops::intern(repository, sources).first;
    const auto edge_view = tyr::datalog::tests::ops::intern(repository, edges).first;
    const auto target_view = tyr::datalog::tests::ops::intern(repository, targets).first;
    auto result = tyr::datalog::tests::ops::distances(source_view, edge_view, target_view, plan);
    EXPECT_EQ(rows(layer().view(result)), (Results { { 1, 3, 2 } }));
    database::incremental::DistanceEvaluator evaluator(layer(), plan);
    evaluator.initialize(source_view, edge_view, target_view);
    repository.clear();
    sources.clear();
    edges.clear();
    targets.clear();
    evaluator.update(std::pair { layer().view(sources), layer().view(sources) }, std::pair { layer().view(edges), layer().view(edges) }, std::pair { layer().view(targets), layer().view(targets) });
    EXPECT_EQ(rows(evaluator.get_result()), rows(layer().view(result)));
    EXPECT_TRUE(evaluator.get_delta().added.empty());
    EXPECT_TRUE(evaluator.get_delta().removed.empty());
}

TEST(YggdrasilTests, DatabaseDistancePlansValidateSchemasAndOutputsMustNotAlias)
{
    RelationBuilder sources(tyr::datalog::tests::ops::cols({ ColumnIndex(10) })), edges(tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) })), targets(tyr::datalog::tests::ops::cols({ ColumnIndex(20) }));
    fill(sources, Vertices { 1 });
    fill(targets, Vertices { 1 });
    const database::DistancePlan plan(sources.columns(), edges.columns(), targets.columns(), ColumnIndex(3));
    EXPECT_THROW((database::DistancePlan(sources.columns(), sources.columns(), targets.columns(), ColumnIndex(3))), std::invalid_argument);
    EXPECT_THROW((database::DistancePlan(sources.columns(), edges.columns(), edges.columns(), ColumnIndex(3))), std::invalid_argument);
    EXPECT_THROW((database::DistancePlan(sources.columns(), edges.columns(), targets.columns(), ColumnIndex(2))), std::invalid_argument);
    const std::vector<ygg::ColumnLayout> wrong_type { { ColumnIndex(20), ygg::type_id<double> } };
    EXPECT_THROW((database::DistancePlan(sources.columns(), edges.columns(), wrong_type, ColumnIndex(3))), std::invalid_argument);

    database::DistanceWorkspace workspace(plan);
    auto output = tyr::datalog::tests::ops::distances(layer().view(sources), layer().view(edges), layer().view(targets), plan);
    EXPECT_EQ(rows(layer().view(output)), (Results { { 1, 1, 0 } }));
    EXPECT_THROW(database::distance(layer().view(sources), layer().view(edges), layer().view(targets), plan, sources, workspace), std::invalid_argument);
    EXPECT_TRUE(sources.contains(std::array { ygg::encode_value(ygg::uint_t(1)) }));

    database::incremental::DistanceEvaluator evaluator(layer(), plan);
    database::incremental::Delta source_delta(layer(), sources.columns()), edge_delta(layer(), edges.columns()), target_delta(layer(), targets.columns());
    EXPECT_THROW((void) evaluator.get_result(), std::logic_error);
    EXPECT_THROW((void) evaluator.get_delta(), std::logic_error);
    EXPECT_THROW(evaluator.update(source_delta.change(), edge_delta.change(), target_delta.change()), std::logic_error);
    evaluator.initialize(layer().view(sources), layer().view(edges), layer().view(targets));
    source_delta.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    source_delta.removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    EXPECT_THROW(evaluator.update(source_delta.change(), edge_delta.change(), target_delta.change()), std::invalid_argument);
    EXPECT_EQ(rows(evaluator.get_result()), rows(layer().view(output)));
    source_delta.clear();
    evaluator.update(source_delta.change(), edge_delta.change(), target_delta.change());
    EXPECT_EQ(rows(evaluator.get_result()), rows(layer().view(output)));
}

TEST(YggdrasilTests, DatabaseDistanceRejectsChangesThatAreNotActualSetChanges)
{
    RelationBuilder sources(tyr::datalog::tests::ops::cols({ ColumnIndex(10) })), edges(tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) })), targets(tyr::datalog::tests::ops::cols({ ColumnIndex(20) }));
    fill(sources, Vertices { 1 });
    fill(edges, Edges { { 1, 2 } });
    fill(targets, Vertices { 2 });
    const database::DistancePlan plan(sources.columns(), edges.columns(), targets.columns(), ColumnIndex(3));
    database::incremental::DistanceEvaluator evaluator(layer(), plan);
    database::incremental::Delta ds(layer(), sources.columns()), de(layer(), edges.columns()), dt(layer(), targets.columns());
    for (size_t input = 0; input < 3; ++input)
        for (const bool removing : { false, true })
        {
            evaluator.initialize(layer().view(sources), layer().view(edges), layer().view(targets));
            ds.clear();
            de.clear();
            dt.clear();
            if (input == 0)
                (removing ? ds.removed : ds.added).insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(removing ? 7 : 1)) });
            else if (input == 1)
                (removing ? de.removed : de.added).insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(removing ? 7 : 2)) });
            else
                (removing ? dt.removed : dt.added).insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(removing ? 7 : 2)) });
            // Invalid changes are rejected before mutation, so the previous evaluation remains.
            EXPECT_THROW(evaluator.update(ds.change(), de.change(), dt.change()), std::invalid_argument);
            EXPECT_EQ(rows(evaluator.get_result()), (Results { { 1, 2, 1 } }));
            EXPECT_TRUE(evaluator.get_delta().added.empty() && evaluator.get_delta().removed.empty());
        }
    evaluator.initialize(layer().view(sources), layer().view(edges), layer().view(targets));
    EXPECT_EQ(rows(evaluator.get_result()), (Results { { 1, 2, 1 } }));
}

TEST(YggdrasilTests, DatabaseDistanceRepairsAlternateShortestPathsAndDisconnectsCycles)
{
    RelationBuilder sources(tyr::datalog::tests::ops::cols({ ColumnIndex(10) })), edges(tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) })), targets(tyr::datalog::tests::ops::cols({ ColumnIndex(20) }));
    fill(sources, Vertices { 0 });
    fill(edges, Edges { { 0, 1 }, { 0, 2 }, { 1, 3 }, { 2, 3 }, { 3, 4 }, { 4, 1 } });
    fill(targets, Vertices { 3, 4 });
    const database::DistancePlan plan(sources.columns(), edges.columns(), targets.columns(), ColumnIndex(3));
    database::incremental::DistanceEvaluator evaluator(layer(), plan);
    evaluator.initialize(layer().view(sources), layer().view(edges), layer().view(targets));
    database::incremental::Delta ds(layer(), sources.columns()), de(layer(), edges.columns()), dt(layer(), targets.columns());
    const Results initial { { 0, 3, 2 }, { 0, 4, 3 } };
    EXPECT_EQ(rows(evaluator.get_result()), initial);
    de.removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    evaluator.update(ds.change(), de.change(), dt.change());
    expect_delta(evaluator, initial, initial);
    de.clear();
    de.removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    evaluator.update(ds.change(), de.change(), dt.change());
    expect_delta(evaluator, initial, {});
    de.clear();
    de.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(static_cast<ygg::uint_t>(4)) });
    evaluator.update(ds.change(), de.change(), dt.change());
    const Results repaired { { 0, 3, 3 }, { 0, 4, 1 } };
    expect_delta(evaluator, {}, repaired);
    de.clear();
    de.removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(static_cast<ygg::uint_t>(4)) });
    de.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    evaluator.update(ds.change(), de.change(), dt.change());
    expect_delta(evaluator, repaired, initial);
}

TEST(YggdrasilTests, DatabaseDistanceMixedReplacementLeavesAnUnchangedChainUnprocessed)
{
    RelationBuilder sources(tyr::datalog::tests::ops::cols({ ColumnIndex(10) })), edges(tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) })), targets(tyr::datalog::tests::ops::cols({ ColumnIndex(20) }));
    constexpr ygg::uint_t last = 130;
    fill(sources, Vertices { 0 });
    Edges edge_set { { 0, 1 }, { 0, 2 }, { 1, 3 } };
    for (ygg::uint_t vertex = 3; vertex < last; ++vertex)
        edge_set.emplace(vertex, vertex + 1);
    fill(edges, edge_set);
    fill(targets, Vertices { last });
    const database::DistancePlan plan(sources.columns(), edges.columns(), targets.columns(), ColumnIndex(3));
    database::incremental::DistanceEvaluator evaluator(layer(), plan);
    evaluator.initialize(layer().view(sources), layer().view(edges), layer().view(targets));
    const Results expected { { 0, last, last - 1 } };
    database::incremental::Delta ds(layer(), sources.columns()), de(layer(), edges.columns()), dt(layer(), targets.columns());
    fill(de.removed, Edges { { 1, 3 } });
    fill(de.added, Edges { { 2, 3 } });
    for (const bool undo : { false, true })
    {
        SCOPED_TRACE(undo);
        database::incremental::detail::distance_work() = {};
        evaluator.update(ds.change(), std::pair { layer().view(undo ? de.removed : de.added), layer().view(undo ? de.added : de.removed) }, dt.change());
        const auto work = database::incremental::detail::distance_work();
        expect_delta(evaluator, expected, expected);
        EXPECT_EQ(work.processed_vertices, 0);
        EXPECT_EQ(work.incoming_edges, 0);
        EXPECT_EQ(work.outgoing_edges, 0);
        EXPECT_LE(work.heap_inserts + work.heap_updates + work.heap_erases + work.heap_pops, 8);
    }
}

TEST(YggdrasilTests, DatabaseDistanceEqualSupportsDoNotRescanDenseUnchangedNeighbors)
{
    RelationBuilder sources(tyr::datalog::tests::ops::cols({ ColumnIndex(10) })), edges(tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) })), targets(tyr::datalog::tests::ops::cols({ ColumnIndex(20) }));
    constexpr ygg::uint_t neighbors = 40;
    fill(sources, Vertices { 0 });
    Edges edge_set { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 0, 4 } };
    Vertices target_set { 3 };
    for (ygg::uint_t vertex = 5; vertex < 5 + neighbors; ++vertex)
    {
        target_set.insert(vertex);
        edge_set.emplace(3, vertex);
        edge_set.emplace(4, vertex);
        for (ygg::uint_t other = 5; other < 5 + neighbors; ++other)
            edge_set.emplace(vertex, other);
    }
    fill(edges, edge_set);
    fill(targets, target_set);
    const database::DistancePlan plan(sources.columns(), edges.columns(), targets.columns(), ColumnIndex(3));
    database::incremental::DistanceEvaluator evaluator(layer(), plan);
    evaluator.initialize(layer().view(sources), layer().view(edges), layer().view(targets));
    const auto before = reference(Vertices { 0 }, edge_set, target_set, 5 + neighbors);
    edge_set.emplace(0, 3);
    const auto after = reference(Vertices { 0 }, edge_set, target_set, 5 + neighbors);
    database::incremental::Delta ds(layer(), sources.columns()), de(layer(), edges.columns()), dt(layer(), targets.columns());
    fill(de.added, Edges { { 0, 3 } });
    for (const bool undo : { false, true })
    {
        SCOPED_TRACE(undo);
        database::incremental::detail::distance_work() = {};
        evaluator.update(ds.change(), std::pair { layer().view(undo ? de.removed : de.added), layer().view(undo ? de.added : de.removed) }, dt.change());
        const auto work = database::incremental::detail::distance_work();
        expect_delta(evaluator, undo ? after : before, undo ? before : after);
        EXPECT_EQ(work.processed_vertices, undo ? 2 : 1);
        EXPECT_LE(work.incoming_edges + work.outgoing_edges, 4 * neighbors + 16);
        EXPECT_LE(work.heap_inserts + work.heap_updates + work.heap_erases + work.heap_pops, 8 * neighbors + 16);
    }
}

TEST(YggdrasilTests, DatabaseDistanceRepairsUnreachableCyclesSelfLoopsAndEdgesIntoTheSource)
{
    RelationBuilder sources(tyr::datalog::tests::ops::cols({ ColumnIndex(10) })), edges(tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) })), targets(tyr::datalog::tests::ops::cols({ ColumnIndex(20) }));
    const Vertices source_set { 0 }, target_set { 0, 1, 2, 3, 4, 5, 6 };
    Edges edge_set { { 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 2 }, { 2, 1 }, { 3, 3 }, { 3, 4 }, { 4, 3 }, { 4, 0 } };
    fill(sources, source_set);
    fill(edges, edge_set);
    fill(targets, target_set);
    const database::DistancePlan plan(sources.columns(), edges.columns(), targets.columns(), ColumnIndex(3));
    database::incremental::DistanceEvaluator evaluator(layer(), plan);
    evaluator.initialize(layer().view(sources), layer().view(edges), layer().view(targets));
    database::incremental::Delta ds(layer(), sources.columns()), de(layer(), edges.columns()), dt(layer(), targets.columns());
    const std::array<std::pair<Edges, Edges>, 5> batches { {
        { Edges { { 2, 5 } }, Edges { { 0, 1 } } },
        { Edges { { 0, 3 } }, Edges {} },
        { Edges { { 4, 1 } }, Edges {} },
        { Edges { { 5, 0 }, { 2, 2 } }, Edges { { 1, 1 }, { 3, 3 }, { 4, 0 } } },
        { Edges { { 0, 5 }, { 5, 3 } }, Edges { { 0, 3 } } },
    } };
    for (size_t step = 0; step < batches.size(); ++step)
    {
        SCOPED_TRACE(step);
        const auto before = reference(source_set, edge_set, target_set, 7);
        const auto& [added, removed] = batches[step];
        for (const auto& edge : removed)
            edge_set.erase(edge);
        edge_set.insert(added.begin(), added.end());
        fill(de.added, added);
        fill(de.removed, removed);
        evaluator.update(ds.change(), de.change(), dt.change());
        expect_delta(evaluator, before, reference(source_set, edge_set, target_set, 7));
        EXPECT_TRUE(evaluator.get_result().contains(std::array { ygg::encode_value(ygg::uint_t(0)), ygg::encode_value(ygg::uint_t(0)), ygg::encode_value(ygg::uint_t(0)) }));
    }
}

TEST(YggdrasilTests, DatabaseDistanceMixedTupleTypesShareCanonicalVertices)
{
    const auto schema = [](ygg::uint_t label, size_t tuples)
    {
        std::vector<ygg::ColumnLayout> result;
        for (size_t i = 0; i < tuples; ++i)
        {
            result.push_back({ ColumnIndex(label++), ygg::type_id<ygg::uint_t> });
            result.push_back({ ColumnIndex(label++), ygg::type_id<double> });
            result.push_back({ ColumnIndex(label++), ygg::type_id<bool> });
        }
        return result;
    };
    urd::builder<ygg::Relation<ygg::Schema>> sources(schema(11, 1)), edges(schema(1, 2)), targets(schema(21, 1));
    sources.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(7)), ygg::encode_value(-0.0), ygg::encode_value(true) });
    edges.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(7)), ygg::encode_value(+0.0), ygg::encode_value(true), ygg::encode_value(static_cast<ygg::uint_t>(8)), ygg::encode_value(1.5), ygg::encode_value(false) });
    targets.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(7)), ygg::encode_value(+0.0), ygg::encode_value(true) });
    targets.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(8)), ygg::encode_value(1.5), ygg::encode_value(false) });
    const database::DistancePlan plan(sources.columns(), edges.columns(), targets.columns(), ColumnIndex(99));
    EXPECT_EQ(plan.arity(), 3);
    EXPECT_EQ(plan.tuple_size(), 3);
    auto result = tyr::datalog::tests::ops::distances(layer().view(sources), layer().view(edges), layer().view(targets), plan);
    ASSERT_EQ(result.size(), 2);
    EXPECT_TRUE(result.contains(std::array { ygg::encode_value(ygg::uint_t(7)), ygg::encode_value(0.0), ygg::encode_value(true), ygg::encode_value(ygg::uint_t(7)), ygg::encode_value(0.0), ygg::encode_value(true), ygg::encode_value(ygg::uint_t(0)) }));
    EXPECT_TRUE(result.contains(std::array { ygg::encode_value(ygg::uint_t(7)), ygg::encode_value(0.0), ygg::encode_value(true), ygg::encode_value(ygg::uint_t(8)), ygg::encode_value(1.5), ygg::encode_value(false), ygg::encode_value(ygg::uint_t(1)) }));
    database::incremental::DistanceEvaluator evaluator(layer(), plan);
    evaluator.initialize(layer().view(sources), layer().view(edges), layer().view(targets));
    database::incremental::Delta ds(layer(), sources.columns()), de(layer(), edges.columns()), dt(layer(), targets.columns());
    de.removed.insert(layer().view(edges).row(0).get_values());
    evaluator.update(ds.change(), de.change(), dt.change());
    EXPECT_EQ(evaluator.get_result().size(), 1);
    EXPECT_TRUE(evaluator.get_delta().added.empty());
    ASSERT_EQ(evaluator.get_delta().removed.size(), 1);
    EXPECT_TRUE(evaluator.get_delta().removed.contains(std::array { ygg::encode_value(ygg::uint_t(7)), ygg::encode_value(0.0), ygg::encode_value(true), ygg::encode_value(ygg::uint_t(8)), ygg::encode_value(1.5), ygg::encode_value(false), ygg::encode_value(ygg::uint_t(1)) }));
    evaluator.update(std::pair { layer().view(ds.removed), layer().view(ds.added) }, std::pair { layer().view(de.removed), layer().view(de.added) }, std::pair { layer().view(dt.removed), layer().view(dt.added) });
    EXPECT_EQ(evaluator.get_result().size(), 2);
    for (size_t i = 0; i < result.size(); ++i)
        EXPECT_TRUE(evaluator.get_result().contains(layer().view(result).row(i)));
}

TEST(YggdrasilTests, DatabaseDistanceNullaryTuplesUseTheSingleEmptyVertex)
{
    RelationBuilder sources, edges, targets;
    targets.insert(std::array<ygg::Value, 0> {});
    const database::DistancePlan plan(sources.columns(), edges.columns(), targets.columns(), ColumnIndex(3));
    EXPECT_EQ(plan.arity(), 0);
    EXPECT_EQ(plan.tuple_size(), 0);
    auto output = tyr::datalog::tests::ops::distances(layer().view(sources), layer().view(edges), layer().view(targets), plan);
    EXPECT_TRUE(output.empty());
    database::incremental::DistanceEvaluator evaluator(layer(), plan);
    evaluator.initialize(layer().view(sources), layer().view(edges), layer().view(targets));
    RelationBuilder empty, truth;
    truth.insert(std::array<ygg::Value, 0> {});
    evaluator.update(std::pair { layer().view(truth), layer().view(empty) }, std::pair { layer().view(empty), layer().view(empty) }, std::pair { layer().view(empty), layer().view(empty) });
    ASSERT_EQ(evaluator.get_result().size(), 1);
    EXPECT_TRUE(evaluator.get_result().contains(std::array { ygg::encode_value(ygg::uint_t(0)) }));
    evaluator.update(std::pair { layer().view(empty), layer().view(empty) }, std::pair { layer().view(truth), layer().view(empty) }, std::pair { layer().view(empty), layer().view(empty) });
    EXPECT_TRUE(evaluator.get_delta().added.empty());
    EXPECT_TRUE(evaluator.get_delta().removed.empty());
    evaluator.update(std::pair { layer().view(empty), layer().view(truth) }, std::pair { layer().view(empty), layer().view(truth) }, std::pair { layer().view(empty), layer().view(empty) });
    EXPECT_TRUE(evaluator.get_result().empty());
    EXPECT_TRUE(evaluator.get_delta().removed.contains(std::array { ygg::encode_value(ygg::uint_t(0)) }));
    sources.insert(std::array<ygg::Value, 0> {});
    database::DistanceWorkspace workspace(plan);
    database::distance(layer().view(sources), layer().view(edges), layer().view(targets), plan, output, workspace);
    EXPECT_TRUE(output.contains(std::array { ygg::encode_value(ygg::uint_t(0)) }));
}

TEST(YggdrasilTests, DatabaseDistanceGrowsTheSharedDictionaryAndResetsToASmallerUniverse)
{
    RelationBuilder sources(tyr::datalog::tests::ops::cols({ ColumnIndex(10) })), edges(tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) })), targets(tyr::datalog::tests::ops::cols({ ColumnIndex(20) }));
    fill(sources, Vertices { 1 });
    fill(targets, Vertices { 1 });
    const database::DistancePlan plan(sources.columns(), edges.columns(), targets.columns(), ColumnIndex(3));
    database::incremental::DistanceEvaluator evaluator(layer(), plan);
    evaluator.initialize(layer().view(sources), layer().view(edges), layer().view(targets));
    const Results initial { { 1, 1, 0 } };
    EXPECT_EQ(rows(evaluator.get_result()), initial);
    database::incremental::Delta ds(layer(), sources.columns()), de(layer(), edges.columns()), dt(layer(), targets.columns());
    fill(ds.added, Vertices { 33, 65 });
    fill(dt.added, Vertices { 80, 97 });
    Edges expanded_edges;
    for (ygg::uint_t vertex = 1; vertex < 80; ++vertex)
        expanded_edges.emplace(vertex, vertex + 1);
    expanded_edges.emplace(80, 97);
    fill(de.added, expanded_edges);
    evaluator.update(ds.change(), de.change(), dt.change());
    const auto expanded = reference(Vertices { 1, 33, 65 }, expanded_edges, Vertices { 1, 80, 97 }, 98);
    ASSERT_EQ(expanded.size(), 7);
    expect_delta(evaluator, initial, expanded);
    evaluator.update(std::pair { layer().view(ds.removed), layer().view(ds.added) }, std::pair { layer().view(de.removed), layer().view(de.added) }, std::pair { layer().view(dt.removed), layer().view(dt.added) });
    expect_delta(evaluator, expanded, initial);

    fill(sources, Vertices { 2 });
    fill(edges, Edges { { 2, 3 } });
    fill(targets, Vertices { 3 });
    evaluator.initialize(layer().view(sources), layer().view(edges), layer().view(targets));
    const Results smaller { { 2, 3, 1 } };
    EXPECT_EQ(rows(evaluator.get_result()), smaller);
    EXPECT_TRUE(evaluator.get_delta().added.empty());
    EXPECT_TRUE(evaluator.get_delta().removed.empty());
    ds.clear();
    de.clear();
    dt.clear();
    ds.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(3)) });
    de.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(3)), ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    dt.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    evaluator.update(ds.change(), de.change(), dt.change());
    expect_delta(evaluator, smaller, Results { { 2, 2, 0 }, { 2, 3, 1 }, { 3, 2, 1 }, { 3, 3, 0 } });
}

TEST(YggdrasilTests, DatabaseDistanceMovesRetainedRepairStorageAndResetsAfterward)
{
    RelationBuilder sources(tyr::datalog::tests::ops::cols({ ColumnIndex(10) })), edges(tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) })), targets(tyr::datalog::tests::ops::cols({ ColumnIndex(20) }));
    fill(sources, Vertices { 0 });
    fill(edges, Edges { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 4 } });
    fill(targets, Vertices { 3, 4 });
    const database::DistancePlan plan(sources.columns(), edges.columns(), targets.columns(), ColumnIndex(3));
    database::incremental::DistanceEvaluator evaluator(layer(), plan);
    evaluator.initialize(layer().view(sources), layer().view(edges), layer().view(targets));
    database::incremental::Delta ds(layer(), sources.columns()), de(layer(), edges.columns()), dt(layer(), targets.columns());
    fill(de.added, Edges { { 0, 3 } });
    const Results initial { { 0, 3, 3 }, { 0, 4, 4 } }, shortcut { { 0, 3, 1 }, { 0, 4, 2 } };
    evaluator.update(ds.change(), de.change(), dt.change());

    database::incremental::DistanceEvaluator moved(std::move(evaluator));
    expect_delta(moved, initial, shortcut);
    moved.update(ds.change(), std::pair { layer().view(de.removed), layer().view(de.added) }, dt.change());
    expect_delta(moved, shortcut, initial);

    database::incremental::DistanceEvaluator assigned(layer(), plan);
    assigned.initialize(layer().view(sources), layer().view(edges), layer().view(targets));
    assigned.update(ds.change(), de.change(), dt.change());
    assigned = std::move(moved);
    expect_delta(assigned, shortcut, initial);
    assigned.update(ds.change(), de.change(), dt.change());
    expect_delta(assigned, initial, shortcut);

    fill(sources, Vertices { 20 });
    fill(edges, Edges { { 20, 21 }, { 21, 21 } });
    fill(targets, Vertices { 20, 21, 22 });
    assigned.initialize(layer().view(sources), layer().view(edges), layer().view(targets));
    const Results reset { { 20, 20, 0 }, { 20, 21, 1 } };
    EXPECT_EQ(rows(assigned.get_result()), reset);
    EXPECT_TRUE(assigned.get_delta().added.empty());
    EXPECT_TRUE(assigned.get_delta().removed.empty());
    fill(de.added, Edges { { 21, 22 } });
    assigned.update(ds.change(), de.change(), dt.change());
    expect_delta(assigned, reset, Results { { 20, 20, 0 }, { 20, 21, 1 }, { 20, 22, 2 } });
}

TEST(YggdrasilTests, DatabaseDistanceRandomBatchesAndUndoMatchIndependentPairwiseBfs)
{
    constexpr ygg::uint_t universe = 9;
    const auto toggle = [](auto& set, auto value)
    {
        if (!set.erase(value))
            set.insert(value);
    };
    for (const uint32_t seed : { 73019U, 42U, 0xdeadbeefU, 49157U })
    {
        SCOPED_TRACE(seed);
        std::mt19937 random(seed);
        RelationBuilder sources(tyr::datalog::tests::ops::cols({ ColumnIndex(10) })), edges(tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) })), targets(tyr::datalog::tests::ops::cols({ ColumnIndex(20) }));
        Vertices source_set { 0, 4 }, target_set { 2, 5, 8 };
        Edges edge_set { { 0, 1 }, { 1, 2 } };
        if (seed % 2 == 0)
            for (ygg::uint_t from = 0; from < universe; ++from)
                for (ygg::uint_t to = 0; to < universe; ++to)
                    if (random() % 3 != 0)
                        edge_set.emplace(from, to);
        fill(sources, source_set);
        fill(edges, edge_set);
        fill(targets, target_set);
        const database::DistancePlan plan(sources.columns(), edges.columns(), targets.columns(), ColumnIndex(3));
        database::DistanceWorkspace workspace(plan);
        RelationBuilder full(plan.output_columns());
        database::incremental::DistanceEvaluator evaluator(layer(), plan);
        evaluator.initialize(layer().view(sources), layer().view(edges), layer().view(targets));
        database::incremental::Delta ds(layer(), sources.columns()), de(layer(), edges.columns()), dt(layer(), targets.columns());
        for (size_t step = 0; step < 160; ++step)
        {
            SCOPED_TRACE(step);
            const auto old_sources = source_set;
            const auto old_edges = edge_set;
            const auto old_targets = target_set;
            const auto before = reference(old_sources, old_edges, old_targets, universe);
            for (size_t i = 0; i < 1 + step % 7; ++i)
            {
                if (step % 3 == 0)
                    toggle(source_set, static_cast<ygg::uint_t>(random() % universe));
                toggle(edge_set, std::pair { static_cast<ygg::uint_t>(random() % universe), static_cast<ygg::uint_t>(random() % universe) });
                if (step % 2 == 0)
                    toggle(target_set, static_cast<ygg::uint_t>(random() % universe));
            }
            changes(ds, old_sources, source_set);
            changes(de, old_edges, edge_set);
            changes(dt, old_targets, target_set);
            evaluator.update(ds.change(), de.change(), dt.change());
            const auto after = reference(source_set, edge_set, target_set, universe);
            expect_delta(evaluator, before, after);
            fill(sources, source_set);
            fill(edges, edge_set);
            fill(targets, target_set);
            database::distance(layer().view(sources), layer().view(edges), layer().view(targets), plan, full, workspace);
            EXPECT_EQ(rows(layer().view(full)), after);
            if (step % 5 == 0)
            {
                evaluator.update(std::pair { layer().view(ds.removed), layer().view(ds.added) }, std::pair { layer().view(de.removed), layer().view(de.added) }, std::pair { layer().view(dt.removed), layer().view(dt.added) });
                expect_delta(evaluator, after, before);
                source_set = old_sources;
                edge_set = old_edges;
                target_set = old_targets;
            }
        }
    }
}

TEST(YggdrasilTests, DatabaseMinDistanceAggregatesTheDistanceColumnFullAndIncrementally)
{
    constexpr auto infinity = std::numeric_limits<ygg::uint_t>::max();
    const auto expected_min = [](const Results& results)
    {
        auto best = infinity;
        for (const auto& row : results)
            best = std::min(best, row[2]);
        return best;
    };

    RelationBuilder sources(tyr::datalog::tests::ops::cols({ ColumnIndex(10) })), edges(tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) })), targets(tyr::datalog::tests::ops::cols({ ColumnIndex(20) }));
    const database::DistancePlan plan(sources.columns(), edges.columns(), targets.columns(), ColumnIndex(3));
    database::DistanceWorkspace workspace(plan);
    RelationBuilder result(plan.output_columns());
    database::incremental::SourceMinDistance evaluator(layer(), plan);
    database::incremental::Delta ds(layer(), sources.columns()), de(layer(), edges.columns()), dt(layer(), targets.columns());

    // Each state changes sources, edges, or targets; the minimum moves up, down, to zero and to infinity.
    const urd::Store<urd::sequence<std::tuple<Vertices, Edges, Vertices>>> states {
        { { 0 }, { { 0, 1 }, { 1, 2 }, { 2, 3 } }, { 3 } },            // 3
        { { 0 }, { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 0, 3 } }, { 3 } },  // shortcut: 1
        { { 0 }, { { 0, 1 }, { 1, 2 }, { 2, 3 } }, { 3 } },            // deleting the minimum reveals 3
        { { 0, 2 }, { { 0, 1 }, { 1, 2 }, { 2, 3 } }, { 3 } },         // a closer source: 1
        { { 0, 2 }, { { 0, 1 }, { 1, 2 }, { 2, 3 } }, { 3, 2 } },      // a target equal to a source: 0
        { { 0 }, { { 1, 2 }, { 2, 3 } }, { 3 } },                      // unreachable: infinity
        { {}, { { 1, 2 } }, { 2 } },                                   // no sources: infinity
    };

    Vertices before_sources, before_targets;
    Edges before_edges;
    for (size_t step = 0; step < states.size(); ++step)
    {
        const auto& [s, e, t] = states[step];
        const auto expected = expected_min(reference(s, e, t, 5));
        fill(sources, s);
        fill(edges, e);
        fill(targets, t);
        EXPECT_EQ(database::min_distance(layer().view(sources), layer().view(edges), layer().view(targets), plan, result, workspace), expected) << "full, step " << step;
        if (step == 0)
            evaluator.initialize(layer().view(sources), layer().view(edges), layer().view(targets));
        else
        {
            changes(ds, before_sources, s);
            changes(de, before_edges, e);
            changes(dt, before_targets, t);
            evaluator.update(ds.change(), de.change(), dt.change());
        }
        EXPECT_EQ(evaluator.get_result(), expected) << "incremental, step " << step;
        before_sources = s;
        before_edges = e;
        before_targets = t;
    }
}

}  // namespace tyr::datalog::tests
