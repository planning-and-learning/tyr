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

#include "tyr/planning/lifted/heuristics/rpg_ff.hpp"

#include "tyr/formalism/planning/parser.hpp"
#include "tyr/planning/algorithms/gbfs_lazy.hpp"
#include "tyr/planning/algorithms/gbfs_lazy/event_handler.hpp"
#include "tyr/planning/factory.hpp"
#include "tyr/planning/lifted/successor_generator.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/node.hpp"

#include <benchmark/benchmark.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <yggdrasil/serialization/json.hpp>
#include <yggdrasil/serialization/json_suite.hpp>

namespace fp = tyr::formalism::planning;
namespace p = tyr::planning;

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

struct BenchmarkCase
{
    std::string name;
    std::filesystem::path domain;
    std::filesystem::path task;
};

std::vector<BenchmarkCase> load_cases()
{
    const auto document = ygg::common::load_json_file(ygg::common::profiling_path("planning/lifted/heuristics/rpg.json"));
    const auto& root = ygg::common::as_object(document, "suite");
    const auto prefix = std::filesystem::path(BENCHMARKS_DIR);
    const auto& domains = ygg::common::as_object(root, "domains", "suite");

    auto result = std::vector<BenchmarkCase>();

    for (const auto& [domain_name_key, domain_value] : domains)
    {
        const auto& domain_object = ygg::common::as_object(domain_value, "domain");
        const auto domain_name = std::string(domain_name_key);
        const auto domain = ygg::common::resolve_path(prefix, ygg::common::as_string(domain_object, "domain_file", "domain"));
        const auto& tasks = ygg::common::as_object(domain_object, "tasks", "domain");

        for (const auto& [task_name_key, task_value] : tasks)
        {
            const auto task_name = std::string(task_name_key);
            const auto run_name = domain_name + "/" + task_name;
            const auto task = ygg::common::resolve_path(prefix, ygg::common::as_string(task_value, "task"));

            result.push_back(BenchmarkCase { run_name, domain, task });
        }
    }

    return result;
}

p::TaskPtr<::tyr::LiftedTag> create_task(const BenchmarkCase& benchmark_case)
{
    return p::Task<::tyr::LiftedTag>::create(fp::Parser(benchmark_case.domain).parse_task(benchmark_case.task));
}

void benchmark_gbfs_lazy_rpg_ff(benchmark::State& state, const BenchmarkCase& benchmark_case)
{
    auto task = create_task(benchmark_case);
    auto execution_context = ygg::ExecutionContext::create(1);
    auto initial_h_value = ygg::float_t(0);
    auto cost = ygg::float_t(0);
    auto length = std::size_t(0);
    auto num_expanded = uint64_t(0);
    auto num_generated = uint64_t(0);
    auto solved = false;

    for (auto _ : state)
    {
        auto axiom_evaluator = p::AxiomEvaluatorFactory<::tyr::LiftedTag>().create(task, execution_context);
        auto state_repository = p::StateRepositoryFactory<::tyr::LiftedTag>().create(task, axiom_evaluator);
        auto successor_generator = p::SuccessorGeneratorFactory<::tyr::LiftedTag>().create(task, execution_context, state_repository);
        auto heuristic = p::FFRPGHeuristic<::tyr::LiftedTag>::create(task, execution_context);
        auto event_handler = p::gbfs_lazy::DefaultEventHandler<::tyr::LiftedTag>::create(0);

        auto options = p::gbfs_lazy::Options<::tyr::LiftedTag>();
        options.start_node = successor_generator->get_initial_node();
        options.event_handler = event_handler;
        initial_h_value = heuristic->evaluate(options.start_node->get_state());

        auto result = p::SearchResult<::tyr::LiftedTag>();
        {
            const auto silence_cout = ScopedCoutSilencer();
            result = p::gbfs_lazy::find_solution(*task, *successor_generator, *heuristic, options);
        }

        num_expanded = event_handler->get_statistics().get_num_expanded();
        num_generated = event_handler->get_statistics().get_num_generated();
        solved = result.status == p::SearchStatus::SOLVED;
        cost = result.plan ? result.plan->get_cost() : ygg::float_t(0);
        length = result.plan ? result.plan->get_length() : std::size_t(0);

        benchmark::DoNotOptimize(static_cast<int>(result.status));
        benchmark::DoNotOptimize(initial_h_value);
        benchmark::DoNotOptimize(cost);
        benchmark::DoNotOptimize(length);
        benchmark::DoNotOptimize(num_expanded);
        benchmark::DoNotOptimize(num_generated);
        benchmark::DoNotOptimize(solved);
    }

    state.counters["initial_h_value"] = benchmark::Counter(static_cast<double>(initial_h_value));
    state.counters["cost"] = benchmark::Counter(static_cast<double>(cost));
    state.counters["length"] = benchmark::Counter(static_cast<double>(length));
    state.counters["num_expanded"] = benchmark::Counter(static_cast<double>(num_expanded));
    state.counters["num_generated"] = benchmark::Counter(static_cast<double>(num_generated));
    state.counters["solved"] = benchmark::Counter(solved ? 1.0 : 0.0);
}
}

int main(int argc, char** argv)
{
    benchmark::Initialize(&argc, argv);

    for (const auto& benchmark_case : load_cases())
    {
        benchmark::RegisterBenchmark((benchmark_case.name + "/gbfs_lazy").c_str(),
                                     [benchmark_case](benchmark::State& state) { benchmark_gbfs_lazy_rpg_ff(state, benchmark_case); });
    }

    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}
