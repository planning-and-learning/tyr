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

#include <barrier>
#include <concepts>
#include <filesystem>
#include <future>
#include <gtest/gtest.h>
#include <memory>
#include <type_traits>
#include <unordered_set>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
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
    auto source_state_repository = state_repository_factory.create(task);
    auto source = successor_generator_factory.create(task, source_context);

    auto worker_context = ygg::ExecutionContext::create(1);
    auto worker_axiom_evaluator = source_axiom_evaluator->make_worker(worker_context);
    auto worker_state_repository = source_state_repository->make_worker();
    auto worker = source->make_worker(worker_context);
    ASSERT_NE(worker_axiom_evaluator, nullptr);
    ASSERT_NE(worker_state_repository, nullptr);
    ASSERT_NE(worker, nullptr);
    auto nested_context = ygg::ExecutionContext::create(1);
    auto nested_axiom_evaluator = worker_axiom_evaluator->make_worker(nested_context);
    auto nested_state_repository = worker_state_repository->make_worker();
    auto nested = worker->make_worker(nested_context);
    ASSERT_NE(nested_axiom_evaluator, nullptr);
    ASSERT_NE(nested_state_repository, nullptr);
    ASSERT_NE(nested, nullptr);

    ASSERT_NE(source_axiom_evaluator, nullptr);
    ASSERT_NE(worker_axiom_evaluator, nullptr);
    ASSERT_NE(nested_axiom_evaluator, nullptr);

    expect_distinct(source, worker, nested);
    expect_distinct(source_state_repository, worker_state_repository, nested_state_repository);
    expect_distinct(source_axiom_evaluator, worker_axiom_evaluator, nested_axiom_evaluator);

    EXPECT_EQ(worker_axiom_evaluator->get_execution_context(), worker_context);
    EXPECT_EQ(nested_axiom_evaluator->get_execution_context(), nested_context);

    if constexpr (std::same_as<Kind, ::tyr::LiftedTag>)
    {
        EXPECT_EQ(&source->get_action_program(), &worker->get_action_program());
        EXPECT_EQ(&source->get_action_program(), &nested->get_action_program());
    }

    EXPECT_EQ(source_state_repository->num_states(), 0);
    EXPECT_EQ(worker_state_repository->num_states(), 0);
    EXPECT_EQ(nested_state_repository->num_states(), 0);

    auto worker_initial_future = std::async(std::launch::async, [&] { return worker->get_initial_node(*worker_state_repository, *worker_axiom_evaluator); });
    auto nested_initial_future = std::async(std::launch::async, [&] { return nested->get_initial_node(*nested_state_repository, *nested_axiom_evaluator); });
    const auto worker_initial = worker_initial_future.get();
    const auto nested_initial = nested_initial_future.get();
    EXPECT_EQ(source_state_repository->num_states(), 0);
    EXPECT_EQ(worker_state_repository->num_states(), 1);
    EXPECT_EQ(nested_state_repository->num_states(), 1);

    const auto source_initial = source->get_initial_node(*source_state_repository, *source_axiom_evaluator);
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

    auto worker_successors_future =
        std::async(std::launch::async, [&] { return worker->get_labeled_successor_nodes(worker_initial, *worker_state_repository, *worker_axiom_evaluator); });
    auto nested_successors_future =
        std::async(std::launch::async, [&] { return nested->get_labeled_successor_nodes(nested_initial, *nested_state_repository, *nested_axiom_evaluator); });
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

    const auto source_successors = source->get_labeled_successor_nodes(source_initial, *source_state_repository, *source_axiom_evaluator);
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

    const auto worker_index = ygg::Index<p::Worker>(0);

    auto pruning = p::PruningStrategy<Kind>::create();
    auto pruning_worker = pruning->make_worker(worker_index);
    ASSERT_NE(pruning_worker, nullptr);
    EXPECT_NE(pruning.get(), pruning_worker.get());
    EXPECT_FALSE(pruning_worker->should_prune_state(worker_initial.get_state()));

    auto conjunctive_goal = p::ConjunctiveGoalStrategy<Kind>::create(*task);
    auto conjunctive_goal_worker = conjunctive_goal->make_worker(worker_index);
    ASSERT_NE(conjunctive_goal_worker, nullptr);
    EXPECT_NE(conjunctive_goal.get(), conjunctive_goal_worker.get());
    EXPECT_EQ(conjunctive_goal->is_dynamic_goal_satisfied(source_initial.get_state(), source_initial.get_state()),
              conjunctive_goal_worker->is_dynamic_goal_satisfied(worker_initial.get_state(), worker_initial.get_state()));

    auto serialized_goal = p::SerializedGoalStrategy<Kind>::create(*task);
    auto serialized_goal_worker = serialized_goal->make_worker(worker_index);
    ASSERT_NE(serialized_goal_worker, nullptr);
    EXPECT_NE(serialized_goal.get(), serialized_goal_worker.get());
    EXPECT_EQ(serialized_goal->is_dynamic_goal_satisfied(source_initial.get_state(), source_initial.get_state()),
              serialized_goal_worker->is_dynamic_goal_satisfied(worker_initial.get_state(), worker_initial.get_state()));

    auto exhaustive_goal = p::ExhaustiveGoalStrategy<Kind>::create();
    auto exhaustive_goal_worker = exhaustive_goal->make_worker(worker_index);
    ASSERT_NE(exhaustive_goal_worker, nullptr);
    EXPECT_NE(exhaustive_goal.get(), exhaustive_goal_worker.get());
    EXPECT_FALSE(exhaustive_goal_worker->is_dynamic_goal_satisfied(worker_initial.get_state(), worker_initial.get_state()));
}

template<::tyr::TaskKind Kind>
void expect_independent_repository_identity(const p::TaskPtr<Kind>& task)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto first_axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto second_axiom_evaluator = first_axiom_evaluator->make_worker(execution_context);
    auto first_repository = p::StateRepositoryFactory<Kind>().create(task);
    auto second_repository = p::StateRepositoryFactory<Kind>().create(task);

    const auto first_state = first_repository->get_initial_state(*first_axiom_evaluator);
    const auto second_state = second_repository->get_initial_state(*second_axiom_evaluator);

    EXPECT_EQ(first_repository->get_index(), 0);
    EXPECT_EQ(second_repository->get_index(), 0);
    EXPECT_EQ(first_state.get_index(), second_state.get_index());
    EXPECT_NE(first_state, second_state);

    auto states = ygg::UnorderedSet<p::StateView<Kind>> {};
    states.insert(first_state);
    states.insert(second_state);
    EXPECT_EQ(states.size(), 2);

    EXPECT_EQ(p::materialize_state(second_state, *first_repository, *first_axiom_evaluator), first_state);
}

template<::tyr::TaskKind Kind>
void expect_shared_worker_cohort(const p::TaskPtr<Kind>& task)
{
    auto source_context = ygg::ExecutionContext::create(1);
    auto source_axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, source_context);
    auto repository_factory = p::StateRepositoryFactory<Kind> {};
    auto source_repository = repository_factory.create_concurrent(task);
    auto other_owner_repository = repository_factory.create_concurrent(task);
    auto other_owner_worker_repository = other_owner_repository->make_worker();
    auto source = p::SuccessorGeneratorFactory<Kind>().create(task, source_context);
    auto worker_context = ygg::ExecutionContext::create(1);
    auto worker_axiom_evaluator = source_axiom_evaluator->make_worker(worker_context);
    auto worker_repository = source_repository->make_worker();
    auto worker = source->make_worker(worker_context);
    ASSERT_NE(worker_axiom_evaluator, nullptr);
    ASSERT_NE(worker_repository, nullptr);
    ASSERT_NE(other_owner_worker_repository, nullptr);
    ASSERT_NE(worker, nullptr);

    const auto& first_repository = source_repository;
    const auto& second_repository = worker_repository;
    EXPECT_NE(source.get(), worker.get());
    EXPECT_NE(first_repository.get(), second_repository.get());
    EXPECT_NE(source->get_index(), worker->get_index());
    EXPECT_NE(first_repository->get_index(), second_repository->get_index());
    EXPECT_TRUE(first_repository->shares_storage_with(*second_repository));
    EXPECT_TRUE(first_repository->is_concurrent());
    EXPECT_TRUE(second_repository->is_concurrent());
    EXPECT_TRUE(other_owner_repository->is_concurrent());
    EXPECT_TRUE(other_owner_worker_repository->is_concurrent());
    EXPECT_FALSE(first_repository->shares_storage_with(*other_owner_repository));
    EXPECT_FALSE(second_repository->shares_storage_with(*other_owner_worker_repository));
    EXPECT_TRUE(other_owner_repository->shares_storage_with(*other_owner_worker_repository));

    auto initial_start = std::barrier(2);
    auto first_initial_future = std::async(std::launch::async,
                                           [&]
                                           {
                                               initial_start.arrive_and_wait();
                                               return source->get_initial_node(*source_repository, *source_axiom_evaluator);
                                           });
    auto second_initial_future = std::async(std::launch::async,
                                            [&]
                                            {
                                                initial_start.arrive_and_wait();
                                                return worker->get_initial_node(*worker_repository, *worker_axiom_evaluator);
                                            });
    const auto first_initial = first_initial_future.get();
    const auto second_initial = second_initial_future.get();
    ASSERT_EQ(first_repository->num_states(), 1);
    ASSERT_EQ(second_repository->num_states(), 1);
    EXPECT_EQ(first_initial.get_state(), second_initial.get_state());
    EXPECT_EQ(first_initial.get_metric(), second_initial.get_metric());
    EXPECT_EQ(p::materialize_state(second_initial.get_state(), *source_repository, *source_axiom_evaluator).get_state_repository(), source_repository);

    auto novelty = p::iw::NoveltyPruningStrategy<Kind>::create(0);
    auto first_novelty_worker = novelty->make_worker(ygg::Index<p::Worker>(0));
    auto second_novelty_worker = novelty->make_worker(ygg::Index<p::Worker>(1));
    ASSERT_NE(first_novelty_worker, nullptr);
    ASSERT_NE(second_novelty_worker, nullptr);
    EXPECT_FALSE(first_novelty_worker->should_prune_state(first_initial.get_state()));
    EXPECT_FALSE(second_novelty_worker->should_prune_state(second_initial.get_state()));

    const auto actions = source->get_applicable_action_bindings(first_initial);
    ASSERT_FALSE(actions.empty());
    const auto action = actions.front();
    auto successor_start = std::barrier(2);
    auto first_future = std::async(std::launch::async,
                                   [&]
                                   {
                                       successor_start.arrive_and_wait();
                                       return source->get_successor_node(first_initial, action, *source_repository, *source_axiom_evaluator);
                                   });
    auto second_future = std::async(std::launch::async,
                                    [&]
                                    {
                                        successor_start.arrive_and_wait();
                                        return worker->get_successor_node(second_initial, action, *worker_repository, *worker_axiom_evaluator);
                                    });
    const auto first_successor = first_future.get();
    const auto second_successor = second_future.get();

    EXPECT_EQ(first_successor.get_state().get_index(), ygg::Index<p::State<Kind>>(1));
    EXPECT_EQ(first_successor.get_state(), second_successor.get_state());
    EXPECT_EQ(first_successor.get_metric(), second_successor.get_metric());
    EXPECT_EQ(first_repository->num_states(), 2);
    EXPECT_EQ(second_repository->num_states(), 2);
    EXPECT_EQ(first_repository->get_registered_state(first_successor.get_state().get_index()),
              second_repository->get_registered_state(second_successor.get_state().get_index()));
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
    expect_independent_repository_identity(lifted_task);
    expect_independent_repository_identity(ground_task);
    expect_shared_worker_cohort(lifted_task);
    expect_shared_worker_cohort(ground_task);
}

TEST(TyrPlanningWorkerTest, UtilizationUsesAggregateWorkerCapacity)
{
    auto result = p::SearchResult<::tyr::GroundTag> {};
    const auto start = std::chrono::steady_clock::time_point {};
    result.statistics.set_search_start_time_point(start);
    result.statistics.set_search_end_time_point(start + std::chrono::nanoseconds(100));
    result.statistics.add_idle_time(std::chrono::nanoseconds(100));
    result.worker_statistics.resize(4);

    EXPECT_DOUBLE_EQ(static_cast<double>(result.get_worker_utilization()), 0.75);

    result.worker_statistics.clear();
    EXPECT_DOUBLE_EQ(static_cast<double>(result.get_worker_utilization()), 0.0);
}

}
