/*
 * Copyright (C) 2025-2026 Dominik Drexler
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "tyr/planning/successors/ground/successor_generator.hpp"

#include "benchmark_successors.hpp"
#include "common.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/planning/axioms/axiom_evaluator.hpp"
#include "tyr/planning/nodes/labeled_node_view.hpp"
#include "tyr/planning/nodes/node_view.hpp"
#include "tyr/planning/successors/lifted/successor_generator.hpp"
#include "tyr/planning/successors/successors.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"

#include <benchmark/benchmark.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>
#include <yggdrasil/execution/onetbb.hpp>

#if defined(TYR_BENCHMARK_GROUND)
using Kind = tyr::GroundTag;
#else
using Kind = tyr::LiftedTag;
#endif

namespace
{
using tyr::benchmarks::BenchmarkCase;

tyr::planning::TaskPtr<Kind> create_task(const BenchmarkCase& benchmark_case, ygg::ExecutionContext& execution_context)
{
    auto lifted = tyr::planning::Task<tyr::LiftedTag>::create(tyr::formalism::Parser(benchmark_case.domain).parse_task(benchmark_case.task));
#if defined(TYR_BENCHMARK_GROUND)
    auto ground = lifted->instantiate_ground_task(execution_context).task;
    if (!ground)
        throw std::runtime_error("Grounding failed.");
    return ground;
#else
    static_cast<void>(execution_context);
    return lifted;
#endif
}

void benchmark_initial_successors(benchmark::State& state, const BenchmarkCase& benchmark_case)
try
{
    // The scratch rows bindings are interned through, reused across iterations.
    auto builder = tyr::formalism::Builder {};
    auto execution_context = ygg::ExecutionContext::create(1);
    auto task = create_task(benchmark_case, *execution_context);
    auto axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto state_repository = tyr::planning::StateRepository<Kind>::create(task);
    auto successor_generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, execution_context);
    const auto initial_node = tyr::planning::get_initial_node(*state_repository, *axiom_evaluator);
    auto successors = tyr::planning::LabeledNodeList<Kind>();

    for ([[maybe_unused]] auto _ : state)
    {
        tyr::benchmarks::labeled_successors(*successor_generator, builder, initial_node, {}, *state_repository, *axiom_evaluator, successors);
        benchmark::DoNotOptimize(successors);
        benchmark::DoNotOptimize(successors.size());
    }

    state.counters["num_successors"] = benchmark::Counter(static_cast<double>(successors.size()));
}
catch (const std::exception& error)
{
    state.SkipWithError(error.what());
}

void benchmark_interned_action_bindings(benchmark::State& state, const BenchmarkCase& benchmark_case)
try
{
    // The scratch rows bindings are interned through, reused across iterations.
    auto builder = tyr::formalism::Builder {};
    auto execution_context = ygg::ExecutionContext::create(1);
    auto task = create_task(benchmark_case, *execution_context);
    auto axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto state_repository = tyr::planning::StateRepository<Kind>::create(task);
    auto successor_generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, execution_context);
    const auto initial_node = tyr::planning::get_initial_node(*state_repository, *axiom_evaluator);
    auto bindings = urd::Store<urd::sequence<tyr::formalism::ActionRowView>>();

    for ([[maybe_unused]] auto _ : state)
    {
        tyr::benchmarks::interned_bindings(*successor_generator, builder, initial_node, {}, bindings);
        benchmark::DoNotOptimize(bindings);
        benchmark::DoNotOptimize(bindings.size());
    }

    state.counters["num_successors"] = benchmark::Counter(static_cast<double>(bindings.size()));
}
catch (const std::exception& error)
{
    state.SkipWithError(error.what());
}

}

int main(int argc, char** argv)
try
{
    const auto suite_path = tyr::benchmarks::extract_suite_path(argc, argv);
    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv))
        return 1;

    for (const auto& benchmark_case : tyr::benchmarks::load_suite(suite_path))
    {
        benchmark::RegisterBenchmark(benchmark_case.name + "/labeled_successors",
                                     [benchmark_case](benchmark::State& state) { benchmark_initial_successors(state, benchmark_case); });
        benchmark::RegisterBenchmark(benchmark_case.name + "/interned_bindings",
                                     [benchmark_case](benchmark::State& state) { benchmark_interned_action_bindings(state, benchmark_case); });
    }

    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}
catch (const std::exception& error)
{
    std::cerr << error.what() << '\n';
    return 1;
}
