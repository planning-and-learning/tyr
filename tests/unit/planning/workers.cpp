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

#include "planning/parser.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/planning/planning.hpp"

#include <concepts>
#include <filesystem>
#include <future>
#include <gtest/gtest.h>
#include <memory>
#include <type_traits>
#include <unordered_set>
#include <yggdrasil/execution/onetbb.hpp>

namespace p = tyr::planning;
namespace fp = tyr::formalism::planning;

static_assert(fp::Repository::thread_safe);

namespace tyr::tests
{
namespace
{
template<typename T>
void expect_distinct(const std::shared_ptr<T>& source, const std::shared_ptr<T>& worker, const std::shared_ptr<T>& nested)
{
    EXPECT_NE(source.get(), worker.get());
    EXPECT_NE(source.get(), nested.get());
    EXPECT_NE(worker.get(), nested.get());
    EXPECT_NE(source->get_index(), worker->get_index());
    EXPECT_NE(source->get_index(), nested->get_index());
    EXPECT_NE(worker->get_index(), nested->get_index());
}

template<::tyr::TaskKind Kind>
void expect_worker_chain(const p::TaskPtr<Kind>& task)
{
    ASSERT_TRUE(task->has_axioms());

    auto source_context = ygg::ExecutionContext::create(1);
    auto axiom_factory = p::AxiomEvaluatorFactory<Kind> {};
    auto state_repository_factory = p::StateRepositoryFactory<Kind> {};
    auto successor_generator_factory = p::SuccessorGeneratorFactory<Kind> {};

    auto source_axiom_evaluator = axiom_factory.create(task, source_context);
    auto source_state_repository = state_repository_factory.create(task, source_axiom_evaluator);
    auto source = successor_generator_factory.create(task, source_context, source_state_repository);

    auto worker_context = ygg::ExecutionContext::create(1);
    auto worker = source->make_worker(worker_context);
    auto nested_context = ygg::ExecutionContext::create(1);
    auto nested = worker->make_worker(nested_context);

    const auto worker_state_repository = worker->get_state_repository();
    const auto nested_state_repository = nested->get_state_repository();
    const auto worker_axiom_evaluator = worker_state_repository->get_axiom_evaluator();
    const auto nested_axiom_evaluator = nested_state_repository->get_axiom_evaluator();

    ASSERT_NE(source_axiom_evaluator, nullptr);
    ASSERT_NE(worker_axiom_evaluator, nullptr);
    ASSERT_NE(nested_axiom_evaluator, nullptr);

    expect_distinct(source, worker, nested);
    expect_distinct(source_state_repository, worker_state_repository, nested_state_repository);
    expect_distinct(source_axiom_evaluator, worker_axiom_evaluator, nested_axiom_evaluator);

    if constexpr (std::same_as<Kind, ::tyr::LiftedTag>)
    {
        EXPECT_EQ(&source->get_action_program(), &worker->get_action_program());
        EXPECT_EQ(&source->get_action_program(), &nested->get_action_program());

        EXPECT_EQ(&source_axiom_evaluator->get_axiom_program(), &worker_axiom_evaluator->get_axiom_program());
        EXPECT_EQ(&source_axiom_evaluator->get_axiom_program(), &nested_axiom_evaluator->get_axiom_program());

        EXPECT_EQ(worker_state_repository->get_execution_context(), worker_context);
        EXPECT_EQ(nested_state_repository->get_execution_context(), nested_context);
    }

    EXPECT_EQ(source_state_repository->num_states(), 0);
    EXPECT_EQ(worker_state_repository->num_states(), 0);
    EXPECT_EQ(nested_state_repository->num_states(), 0);

    auto worker_initial_future = std::async(std::launch::async, [&] { return worker->get_initial_node(); });
    auto nested_initial_future = std::async(std::launch::async, [&] { return nested->get_initial_node(); });
    const auto worker_initial = worker_initial_future.get();
    const auto nested_initial = nested_initial_future.get();
    EXPECT_EQ(source_state_repository->num_states(), 0);
    EXPECT_EQ(worker_state_repository->num_states(), 1);
    EXPECT_EQ(nested_state_repository->num_states(), 1);

    const auto source_initial = source->get_initial_node();
    EXPECT_EQ(source_state_repository->num_states(), 1);
    EXPECT_EQ(source_initial.get_state().get_index(), worker_initial.get_state().get_index());
    EXPECT_EQ(source_initial.get_state().get_index(), nested_initial.get_state().get_index());
    EXPECT_NE(source_initial.get_state(), worker_initial.get_state());
    EXPECT_NE(source_initial.get_state(), nested_initial.get_state());
    EXPECT_EQ(source_initial.get_metric(), worker_initial.get_metric());
    EXPECT_EQ(source_initial.get_metric(), nested_initial.get_metric());

    auto initial_fdr_variable_count = size_t { 0 };
    if constexpr (std::same_as<Kind, ::tyr::LiftedTag>)
        initial_fdr_variable_count = task->get_fdr_context()->get_variables().size();

    auto worker_successors_future = std::async(std::launch::async, [&] { return worker->get_labeled_successor_nodes(worker_initial); });
    auto nested_successors_future = std::async(std::launch::async, [&] { return nested->get_labeled_successor_nodes(nested_initial); });
    const auto worker_successors = worker_successors_future.get();
    const auto nested_successors = nested_successors_future.get();

    auto worker_fdr_variable_count = size_t { 0 };
    if constexpr (std::same_as<Kind, ::tyr::LiftedTag>)
    {
        const auto& variables = task->get_fdr_context()->get_variables();
        worker_fdr_variable_count = variables.size();
        EXPECT_GT(worker_fdr_variable_count, initial_fdr_variable_count);

        auto indices = std::unordered_set<ygg::uint_t> {};
        for (const auto variable : variables)
            indices.insert(ygg::uint_t(variable.get_index()));
        EXPECT_EQ(indices.size(), variables.size());
    }

    const auto source_successors = source->get_labeled_successor_nodes(source_initial);
    if constexpr (std::same_as<Kind, ::tyr::LiftedTag>)
    {
        EXPECT_EQ(task->get_fdr_context()->get_variables().size(), worker_fdr_variable_count);
    }
    ASSERT_FALSE(source_successors.empty());
    ASSERT_EQ(source_successors.size(), worker_successors.size());
    ASSERT_EQ(source_successors.size(), nested_successors.size());
    for (size_t i = 0; i < source_successors.size(); ++i)
    {
        EXPECT_EQ(source_successors[i].label, worker_successors[i].label);
        EXPECT_EQ(source_successors[i].label, nested_successors[i].label);
    }

    EXPECT_EQ(source->ground_action(source_successors.front().label), worker->ground_action(worker_successors.front().label));
    EXPECT_EQ(source->ground_action(source_successors.front().label), nested->ground_action(nested_successors.front().label));
    EXPECT_EQ(source_state_repository->num_states(), worker_state_repository->num_states());
    EXPECT_EQ(source_state_repository->num_states(), nested_state_repository->num_states());

    auto blind = p::BlindHeuristic<Kind>::create();
    auto blind_worker = blind->make_worker(worker_context);
    EXPECT_NE(blind.get(), blind_worker.get());
    EXPECT_EQ(blind->evaluate(source_initial.get_state()), blind_worker->evaluate(worker_initial.get_state()));

    auto goal_count = p::GoalCountHeuristic<Kind>::create(task);
    auto goal_count_worker = goal_count->make_worker(worker_context);
    EXPECT_NE(goal_count.get(), goal_count_worker.get());
    EXPECT_EQ(goal_count->evaluate(source_initial.get_state()), goal_count_worker->evaluate(worker_initial.get_state()));
}
}

TEST(TyrPlanningWorkerTest, GroundAndLiftedWorkersOwnMutableStateAndShareDefinitions)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto lifted_task = p::Task<::tyr::LiftedTag>::create(
        make_test_parser(root / "classical/tests/philosophers/domain.pddl").parse_task(root / "classical/tests/philosophers/test-1.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    expect_worker_chain(lifted_task);
    expect_worker_chain(ground_task);
}

}
