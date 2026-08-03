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

#include "tyr/planning/lifted/successor_generator.hpp"

#include "tyr/formalism/planning/parser.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/factory.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/node.hpp"

#include <benchmark/benchmark.h>
#include <filesystem>
#include <string>
#include <vector>
#include <yggdrasil/serialization/json.hpp>
#include <yggdrasil/serialization/json_suite.hpp>

namespace fp = tyr::formalism::planning;
namespace p = tyr::planning;

namespace
{
struct BenchmarkCase
{
    std::string name;
    std::filesystem::path domain;
    std::filesystem::path task;
};

std::vector<BenchmarkCase> load_cases()
{
    const auto document = ygg::common::load_json_file(ygg::common::profiling_path("planning/lifted/successor_generator.json"));
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

p::SuccessorGeneratorPtr<::tyr::LiftedTag> create_successor_generator(p::TaskPtr<::tyr::LiftedTag> task)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<::tyr::LiftedTag>().create(task, execution_context);
    auto state_repository = p::StateRepositoryFactory<::tyr::LiftedTag>().create(task, axiom_evaluator);
    return p::SuccessorGeneratorFactory<::tyr::LiftedTag>().create(task, execution_context, state_repository);
}

void benchmark_initial_successors(benchmark::State& state, const BenchmarkCase& benchmark_case)
{
    auto task = create_task(benchmark_case);
    auto successor_generator = create_successor_generator(task);
    const auto initial_node = successor_generator->get_initial_node();
    auto successors = std::vector<p::LabeledNode<::tyr::LiftedTag>>();

    for (auto _ : state)
    {
        successor_generator->get_labeled_successor_nodes(initial_node, successors);
        benchmark::DoNotOptimize(successors.data());
        benchmark::DoNotOptimize(successors.size());
    }

    state.counters["num_successors"] = benchmark::Counter(static_cast<double>(successors.size()));
}

void benchmark_interned_action_bindings(benchmark::State& state, const BenchmarkCase& benchmark_case)
{
    auto task = create_task(benchmark_case);
    auto successor_generator = create_successor_generator(task);
    const auto initial_node = successor_generator->get_initial_node();
    auto bindings = std::vector<fp::ActionBindingView>();

    for (auto _ : state)
    {
        successor_generator->get_applicable_action_bindings(initial_node, bindings);
        benchmark::DoNotOptimize(bindings.data());
        benchmark::DoNotOptimize(bindings.size());
    }

    state.counters["num_successors"] = benchmark::Counter(static_cast<double>(bindings.size()));
}

}

int main(int argc, char** argv)
{
    benchmark::Initialize(&argc, argv);

    for (const auto& benchmark_case : load_cases())
    {
        benchmark::RegisterBenchmark((benchmark_case.name + "/labeled_successors").c_str(),
                                     [benchmark_case](benchmark::State& state) { benchmark_initial_successors(state, benchmark_case); });
        benchmark::RegisterBenchmark((benchmark_case.name + "/interned_bindings").c_str(),
                                     [benchmark_case](benchmark::State& state) { benchmark_interned_action_bindings(state, benchmark_case); });
    }

    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}
