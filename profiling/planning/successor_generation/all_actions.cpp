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

#include "../common.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/factory.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/node.hpp"

#include <benchmark/benchmark.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fp = tyr::formalism::planning;
namespace p = tyr::planning;
#if defined(TYR_PROFILE_GROUND)
using Kind = tyr::GroundTag;
#else
using Kind = tyr::LiftedTag;
#endif

namespace
{
using tyr::profiling::BenchmarkCase;

p::TaskPtr<Kind> create_task(const BenchmarkCase& benchmark_case, ygg::ExecutionContext& execution_context)
{
    auto lifted = p::Task<tyr::LiftedTag>::create(fp::Parser(benchmark_case.domain).parse_task(benchmark_case.task));
#if defined(TYR_PROFILE_GROUND)
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
    auto execution_context = ygg::ExecutionContext::create(1);
    auto task = create_task(benchmark_case, *execution_context);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto state_repository = p::StateRepositoryFactory<Kind>().create(task);
    auto successor_generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context);
    const auto initial_node = successor_generator->get_initial_node(*state_repository, *axiom_evaluator);
    auto successors = std::vector<p::LabeledNode<Kind>>();

    for (auto _ : state)
    {
        successor_generator->get_labeled_successor_nodes(initial_node, *state_repository, *axiom_evaluator, successors);
        benchmark::DoNotOptimize(successors.data());
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
    auto execution_context = ygg::ExecutionContext::create(1);
    auto task = create_task(benchmark_case, *execution_context);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto state_repository = p::StateRepositoryFactory<Kind>().create(task);
    auto successor_generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context);
    const auto initial_node = successor_generator->get_initial_node(*state_repository, *axiom_evaluator);
    auto bindings = std::vector<fp::ActionBindingView>();

    for (auto _ : state)
    {
        successor_generator->get_applicable_action_bindings(initial_node, bindings);
        benchmark::DoNotOptimize(bindings.data());
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
    const auto suite_path = tyr::profiling::extract_suite_path(argc, argv);
    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv))
        return 1;

    for (const auto& benchmark_case : tyr::profiling::load_suite(suite_path))
    {
        benchmark::RegisterBenchmark((benchmark_case.name + "/labeled_successors").c_str(),
                                     [benchmark_case](benchmark::State& state) { benchmark_initial_successors(state, benchmark_case); });
        benchmark::RegisterBenchmark((benchmark_case.name + "/interned_bindings").c_str(),
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
