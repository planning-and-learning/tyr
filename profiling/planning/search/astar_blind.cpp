/*
 * Copyright (C) 2026 Dominik Drexler
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
#include "tyr/planning/algorithms/astar_eager.hpp"
#include "tyr/planning/factory.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/heuristics/blind.hpp"
#include "tyr/planning/lifted/task.hpp"

#include <algorithm>
#include <array>
#include <benchmark/benchmark.h>
#include <charconv>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fp = tyr::formalism::planning;
namespace p = tyr::planning;

namespace
{
using Kind = tyr::TYR_PROFILE_TASK_KIND;
using Clock = std::chrono::steady_clock;
using tyr::profiling::BenchmarkCase;

struct Options
{
    std::vector<ygg::uint_t> cores { 1, 2, 4, 8, 16 };
    double search_timeout = 60;
    ygg::uint_t max_states = 1'000'000;
    p::astar_eager::ParallelSearchMode search_mode = p::astar_eager::ParallelSearchMode::SYNCHRONOUS;
};

ygg::uint_t parse_count(std::string_view value)
{
    auto result = ygg::uint_t {};
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), result);
    if (error != std::errc {} || end != value.data() + value.size())
        throw std::invalid_argument("Expected a nonnegative integer: " + std::string(value));
    return result;
}

Options parse_options(int& argc, char** argv)
{
    auto options = Options {};
    auto remaining = 1;
    for (int i = 1; i < argc; ++i)
    {
        const auto argument = std::string_view(argv[i]);
        const auto separator = argument.find('=');
        const auto name = argument.substr(0, separator);
        if (name != "--cores" && name != "--search-timeout" && name != "--max-states" && name != "--search-mode")
        {
            argv[remaining++] = argv[i];
            continue;
        }
        if (separator == std::string_view::npos && i + 1 == argc)
            throw std::invalid_argument("Missing value for " + std::string(name));
        const auto value = separator == std::string_view::npos ? std::string_view(argv[++i]) : argument.substr(separator + 1);
        if (name == "--cores")
        {
            options.cores.clear();
            for (size_t start = 0; start <= value.size();)
            {
                const auto end = value.find(',', start);
                const auto cores = parse_count(value.substr(start, end == std::string_view::npos ? end : end - start));
                if (cores == 0 || std::ranges::find(options.cores, cores) != options.cores.end())
                    throw std::invalid_argument("--cores must contain distinct positive integers.");
                options.cores.push_back(cores);
                if (end == std::string_view::npos)
                    break;
                start = end + 1;
            }
        }
        else if (name == "--search-timeout")
        {
            auto parsed = size_t {};
            options.search_timeout = std::stod(std::string(value), &parsed);
            if (parsed != value.size() || !std::isfinite(options.search_timeout) || options.search_timeout <= 0
                || options.search_timeout >= std::chrono::duration<double>(Clock::duration::max()).count())
                throw std::invalid_argument("--search-timeout must be a finite positive duration in seconds.");
        }
        else if (name == "--max-states")
            options.max_states = parse_count(value);
        else if (value == "sync")
            options.search_mode = p::astar_eager::ParallelSearchMode::SYNCHRONOUS;
        else if (value == "async")
            options.search_mode = p::astar_eager::ParallelSearchMode::ASYNCHRONOUS;
        else
            throw std::invalid_argument("--search-mode must be sync or async.");
    }
    argc = remaining;
    argv[argc] = nullptr;
    return options;
}

struct PreparedCase
{
    p::TaskPtr<Kind> task;
    p::AxiomEvaluatorPtr<Kind> axiom_evaluator;
    p::SuccessorGeneratorPtr<Kind> successor_generator;
    double parse_seconds = 0;
    double ground_seconds = 0;
    double component_setup_seconds = 0;
    std::optional<ygg::float_t> reference_cost;
};

template<tyr::TaskKind T>
p::TaskPtr<T> create_task(const BenchmarkCase& benchmark_case, PreparedCase& prepared, [[maybe_unused]] ygg::ExecutionContext& execution_context)
{
    const auto parse_start = Clock::now();
    auto lifted_task = p::Task<tyr::LiftedTag>::create(fp::Parser(benchmark_case.domain).parse_task(benchmark_case.task));
    prepared.parse_seconds = std::chrono::duration<double>(Clock::now() - parse_start).count();
    if constexpr (std::same_as<T, tyr::GroundTag>)
    {
        const auto ground_start = Clock::now();
        auto result = lifted_task->instantiate_ground_task(execution_context);
        prepared.ground_seconds = std::chrono::duration<double>(Clock::now() - ground_start).count();
        if (!result.task)
            throw std::runtime_error("Grounding proved the task unsolvable; no ground search was run.");
        return result.task;
    }
    else
        return lifted_task;
}

template p::TaskPtr<Kind> create_task<Kind>(const BenchmarkCase&, PreparedCase&, ygg::ExecutionContext&);

void run(benchmark::State& state, const BenchmarkCase& benchmark_case, PreparedCase& prepared, ygg::uint_t cores, const Options& options)
{
    try
    {
        auto execution_context = ygg::ExecutionContext::create(1);
        if (!prepared.successor_generator)
        {
            prepared.task = create_task<Kind>(benchmark_case, prepared, *execution_context);
            const auto component_start = Clock::now();
            prepared.axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(prepared.task, execution_context);
            prepared.successor_generator = p::SuccessorGeneratorFactory<Kind>().create(prepared.task, execution_context);
            prepared.component_setup_seconds = std::chrono::duration<double>(Clock::now() - component_start).count();
        }

        for (auto _ : state)
        {
            const auto worker_setup_start = Clock::now();
            auto repository = p::StateRepositoryFactory<Kind>().create(prepared.task);
            auto axiom_evaluator = prepared.axiom_evaluator->make_worker(execution_context);
            auto successor_generator = prepared.successor_generator->make_worker(execution_context);
            auto heuristic = p::BlindHeuristic<Kind> {};
            auto search_options = p::astar_eager::Options<Kind> {};
            search_options.num_search_workers = cores;
            search_options.parallel_search_mode = options.search_mode;
            search_options.search_budget.max_num_states = options.max_states;
            search_options.search_budget.max_time = std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(options.search_timeout));
            search_options.start_node = successor_generator->get_initial_node(*repository, *axiom_evaluator);
            const auto worker_setup_seconds = std::chrono::duration<double>(Clock::now() - worker_setup_start).count();

            const auto search_start = Clock::now();
            const auto result = p::astar_eager::find_solution(*prepared.task, *repository, *axiom_evaluator, *successor_generator, heuristic, search_options);
            state.SetIterationTime(std::chrono::duration<double>(Clock::now() - search_start).count());

            if (result.worker_statistics.size() != cores)
                throw std::runtime_error("Search did not use the requested number of workers.");
            const auto solved = result.status == p::SearchStatus::SOLVED;
            if (solved)
            {
                if (!result.plan || !std::isfinite(result.plan->get_cost()))
                    throw std::runtime_error("Solved search returned no finite-cost plan.");
                const auto cost = result.plan->get_cost();
                if (prepared.reference_cost
                    && std::abs(cost - *prepared.reference_cost) > ygg::FloatTolerance<ygg::float_t>::tolerance(cost, *prepared.reference_cost))
                    throw std::runtime_error("A* plan cost differs between worker counts or repetitions.");
                prepared.reference_cost = cost;
            }

            static constexpr auto status_names =
                std::array { "IN_PROGRESS", "OUT_OF_TIME", "OUT_OF_MEMORY", "OUT_OF_STATES", "FAILED", "EXHAUSTED", "CYCLE", "SOLVED", "UNSOLVABLE" };
            state.SetLabel(status_names.at(static_cast<size_t>(result.status)));
            state.counters["status"] = static_cast<double>(result.status);
            state.counters["solved"] = solved ? 1 : 0;
            state.counters["cost"] = result.plan ? result.plan->get_cost() : -1;
            state.counters["length"] = result.plan ? static_cast<double>(result.plan->get_length()) : -1;
            state.counters["workers"] = static_cast<double>(result.worker_statistics.size());
            state.counters["num_expanded"] = static_cast<double>(result.statistics.get_num_expanded());
            state.counters["num_generated_successors"] = static_cast<double>(result.statistics.get_num_generated_successors());
            state.counters["num_generated_candidates"] = static_cast<double>(result.statistics.get_num_generated_candidates());
            state.counters["num_registered_states"] = static_cast<double>(result.statistics.get_num_registered_states());
            state.counters["num_transferred_candidates"] = static_cast<double>(result.statistics.get_num_transferred_candidates());
            state.counters["state_storage_bytes"] = static_cast<double>(result.statistics.get_state_storage_memory_usage());
            state.counters["worker_utilization"] = result.get_worker_utilization();
            state.counters["parse_seconds"] = prepared.parse_seconds;
            state.counters["ground_seconds"] = prepared.ground_seconds;
            state.counters["component_setup_seconds"] = prepared.component_setup_seconds;
            state.counters["worker_setup_seconds"] = worker_setup_seconds;
        }
    }
    catch (const std::exception& error)
    {
        state.SkipWithError(error.what());
    }
}
}

int main(int argc, char** argv)
{
    try
    {
        const auto suite = tyr::profiling::extract_suite_path(argc, argv);
        const auto options = parse_options(argc, argv);
        benchmark::Initialize(&argc, argv);
        if (benchmark::ReportUnrecognizedArguments(argc, argv))
            return 1;
        const auto mode = options.search_mode == p::astar_eager::ParallelSearchMode::SYNCHRONOUS ? "sync" : "async";
        for (const auto& benchmark_case : tyr::profiling::load_suite(suite))
        {
            const auto prepared = std::make_shared<PreparedCase>();
            for (const auto cores : options.cores)
                benchmark::RegisterBenchmark((benchmark_case.name + "/astar_blind/" + mode + "/workers:" + std::to_string(cores)).c_str(),
                                             [benchmark_case, prepared, cores, options](benchmark::State& state)
                                             { run(state, benchmark_case, *prepared, cores, options); })
                    ->Iterations(1)
                    ->UseManualTime();
        }

        benchmark::RunSpecifiedBenchmarks();
        benchmark::Shutdown();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
