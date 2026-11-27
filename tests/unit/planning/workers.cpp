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
#include "planning/successors/successor_compositions.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/planning/algorithms/iw.hpp"
#include "tyr/planning/algorithms/iw/event_handler.hpp"
#include "tyr/planning/algorithms/strategies/goal.hpp"
#include "tyr/planning/algorithms/strategies/pruning.hpp"
#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/axioms/axiom_evaluator.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/successors/ground/successor_generator.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/heuristics/blind.hpp"
#include "tyr/planning/heuristics/goal_count.hpp"
#include "tyr/planning/successors/lifted/successor_generator.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"
#include "tyr/planning/successors/successors.hpp"

#include <barrier>
#include <concepts>
#include <filesystem>
#include <future>
#include <gtest/gtest.h>
#include <memory>
#include <type_traits>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/values/std_set.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <vector>
#include <yggdrasil/execution/onetbb.hpp>

static_assert(tyr::formalism::Repository::thread_safe);

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
}

template<TaskKind Kind>
void expect_worker_chain(const tyr::planning::TaskPtr<Kind>& task)
{
    ASSERT_TRUE(task->has_axioms());

    auto source_context = ygg::ExecutionContext::create(1);
    auto source_axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, source_context);
    auto source_state_repository = tyr::planning::StateRepository<Kind>::create(task);
    auto source = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, source_context);

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

    if constexpr (std::same_as<Kind, LiftedTag>)
    {
        EXPECT_EQ(&source->get_action_program(), &worker->get_action_program());
        EXPECT_EQ(&source->get_action_program(), &nested->get_action_program());
    }

    EXPECT_EQ(source_state_repository->num_states(), 0);
    EXPECT_EQ(worker_state_repository->num_states(), 0);
    EXPECT_EQ(nested_state_repository->num_states(), 0);

    auto worker_initial_future =
        std::async(std::launch::async, [&] { return tyr::tests::owned_initial_node(*worker_state_repository, *worker_axiom_evaluator); });
    auto nested_initial_future =
        std::async(std::launch::async, [&] { return tyr::tests::owned_initial_node(*nested_state_repository, *nested_axiom_evaluator); });
    const auto worker_initial = worker_initial_future.get();
    const auto nested_initial = nested_initial_future.get();
    EXPECT_EQ(source_state_repository->num_states(), 0);
    EXPECT_EQ(worker_state_repository->num_states(), 1);
    EXPECT_EQ(nested_state_repository->num_states(), 1);

    const auto source_initial = tyr::tests::owned_initial_node(*source_state_repository, *source_axiom_evaluator);
    EXPECT_EQ(source_state_repository->num_states(), 1);
    EXPECT_EQ(tyr::tests::built_state(source_initial).pack().index(), tyr::tests::built_state(worker_initial).pack().index());
    EXPECT_EQ(tyr::tests::built_state(source_initial).pack().index(), tyr::tests::built_state(nested_initial).pack().index());
    EXPECT_NE(source_initial.get_state(), worker_initial.get_state());
    EXPECT_NE(source_initial.get_state(), nested_initial.get_state());
    EXPECT_EQ(source_initial.get_metric(), worker_initial.get_metric());
    EXPECT_EQ(source_initial.get_metric(), nested_initial.get_metric());

    auto initial_fdr_variable_count = size_t { 0 };
    if constexpr (std::same_as<Kind, LiftedTag>)
        initial_fdr_variable_count = task->get_fdr_context()->get_variables().size();

    auto worker_successors_future =
        std::async(std::launch::async,
                   [&] { return tyr::tests::get_labeled_successor_nodes(*worker, worker_initial, {}, *worker_state_repository, *worker_axiom_evaluator); });
    auto nested_successors_future =
        std::async(std::launch::async,
                   [&] { return tyr::tests::get_labeled_successor_nodes(*nested, nested_initial, {}, *nested_state_repository, *nested_axiom_evaluator); });
    const auto worker_successors = worker_successors_future.get();
    const auto nested_successors = nested_successors_future.get();

    auto worker_fdr_variable_count = size_t { 0 };
    if constexpr (std::same_as<Kind, LiftedTag>)
    {
        const auto& variables = task->get_fdr_context()->get_variables();
        worker_fdr_variable_count = variables.size();
        EXPECT_GT(worker_fdr_variable_count, initial_fdr_variable_count);

        auto indices = urd::Store<urd::set<ygg::uint_t>> {};
        for (const auto variable : variables)
            indices.insert(ygg::uint_t(variable.index().value));
        EXPECT_EQ(indices.size(), variables.size());
    }

    const auto source_successors = tyr::tests::get_labeled_successor_nodes(*source, source_initial, {}, *source_state_repository, *source_axiom_evaluator);
    if constexpr (std::same_as<Kind, LiftedTag>)
    {
        EXPECT_EQ(task->get_fdr_context()->get_variables().size(), worker_fdr_variable_count);
    }
    ASSERT_FALSE(source_successors.empty());
    ASSERT_EQ(source_successors.size(), worker_successors.size());
    ASSERT_EQ(source_successors.size(), nested_successors.size());
    for (size_t i = 0; i < source_successors.size(); ++i)
    {
        EXPECT_EQ(source_successors[i].get_label(), worker_successors[i].get_label());
        EXPECT_EQ(source_successors[i].get_label(), nested_successors[i].get_label());
    }

    EXPECT_EQ(source->ground_action(urd::interned_view(source_successors.front().get_label())), worker->ground_action(urd::interned_view(worker_successors.front().get_label())));
    EXPECT_EQ(source->ground_action(urd::interned_view(source_successors.front().get_label())), nested->ground_action(urd::interned_view(nested_successors.front().get_label())));
    EXPECT_EQ(source_state_repository->num_states(), worker_state_repository->num_states());
    EXPECT_EQ(source_state_repository->num_states(), nested_state_repository->num_states());

    auto blind = tyr::planning::BlindHeuristic<Kind>::create();
    auto blind_worker = blind->make_worker(worker_context);
    EXPECT_NE(blind.get(), blind_worker.get());
    EXPECT_EQ(blind->evaluate(tyr::tests::built_state(source_initial)), blind_worker->evaluate(tyr::tests::built_state(worker_initial)));

    auto goal_count = tyr::planning::GoalCountHeuristic<Kind>::create(task);
    auto goal_count_worker = goal_count->make_worker(worker_context);
    EXPECT_NE(goal_count.get(), goal_count_worker.get());
    EXPECT_EQ(goal_count->evaluate(tyr::tests::built_state(source_initial)), goal_count_worker->evaluate(tyr::tests::built_state(worker_initial)));

    const auto worker_index = urd::index<tyr::planning::Worker>(0);

    auto pruning = tyr::planning::PruningStrategy<Kind>::create();
    auto pruning_worker = pruning->make_worker(worker_index);
    ASSERT_NE(pruning_worker, nullptr);
    EXPECT_NE(pruning.get(), pruning_worker.get());
    EXPECT_FALSE(pruning_worker->should_prune_state(tyr::tests::built_state(worker_initial)));

    auto conjunctive_goal = tyr::planning::ConjunctiveGoalStrategy<Kind>::create(*task);
    auto conjunctive_goal_worker = conjunctive_goal->make_worker(worker_index);
    ASSERT_NE(conjunctive_goal_worker, nullptr);
    EXPECT_NE(conjunctive_goal.get(), conjunctive_goal_worker.get());
    EXPECT_EQ(conjunctive_goal->is_dynamic_goal_satisfied(tyr::tests::built_state(source_initial), tyr::tests::built_state(source_initial)),
              conjunctive_goal_worker->is_dynamic_goal_satisfied(tyr::tests::built_state(worker_initial), tyr::tests::built_state(worker_initial)));

    auto serialized_goal = tyr::planning::SerializedGoalStrategy<Kind>::create(*task);
    auto serialized_goal_worker = serialized_goal->make_worker(worker_index);
    ASSERT_NE(serialized_goal_worker, nullptr);
    EXPECT_NE(serialized_goal.get(), serialized_goal_worker.get());
    EXPECT_EQ(serialized_goal->is_dynamic_goal_satisfied(tyr::tests::built_state(source_initial), tyr::tests::built_state(source_initial)),
              serialized_goal_worker->is_dynamic_goal_satisfied(tyr::tests::built_state(worker_initial), tyr::tests::built_state(worker_initial)));

    auto exhaustive_goal = tyr::planning::ExhaustiveGoalStrategy<Kind>::create();
    auto exhaustive_goal_worker = exhaustive_goal->make_worker(worker_index);
    ASSERT_NE(exhaustive_goal_worker, nullptr);
    EXPECT_NE(exhaustive_goal.get(), exhaustive_goal_worker.get());
    EXPECT_FALSE(exhaustive_goal_worker->is_dynamic_goal_satisfied(tyr::tests::built_state(worker_initial), tyr::tests::built_state(worker_initial)));

    // A state being built is a goal exactly where it is, registered.
    auto pool = urd::Store<urd::slots<urd::builder<tyr::planning::State<Kind>, tyr::planning::StateRepository<Kind>>>> {};
    auto building = pool.make_unique();
    building->assign(tyr::tests::built_state(source_initial));
    const auto built = urd::resolve(*building, *source_state_repository);
    static_assert(std::same_as<std::remove_cvref_t<decltype(built)>, tyr::planning::StateBuilderView<Kind>>);
    for (const auto& goal : { tyr::planning::GoalStrategyPtr<Kind>(conjunctive_goal),
                              tyr::planning::GoalStrategyPtr<Kind>(serialized_goal),
                              tyr::planning::GoalStrategyPtr<Kind>(exhaustive_goal) })
        EXPECT_EQ(goal->is_dynamic_goal_satisfied(tyr::tests::built_state(source_initial), built),
                  goal->is_dynamic_goal_satisfied(tyr::tests::built_state(source_initial), tyr::tests::built_state(source_initial)));
}

template<TaskKind Kind>
void expect_independent_repository_identity(const tyr::planning::TaskPtr<Kind>& task)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto first_axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto second_axiom_evaluator = first_axiom_evaluator->make_worker(execution_context);
    auto first_repository = tyr::planning::StateRepository<Kind>::create(task);
    auto second_repository = tyr::planning::StateRepository<Kind>::create(task);

    const auto first_state = first_repository->share(first_repository->get_initial_state(*first_axiom_evaluator));
    const auto second_state = second_repository->share(second_repository->get_initial_state(*second_axiom_evaluator));

    EXPECT_EQ(first_repository->get_worker_index(), 0);
    EXPECT_EQ(second_repository->get_worker_index(), 0);
    EXPECT_EQ(first_state.pack().index(), second_state.pack().index());
    EXPECT_NE(first_state, second_state);
    EXPECT_NE(first_state.pack(), second_state.pack());

    auto states = urd::Store<urd::set<tyr::planning::SharedStateBuilderView<Kind>>> {};
    states.insert(first_state);
    states.insert(second_state);
    EXPECT_EQ(states.size(), 2);

    EXPECT_EQ(first_repository->share(tyr::planning::materialize_state(second_state, *first_repository, *first_axiom_evaluator)), first_state);
}

template<TaskKind Kind>
void expect_shared_worker_cohort(const tyr::planning::TaskPtr<Kind>& task)
{
    auto source_context = ygg::ExecutionContext::create(1);
    auto source_axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, source_context);
    auto source_repository = tyr::planning::StateRepository<Kind>::create(task, true);
    auto other_owner_repository = tyr::planning::StateRepository<Kind>::create(task, true);
    auto other_owner_worker_repository = other_owner_repository->make_worker();
    auto source = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, source_context);
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
    EXPECT_NE(first_repository->get_worker_index(), second_repository->get_worker_index());
    EXPECT_EQ(first_repository->layer(), second_repository->layer());
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
                                               return tyr::tests::owned_initial_node(*source_repository, *source_axiom_evaluator);
                                           });
    auto second_initial_future = std::async(std::launch::async,
                                            [&]
                                            {
                                                initial_start.arrive_and_wait();
                                                return tyr::tests::owned_initial_node(*worker_repository, *worker_axiom_evaluator);
                                            });
    const auto first_initial = first_initial_future.get();
    const auto second_initial = second_initial_future.get();
    ASSERT_EQ(first_repository->num_states(), 1);
    ASSERT_EQ(second_repository->num_states(), 1);
    EXPECT_EQ(first_initial.get_state(), second_initial.get_state());
    EXPECT_EQ(tyr::tests::built_state(first_initial).pack(), tyr::tests::built_state(second_initial).pack());
    EXPECT_EQ(urd::Hash<tyr::planning::StateView<Kind>> {}(tyr::tests::built_state(first_initial).pack()),
              urd::Hash<tyr::planning::StateView<Kind>> {}(tyr::tests::built_state(second_initial).pack()));
    EXPECT_EQ(first_initial.get_metric(), second_initial.get_metric());
    EXPECT_FALSE(tyr::planning::materialize_state(tyr::tests::built_state(second_initial), *source_repository, *source_axiom_evaluator)->index.is_none());

    auto novelty = tyr::planning::iw::NoveltyPruningStrategy<Kind>::create(0);
    auto first_novelty_worker = novelty->make_worker(urd::index<tyr::planning::Worker>(0));
    auto second_novelty_worker = novelty->make_worker(urd::index<tyr::planning::Worker>(1));
    ASSERT_NE(first_novelty_worker, nullptr);
    ASSERT_NE(second_novelty_worker, nullptr);
    EXPECT_FALSE(first_novelty_worker->should_prune_state(tyr::tests::built_state(first_initial)));
    EXPECT_FALSE(second_novelty_worker->should_prune_state(tyr::tests::built_state(second_initial)));

    const auto actions = tyr::tests::get_applicable_action_bindings(*source, first_initial, {});
    ASSERT_FALSE(actions.empty());
    const auto action = actions.front();
    auto successor_start = std::barrier(2);
    auto first_future = std::async(std::launch::async,
                                   [&]
                                   {
                                       successor_start.arrive_and_wait();
                                       return tyr::tests::get_successor_node(*source, first_initial, action, *source_repository, *source_axiom_evaluator);
                                   });
    auto second_future = std::async(std::launch::async,
                                    [&]
                                    {
                                        successor_start.arrive_and_wait();
                                        return tyr::tests::get_successor_node(*worker, second_initial, action, *worker_repository, *worker_axiom_evaluator);
                                    });
    const auto first_successor = first_future.get();
    const auto second_successor = second_future.get();

    EXPECT_EQ(tyr::tests::built_state(first_successor).pack().index(), urd::index<tyr::planning::State<Kind>>(1));
    EXPECT_EQ(first_successor.get_state(), second_successor.get_state());
    EXPECT_EQ(first_successor.get_metric(), second_successor.get_metric());
    EXPECT_EQ(first_repository->num_states(), 2);
    EXPECT_EQ(second_repository->num_states(), 2);
    EXPECT_EQ(first_repository->share(first_repository->get_registered_state(tyr::tests::built_state(first_successor).pack().index())),
              second_repository->share(second_repository->get_registered_state(tyr::tests::built_state(second_successor).pack().index())));
}
}

TEST(TyrPlanningWorkerTest, GroundAndLiftedWorkersOwnMutableStateAndShareDefinitions)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto lifted_task = tyr::planning::Task<LiftedTag>::create(
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
    auto result = tyr::planning::SearchResult<GroundTag> {};
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
