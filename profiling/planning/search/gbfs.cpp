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
#include "tyr/planning/algorithms/gbfs_lazy.hpp"
#include "tyr/planning/factory.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/heuristics/lmcut.hpp"
#include "tyr/planning/heuristics/rpg_add.hpp"
#include "tyr/planning/heuristics/rpg_ff.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/node.hpp"

#include <benchmark/benchmark.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace fp = tyr::formalism::planning;
namespace p = tyr::planning;
#if defined(TYR_PROFILE_GROUND)
using Kind = tyr::GroundTag;
#else
using Kind = tyr::LiftedTag;
#endif
using Heuristic = p::TYR_PROFILE_HEURISTIC<Kind>;

namespace
{
class ScopedCoutSilencer
{
public:
    ScopedCoutSilencer() : m_null_stream("/dev/null"), m_old_buffer(std::cout.rdbuf(m_null_stream.rdbuf())) {}
    ~ScopedCoutSilencer() { std::cout.rdbuf(m_old_buffer); }

    ScopedCoutSilencer(const ScopedCoutSilencer&) = delete;
    ScopedCoutSilencer& operator=(const ScopedCoutSilencer&) = delete;

private:
    std::ofstream m_null_stream;
    std::streambuf* m_old_buffer;
};

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

void benchmark_gbfs_lazy(benchmark::State& state, const BenchmarkCase& benchmark_case, ygg::ExecutionContext::uint_t evaluation_threads)
try
{
    auto execution_context = ygg::ExecutionContext::create(evaluation_threads);
    auto task = create_task(benchmark_case, *execution_context);
    auto initial_h_value = ygg::float_t(0);
    auto cost = ygg::float_t(0);
    auto length = std::size_t(0);
    auto num_expanded = uint64_t(0);
    auto num_generated_successors = uint64_t(0);
    auto solved = false;

    for (auto _ : state)
    {
        auto axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
        auto state_repository = p::StateRepositoryFactory<Kind>().create(task);
        auto successor_generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context);
        auto heuristic = Heuristic::create(task, execution_context);
        auto options = p::gbfs_lazy::Options<Kind>();
        options.start_node = successor_generator->get_initial_node(*state_repository, *axiom_evaluator);
        initial_h_value = heuristic->evaluate(options.start_node->get_state());

        auto result = p::SearchResult<Kind>();
        {
            const auto silence_cout = ScopedCoutSilencer();
            result = p::gbfs_lazy::find_solution(*task, *state_repository, *axiom_evaluator, *successor_generator, *heuristic, options);
        }

        num_expanded = result.statistics.get_num_expanded();
        num_generated_successors = result.statistics.get_num_generated_successors();
        solved = result.status == p::SearchStatus::SOLVED;
        cost = result.plan ? result.plan->get_cost() : ygg::float_t(0);
        length = result.plan ? result.plan->get_length() : std::size_t(0);

        benchmark::DoNotOptimize(static_cast<int>(result.status));
        benchmark::DoNotOptimize(initial_h_value);
        benchmark::DoNotOptimize(cost);
        benchmark::DoNotOptimize(length);
        benchmark::DoNotOptimize(num_expanded);
        benchmark::DoNotOptimize(num_generated_successors);
        benchmark::DoNotOptimize(solved);
    }

    state.counters["initial_h_value"] = benchmark::Counter(static_cast<double>(initial_h_value));
    state.counters["cost"] = benchmark::Counter(static_cast<double>(cost));
    state.counters["length"] = benchmark::Counter(static_cast<double>(length));
    state.counters["num_expanded"] = benchmark::Counter(static_cast<double>(num_expanded));
    state.counters["num_generated_successors"] = benchmark::Counter(static_cast<double>(num_generated_successors));
    state.counters["solved"] = benchmark::Counter(solved ? 1.0 : 0.0);
    state.counters["evaluation_threads"] = static_cast<double>(evaluation_threads);
    state.counters["search_workers"] = 1;
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
        for (const auto evaluation_threads : { ygg::ExecutionContext::uint_t(1), ygg::ExecutionContext::uint_t(8) })
        {
            if (evaluation_threads != 1 && !std::is_same_v<Heuristic, p::FFRPGHeuristic<Kind>>)
                continue;
            const auto name = benchmark_case.name + "/gbfs_lazy/evaluation_threads:" + std::to_string(evaluation_threads);
            benchmark::RegisterBenchmark(name.c_str(),
                                         [benchmark_case, evaluation_threads](benchmark::State& state)
                                         { benchmark_gbfs_lazy(state, benchmark_case, evaluation_threads); })
                ->UseRealTime();
        }
    }

    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}
catch (const std::exception& error)
{
    std::cerr << error.what() << '\n';
    return 1;
}
