/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/semantics/distance.hpp"

#include "tyr/datalog/lifted/database/semantics/incremental/distance.hpp"

#include <benchmark/benchmark.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <tuple>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>

namespace tyr::datalog::benchmarks
{
namespace
{
using Values = boost::mp11::mp_list<ygg::uint_t>;
using ColumnIndex = urd::index<ygg::Column>;
using RelationBuilder = urd::builder<ygg::Relation<ygg::Schema>>;

/// The first labels of the source and the target columns; the edge columns are labeled from 0.
constexpr ygg::uint_t first_source_label = 1000;
constexpr ygg::uint_t first_target_label = 2000;
/// What the fields of one vertex differ by.
constexpr ygg::uint_t field_offset = 100000;
/// How far the second edge of a vertex reaches, as it is and changed.
constexpr ygg::uint_t shortcut = 8;
constexpr ygg::uint_t changed_shortcut = 9;
/// The evaluations run before the measured ones.
constexpr size_t warm_up_rounds = 8;

std::vector<ygg::ColumnLayout> columns(size_t arity, ygg::uint_t first_label)
{
    std::vector<ygg::ColumnLayout> result;
    result.reserve(arity);
    for (size_t i = 0; i < arity; ++i)
        result.push_back({ ColumnIndex(first_label + static_cast<ygg::uint_t>(i)), ygg::type_id<ygg::uint_t> });
    return result;
}

/// The layer the benchmarks read their builders through.
const ygg::FreeRelationRepository& layer()
{
    static const ygg::FreeRelationRepository repository;
    return repository;
}

ygg::FreeBuilderRelationView borrow(const RelationBuilder& relation) { return layer().view(relation); }

/// The distances of the sources and targets over `edges`, fully evaluated.
RelationBuilder full_distance(const RelationBuilder& sources, const RelationBuilder& edges, const RelationBuilder& targets, const database::DistancePlan& plan)
{
    RelationBuilder out(plan.output_columns());
    database::DistanceWorkspace workspace(plan);
    database::distance(borrow(sources), borrow(edges), borrow(targets), plan, out, workspace);
    return out;
}

void append_vertex(urd::Store<urd::sequence<ygg::Value>>& values, size_t arity, ygg::uint_t vertex)
{
    for (size_t field = 0; field < arity; ++field)
        values.push_back(ygg::encode_value(vertex + field_offset * static_cast<ygg::uint_t>(field)));
}

void insert_vertex(RelationBuilder& relation, size_t arity, ygg::uint_t vertex)
{
    urd::Store<urd::sequence<ygg::Value>> values;
    append_vertex(values, arity, vertex);
    relation.insert(std::span<const ygg::Value>(values));
}

void insert_edge(RelationBuilder& relation, size_t arity, ygg::uint_t from, ygg::uint_t to)
{
    urd::Store<urd::sequence<ygg::Value>> values;
    append_vertex(values, arity, from);
    append_vertex(values, arity, to);
    relation.insert(std::span<const ygg::Value>(values));
}

enum class TraceKind : std::uint8_t
{
    shortcuts,
    self_loops,
    witness_tail,
    dense_ties,
};

struct Trace
{
    RelationBuilder sources;
    RelationBuilder edges;
    RelationBuilder changed_edges;
    RelationBuilder targets;
    database::incremental::Delta edge_change;
    RelationBuilder no_sources;
    RelationBuilder no_targets;

    Trace(size_t arity, ygg::uint_t vertices, ygg::uint_t source_count, ygg::uint_t changed, TraceKind kind = TraceKind::shortcuts) :
        sources(columns(arity, first_source_label)),
        edges(columns(2 * arity, 0)),
        changed_edges(edges.columns()),
        targets(columns(arity, first_target_label)),
        edge_change(layer(), edges.columns()),
        no_sources(sources.columns()),
        no_targets(targets.columns())
    {
        const auto common_edge = [&](ygg::uint_t from, ygg::uint_t to)
        {
            insert_edge(edges, arity, from, to);
            insert_edge(changed_edges, arity, from, to);
        };
        for (ygg::uint_t source = 0; source < source_count; ++source)
            insert_vertex(sources, arity, source);
        if (kind == TraceKind::witness_tail)
        {
            const auto old_witness = source_count;
            const auto new_witness = source_count + 1;
            const auto first_tail = source_count + 2;
            for (ygg::uint_t source = 0; source < source_count; ++source)
            {
                common_edge(source, old_witness);
                common_edge(source, new_witness);
            }
            insert_edge(edges, arity, old_witness, first_tail);
            insert_edge(changed_edges, arity, new_witness, first_tail);
            insert_edge(edge_change.removed, arity, old_witness, first_tail);
            insert_edge(edge_change.added, arity, new_witness, first_tail);
            for (ygg::uint_t vertex = first_tail; vertex < vertices; ++vertex)
            {
                if (vertex + 1 < vertices)
                    common_edge(vertex, vertex + 1);
                insert_vertex(targets, arity, vertex);
            }
            return;
        }
        if (kind == TraceKind::dense_ties)
        {
            const auto width = vertices;
            const auto first_parent = source_count;
            const auto moving = first_parent + width;
            const auto first_neighbor = moving + 1;
            for (ygg::uint_t source = 0; source < source_count; ++source)
                for (ygg::uint_t parent = first_parent; parent < moving; ++parent)
                    common_edge(source, parent);
            common_edge(first_parent, moving);
            for (ygg::uint_t neighbor = first_neighbor; neighbor < first_neighbor + width; ++neighbor)
            {
                common_edge(moving, neighbor);
                for (ygg::uint_t parent = first_parent; parent < moving; ++parent)
                    common_edge(parent, neighbor);
                insert_vertex(targets, arity, neighbor);
            }
            // Only this vertex changes distance; its dense neighbors gain/lose one tied witness.
            insert_vertex(targets, arity, moving);
            insert_edge(changed_edges, arity, 0, moving);
            insert_edge(edge_change.added, arity, 0, moving);
            return;
        }
        const bool harmless = kind == TraceKind::self_loops;
        for (ygg::uint_t vertex = 0; vertex < vertices; ++vertex)
        {
            insert_edge(edges, arity, vertex, (vertex + 1) % vertices);
            insert_edge(edges, arity, vertex, (vertex + shortcut) % vertices);
            insert_edge(changed_edges, arity, vertex, (vertex + 1) % vertices);
            insert_edge(changed_edges, arity, vertex, (vertex + (!harmless && vertex < changed ? changed_shortcut : shortcut)) % vertices);
            if (vertex % 4 == 0)
                insert_vertex(targets, arity, vertex);
        }
        for (ygg::uint_t vertex = 0; vertex < changed; ++vertex)
        {
            if (harmless)
            {
                insert_edge(edge_change.added, arity, vertex, vertex);
                insert_edge(changed_edges, arity, vertex, vertex);
            }
            else
            {
                insert_edge(edge_change.removed, arity, vertex, (vertex + shortcut) % vertices);
                insert_edge(edge_change.added, arity, vertex, (vertex + changed_shortcut) % vertices);
            }
        }
    }
};

bool equal_rows(ygg::FreeBuilderRelationView actual, const RelationBuilder& expected)
{
    if (actual.size() != expected.size())
        return false;
    for (const auto row : borrow(expected).rows())
        if (!actual.contains(row))
            return false;
    return true;
}

bool equal_delta(const database::incremental::Delta& actual, const RelationBuilder& before, const RelationBuilder& after)
{
    size_t added = 0, removed = 0;
    for (const auto row : borrow(after).rows())
        if (!before.contains(row))
        {
            ++added;
            if (!actual.added.contains(row))
                return false;
        }
    for (const auto row : borrow(before).rows())
        if (!after.contains(row))
        {
            ++removed;
            if (!actual.removed.contains(row))
                return false;
        }
    return actual.added.size() == added && actual.removed.size() == removed;
}

template<bool Incremental, TraceKind Kind = TraceKind::shortcuts>
void distance_changes(benchmark::State& state)
{
    const auto arity = static_cast<size_t>(state.range(0));
    const auto vertices = static_cast<ygg::uint_t>(state.range(1));
    const auto source_count = static_cast<ygg::uint_t>(state.range(2));
    const auto changed = static_cast<ygg::uint_t>(state.range(3));
    Trace trace(arity, vertices, source_count, changed, Kind);
    const database::DistancePlan plan(trace.sources.columns(), trace.edges.columns(), trace.targets.columns(), ColumnIndex(5000));
    database::DistanceWorkspace workspace(plan);
    auto evaluation = [&]
    {
        if constexpr (Incremental)
            return database::incremental::DistanceEvaluator(layer(), plan);
        else
            return RelationBuilder(plan.output_columns());
    }();
    if constexpr (Incremental)
        evaluation.initialize(borrow(trace.sources), borrow(trace.edges), borrow(trace.targets));
    const auto update = [&](bool forward)
    {
        if constexpr (Incremental)
            evaluation.update(std::pair { borrow(trace.no_sources), borrow(trace.no_sources) },
                              std::pair { borrow(forward ? trace.edge_change.added : trace.edge_change.removed),
                                          borrow(forward ? trace.edge_change.removed : trace.edge_change.added) },
                              std::pair { borrow(trace.no_targets), borrow(trace.no_targets) });
        else
            database::distance(borrow(trace.sources), borrow(forward ? trace.changed_edges : trace.edges), borrow(trace.targets), plan, evaluation, workspace);
    };
    const auto result = [&]() -> ygg::FreeBuilderRelationView
    {
        if constexpr (Incremental)
            return evaluation.get_result();
        else
            return borrow(evaluation);
    };

    // Preparation, exact result/delta checks, and warmup are outside the timed loop.
    auto before = full_distance(trace.sources, trace.edges, trace.targets, plan);
    auto after = full_distance(trace.sources, trace.changed_edges, trace.targets, plan);
    update(true);
    if (!equal_rows(result(), after))
    {
        state.SkipWithError("distance forward result disagrees with full evaluation");
        return;
    }
    if constexpr (Incremental)
        if (!equal_delta(evaluation.get_delta(), before, after))
        {
            state.SkipWithError("distance forward delta disagrees with set difference");
            return;
        }
    update(false);
    if (!equal_rows(result(), before))
    {
        state.SkipWithError("distance undo result disagrees with full evaluation");
        return;
    }
    if constexpr (Incremental)
        if (!equal_delta(evaluation.get_delta(), after, before))
        {
            state.SkipWithError("distance undo delta disagrees with set difference");
            return;
        }
    for (size_t i = 0; i < warm_up_rounds; ++i)
    {
        update(true);
        update(false);
    }
    for (auto _ : state)
    {
        update(true);
        benchmark::DoNotOptimize(result().relation_identity());
        benchmark::ClobberMemory();
        update(false);
        benchmark::DoNotOptimize(result().relation_identity());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * 2);
    state.counters["output_rows"] = static_cast<double>(result().size());
    state.counters["input_delta_rows"] = static_cast<double>(trace.edge_change.added.size() + trace.edge_change.removed.size());
    state.counters["updates_per_iteration"] = 2;
    state.counters["graph_edges"] = static_cast<double>(trace.edges.size());
    if constexpr (Incremental)
        state.counters["retained_bytes"] = static_cast<double>(evaluation.memory_usage());
    else
        state.counters["retained_bytes"] = static_cast<double>(evaluation.memory_usage() + workspace.memory_usage());
}

template<bool Incremental>
void distance_initialize_warm(benchmark::State& state)
{
    Trace trace(static_cast<size_t>(state.range(0)), static_cast<ygg::uint_t>(state.range(1)), static_cast<ygg::uint_t>(state.range(2)), 0);
    const database::DistancePlan plan(trace.sources.columns(), trace.edges.columns(), trace.targets.columns(), ColumnIndex(5000));
    database::DistanceWorkspace workspace(plan);
    auto evaluation = [&]
    {
        if constexpr (Incremental)
            return database::incremental::DistanceEvaluator(layer(), plan);
        else
            return RelationBuilder(plan.output_columns());
    }();
    const auto initialize = [&]
    {
        if constexpr (Incremental)
            evaluation.initialize(borrow(trace.sources), borrow(trace.edges), borrow(trace.targets));
        else
            database::distance(borrow(trace.sources), borrow(trace.edges), borrow(trace.targets), plan, evaluation, workspace);
    };
    const auto result = [&]() -> ygg::FreeBuilderRelationView
    {
        if constexpr (Incremental)
            return evaluation.get_result();
        else
            return borrow(evaluation);
    };
    auto reference = full_distance(trace.sources, trace.edges, trace.targets, plan);
    for (size_t i = 0; i < warm_up_rounds; ++i)
        initialize();
    if (!equal_rows(result(), reference))
    {
        state.SkipWithError("distance initialization disagrees with full evaluation");
        return;
    }
    for (auto _ : state)
    {
        initialize();
        benchmark::DoNotOptimize(result().relation_identity());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
    state.counters["initializations_per_iteration"] = 1;
    state.counters["output_rows"] = static_cast<double>(result().size());
    state.counters["graph_edges"] = static_cast<double>(trace.edges.size());
    if constexpr (Incremental)
        state.counters["retained_bytes"] = static_cast<double>(evaluation.memory_usage());
    else
        state.counters["retained_bytes"] = static_cast<double>(evaluation.memory_usage() + workspace.memory_usage());
}

// NOLINTBEGIN(bugprone-throwing-static-initialization, cppcoreguidelines-avoid-magic-numbers): benchmarks register themselves here, with their sizes
[[maybe_unused]] const auto registered = []
{
    const auto configure = [](benchmark::Benchmark* benchmark)
    {
        benchmark->ArgNames({ "arity", "vertices", "sources", "delta" })->Unit(benchmark::kMicrosecond);
        for (const auto arity : { 1, 2, 4, 8 })
        {
            benchmark->Args({ arity, 64, 1, 1 });
            benchmark->Args({ arity, 256, 4, 16 });
            benchmark->Args({ arity, 256, 16, 1 });
        }
    };
    configure(benchmark::RegisterBenchmark("database/distance/full", distance_changes<false>));
    configure(benchmark::RegisterBenchmark("database/distance/incremental", distance_changes<true>));
    const auto harmless = [](benchmark::Benchmark* benchmark)
    { benchmark->ArgNames({ "arity", "vertices", "sources", "delta" })->Unit(benchmark::kMicrosecond)->Args({ 1, 256, 4, 16 })->Args({ 8, 256, 4, 16 }); };
    harmless(benchmark::RegisterBenchmark("database/distance/unchanged/full", distance_changes<false, TraceKind::self_loops>));
    harmless(benchmark::RegisterBenchmark("database/distance/unchanged/incremental", distance_changes<true, TraceKind::self_loops>));
    const auto witness_tail = [](benchmark::Benchmark* benchmark)
    {
        benchmark->ArgNames({ "arity", "vertices", "sources", "delta" })->Unit(benchmark::kMicrosecond);
        for (const auto vertices : { 64, 256, 1024, 4096 })
            benchmark->Args({ 1, vertices, 1, 1 });
    };
    witness_tail(benchmark::RegisterBenchmark("database/distance/witness_tail/full", distance_changes<false, TraceKind::witness_tail>));
    witness_tail(benchmark::RegisterBenchmark("database/distance/witness_tail/incremental", distance_changes<true, TraceKind::witness_tail>));
    const auto dense_ties = [](benchmark::Benchmark* benchmark)
    {
        benchmark->ArgNames({ "arity", "neighbors", "sources", "delta" })->Unit(benchmark::kMicrosecond);
        for (const auto neighbors : { 16, 64, 256 })
            benchmark->Args({ 1, neighbors, 1, 1 });
    };
    dense_ties(benchmark::RegisterBenchmark("database/distance/dense_ties/full", distance_changes<false, TraceKind::dense_ties>));
    dense_ties(benchmark::RegisterBenchmark("database/distance/dense_ties/incremental", distance_changes<true, TraceKind::dense_ties>));
    const auto initialization = [](benchmark::Benchmark* benchmark)
    { benchmark->ArgNames({ "arity", "vertices", "sources" })->Unit(benchmark::kMicrosecond)->Args({ 1, 64, 1 })->Args({ 8, 256, 16 }); };
    initialization(benchmark::RegisterBenchmark("database/distance/initialize_warm/full", distance_initialize_warm<false>));
    initialization(benchmark::RegisterBenchmark("database/distance/initialize_warm/incremental", distance_initialize_warm<true>));
    return true;
}();
// NOLINTEND(bugprone-throwing-static-initialization, cppcoreguidelines-avoid-magic-numbers)
}  // namespace
}  // namespace tyr::datalog::benchmarks
