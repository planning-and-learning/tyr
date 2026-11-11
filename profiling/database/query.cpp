/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "tyr/datalog/lifted/database/semantics/incremental/query.hpp"

#include "tyr/datalog/lifted/database/optimization/optimization.hpp"
#include "tyr/datalog/lifted/database/semantics/generic_join.hpp"
#include "tyr/datalog/lifted/database/semantics/query_evaluation.hpp"

#include <array>
#include <benchmark/benchmark.h>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <tuple>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace ygg::profiling
{
namespace
{
using Relation = urd::builder<ygg::Relation<ygg::Schema>>;
using Column = urd::index<ygg::Column>;

/// The layer the benchmarks read their builders through.
const ygg::FreeRelationRepository& layer()
{
    static const ygg::FreeRelationRepository repository;
    return repository;
}

ygg::FreeBuilderRelationView borrow(const Relation& relation) { return layer().view(relation); }

/// ygg::Columns of type ygg::uint_t with these labels.
std::vector<ygg::ColumnLayout> cols(std::initializer_list<Column> labels)
{
    std::vector<ygg::ColumnLayout> result;
    for (const auto label : labels)
        result.push_back({ label, ygg::type_id<ygg::uint_t> });
    return result;
}

/// The relations as views.
std::array<ygg::FreeBuilderRelationView, 3> borrow_all(const std::array<Relation, 3>& relations)
{ return { borrow(relations[0]), borrow(relations[1]), borrow(relations[2]) }; }

struct Triangle
{
    std::array<Relation, 3> initial { Relation(cols({ Column(0), Column(1) })),
                                      Relation(cols({ Column(1), Column(2) })),
                                      Relation(cols({ Column(2), Column(0) })) };
    std::array<Relation, 3> changed { Relation(cols({ Column(0), Column(1) })),
                                      Relation(cols({ Column(1), Column(2) })),
                                      Relation(cols({ Column(2), Column(0) })) };
    std::array<Relation, 3> added { Relation(cols({ Column(0), Column(1) })),
                                    Relation(cols({ Column(1), Column(2) })),
                                    Relation(cols({ Column(2), Column(0) })) };
    std::array<Relation, 3> removed { Relation(cols({ Column(0), Column(1) })),
                                      Relation(cols({ Column(1), Column(2) })),
                                      Relation(cols({ Column(2), Column(0) })) };
    database::QueryRepository repository;
    database::QueryBuilder builder;
    database::QueryView<> root;

    explicit Triangle(ygg::uint_t count) : root(make_query(count)) {}

    database::QueryView<> make_query(ygg::uint_t count)
    {
        for (size_t input = 0; input < 3; ++input)
        {
            for (ygg::uint_t i = 0; i < count; ++i)
                initial[input].insert(std::array { ygg::encode_value(i), ygg::encode_value(i) });
            for (ygg::uint_t i = 1; i <= count; ++i)
                changed[input].insert(std::array { ygg::encode_value(i), ygg::encode_value(i) });
            added[input].insert(std::array { ygg::encode_value(count), ygg::encode_value(count) });
            removed[input].insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(static_cast<ygg::uint_t>(0)) });
        }
        return join(join(input(0), input(1)), input(2));
    }

    database::QueryView<> input(size_t slot)
    {
        auto data = database::checkout<database::Query<database::QueryInputTag>>(builder);
        data->input_slot = slot;
        const auto columns = initial[slot].columns();
        data->columns.set(columns.begin(), columns.end());
        return database::insert_query(repository, builder, *data);
    }

    database::QueryView<> join(database::QueryView<> lhs, database::QueryView<> rhs)
    {
        auto data = database::checkout<database::Query<database::QueryJoinTag>>(builder);
        data->lhs = lhs.index();
        data->rhs = rhs.index();
        return database::insert_query(repository, builder, *data);
    }

    auto deltas(bool forward) const
    {
        using Change = std::pair<ygg::FreeBuilderRelationView, ygg::FreeBuilderRelationView>;
        const auto& a = forward ? added : removed;
        const auto& r = forward ? removed : added;
        return std::array { Change { borrow(a[0]), borrow(r[0]) }, Change { borrow(a[1]), borrow(r[1]) }, Change { borrow(a[2]), borrow(r[2]) } };
    }

    database::Statistics statistics() const { return database::collect_statistics(borrow_all(initial)); }
};

bool equal(ygg::FreeBuilderRelationView left, ygg::FreeBuilderRelationView right)
{
    if (left.size() != right.size() || !std::ranges::equal(left.columns(), right.columns()))
        return false;
    for (const auto row : left.rows())
        if (!right.contains(row))
            return false;
    return true;
}

bool exact_delta(const database::incremental::Delta& delta, const Relation& before, const Relation& after)
{
    size_t added = 0, removed = 0;
    for (const auto row : borrow(after).rows())
        if (!before.contains(row))
        {
            ++added;
            if (!delta.added.contains(row))
                return false;
        }
    for (const auto row : borrow(before).rows())
        if (!after.contains(row))
        {
            ++removed;
            if (!delta.removed.contains(row))
                return false;
        }
    return added == delta.added.size() && removed == delta.removed.size();
}

enum class Planning : std::uint8_t
{
    original,
    structural,
    measured
};

database::QueryPlan plan(const Triangle& trace, Planning planning, const database::Statistics& statistics)
{
    switch (planning)
    {
        case Planning::original:
            return database::compile({ trace.root });
        case Planning::structural:
            return database::optimize(trace.root);
        case Planning::measured:
            return database::optimize(trace.root, statistics);
    }
    return {};
}

template<Planning Mode>
void optimize_query(benchmark::State& state)
{
    Triangle trace(static_cast<ygg::uint_t>(state.range(0)));
    const auto statistics = trace.statistics();
    for (auto _ : state)
        benchmark::DoNotOptimize(plan(trace, Mode, statistics).node_count());
    state.counters["input_rows"] = static_cast<double>(trace.initial[0].size() * 3);
}

template<bool Incremental, Planning Mode>
void query_updates(benchmark::State& state)
{
    Triangle trace(static_cast<ygg::uint_t>(state.range(0)));
    const auto plan = ::ygg::profiling::plan(trace, Mode, trace.statistics());
    const auto initial = borrow_all(trace.initial);
    const auto changed = borrow_all(trace.changed);
    const auto forward = trace.deltas(true), backward = trace.deltas(false);
    database::QueryEvaluator reference(layer(), database::compile({ trace.root }));
    reference.evaluate(initial);
    Relation before(reference.get_result().columns());
    database::assign(before, reference.get_result());
    reference.evaluate(changed);
    Relation after(reference.get_result().columns());
    database::assign(after, reference.get_result());
    auto evaluator = [&]
    {
        if constexpr (Incremental)
            return database::incremental::QueryEvaluator(layer(), plan);
        else
            return database::QueryEvaluator(layer(), plan);
    }();
    if constexpr (Incremental)
        evaluator.initialize(initial);
    const auto update = [&](bool next)
    {
        if constexpr (Incremental)
            evaluator.update(next ? forward : backward);
        else
            evaluator.evaluate(next ? changed : initial);
    };
    update(true);
    if (!equal(evaluator.get_result(), borrow(after)))
    {
        state.SkipWithError("query forward result mismatch");
        return;
    }
    if constexpr (Incremental)
        if (!exact_delta(evaluator.get_delta(), before, after))
        {
            state.SkipWithError("query forward delta mismatch");
            return;
        }
    update(false);
    if (!equal(evaluator.get_result(), borrow(before)))
    {
        state.SkipWithError("query undo result mismatch");
        return;
    }
    if constexpr (Incremental)
        if (!exact_delta(evaluator.get_delta(), after, before))
        {
            state.SkipWithError("query undo delta mismatch");
            return;
        }
    for (auto _ : state)
    {
        update(true);
        benchmark::DoNotOptimize(evaluator.get_result().relation_identity());
        benchmark::ClobberMemory();
        update(false);
        benchmark::DoNotOptimize(evaluator.get_result().relation_identity());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * 2);
    state.counters["retained_bytes"] = static_cast<double>(evaluator.memory_usage());
    state.counters["output_rows"] = static_cast<double>(evaluator.get_result().size());
    state.counters["delta_rows"] = 6;
}

template<bool Cold>
void generic_join_full(benchmark::State& state)
{
    Triangle trace(static_cast<ygg::uint_t>(state.range(0)));
    const auto input = borrow_all(trace.initial);
    urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::ColumnLayout>>>> schemas;
    schemas.reserve(trace.initial.size());
    for (const auto& value : trace.initial)
        schemas.emplace_back(value.columns().begin(), value.columns().end());
    const std::array order { Column(0), Column(1), Column(2) };
    const database::GenericJoinPlan plan(schemas, order, order);
    database::GenericJoinWorkspace workspace(plan);
    Relation output(plan.output_columns());
    database::generic_join(input, plan, output, workspace);
    database::QueryEvaluator reference(layer(), database::compile({ trace.root }));
    reference.evaluate(input);
    if (!equal(borrow(output), reference.get_result()))
    {
        state.SkipWithError("generic join disagrees with binary join");
        return;
    }
    for (auto _ : state)
    {
        if constexpr (Cold)
        {
            database::GenericJoinWorkspace temporary_workspace(plan);
            Relation temporary_output(plan.output_columns());
            database::generic_join(input, plan, temporary_output, temporary_workspace);
            benchmark::DoNotOptimize(std::addressof(temporary_output));
        }
        else
        {
            database::generic_join(input, plan, output, workspace);
            benchmark::DoNotOptimize(std::addressof(output));
        }
        benchmark::ClobberMemory();
    }
    state.counters["retained_bytes"] = static_cast<double>(workspace.memory_usage() + output.memory_usage());
    state.counters["output_rows"] = static_cast<double>(output.size());
}

[[maybe_unused]] const auto registered = []
{
    const auto configure = [](benchmark::Benchmark* bench) { bench->ArgName("rows")->Arg(64)->Arg(1024)->Unit(benchmark::kMicrosecond); };
    configure(benchmark::RegisterBenchmark("database/query/optimize/structural", optimize_query<Planning::structural>));
    configure(benchmark::RegisterBenchmark("database/query/optimize/measured", optimize_query<Planning::measured>));
    configure(benchmark::RegisterBenchmark("database/query/full/original", query_updates<false, Planning::original>));
    configure(benchmark::RegisterBenchmark("database/query/full/structural", query_updates<false, Planning::structural>));
    configure(benchmark::RegisterBenchmark("database/query/full/measured", query_updates<false, Planning::measured>));
    configure(benchmark::RegisterBenchmark("database/query/incremental/original", query_updates<true, Planning::original>));
    configure(benchmark::RegisterBenchmark("database/query/incremental/structural", query_updates<true, Planning::structural>));
    configure(benchmark::RegisterBenchmark("database/query/incremental/measured", query_updates<true, Planning::measured>));
    configure(benchmark::RegisterBenchmark("database/query/generic_join/cold", generic_join_full<true>));
    configure(benchmark::RegisterBenchmark("database/query/generic_join/warm", generic_join_full<false>));
    return true;
}();
}  // namespace
}  // namespace ygg::profiling
