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
#include "tyr/planning/planning.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace p = tyr::planning;

namespace tyr::tests
{
namespace
{

struct TaskPair
{
    p::TaskPtr<LiftedTag> lifted;
    p::TaskPtr<GroundTag> ground;
};

TaskPair make_task_pair(p::TaskPtr<LiftedTag> lifted)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto ground = lifted->instantiate_ground_task(*execution_context).task;
    if (!ground)
        throw std::runtime_error("Failed to instantiate parallel-search edge-case task.");
    return { std::move(lifted), std::move(ground) };
}

TaskPair parse_tasks(const std::filesystem::path& problem)
{
    const auto fixture_root = std::filesystem::path(ROOT_DIR) / "tests/fixtures/planning/algorithms";
    return make_task_pair(
        p::Task<LiftedTag>::create(make_test_parser(fixture_root / "parallel_search_edge_cases_domain.pddl").parse_task(fixture_root / problem)));
}

TaskPair parse_weighted_tasks()
{
    const auto fixture_root = std::filesystem::path(ROOT_DIR) / "tests/fixtures/planning/algorithms";
    const auto benchmark_root = std::filesystem::path(BENCHMARKS_DIR);
    return make_task_pair(p::Task<LiftedTag>::create(
        make_test_parser(benchmark_root / "classical/tests/transport/domain.pddl").parse_task(fixture_root / "parallel_astar_weighted.pddl")));
}

template<TaskKind Kind>
struct SearchContext
{
    ygg::ExecutionContextPtr execution_context;
    p::StateRepositoryPtr<Kind> repository;
    p::SuccessorGeneratorPtr<Kind> successor_generator;
};

template<TaskKind Kind>
SearchContext<Kind> make_search_context(const p::TaskPtr<Kind>& task)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto repository = p::StateRepositoryFactory<Kind>().create(task, axiom_evaluator);
    auto successor_generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context, repository);
    return { std::move(execution_context), std::move(repository), std::move(successor_generator) };
}

template<TaskKind Kind>
class GoalAsDeadEndHeuristic final : public p::Heuristic<Kind>
{
public:
    explicit GoalAsDeadEndHeuristic(::tyr::formalism::planning::GroundConjunctiveConditionView goal) : m_goal(goal) {}

    void set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal) override { m_goal = goal; }

    ygg::float_t evaluate(const p::StateView<Kind>& state) override
    {
        const auto context = p::StateContext<Kind>(*state.get_state_repository()->get_task(), state.get_state_builder(), 0);
        return p::is_dynamically_applicable(m_goal, context) ? std::numeric_limits<ygg::float_t>::infinity() : 0;
    }

    p::HeuristicPtr<Kind> make_worker(ygg::ExecutionContextPtr) const override { return std::make_shared<GoalAsDeadEndHeuristic>(m_goal); }

private:
    ::tyr::formalism::planning::GroundConjunctiveConditionView m_goal;
};

template<TaskKind Kind>
class NullWorkerHeuristic final : public p::Heuristic<Kind>
{
public:
    void set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView) override {}
    ygg::float_t evaluate(const p::StateView<Kind>&) override { return 0; }
    p::HeuristicPtr<Kind> make_worker(ygg::ExecutionContextPtr) const override { return nullptr; }
};

template<TaskKind Kind>
class DuplicatePruningStrategy final : public p::PruningStrategy<Kind>
{
public:
    p::PruningStrategyPtr<Kind> make_worker(ygg::Index<p::Worker>) const override { return std::make_shared<DuplicatePruningStrategy>(); }

    bool should_prune_successor_state(const p::StateView<Kind>&, const p::StateView<Kind>&, bool is_new) override { return !is_new; }
};

template<TaskKind Kind>
class AlwaysGoalStrategy final : public p::GoalStrategy<Kind>
{
public:
    p::GoalStrategyPtr<Kind> make_worker(ygg::Index<p::Worker>) const override { return std::make_shared<AlwaysGoalStrategy>(); }
    bool is_static_goal_satisfied(const p::Task<Kind>&) override { return true; }
    bool is_dynamic_goal_satisfied(const p::StateView<Kind>&, const p::StateView<Kind>&) override { return true; }
};

template<typename Callback>
void for_each_execution_mode(Callback&& callback)
{
    callback(size_t { 1 }, p::StateRepositoryMode::HASH_DISTRIBUTED);
    callback(size_t { 2 }, p::StateRepositoryMode::HASH_DISTRIBUTED);
    callback(size_t { 2 }, p::StateRepositoryMode::SHARED);
}

template<TaskKind Kind>
void expect_astar_goal_precedes_heuristic(const p::TaskPtr<Kind>& task)
{
    for_each_execution_mode(
        [&](size_t num_workers, p::StateRepositoryMode mode)
        {
            auto context = make_search_context(task);
            auto heuristic = GoalAsDeadEndHeuristic<Kind>(task->get_task().get_goal());
            auto options = p::astar_eager::Options<Kind> {};
            options.num_search_workers = num_workers;
            options.state_repository_mode = mode;

            const auto result = p::astar_eager::find_solution(*task, *context.successor_generator, heuristic, options);
            ASSERT_EQ(result.status, p::SearchStatus::SOLVED);
            ASSERT_TRUE(result.plan);
            EXPECT_EQ(result.plan->get_length(), 1);
        });
}

template<TaskKind Kind>
void expect_astar_duplicate_pruning_preserves_open_state(const p::TaskPtr<Kind>& task)
{
    for_each_execution_mode(
        [&](size_t num_workers, p::StateRepositoryMode mode)
        {
            auto context = make_search_context(task);
            auto heuristic = p::BlindHeuristic<Kind>::create();
            auto options = p::astar_eager::Options<Kind> {};
            options.num_search_workers = num_workers;
            options.state_repository_mode = mode;
            options.pruning_strategy = std::make_shared<DuplicatePruningStrategy<Kind>>();

            const auto result = p::astar_eager::find_solution(*task, *context.successor_generator, *heuristic, options);
            ASSERT_EQ(result.status, p::SearchStatus::SOLVED);
            ASSERT_TRUE(result.plan);
            EXPECT_EQ(result.plan->get_length(), 3);
        });
}

template<TaskKind Kind>
void expect_astar_pruned_improvement_preserves_existing_state(const p::TaskPtr<Kind>& task)
{
    for_each_execution_mode(
        [&](size_t num_workers, p::StateRepositoryMode mode)
        {
            auto context = make_search_context(task);
            auto heuristic = p::BlindHeuristic<Kind>::create();
            auto options = p::astar_eager::Options<Kind> {};
            options.num_search_workers = num_workers;
            options.state_repository_mode = mode;
            options.pruning_strategy = std::make_shared<DuplicatePruningStrategy<Kind>>();

            const auto result = p::astar_eager::find_solution(*task, *context.successor_generator, *heuristic, options);
            ASSERT_EQ(result.status, p::SearchStatus::SOLVED);
            ASSERT_TRUE(result.plan);
            EXPECT_EQ(result.plan->get_cost(), 100);
        });
}

template<TaskKind Kind>
void expect_root_inclusive_state_limits(const p::TaskPtr<Kind>& task)
{
    for_each_execution_mode(
        [&](size_t num_workers, p::StateRepositoryMode mode)
        {
            const auto solve = [&](ygg::uint_t limit, p::GoalStrategyPtr<Kind> goal_strategy = nullptr)
            {
                auto context = make_search_context(task);
                auto options = p::brfs::Options<Kind> {};
                options.num_search_workers = num_workers;
                options.state_repository_mode = mode;
                options.max_num_states = limit;
                options.goal_strategy = std::move(goal_strategy);
                return p::brfs::find_solution(*task, *context.successor_generator, options);
            };

            EXPECT_EQ(solve(0).status, p::SearchStatus::OUT_OF_STATES);
            EXPECT_EQ(solve(1).status, p::SearchStatus::OUT_OF_STATES);
            EXPECT_EQ(solve(2).status, p::SearchStatus::SOLVED);
            EXPECT_EQ(solve(0, std::make_shared<AlwaysGoalStrategy<Kind>>()).status, p::SearchStatus::SOLVED);
        });

    auto context = make_search_context(task);
    const auto initial = context.successor_generator->get_initial_node();
    auto successors = context.successor_generator->get_labeled_successor_nodes(initial);
    ASSERT_EQ(successors.size(), 1);
    auto options = p::brfs::Options<Kind> {};
    options.max_num_states = 2;
    EXPECT_EQ(p::brfs::find_solution(*task, *context.successor_generator, options).status, p::SearchStatus::SOLVED);
}

template<TaskKind Kind>
void expect_foreign_start_handling(const p::TaskPtr<Kind>& task, const p::TaskPtr<Kind>& other_task)
{
    for_each_execution_mode(
        [&](size_t num_workers, p::StateRepositoryMode mode)
        {
            auto caller = make_search_context(task);
            auto foreign = make_search_context(task);
            auto heuristic = p::BlindHeuristic<Kind>::create();
            auto options = p::astar_eager::Options<Kind> {};
            options.start_node = p::Node<Kind>(foreign.successor_generator->get_initial_node().get_state(), 7);
            options.num_search_workers = num_workers;
            options.state_repository_mode = mode;

            const auto result = p::astar_eager::find_solution(*task, *caller.successor_generator, *heuristic, options);
            ASSERT_EQ(result.status, p::SearchStatus::SOLVED);
            ASSERT_TRUE(result.plan);
            EXPECT_EQ(result.plan->get_start_node().get_state().get_state_repository(), caller.repository);
            EXPECT_EQ(result.plan->get_start_node().get_metric(), 7);
        });

    auto caller = make_search_context(task);
    auto foreign = make_search_context(other_task);
    auto heuristic = p::BlindHeuristic<Kind>::create();
    auto options = p::astar_eager::Options<Kind> {};
    options.start_node = foreign.successor_generator->get_initial_node();
    EXPECT_THROW(p::astar_eager::find_solution(*task, *caller.successor_generator, *heuristic, options), std::invalid_argument);
}

template<TaskKind Kind>
void expect_null_heuristic_worker_is_rejected(const p::TaskPtr<Kind>& task)
{
    auto context = make_search_context(task);
    auto heuristic = NullWorkerHeuristic<Kind> {};
    auto options = p::astar_eager::Options<Kind> {};
    options.num_search_workers = 2;
    EXPECT_THROW(p::astar_eager::find_solution(*task, *context.successor_generator, heuristic, options), std::invalid_argument);
}

template<TaskKind Kind>
void expect_destination_lock_statistics(const p::TaskPtr<Kind>& task)
{
    for (const auto mode : { p::StateRepositoryMode::HASH_DISTRIBUTED, p::StateRepositoryMode::SHARED })
    {
        for (const auto collect : { false, true })
        {
            auto context = make_search_context(task);
            auto options = p::brfs::Options<Kind> {};
            options.num_search_workers = 2;
            options.state_repository_mode = mode;
            options.collect_destination_lock_statistics = collect;

            const auto result = p::brfs::find_solution(*task, *context.successor_generator, options);
            ASSERT_EQ(result.status, p::SearchStatus::SOLVED);

            auto acquisitions = uint64_t { 0 };
            auto wait_time = std::chrono::nanoseconds { 0 };
            auto hold_time = std::chrono::nanoseconds { 0 };
            for (const auto& worker : result.worker_statistics)
            {
                acquisitions += worker.get_num_destination_lock_acquisitions();
                wait_time += worker.get_destination_lock_wait_time();
                hold_time += worker.get_destination_lock_hold_time();
            }

            EXPECT_EQ(acquisitions, result.statistics.get_num_destination_lock_acquisitions());
            EXPECT_EQ(wait_time, result.statistics.get_destination_lock_wait_time());
            EXPECT_EQ(hold_time, result.statistics.get_destination_lock_hold_time());
            if (collect)
                EXPECT_GT(acquisitions, 0);
            else
            {
                EXPECT_EQ(acquisitions, 0);
                EXPECT_EQ(wait_time, std::chrono::nanoseconds(0));
                EXPECT_EQ(hold_time, std::chrono::nanoseconds(0));
            }
        }
    }
}

TEST(TyrPlanningSearchEngineTest, ChecksGoalsBeforeHeuristics)
{
    const auto tasks = parse_tasks("parallel_search_simple.pddl");
    expect_astar_goal_precedes_heuristic(tasks.ground);
    expect_astar_goal_precedes_heuristic(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, PrunedDuplicatesDoNotCloseAcceptedAStarStates)
{
    const auto tasks = parse_tasks("parallel_search_diamond.pddl");
    expect_astar_duplicate_pruning_preserves_open_state(tasks.ground);
    expect_astar_duplicate_pruning_preserves_open_state(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, PrunedImprovementsDoNotOverwriteAcceptedAStarStates)
{
    const auto tasks = parse_weighted_tasks();
    expect_astar_pruned_improvement_preserves_existing_state(tasks.ground);
    expect_astar_pruned_improvement_preserves_existing_state(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, StateLimitsAreRootInclusiveAndSolveLocal)
{
    const auto tasks = parse_tasks("parallel_search_simple.pddl");
    expect_root_inclusive_state_limits(tasks.ground);
    expect_root_inclusive_state_limits(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, MaterializesCompatibleForeignStartsAndRejectsDifferentTasks)
{
    const auto tasks = parse_tasks("parallel_search_simple.pddl");
    const auto other_tasks = parse_tasks("parallel_search_simple.pddl");
    expect_foreign_start_handling(tasks.ground, other_tasks.ground);
    expect_foreign_start_handling(tasks.lifted, other_tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, RejectsNullParallelHeuristicWorkers)
{
    const auto tasks = parse_tasks("parallel_search_simple.pddl");
    expect_null_heuristic_worker_is_rejected(tasks.ground);
    expect_null_heuristic_worker_is_rejected(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, DestinationLockStatisticsAreOptInAndAggregated)
{
    const auto tasks = parse_tasks("parallel_search_simple.pddl");
    expect_destination_lock_statistics(tasks.ground);
    expect_destination_lock_statistics(tasks.lifted);
}

}
}
