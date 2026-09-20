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

#include "../../common.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/factory.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/node.hpp"

#include <algorithm>
#include <benchmark/benchmark.h>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#if defined(__GLIBC__)
#include <malloc.h>
#endif

namespace fp = tyr::formalism::planning;
namespace p = tyr::planning;
using tyr::LiftedTag;

namespace
{
#if defined(__GLIBC__)
std::int64_t allocated_bytes()
{
    const auto info = mallinfo2();
    return static_cast<std::int64_t>(info.uordblks) + static_cast<std::int64_t>(info.hblkhd);
}
#endif

struct PreparedCase
{
    p::TaskPtr<LiftedTag> task;
    p::SuccessorGeneratorPtr<LiftedTag> generator;
    std::int64_t generator_bytes = 0;
};

struct BenchmarkCase
{
    tyr::profiling::BenchmarkCase paths;
    std::shared_ptr<PreparedCase> prepared = std::make_shared<PreparedCase>();
};

enum class Operation
{
    ALL_BINDINGS,
    ALL_SUCCESSORS,
    FILTER_BINDINGS,
    FILTER_SUCCESSORS,
    SCHEMA_BINDINGS,
    SCHEMA_SUCCESSORS,
    CONSTRUCTION,
    WORKER
};

template<Operation operation>
void run(benchmark::State& state, const BenchmarkCase& benchmark_case)
try
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto& prepared = *benchmark_case.prepared;
    if (!prepared.task)
    {
        prepared.task = p::Task<LiftedTag>::create(fp::Parser(benchmark_case.paths.domain).parse_task(benchmark_case.paths.task));
#if defined(__GLIBC__)
        const auto before = allocated_bytes();
#endif
        prepared.generator = p::SuccessorGeneratorFactory<LiftedTag>().create(prepared.task, execution_context);
#if defined(__GLIBC__)
        prepared.generator_bytes = allocated_bytes() - before;
#endif
    }
#if defined(__GLIBC__)
    const auto before_worker = allocated_bytes();
#endif
    auto generator = prepared.generator->make_worker(execution_context);
#if defined(__GLIBC__)
    state.counters["generator_glibc_bytes"] = static_cast<double>(prepared.generator_bytes);
    state.counters["worker_glibc_bytes"] = static_cast<double>(allocated_bytes() - before_worker);
#endif
    auto evaluator = p::AxiomEvaluatorFactory<LiftedTag>().create(prepared.task, execution_context);
    auto repository = p::StateRepositoryFactory<LiftedTag>().create(prepared.task);
    const auto node = generator->get_initial_node(*repository, *evaluator);
    auto expected_bindings = generator->get_applicable_action_bindings(node);
    const auto schemas = prepared.task->get_task().get_domain().get_actions();
    const auto selected = std::ranges::find_if(
        schemas,
        [&](auto schema) { return std::ranges::any_of(expected_bindings, [&](auto binding) { return binding.get_relation() == schema; }); });
    if (selected == schemas.end())
    {
        state.SkipWithError("No initially applicable schema.");
        return;
    }
    const auto schema = *selected;
    const auto reject_binding = [&](auto binding) { return binding.get_relation() != schema; };
    state.SetLabel(std::string(schema.get_name()));
    state.counters["num_schemas"] = static_cast<double>(schemas.size());
    state.counters["num_applicable_bindings"] = static_cast<double>(expected_bindings.size());
    std::erase_if(expected_bindings, reject_binding);
    state.counters["num_selected_bindings"] = static_cast<double>(expected_bindings.size());

    auto bindings = std::vector<fp::ActionBindingView> {};
    auto successors = p::LabeledNodeList<LiftedTag> {};
    const auto generate = [&]
    {
        if constexpr (operation == Operation::CONSTRUCTION)
        {
            auto result = p::SuccessorGeneratorFactory<LiftedTag>().create(prepared.task, execution_context);
            benchmark::DoNotOptimize(result);
        }
        else if constexpr (operation == Operation::WORKER)
        {
            auto result = prepared.generator->make_worker(execution_context);
            benchmark::DoNotOptimize(result);
        }
        else if constexpr (operation == Operation::SCHEMA_BINDINGS)
        {
            generator->get_applicable_action_bindings(node, schema, bindings);
        }
        else if constexpr (operation == Operation::SCHEMA_SUCCESSORS)
        {
            generator->get_labeled_successor_nodes(node, schema, *repository, *evaluator, successors);
        }
        else if constexpr (operation == Operation::ALL_SUCCESSORS)
        {
            generator->get_labeled_successor_nodes(node, *repository, *evaluator, successors);
        }
        else
        {
            generator->get_applicable_action_bindings(node, bindings);
            if constexpr (operation != Operation::ALL_BINDINGS)
                std::erase_if(bindings, reject_binding);
            if constexpr (operation == Operation::FILTER_SUCCESSORS)
            {
                successors.clear();
                for (const auto binding : bindings)
                    successors.emplace_back(binding, generator->get_successor_node(node, binding, *repository, *evaluator));
            }
        }
        benchmark::DoNotOptimize(bindings.data());
        benchmark::DoNotOptimize(bindings.size());
        benchmark::DoNotOptimize(successors.data());
        benchmark::DoNotOptimize(successors.size());
    };

    generate();
    if constexpr (operation == Operation::FILTER_BINDINGS || operation == Operation::SCHEMA_BINDINGS)
    {
        std::ranges::sort(expected_bindings);
        std::ranges::sort(bindings);
        if (expected_bindings != bindings)
            throw std::runtime_error("Selected binding generation disagrees with global filtering.");
    }
    if constexpr (operation == Operation::FILTER_SUCCESSORS || operation == Operation::SCHEMA_SUCCESSORS)
    {
        auto expected = generator->get_labeled_successor_nodes(node, *repository, *evaluator);
        std::erase_if(expected, [&](const auto& successor) { return reject_binding(successor.label); });
        const auto by_label = [](const auto& lhs, const auto& rhs) { return lhs.label < rhs.label; };
        std::ranges::sort(expected, by_label);
        std::ranges::sort(successors, by_label);
        if (!std::ranges::equal(expected, successors, [](const auto& lhs, const auto& rhs) { return lhs.label == rhs.label && lhs.node == rhs.node; }))
            throw std::runtime_error("Selected successor generation disagrees with global filtering.");
    }
    for (auto _ : state)
        generate();
}
catch (const std::exception& error)
{
    state.SkipWithError(error.what());
}

template<Operation operation>
void register_case(const BenchmarkCase& benchmark_case, const std::string& name)
{
    benchmark::RegisterBenchmark((benchmark_case.paths.name + "/" + name).c_str(),
                                 [benchmark_case](benchmark::State& state) { run<operation>(state, benchmark_case); });
}
}

int main(int argc, char** argv)
try
{
    const auto suite_path = tyr::profiling::extract_suite_path(argc, argv);
    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv))
        return 1;
    for (const auto& paths : tyr::profiling::load_suite(suite_path))
    {
        const auto benchmark_case = BenchmarkCase { paths };
        register_case<Operation::ALL_SUCCESSORS>(benchmark_case, "labeled_successors");
        register_case<Operation::ALL_BINDINGS>(benchmark_case, "interned_bindings");
        register_case<Operation::FILTER_BINDINGS>(benchmark_case, "schema_bindings/global");
        register_case<Operation::FILTER_SUCCESSORS>(benchmark_case, "schema_successors/global");
        register_case<Operation::SCHEMA_BINDINGS>(benchmark_case, "schema_bindings/per_schema");
        register_case<Operation::SCHEMA_SUCCESSORS>(benchmark_case, "schema_successors/per_schema");
        register_case<Operation::CONSTRUCTION>(benchmark_case, "construction");
        register_case<Operation::WORKER>(benchmark_case, "worker");
    }
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}
catch (const std::exception& error)
{
    std::cerr << error.what() << '\n';
    return 1;
}
