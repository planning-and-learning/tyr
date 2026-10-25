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
#include "tyr/planning/action_executor.hpp"
#include "tyr/planning/applicability.hpp"
#include "tyr/planning/factory.hpp"
#include "tyr/planning/ground/match_tree/match_tree.hpp"
#include "tyr/planning/ground/successor_generator.hpp"
#include "tyr/planning/ground/task.hpp"
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
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/execution/onetbb.hpp>

using tyr::GroundTag;
using tyr::LiftedTag;

namespace
{
using Tree = tyr::planning::match_tree::MatchTree<tyr::formalism::planning::Action<GroundTag>>;
using Forest =
    ygg::UnorderedMap<tyr::formalism::planning::ActionView<LiftedTag>, tyr::planning::match_tree::MatchTreePtr<tyr::formalism::planning::Action<GroundTag>>>;

struct PreparedCase
{
    tyr::planning::TaskPtr<GroundTag> task;
    tyr::planning::match_tree::MatchTreePtr<tyr::formalism::planning::Action<GroundTag>> tree;
    Forest forest;
    tyr::planning::SuccessorGeneratorPtr<GroundTag> generator;
};

struct BenchmarkCase
{
    tyr::profiling::BenchmarkCase paths;
    std::shared_ptr<PreparedCase> prepared = std::make_shared<PreparedCase>();
};

Forest make_forest(const tyr::planning::Task<GroundTag>& task)
{
    auto groups = ygg::UnorderedMap<tyr::formalism::planning::ActionView<LiftedTag>, tyr::formalism::planning::ActionViewList<GroundTag>> {};
    for (const auto schema : task.get_task().get_domain().get_actions())
        groups.try_emplace(schema);
    for (const auto action : task.get_task().get_ground_actions())
        groups.at(action.get_action()).push_back(action);
    auto forest = Forest {};
    for (auto& [schema, actions] : groups)
        forest.emplace(schema, Tree::create(std::move(actions), task.get_task().get_context()));
    return forest;
}

void generate_forest(Forest& forest,
                     const tyr::planning::StateContext<GroundTag>& context,
                     tyr::formalism::planning::ActionViewList<GroundTag>& scratch,
                     tyr::formalism::planning::ActionViewList<GroundTag>& out)
{
    out.clear();
    for (auto& [schema, tree] : forest)
    {
        tree->generate(context, scratch);
        out.insert(out.end(), scratch.begin(), scratch.end());
    }
}

enum class Operation : std::uint8_t
{
    ONE,
    ALL,
    BUILD,
    BINDINGS,
    SUCCESSORS
};

template<Operation operation, bool per_schema>
void run(benchmark::State& state, const BenchmarkCase& benchmark_case)
try
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto& prepared = *benchmark_case.prepared;
    if (!prepared.task)
    {
        auto lifted_task =
            tyr::planning::Task<LiftedTag>::create(tyr::formalism::planning::Parser(benchmark_case.paths.domain).parse_task(benchmark_case.paths.task));
        prepared.task = lifted_task->instantiate_ground_task(*execution_context).task;
        if (prepared.task)
        {
            const auto actions = prepared.task->get_task().get_ground_actions();
            prepared.tree =
                Tree::create(tyr::formalism::planning::ActionViewList<GroundTag>(actions.begin(), actions.end()), prepared.task->get_task().get_context());
            prepared.forest = make_forest(*prepared.task);
            prepared.generator = tyr::planning::SuccessorGeneratorFactory<GroundTag>().create(prepared.task, execution_context);
        }
    }
    auto task = prepared.task;
    if (!task)
    {
        state.SkipWithError("Grounding failed.");
        return;
    }
    const auto actions = task->get_task().get_ground_actions();
    const auto all_actions = tyr::formalism::planning::ActionViewList<GroundTag>(actions.begin(), actions.end());
    auto tree = prepared.tree->make_worker();
    auto forest = Forest {};
    for (const auto& [schema, prototype] : prepared.forest)
        forest.emplace(schema, prototype->make_worker());
    auto evaluator = tyr::planning::AxiomEvaluatorFactory<GroundTag>().create(task, execution_context);
    auto repository = tyr::planning::StateRepositoryFactory<GroundTag>().create(task);
    auto generator = prepared.generator->make_worker(execution_context);
    const auto node = generator->get_initial_node(*repository, *evaluator);
    const auto context = tyr::planning::StateContext<GroundTag>(*task, node.get_state().get_state_builder(), node.get_metric());
    auto applicable = tyr::formalism::planning::ActionViewList<GroundTag> {};
    auto scratch = tyr::formalism::planning::ActionViewList<GroundTag> {};
    auto expected = tyr::formalism::planning::ActionViewList<GroundTag> {};
    tree->generate(context, expected);
    generate_forest(forest, context, scratch, applicable);
    const auto by_index = [](auto lhs, auto rhs) { return lhs.get_index() < rhs.get_index(); };
    std::ranges::sort(expected, by_index);
    std::ranges::sort(applicable, by_index);
    if (expected != applicable)
        throw std::runtime_error("Global and per-schema match trees disagree.");

    auto all_bindings = generator->get_applicable_action_bindings(node);
    auto schemas = task->get_task().get_domain().get_actions();
    const auto selected =
        std::ranges::find_if(schemas,
                             [&](auto schema) { return std::ranges::any_of(all_bindings, [&](auto binding) { return binding.get_relation() == schema; }); });
    if (selected == schemas.end())
    {
        state.SkipWithError("No initially applicable schema.");
        return;
    }
    const auto schema = *selected;
    const auto reject_action = [&](auto action) { return action.get_action() != schema; };
    const auto reject_binding = [&](auto binding) { return binding.get_relation() != schema; };
    std::erase_if(expected, reject_action);
    forest.at(schema)->generate(context, applicable);
    std::ranges::sort(applicable, by_index);
    if (expected != applicable)
        throw std::runtime_error("Selected schema match tree disagrees with global filtering.");

    auto bindings = std::vector<tyr::formalism::planning::ActionBindingView> {};
    auto successors = tyr::planning::LabeledNodeList<GroundTag> {};
    auto executor = tyr::planning::ActionExecutor {};
    const auto generate = [&]
    {
        if constexpr (operation == Operation::BUILD)
        {
            if constexpr (per_schema)
            {
                auto result = make_forest(*task);
                benchmark::DoNotOptimize(result);
            }
            else
            {
                auto result = Tree::create(all_actions, task->get_task().get_context());
                benchmark::DoNotOptimize(result);
            }
        }
        else if constexpr (operation == Operation::BINDINGS)
        {
            if constexpr (per_schema)
                generator->get_applicable_action_bindings(node, schema, bindings);
            else
            {
                generator->get_applicable_action_bindings(node, bindings);
                std::erase_if(bindings, reject_binding);
            }
            benchmark::DoNotOptimize(bindings.data());
            benchmark::DoNotOptimize(bindings.size());
        }
        else if constexpr (operation == Operation::SUCCESSORS && per_schema)
        {
            generator->get_labeled_successor_nodes(node, schema, *repository, *evaluator, successors);
            benchmark::DoNotOptimize(successors.data());
            benchmark::DoNotOptimize(successors.size());
        }
        else
        {
            if constexpr (per_schema && operation == Operation::ALL)
                generate_forest(forest, context, scratch, applicable);
            else if constexpr (per_schema)
                forest.at(schema)->generate(context, applicable);
            else
            {
                tree->generate(context, applicable);
                if constexpr (operation != Operation::ALL)
                    std::erase_if(applicable, reject_action);
            }
            if constexpr (operation == Operation::SUCCESSORS)
            {
                successors.clear();
                for (const auto action : applicable)
                    if (executor.is_applicable_if_fires(action, context))
                        successors.emplace_back(action.get_row(), generator->get_successor_node(node, action, *repository, *evaluator));
                benchmark::DoNotOptimize(successors.data());
                benchmark::DoNotOptimize(successors.size());
            }
            benchmark::DoNotOptimize(applicable.data());
            benchmark::DoNotOptimize(applicable.size());
        }
    };

    generate();
    if constexpr (operation == Operation::BINDINGS)
    {
        std::erase_if(all_bindings, reject_binding);
        std::ranges::sort(all_bindings);
        std::ranges::sort(bindings);
        if (all_bindings != bindings)
            throw std::runtime_error("Schema binding API disagrees with global filtering.");
    }
    if constexpr (operation == Operation::SUCCESSORS)
    {
        auto reference = generator->get_labeled_successor_nodes(node, *repository, *evaluator);
        std::erase_if(reference, [&](const auto& successor) { return reject_binding(successor.label); });
        const auto by_label = [](const auto& lhs, const auto& rhs) { return lhs.label < rhs.label; };
        std::ranges::sort(reference, by_label);
        std::ranges::sort(successors, by_label);
        if (reference.size() != successors.size()
            || !std::ranges::equal(reference, successors, [](const auto& lhs, const auto& rhs) { return lhs.label == rhs.label && lhs.node == rhs.node; }))
            throw std::runtime_error("Schema successor API disagrees with global filtering.");
    }
    for ([[maybe_unused]] auto _ : state)
        generate();
    state.SetLabel(std::string(schema.get_name()));
    state.counters["num_ground_actions"] = static_cast<double>(all_actions.size());
    state.counters["num_schemas"] = static_cast<double>(schemas.size());
    state.counters["num_selected_actions"] = static_cast<double>(expected.size());
}
catch (const std::exception& error)
{
    state.SkipWithError(error.what());
}

template<Operation operation>
void register_pair(const BenchmarkCase& benchmark_case, const std::string& name)
{
    benchmark::RegisterBenchmark(benchmark_case.paths.name + "/" + name + "/global",
                                 [benchmark_case](benchmark::State& state) { run<operation, false>(state, benchmark_case); });
    benchmark::RegisterBenchmark(benchmark_case.paths.name + "/" + name + "/per_schema",
                                 [benchmark_case](benchmark::State& state) { run<operation, true>(state, benchmark_case); });
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
        register_pair<Operation::ONE>(benchmark_case, "one_schema");
        register_pair<Operation::ALL>(benchmark_case, "all_schemas");
        register_pair<Operation::BUILD>(benchmark_case, "construction");
        register_pair<Operation::BINDINGS>(benchmark_case, "bindings");
        register_pair<Operation::SUCCESSORS>(benchmark_case, "successors");
    }
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}
catch (const std::exception& error)
{
    std::cerr << error.what() << '\n';
    return 1;
}
