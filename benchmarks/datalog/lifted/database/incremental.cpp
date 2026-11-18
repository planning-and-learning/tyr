/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "yggdrasil/formalism/relation_view.hpp"

#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

#ifndef TYR_DATALOG_DATABASE_APPEND_ONLY_PROFILE
#include "tyr/datalog/lifted/database/semantics/incremental/join.hpp"
#include "../../../../tests/unit/datalog/lifted/database/query_helpers.hpp"
#include "tyr/datalog/lifted/database/semantics/incremental/projection.hpp"
#include "tyr/datalog/lifted/database/semantics/relation_view.hpp"
#endif

#include <algorithm>
#include <array>
#include <benchmark/benchmark.h>
#include <chrono>
#include <cstddef>
#include <initializer_list>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace tyr::datalog::benchmarks
{
using tyr::datalog::tests::borrow;
using tyr::datalog::tests::Change;
using tyr::datalog::tests::Relation;
using tyr::datalog::tests::scratch;
using tyr::datalog::tests::value_of;
using tyr::datalog::tests::values_of;
namespace
{
using ColumnIndex = urd::index<ygg::Column>;

/// ygg::Columns of type ygg::uint_t with these labels.
std::vector<ygg::ColumnLayout> cols(std::initializer_list<ColumnIndex> labels)
{
    std::vector<ygg::ColumnLayout> result;
    for (const auto label : labels)
        result.emplace_back(label, ygg::type_id<ygg::uint_t>);
    return result;
}

// Define TYR_DATALOG_DATABASE_APPEND_ONLY_PROFILE to isolate the append/clear workload
// without pulling in the incremental evaluators.
void append_only(benchmark::State& state)
{
    const auto rows = static_cast<size_t>(state.range(0));
    const auto repetitions = static_cast<size_t>(state.range(1));
    Relation relation { cols({ ColumnIndex(1), ColumnIndex(2) }) };
    const auto refill = [&]
    {
        relation.clear();
        for (size_t i = 0; i < rows; ++i)
            relation.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(i / repetitions)),
                                         ygg::encode_value(static_cast<ygg::uint_t>(i / repetitions + rows)) });
    };
    refill();
    for ([[maybe_unused]] auto _ : state)
    {
        refill();
        benchmark::DoNotOptimize(std::addressof(relation));
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<benchmark::IterationCount>(rows));
    state.counters["retained_bytes"] = static_cast<double>(relation.memory_usage());
    state.counters["output_rows"] = static_cast<double>(relation.size());
}

#ifndef TYR_DATALOG_DATABASE_APPEND_ONLY_PROFILE

/// The transitions of a trace, which return it to its initial rows.
constexpr size_t num_transitions = 6;
/// The times a trace is run before it is measured.
constexpr size_t warm_up_rounds = 8;
/// The iterations of a latency case.
constexpr benchmark::IterationCount latency_iterations = 256;

struct Transition
{
    Change delta { cols({ ColumnIndex(1), ColumnIndex(2) }) };
    Relation snapshot { cols({ ColumnIndex(1), ColumnIndex(2) }) };
};

struct Trace
{
    Relation initial { cols({ ColumnIndex(1), ColumnIndex(2) }) };
    std::array<Transition, num_transitions> transitions {};
    size_t rows;
    size_t changed_rows;
    size_t witnesses;
    bool changing_keys;

    Trace(size_t rows_, size_t changed_rows_, size_t witnesses_, bool changing_keys_) :
        rows(rows_),
        changed_rows(changed_rows_),
        witnesses(witnesses_),
        changing_keys(changing_keys_)
    {
        if (rows == 0 || witnesses == 0 || changed_rows > rows / 3)
            throw std::invalid_argument("Benchmark trace requires positive rows/witnesses and three disjoint change blocks.");
        if (rows > std::numeric_limits<ygg::uint_t>::max() / 4)
            throw std::overflow_error("Benchmark trace row count exceeds the tuple value range.");
        for (size_t i = 0; i < rows; ++i)
            initial.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(i / witnesses)), ygg::encode_value(static_cast<ygg::uint_t>(i)) });
        refresh(0);
    }

    void refresh(ygg::uint_t generation)
    {
        // The largest generated value is (generation + 4) * rows - 1.
        // Check before multiplication and the conversion to ygg::uint_t below.
        if (generation > std::numeric_limits<ygg::uint_t>::max() / rows - 4)
            throw std::overflow_error("Benchmark trace generation exceeds the tuple value range.");
        urd::Store<urd::sequence<std::array<ygg::uint_t, 2>>> current;
        current.reserve(rows);
        for (size_t i = 0; i < rows; ++i)
            current.push_back({ static_cast<ygg::uint_t>(i / witnesses), static_cast<ygg::uint_t>(i) });
        size_t position = 0;
        const auto replace = [&](size_t begin, ygg::uint_t phase)
        {
            auto& step = transitions[position++];
            step.delta.clear();
            step.snapshot.clear();
            const auto offset = phase == 0 ? ygg::uint_t { 0 } : static_cast<ygg::uint_t>((generation + phase) * rows);
            for (size_t i = begin; i < begin + changed_rows; ++i)
            {
                step.delta.removed.insert(std::array { ygg::encode_value(current[i][0]), ygg::encode_value(current[i][1]) });
                current[i] = { static_cast<ygg::uint_t>(i / witnesses + (changing_keys ? offset : 0)), static_cast<ygg::uint_t>(i + offset) };
                step.delta.added.insert(std::array { ygg::encode_value(current[i][0]), ygg::encode_value(current[i][1]) });
            }
            for (const auto& row : current)
                step.snapshot.insert(std::array { ygg::encode_value(row[0]), ygg::encode_value(row[1]) });
        };
        // A -> B -> C -> B -> A -> D -> A: two levels, undo, then a sibling.
        replace(0, 1);
        replace(changed_rows, 2);
        replace(changed_rows, 0);
        replace(0, 0);
        replace(2 * changed_rows, 3);
        replace(2 * changed_rows, 0);
    }
};

struct JoinTrace
{
    Trace left;
    Trace right;
    size_t rows;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers): the positions of the transitions, 0 to 5
    std::array<size_t, num_transitions> transitions { 0, 1, 2, 3, 4, 5 };

    JoinTrace(size_t rows_, size_t changed_rows, size_t witnesses) :
        left(rows_, changed_rows, witnesses, true),
        right(rows_, changed_rows, witnesses, true),
        rows(rows_)
    {
        const auto columns = cols({ ColumnIndex(1), ColumnIndex(3) });
        right.initial.rename(columns);
        for (auto& transition : right.transitions)
        {
            transition.delta.added.rename(columns);
            transition.delta.removed.rename(columns);
            transition.snapshot.rename(columns);
        }
    }

    void refresh(ygg::uint_t generation)
    {
        left.refresh(generation);
        right.refresh(generation);
    }
};

bool equal_rows(ygg::FreeRelationBuilderView lhs, const Relation& rhs)
{
    if (lhs.size() != rhs.size())
        return false;
    for (const auto& row : lhs.get_rows())
        if (!rhs.contains(values_of(row)))
            return false;
    return true;
}

size_t changed_rows(const Relation& before, const Relation& after)
{
    size_t result = 0;
    for (const auto& row : borrow(before).get_rows())
        result += !after.contains(values_of(row));
    for (const auto& row : borrow(after).get_rows())
        result += !before.contains(values_of(row));
    return result;
}

bool equal_delta(const database::incremental::Delta& delta, const Relation& before, const Relation& after)
{
    size_t added = 0;
    size_t removed = 0;
    for (const auto& row : borrow(before).get_rows())
        if (!after.contains(values_of(row)))
        {
            ++removed;
            if (!delta.removed->view().contains(values_of(row)))
                return false;
        }
    for (const auto& row : borrow(after).get_rows())
        if (!before.contains(values_of(row)))
        {
            ++added;
            if (!delta.added->view().contains(values_of(row)))
                return false;
        }
    return delta.added->view().size() == added && delta.removed->view().size() == removed;
}

template<bool Latency, typename TraceType, typename Update, typename Result, typename Memory>
void measure(benchmark::State& state, TraceType& trace, Update update, Result result, Memory memory)
{
    // Run the entire reversible workload before measurement, including all
    // intermediate cardinalities. Every iteration then starts at the same root.
    for (size_t warmup = 0; warmup < warm_up_rounds; ++warmup)
        for (const auto& transition : trace.transitions)
            update(transition);

    auto maximum = std::chrono::steady_clock::duration::zero();
    ygg::uint_t generation = 3;
    for (auto _ : state)
    {
        if constexpr (Latency)
        {
            // Novel tuple values expose eventual hash-table churn. Corpus work
            // is excluded equally for full snapshots and incremental batches.
            state.PauseTiming();
            trace.refresh(generation);
            generation += 3;
            if (generation >= std::numeric_limits<ygg::uint_t>::max() / trace.rows - 4)
                generation = 3;
            state.ResumeTiming();
        }
        auto elapsed = std::chrono::steady_clock::duration::zero();
        for (const auto& transition : trace.transitions)
        {
            if constexpr (Latency)
            {
                const auto begin = std::chrono::steady_clock::now();
                update(transition);
                benchmark::DoNotOptimize(result().size());
                benchmark::ClobberMemory();
                const auto duration = std::chrono::steady_clock::now() - begin;
                maximum = std::max(maximum, duration);
                elapsed += duration;
            }
            else
            {
                update(transition);
                benchmark::DoNotOptimize(result().size());
                benchmark::ClobberMemory();
            }
        }
        if constexpr (Latency)
            state.SetIterationTime(std::chrono::duration<double>(elapsed).count());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<benchmark::IterationCount>(trace.transitions.size()));
    state.counters["retained_bytes"] = static_cast<double>(memory());
    state.counters["output_rows"] = static_cast<double>(result().size());
    state.counters["input_delta_rows"] = static_cast<double>(2 * state.range(1));
    state.counters["seconds_per_update"] =
        benchmark::Counter(static_cast<double>(trace.transitions.size()), benchmark::Counter::kIsIterationInvariantRate | benchmark::Counter::kInvert);
    if constexpr (Latency)
        state.counters["max_update_ns"] = std::chrono::duration<double, std::nano>(maximum).count();
}

template<bool Incremental, bool Latency, bool ReplaceWitnesses>
void projection(benchmark::State& state)
{
    // Witness replacements change payloads while preserving projected keys.
    Trace trace(static_cast<size_t>(state.range(0)), static_cast<size_t>(state.range(1)), static_cast<size_t>(state.range(2)), !ReplaceWitnesses);
    const database::ProjectionPlan plan(trace.initial.columns(), std::array { ColumnIndex(1) });
    database::Workspace workspace;
    auto evaluation = [&]
    {
        if constexpr (Incremental)
            return database::incremental::ProjectionEvaluator(scratch(), plan);
        else
            return Relation(plan.output_columns());
    }();
    if constexpr (Incremental)
        evaluation.initialize(borrow(trace.initial), workspace);
    const auto update = [&](const Transition& transition)
    {
        if constexpr (Incremental)
            evaluation.update(transition.delta.change(), workspace);
        else
            database::project(borrow(transition.snapshot), plan, evaluation, workspace);
    };
    const auto result = [&]() -> ygg::FreeRelationBuilderView
    {
        if constexpr (Incremental)
            return evaluation.get_result();
        else
            return borrow(evaluation);
    };
    // Validate every transition outside timing, not just the restored root.
    Relation reference(plan.output_columns());
    Relation previous(plan.output_columns());
    database::Workspace reference_workspace;
    database::project(borrow(trace.initial), plan, previous, reference_workspace);
    size_t output_delta_rows = 0;
    for (const auto& transition : trace.transitions)
    {
        update(transition);
        database::project(borrow(transition.snapshot), plan, reference, reference_workspace);
        if (!equal_rows(result(), reference))
        {
            state.SkipWithError("projection trace disagrees with full recomputation");
            return;
        }
        if constexpr (Incremental)
            if (!equal_delta(evaluation.get_delta(), previous, reference))
            {
                state.SkipWithError("projection delta disagrees with before/after set difference");
                return;
            }
        output_delta_rows += changed_rows(previous, reference);
        std::swap(previous, reference);
    }
    state.counters["mean_output_delta_rows"] = static_cast<double>(output_delta_rows) / trace.transitions.size();
    measure<Latency>(state, trace, update, result, [&] { return evaluation.memory_usage() + workspace.memory_usage(); });
}

template<bool Incremental, bool Latency>
void fixed_input_join(benchmark::State& state)
{
    const auto rows = static_cast<size_t>(state.range(0));
    const auto fanout = static_cast<size_t>(state.range(2));
    Trace trace(rows, static_cast<size_t>(state.range(1)), 1, false);
    auto fixed = std::optional<Relation>(std::in_place, cols({ ColumnIndex(1), ColumnIndex(3) }));
    for (size_t key = 0; key < rows; ++key)
        for (size_t match = 0; match < fanout; ++match)
            fixed->insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(key)), ygg::encode_value(static_cast<ygg::uint_t>(match)) });
    const database::JoinPlan plan(trace.initial.columns(), fixed->columns());
    const Relation unchanged(fixed->columns());
    database::Workspace workspace;
    auto evaluation = [&]
    {
        if constexpr (Incremental)
            return database::incremental::JoinEvaluator(scratch(), plan);
        else
            return Relation(plan.output_columns());
    }();
    if constexpr (Incremental)
        evaluation.initialize(borrow(trace.initial), borrow(*fixed), workspace);
    const auto update = [&](const Transition& transition)
    {
        if constexpr (Incremental)
            evaluation.update(transition.delta.change(), std::pair { borrow(unchanged), borrow(unchanged) }, workspace);
        else
            database::join(borrow(transition.snapshot), borrow(*fixed), plan, evaluation, workspace);
    };
    const auto result = [&]() -> ygg::FreeRelationBuilderView
    {
        if constexpr (Incremental)
            return evaluation.get_result();
        else
            return borrow(evaluation);
    };
    Relation reference(plan.output_columns());
    Relation previous(plan.output_columns());
    database::Workspace reference_workspace;
    database::join(borrow(trace.initial), borrow(*fixed), plan, previous, reference_workspace);
    size_t output_delta_rows = 0;
    for (const auto& transition : trace.transitions)
    {
        update(transition);
        database::join(borrow(transition.snapshot), borrow(*fixed), plan, reference, reference_workspace);
        if (!equal_rows(result(), reference))
        {
            state.SkipWithError("fixed-input join trace disagrees with full recomputation");
            return;
        }
        if constexpr (Incremental)
            if (!equal_delta(evaluation.get_delta(), previous, reference))
            {
                state.SkipWithError("fixed-input-join delta disagrees with before/after set difference");
                return;
            }
        output_delta_rows += changed_rows(previous, reference);
        std::swap(previous, reference);
    }
    state.counters["mean_output_delta_rows"] = static_cast<double>(output_delta_rows) / trace.transitions.size();
    measure<Latency>(state,
                     trace,
                     update,
                     result,
                     [&] { return evaluation.memory_usage() + workspace.memory_usage(); });  // ponytail: excludes the join cache, which reports no memory
}

template<bool Incremental, bool Latency>
void changing_join(benchmark::State& state)
{
    JoinTrace trace(static_cast<size_t>(state.range(0)), static_cast<size_t>(state.range(1)), static_cast<size_t>(state.range(2)));
    const database::JoinPlan plan(trace.left.initial.columns(), trace.right.initial.columns());
    database::Workspace workspace;
    auto evaluation = [&]
    {
        if constexpr (Incremental)
            return database::incremental::JoinEvaluator(scratch(), plan);
        else
            return Relation(plan.output_columns());
    }();
    if constexpr (Incremental)
        evaluation.initialize(borrow(trace.left.initial), borrow(trace.right.initial), workspace);
    const auto update = [&](size_t position)
    {
        const auto& left = trace.left.transitions[position];
        const auto& right = trace.right.transitions[position];
        if constexpr (Incremental)
            evaluation.update(left.delta.change(), right.delta.change(), workspace);
        else
            // Both inputs change, so full recomputation rebuilds its transient
            // hash index in retained storage using the already prepared plan.
            database::join(borrow(left.snapshot), borrow(right.snapshot), plan, evaluation, workspace);
    };
    const auto result = [&]() -> ygg::FreeRelationBuilderView
    {
        if constexpr (Incremental)
            return evaluation.get_result();
        else
            return borrow(evaluation);
    };
    Relation reference(plan.output_columns());
    Relation previous(plan.output_columns());
    database::Workspace reference_workspace;
    database::join(borrow(trace.left.initial), borrow(trace.right.initial), plan, previous, reference_workspace);
    size_t output_delta_rows = 0;
    for (const auto position : trace.transitions)
    {
        update(position);
        database::join(borrow(trace.left.transitions[position].snapshot),
                       borrow(trace.right.transitions[position].snapshot),
                       plan,
                       reference,
                       reference_workspace);
        if (!equal_rows(result(), reference))
        {
            state.SkipWithError("changing join trace disagrees with full recomputation");
            return;
        }
        if constexpr (Incremental)
            if (!equal_delta(evaluation.get_delta(), previous, reference))
            {
                state.SkipWithError("changing join delta disagrees with before/after set difference");
                return;
            }
        output_delta_rows += changed_rows(previous, reference);
        std::swap(previous, reference);
    }
    state.counters["mean_output_delta_rows"] = static_cast<double>(output_delta_rows) / trace.transitions.size();
    measure<Latency>(state,
                     trace,
                     update,
                     result,
                     [&] { return evaluation.memory_usage() + workspace.memory_usage() + urd::memory_usage(workspace.join_index); });
    // Each side removes and adds range(1) rows in the same overlapping key groups.
    state.counters["input_delta_rows"] = static_cast<double>(4 * state.range(1));
}

template<bool Latency>
void register_matrix(const std::string& suffix)
{
    const auto configure = [](benchmark::Benchmark* benchmark)
    {
        benchmark->ArgNames({ "rows", "delta", "multiplicity" })->Unit(benchmark::kMicrosecond);
        for (const auto rows : { 1024, 16384 })
            for (const auto delta : { 1, 16, 256 })
                for (const auto multiplicity : { 1, 8 })
                    benchmark->Args({ rows, delta, multiplicity });
        if constexpr (Latency)
            // Bound wall time: refreshing snapshots is deliberately untimed.
            benchmark->UseManualTime()->Iterations(latency_iterations);
    };
    configure(benchmark::RegisterBenchmark("database/projection/full" + suffix, projection<false, Latency, false>));
    configure(benchmark::RegisterBenchmark("database/projection/incremental" + suffix, projection<true, Latency, false>));
    configure(benchmark::RegisterBenchmark("database/projection_witness_replacement/full" + suffix, projection<false, Latency, true>));
    configure(benchmark::RegisterBenchmark("database/projection_witness_replacement/incremental" + suffix, projection<true, Latency, true>));
    configure(benchmark::RegisterBenchmark("database/fixed_input_join/full" + suffix, fixed_input_join<false, Latency>));
    configure(benchmark::RegisterBenchmark("database/fixed_input_join/incremental" + suffix, fixed_input_join<true, Latency>));
    configure(benchmark::RegisterBenchmark("database/changing_join/full" + suffix, changing_join<false, Latency>));
    configure(benchmark::RegisterBenchmark("database/changing_join/incremental" + suffix, changing_join<true, Latency>));
}
#endif

// NOLINTBEGIN(bugprone-throwing-static-initialization, cppcoreguidelines-avoid-magic-numbers): benchmarks register themselves here, with their sizes
[[maybe_unused]] const auto registered = []
{
    benchmark::RegisterBenchmark("database/relation/append", append_only)
        ->ArgNames({ "rows", "repetitions" })
        ->Args({ 1024, 1 })
        ->Args({ 1024, 8 })
        ->Args({ 16384, 1 })
        ->Args({ 16384, 8 });
#ifndef TYR_DATALOG_DATABASE_APPEND_ONLY_PROFILE
    register_matrix<false>("");
    // Clock calls live only in these explicit latency cases. The maximum is a
    // diagnostic of occasional slow updates; it is not a stable CI threshold.
    register_matrix<true>("/latency");
#endif
    return true;
}();
// NOLINTEND(bugprone-throwing-static-initialization, cppcoreguidelines-avoid-magic-numbers)
}  // namespace
}  // namespace tyr::datalog::benchmarks
