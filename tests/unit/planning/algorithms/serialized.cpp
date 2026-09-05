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

#include "planning/parser.hpp"

#include <algorithm>
#include <chrono>
#include <deque>
#include <filesystem>
#include <gtest/gtest.h>
#include <limits>
#include <stdexcept>
#include <thread>
#include <tyr/formalism/formalism.hpp>
#include <tyr/planning/planning.hpp>
#include <vector>
#include <yggdrasil/serialization/json.hpp>
#include <yggdrasil/serialization/json_suite.hpp>

namespace p = tyr::planning;
namespace fp = tyr::formalism::planning;

namespace tyr::tests
{
namespace
{
struct GroundSearchContext
{
    p::TaskPtr<::tyr::GroundTag> task;
    p::StateRepositoryPtr<::tyr::GroundTag> state_repository;
    p::AxiomEvaluatorPtr<::tyr::GroundTag> axiom_evaluator;
    p::SuccessorGeneratorPtr<::tyr::GroundTag> successor_generator;
};

GroundSearchContext create_gripper_context()
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    const auto domain_file = root / "classical/tests/gripper/domain.pddl";
    const auto task_file = root / "classical/tests/gripper/test-1.pddl";

    auto execution_context = ygg::ExecutionContext::create(1);
    auto task = p::Task<::tyr::LiftedTag>(make_test_parser(domain_file).parse_task(task_file)).instantiate_ground_task(*execution_context).task;
    auto axiom_evaluator = p::AxiomEvaluatorFactory<::tyr::GroundTag>().create(task, execution_context);
    auto state_repository = p::StateRepositoryFactory<::tyr::GroundTag>().create(task);
    auto successor_generator = p::SuccessorGeneratorFactory<::tyr::GroundTag>().create(task, execution_context);

    return GroundSearchContext { std::move(task), std::move(state_repository), std::move(axiom_evaluator), std::move(successor_generator) };
}

GroundSearchContext make_worker_context(const GroundSearchContext& source)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    return GroundSearchContext { source.task,
                                 source.state_repository->make_worker(),
                                 source.axiom_evaluator->make_worker(execution_context),
                                 source.successor_generator->make_worker(execution_context) };
}

class NeverSatisfiedGoalStrategy : public p::GoalStrategy<::tyr::GroundTag>
{
public:
    bool is_static_goal_satisfied(const p::Task<::tyr::GroundTag>& task) override
    {
        static_cast<void>(task);
        return true;
    }

    bool is_dynamic_goal_satisfied(const p::StateView<::tyr::GroundTag>& seed_state, const ygg::Builder<p::State<::tyr::GroundTag>>& state) override
    {
        static_cast<void>(seed_state);
        static_cast<void>(state);
        return false;
    }
};

class SatisfiedGoalStrategy : public NeverSatisfiedGoalStrategy
{
public:
    bool is_dynamic_goal_satisfied(const p::StateView<::tyr::GroundTag>&, const ygg::Builder<p::State<::tyr::GroundTag>>&) override { return true; }
};

class StaticallyImpossibleGoalStrategy : public NeverSatisfiedGoalStrategy
{
public:
    bool is_static_goal_satisfied(const p::Task<::tyr::GroundTag>&) override { return false; }
};

class TrackingGoalStrategy : public NeverSatisfiedGoalStrategy
{
public:
    size_t num_static_calls = 0;
    std::vector<const p::StateRepository<::tyr::GroundTag>*> seed_repositories;
    std::vector<ygg::Index<p::State<::tyr::GroundTag>>> seed_indices;

    bool is_static_goal_satisfied(const p::Task<::tyr::GroundTag>&) override
    {
        ++num_static_calls;
        return true;
    }

    bool is_dynamic_goal_satisfied(const p::StateView<::tyr::GroundTag>& seed_state, const ygg::Builder<p::State<::tyr::GroundTag>>&) override
    {
        seed_repositories.push_back(seed_state.get_state_repository().get());
        seed_indices.push_back(seed_state.get_index());
        return false;
    }
};

class AlwaysPruningStrategy : public p::PruningStrategy<::tyr::GroundTag>
{
public:
    bool should_prune_state(const p::StateView<::tyr::GroundTag>&) override { return true; }
    bool should_prune_successor_state(const p::StateView<::tyr::GroundTag>&, const p::StateView<::tyr::GroundTag>&, bool) override { return true; }
};

class ScriptedSolver
{
private:
    std::shared_ptr<std::deque<p::SearchResult<::tyr::GroundTag>>> m_results;
    p::TaskPtr<::tyr::GroundTag> m_task;
    p::StateRepositoryPtr<::tyr::GroundTag> m_state_repository;
    p::AxiomEvaluatorPtr<::tyr::GroundTag> m_axiom_evaluator;

public:
    using EventHandlerType = p::brfs::EventHandler<::tyr::GroundTag>;

    p::brfs::Options<::tyr::GroundTag> options;

    explicit ScriptedSolver(std::deque<p::SearchResult<::tyr::GroundTag>> results,
                            p::TaskPtr<::tyr::GroundTag> task = nullptr,
                            p::StateRepositoryPtr<::tyr::GroundTag> state_repository = nullptr,
                            p::AxiomEvaluatorPtr<::tyr::GroundTag> axiom_evaluator = nullptr) :
        m_results(std::make_shared<std::deque<p::SearchResult<::tyr::GroundTag>>>(std::move(results))),
        m_task(std::move(task)),
        m_state_repository(std::move(state_repository)),
        m_axiom_evaluator(std::move(axiom_evaluator))
    {
        options.event_handler = p::brfs::DefaultEventHandler<::tyr::GroundTag>::create();
    }

    p::Node<::tyr::GroundTag> normalize_start_node(std::optional<p::Node<::tyr::GroundTag>> start_node)
    {
        if (!start_node)
            start_node = options.start_node;
        if (!m_task || !m_state_repository || !m_axiom_evaluator)
            throw std::invalid_argument("ScriptedSolver::normalize_start_node(): context is required.");
        return p::normalize_start_node(*m_task, *m_state_repository, *m_axiom_evaluator, std::move(start_node));
    }

    p::SearchResult<::tyr::GroundTag> solve()
    {
        auto result = std::move(m_results->front());
        m_results->pop_front();
        return result;
    }
};

template<typename Solver>
class RecordingSerializedEventHandler : public p::serialized::EventHandler<::tyr::GroundTag, Solver>
{
public:
    size_t num_search_starts = 0;
    size_t num_search_ends = 0;
    size_t num_subsearch_starts = 0;
    size_t num_subsearch_ends = 0;
    std::chrono::steady_clock::duration start_subsearch_delay {};

    void on_start_search() override { ++num_search_starts; }
    void on_start_subsearch(ygg::uint_t) override
    {
        ++num_subsearch_starts;
        std::this_thread::sleep_for(start_subsearch_delay);
    }
    void add_subsearch_statistics(const p::Statistics&, const typename Solver::EventHandlerType::StatisticsType&) override {}
    void on_end_subsearch(ygg::uint_t, p::SearchStatus) override { ++num_subsearch_ends; }
    void on_end_search(p::SearchStatus, const p::Statistics&) override { ++num_search_ends; }
    void on_solved(const p::Plan<::tyr::GroundTag>&) override {}
};

class RecordingIwEventHandler : public p::iw::EventHandler<::tyr::GroundTag>
{
public:
    explicit RecordingIwEventHandler(std::chrono::steady_clock::duration end_arity_delay = {}) : m_end_arity_delay(end_arity_delay) {}

    size_t num_search_starts = 0;
    size_t num_search_ends = 0;
    size_t num_arity_starts = 0;
    size_t num_arity_ends = 0;
    std::chrono::steady_clock::duration start_arity_delay {};

    void on_start_search(ygg::uint_t) override
    {
        m_statistics.clear();
        ++num_search_starts;
    }
    void on_start_arity(ygg::uint_t) override
    {
        ++num_arity_starts;
        std::this_thread::sleep_for(start_arity_delay);
    }
    void on_end_arity(ygg::uint_t, p::SearchStatus) override
    {
        ++num_arity_ends;
        std::this_thread::sleep_for(m_end_arity_delay);
    }
    void on_end_search(p::SearchStatus, const p::Statistics&) override { ++num_search_ends; }
    void on_solved(ygg::uint_t arity) override { m_statistics.set_solution_arity(arity); }
    const p::iw::Statistics<::tyr::GroundTag>& get_statistics() const override { return m_statistics; }

private:
    p::iw::Statistics<::tyr::GroundTag> m_statistics;
    std::chrono::steady_clock::duration m_end_arity_delay;
};
}

TEST(TyrPlanningSerialized, StatisticsClearResetsCountersAndProgressSnapshots)
{
    auto statistics = p::Statistics();
    statistics.increment_num_generated_successors();
    statistics.increment_num_expanded();
    statistics.increment_num_deadends();
    statistics.increment_num_pruned();
    statistics.increment_num_generated_candidates(false);
    statistics.increment_num_generated_candidates(true);

    auto progress_statistics = p::ProgressStatistics();
    progress_statistics.add_snapshot(statistics);

    ASSERT_EQ(progress_statistics.size(), 1);
    ASSERT_FALSE(progress_statistics.empty());
    ASSERT_EQ(progress_statistics.get_snapshots().size(), 1);
    EXPECT_EQ(progress_statistics.get_snapshots().front().get_num_generated_successors(), 1);
    EXPECT_EQ(progress_statistics.get_snapshots().front().get_num_generated_candidates(), 2);
    EXPECT_EQ(progress_statistics.get_snapshots().front().get_num_transferred_candidates(), 1);
    EXPECT_EQ(statistics.get_num_generated_successors(), 1);
    EXPECT_EQ(statistics.get_num_expanded(), 1);
    EXPECT_EQ(statistics.get_num_deadends(), 1);
    EXPECT_EQ(statistics.get_num_pruned(), 1);
    EXPECT_EQ(statistics.get_num_generated_candidates(), 2);
    EXPECT_EQ(statistics.get_num_transferred_candidates(), 1);
    EXPECT_DOUBLE_EQ(statistics.get_communication_overhead(), 0.5);

    statistics.clear();
    EXPECT_EQ(progress_statistics.get_snapshots().front().get_num_generated_candidates(), 2);
    EXPECT_EQ(progress_statistics.get_snapshots().front().get_num_transferred_candidates(), 1);
    progress_statistics.clear();

    EXPECT_EQ(statistics.get_num_generated_successors(), 0);
    EXPECT_EQ(statistics.get_num_expanded(), 0);
    EXPECT_EQ(statistics.get_num_deadends(), 0);
    EXPECT_EQ(statistics.get_num_pruned(), 0);
    EXPECT_EQ(statistics.get_num_generated_candidates(), 0);
    EXPECT_EQ(statistics.get_num_transferred_candidates(), 0);
    EXPECT_DOUBLE_EQ(statistics.get_communication_overhead(), 0.0);
    EXPECT_TRUE(progress_statistics.empty());
    EXPECT_EQ(progress_statistics.size(), 0);
    EXPECT_TRUE(progress_statistics.get_snapshots().empty());
}

TEST(TyrPlanningSerialized, StatisticsAggregatesAndClearsDestinationLockMetrics)
{
    auto statistics = p::Statistics {};
    statistics.add_destination_lock_statistics(std::chrono::nanoseconds(3), std::chrono::nanoseconds(5));
    statistics.increment_num_generated_candidates(false);

    auto other = p::Statistics {};
    other.add_destination_lock_statistics(std::chrono::nanoseconds(7), std::chrono::nanoseconds(11));
    other.increment_num_generated_candidates(true);
    other.increment_num_generated_candidates(true);
    statistics.add(other);

    EXPECT_EQ(statistics.get_num_destination_lock_acquisitions(), 2);
    EXPECT_EQ(statistics.get_destination_lock_wait_time(), std::chrono::nanoseconds(10));
    EXPECT_EQ(statistics.get_destination_lock_hold_time(), std::chrono::nanoseconds(16));
    EXPECT_EQ(statistics.get_num_generated_candidates(), 3);
    EXPECT_EQ(statistics.get_num_transferred_candidates(), 2);
    EXPECT_DOUBLE_EQ(statistics.get_communication_overhead(), 2.0 / 3.0);

    statistics.clear();
    EXPECT_EQ(statistics.get_num_destination_lock_acquisitions(), 0);
    EXPECT_EQ(statistics.get_destination_lock_wait_time(), std::chrono::nanoseconds(0));
    EXPECT_EQ(statistics.get_destination_lock_hold_time(), std::chrono::nanoseconds(0));
    EXPECT_EQ(statistics.get_num_generated_candidates(), 0);
    EXPECT_EQ(statistics.get_num_transferred_candidates(), 0);
    EXPECT_DOUBLE_EQ(statistics.get_communication_overhead(), 0.0);
}

TEST(TyrPlanningSerialized, AStarDefaultWorkerEventsRequireTraceVerbosity)
{
    const auto worker = ygg::Index<p::Worker>(0);
    for (const auto verbosity : { 0, 1, 2 })
    {
        EXPECT_EQ(p::astar_eager::DefaultEventHandler<::tyr::GroundTag>(verbosity).make_worker(worker) != nullptr, verbosity >= 2);
        EXPECT_EQ(p::astar_eager::DefaultEventHandler<::tyr::LiftedTag>(verbosity).make_worker(worker) != nullptr, verbosity >= 2);
    }
}

TEST(TyrPlanningSerialized, BrfsEventHandlerClearsProgressSnapshotsOnSearchStart)
{
    auto context = create_gripper_context();
    auto node = context.successor_generator->get_initial_node(*context.state_repository, *context.axiom_evaluator);
    auto event_handler = p::brfs::DefaultEventHandler<::tyr::GroundTag>(0);
    auto statistics = p::Statistics {};
    statistics.increment_num_generated_successors();

    event_handler.on_start_search(node);
    event_handler.on_finish_layer(0, statistics);

    ASSERT_EQ(event_handler.get_progress_statistics().size(), 1);
    EXPECT_EQ(event_handler.get_progress_statistics().get_snapshots().front().get_num_generated_successors(), 1);

    event_handler.on_start_search(node);

    EXPECT_TRUE(event_handler.get_progress_statistics().empty());
    EXPECT_EQ(event_handler.get_progress_statistics().size(), 0);

    EXPECT_EQ(event_handler.make_worker(ygg::Index<p::Worker>(0)), nullptr);
    EXPECT_NE(p::brfs::DefaultEventHandler<::tyr::GroundTag>(2).make_worker(ygg::Index<p::Worker>(0)), nullptr);
}

TEST(TyrPlanningSerialized, ThrowsWhenSubgoalStrategyIsMissing)
{
    auto solver = ScriptedSolver({});
    auto options = p::serialized::Options<::tyr::GroundTag, ScriptedSolver> {};
    options.goal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();

    EXPECT_THROW(static_cast<void>(p::serialized::find_solution(solver, options)), std::invalid_argument);
}

TEST(TyrPlanningSerialized, ThrowsWhenGoalStrategyIsMissing)
{
    auto solver = ScriptedSolver({});
    auto options = p::serialized::Options<::tyr::GroundTag, ScriptedSolver> {};
    options.subgoal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();

    EXPECT_THROW(static_cast<void>(p::serialized::find_solution(solver, options)), std::invalid_argument);
}

TEST(TyrPlanningSerialized, ZeroSubsearchesUsesDefaultStartAndReturnsExhaustedPartialPlan)
{
    auto context = create_gripper_context();
    auto solver = p::brfs::Solver<::tyr::GroundTag> { context.task, context.state_repository, context.axiom_evaluator, context.successor_generator, {} };
    solver.options.search_budget.max_time = std::chrono::steady_clock::duration::zero();
    auto options = p::serialized::Options<::tyr::GroundTag, decltype(solver)> {};
    auto event_handler = std::make_shared<RecordingSerializedEventHandler<decltype(solver)>>();
    options.event_handler = event_handler;
    options.subgoal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.goal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.max_num_subsearches = 0;

    const auto result = p::serialized::find_solution(solver, options);
    const auto initial_node = context.successor_generator->get_initial_node(*context.state_repository, *context.axiom_evaluator);

    ASSERT_EQ(result.status, p::SearchStatus::EXHAUSTED);
    ASSERT_TRUE(result.plan);
    ASSERT_TRUE(result.goal_node);
    EXPECT_TRUE(result.plan->empty());
    EXPECT_EQ(result.plan->get_start_node(), initial_node);
    EXPECT_EQ(*result.goal_node, initial_node);
    EXPECT_EQ(event_handler->num_search_starts, 1);
    EXPECT_EQ(event_handler->num_search_ends, 1);
    EXPECT_EQ(event_handler->num_subsearch_starts, 0);
    EXPECT_EQ(event_handler->num_subsearch_ends, 0);
}

TEST(TyrPlanningSerialized, ZeroSubsearchesReturnsSolvedForSatisfiedStart)
{
    auto context = create_gripper_context();
    auto solver = p::brfs::Solver<::tyr::GroundTag> { context.task, context.state_repository, context.axiom_evaluator, context.successor_generator, {} };
    auto options = p::serialized::Options<::tyr::GroundTag, decltype(solver)> {};
    options.subgoal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.goal_strategy = std::make_shared<SatisfiedGoalStrategy>();
    options.max_num_subsearches = 0;
    options.max_time = std::chrono::steady_clock::duration::zero();

    const auto result = p::serialized::find_solution(solver, options);

    EXPECT_EQ(result.status, p::SearchStatus::SOLVED);
    ASSERT_TRUE(result.plan);
    EXPECT_TRUE(result.plan->empty());
}

TEST(TyrPlanningSerialized, ZeroSubsearchesReturnsUnsolvableForStaticGoal)
{
    auto context = create_gripper_context();
    auto solver = p::brfs::Solver<::tyr::GroundTag> { context.task, context.state_repository, context.axiom_evaluator, context.successor_generator, {} };
    auto options = p::serialized::Options<::tyr::GroundTag, decltype(solver)> {};
    options.subgoal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.goal_strategy = std::make_shared<StaticallyImpossibleGoalStrategy>();
    options.max_num_subsearches = 0;
    options.max_time = std::chrono::steady_clock::duration::zero();

    const auto result = p::serialized::find_solution(solver, options);

    EXPECT_EQ(result.status, p::SearchStatus::UNSOLVABLE);
    EXPECT_FALSE(result.plan);
    EXPECT_FALSE(result.goal_node);
}

TEST(TyrPlanningSerialized, ZeroSubsearchesMaterializesCompatibleForeignStart)
{
    auto context = create_gripper_context();
    auto foreign = make_worker_context(context);
    const auto foreign_start =
        p::Node<::tyr::GroundTag>(foreign.successor_generator->get_initial_node(*foreign.state_repository, *foreign.axiom_evaluator).get_state(), 7);
    auto solver = p::brfs::Solver<::tyr::GroundTag> { context.task, context.state_repository, context.axiom_evaluator, context.successor_generator, {} };
    auto options = p::serialized::Options<::tyr::GroundTag, decltype(solver)> {};
    options.start_node = foreign_start;
    options.subgoal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.goal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.max_num_subsearches = 0;

    const auto result = p::serialized::find_solution(solver, options);

    ASSERT_TRUE(result.plan);
    EXPECT_EQ(result.plan->get_start_node().get_state().get_state_repository(), context.state_repository);
    EXPECT_NE(result.plan->get_start_node().get_state().get_state_repository(), foreign_start.get_state().get_state_repository());
    EXPECT_EQ(result.plan->get_start_node().get_metric(), 7);
}

TEST(TyrPlanningSerialized, RejectsStartFromDifferentTaskBeforeZeroSubsearchShortcut)
{
    auto context = create_gripper_context();
    auto other_context = create_gripper_context();
    auto solver = p::brfs::Solver<::tyr::GroundTag> { context.task, context.state_repository, context.axiom_evaluator, context.successor_generator, {} };
    auto options = p::serialized::Options<::tyr::GroundTag, decltype(solver)> {};
    options.start_node = other_context.successor_generator->get_initial_node(*other_context.state_repository, *other_context.axiom_evaluator);
    options.subgoal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.goal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.max_num_subsearches = 0;

    EXPECT_THROW(static_cast<void>(p::serialized::find_solution(solver, options)), std::invalid_argument);
}

TEST(TyrPlanningSerialized, RejectsNaNStartMetricBeforeZeroSubsearchShortcut)
{
    auto context = create_gripper_context();
    auto solver = p::brfs::Solver<::tyr::GroundTag> { context.task, context.state_repository, context.axiom_evaluator, context.successor_generator, {} };
    auto options = p::serialized::Options<::tyr::GroundTag, decltype(solver)> {};
    options.start_node =
        p::Node<::tyr::GroundTag>(context.successor_generator->get_initial_node(*context.state_repository, *context.axiom_evaluator).get_state(),
                                  std::numeric_limits<ygg::float_t>::quiet_NaN());
    options.subgoal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.goal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.max_num_subsearches = 0;

    EXPECT_THROW(static_cast<void>(p::serialized::find_solution(solver, options)), std::runtime_error);
}

TEST(TyrPlanningSerialized, RejectsNonFiniteSubplanMetrics)
{
    auto context = create_gripper_context();
    const auto start = context.successor_generator->get_initial_node(*context.state_repository, *context.axiom_evaluator);
    const auto successors = context.successor_generator->get_labeled_successor_nodes(start, *context.state_repository, *context.axiom_evaluator);
    ASSERT_FALSE(successors.empty());

    auto sub_result = p::SearchResult<::tyr::GroundTag> {};
    sub_result.status = p::SearchStatus::SOLVED;
    sub_result.goal_node = successors.front().node;
    sub_result.plan =
        p::Plan<::tyr::GroundTag>(p::Node<::tyr::GroundTag>(start.get_state(), std::numeric_limits<ygg::float_t>::infinity()), { successors.front() });

    auto solver = ScriptedSolver({ std::move(sub_result) }, context.task, context.state_repository, context.axiom_evaluator);
    auto options = p::serialized::Options<::tyr::GroundTag, ScriptedSolver> {};
    options.subgoal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.goal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.max_num_subsearches = 1;

    EXPECT_THROW(static_cast<void>(p::serialized::find_solution(solver, options)), std::runtime_error);
}

TEST(TyrPlanningSerialized, ExpiredBudgetDoesNotStartSubsearch)
{
    auto context = create_gripper_context();
    auto solver = p::brfs::Solver<::tyr::GroundTag> { context.task, context.state_repository, context.axiom_evaluator, context.successor_generator, {} };
    solver.options.search_budget.max_time = std::chrono::steady_clock::duration::zero();
    auto options = p::serialized::Options<::tyr::GroundTag, decltype(solver)> {};
    auto event_handler = std::make_shared<RecordingSerializedEventHandler<decltype(solver)>>();
    options.event_handler = event_handler;
    options.subgoal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.goal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();

    const auto result = p::serialized::find_solution(solver, options);

    EXPECT_EQ(result.status, p::SearchStatus::OUT_OF_TIME);
    EXPECT_EQ(event_handler->num_search_starts, 1);
    EXPECT_EQ(event_handler->num_search_ends, 1);
    EXPECT_EQ(event_handler->num_subsearch_starts, 0);
    EXPECT_EQ(event_handler->num_subsearch_ends, 0);
}

TEST(TyrPlanningSerialized, SlowSubsearchStartDoesNotRegrantNestedBudget)
{
    auto context = create_gripper_context();
    auto solver = p::brfs::Solver<::tyr::GroundTag> { context.task, context.state_repository, context.axiom_evaluator, context.successor_generator, {} };
    auto options = p::serialized::Options<::tyr::GroundTag, decltype(solver)> {};
    auto event_handler = std::make_shared<RecordingSerializedEventHandler<decltype(solver)>>();
    event_handler->start_subsearch_delay = std::chrono::milliseconds(110);
    options.event_handler = event_handler;
    options.subgoal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.goal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.max_num_subsearches = 1;
    options.max_time = std::chrono::milliseconds(100);
    solver.options.search_budget.max_num_states = 1;

    const auto result = p::serialized::find_solution(solver, options);

    EXPECT_EQ(result.status, p::SearchStatus::OUT_OF_TIME);
    EXPECT_EQ(event_handler->num_subsearch_starts, 1);
    EXPECT_EQ(event_handler->num_subsearch_ends, 1);
}

TEST(TyrPlanningSerialized, SlowArityStartDoesNotRegrantNestedBudget)
{
    auto context = create_gripper_context();
    auto brfs_solver = p::brfs::Solver<::tyr::GroundTag> { context.task, context.state_repository, context.axiom_evaluator, context.successor_generator, {} };
    auto event_handler = std::make_shared<RecordingIwEventHandler>();
    event_handler->start_arity_delay = std::chrono::milliseconds(110);
    auto options = p::iw::Options<::tyr::GroundTag> {};
    options.event_handler = event_handler;
    options.goal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.search_budget.max_num_states = 1;
    options.search_budget.max_time = std::chrono::milliseconds(100);

    const auto result = p::iw::find_solution(brfs_solver, 0, options);

    EXPECT_EQ(result.status, p::SearchStatus::OUT_OF_TIME);
    EXPECT_EQ(event_handler->num_arity_starts, 1);
    EXPECT_EQ(event_handler->num_arity_ends, 1);
}

TEST(TyrPlanningSerialized, IwDeadlineStopsBeforeStartingAnotherWidth)
{
    auto context = create_gripper_context();
    auto brfs_solver = p::brfs::Solver<::tyr::GroundTag> { context.task, context.state_repository, context.axiom_evaluator, context.successor_generator, {} };
    brfs_solver.options.pruning_strategy = std::make_shared<AlwaysPruningStrategy>();
    auto event_handler = std::make_shared<RecordingIwEventHandler>(std::chrono::milliseconds(10));
    auto options = p::iw::Options<::tyr::GroundTag> {};
    options.event_handler = event_handler;
    options.goal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.search_budget.max_time = std::chrono::milliseconds(1);

    const auto result = p::iw::find_solution(brfs_solver, 1, options);

    EXPECT_EQ(result.status, p::SearchStatus::OUT_OF_TIME);
    EXPECT_EQ(event_handler->num_search_starts, 1);
    EXPECT_EQ(event_handler->num_search_ends, 1);
    EXPECT_EQ(event_handler->num_arity_starts, 1);
    EXPECT_EQ(event_handler->num_arity_ends, 1);
}

TEST(TyrPlanningSerialized, SiwUsesUnderlyingBrfsBudgetBeforeStartingIw)
{
    auto context = create_gripper_context();
    auto brfs_options = p::brfs::Options<::tyr::GroundTag> {};
    brfs_options.search_budget.max_time = std::chrono::steady_clock::duration::zero();
    auto brfs_solver =
        p::brfs::Solver<::tyr::GroundTag> { context.task, context.state_repository, context.axiom_evaluator, context.successor_generator, brfs_options };
    auto iw_solver = p::iw::Solver<::tyr::GroundTag> { std::move(brfs_solver), 1, {} };
    auto iw_event_handler = std::make_shared<RecordingIwEventHandler>();
    iw_solver.options.event_handler = iw_event_handler;

    const auto result = p::siw::find_solution(iw_solver);

    EXPECT_EQ(result.status, p::SearchStatus::OUT_OF_TIME);
    EXPECT_EQ(iw_event_handler->num_search_starts, 0);
    EXPECT_EQ(iw_event_handler->num_search_ends, 0);
    EXPECT_EQ(iw_event_handler->num_arity_starts, 0);
    EXPECT_EQ(iw_event_handler->num_arity_ends, 0);
}

TEST(TyrPlanningSerialized, FinalGoalUsesNormalizedStartAsStableSeed)
{
    auto context = create_gripper_context();
    const auto initial_node = context.successor_generator->get_initial_node(*context.state_repository, *context.axiom_evaluator);
    const auto successors = context.successor_generator->get_labeled_successor_nodes(initial_node, *context.state_repository, *context.axiom_evaluator);
    const auto successor_it = std::ranges::find_if(successors, [&](const auto& successor) { return successor.node.get_state() != initial_node.get_state(); });
    ASSERT_NE(successor_it, successors.end());

    auto sub_result = p::SearchResult<::tyr::GroundTag> {};
    sub_result.status = p::SearchStatus::SOLVED;
    sub_result.goal_node = successor_it->node;
    sub_result.plan = p::Plan<::tyr::GroundTag>(initial_node, { *successor_it });

    auto solver = ScriptedSolver({ std::move(sub_result) }, context.task, context.state_repository, context.axiom_evaluator);
    auto foreign = make_worker_context(context);
    auto goal_strategy = std::make_shared<TrackingGoalStrategy>();
    auto options = p::serialized::Options<::tyr::GroundTag, ScriptedSolver> {};
    options.start_node = foreign.successor_generator->get_initial_node(*foreign.state_repository, *foreign.axiom_evaluator);
    options.subgoal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.goal_strategy = goal_strategy;
    options.max_num_subsearches = 1;

    const auto result = p::serialized::find_solution(solver, options);

    EXPECT_EQ(result.status, p::SearchStatus::EXHAUSTED);
    EXPECT_EQ(goal_strategy->num_static_calls, 1);
    ASSERT_EQ(goal_strategy->seed_repositories.size(), 2);
    EXPECT_EQ(goal_strategy->seed_repositories[0], context.state_repository.get());
    EXPECT_EQ(goal_strategy->seed_repositories[1], goal_strategy->seed_repositories[0]);
    EXPECT_EQ(goal_strategy->seed_indices[0], initial_node.get_state().get_index());
    EXPECT_EQ(goal_strategy->seed_indices[1], goal_strategy->seed_indices[0]);
}

TEST(TyrPlanningSerialized, BrfsSubsolverMatchesDirectBrfs)
{
    auto direct_context = create_gripper_context();
    auto serialized_context = create_gripper_context();

    auto direct_options = p::brfs::Options<::tyr::GroundTag> {};
    direct_options.event_handler = p::brfs::DefaultEventHandler<::tyr::GroundTag>::create();
    const auto direct_result = p::brfs::find_solution(*direct_context.task,
                                                      *direct_context.state_repository,
                                                      *direct_context.axiom_evaluator,
                                                      *direct_context.successor_generator,
                                                      direct_options);

    auto brfs_solver = p::brfs::Solver<::tyr::GroundTag> { serialized_context.task,
                                                           serialized_context.state_repository,
                                                           serialized_context.axiom_evaluator,
                                                           serialized_context.successor_generator,
                                                           p::brfs::Options<::tyr::GroundTag> {} };
    brfs_solver.options.event_handler = p::brfs::DefaultEventHandler<::tyr::GroundTag>::create();

    auto serialized_options = p::serialized::Options<::tyr::GroundTag, decltype(brfs_solver)> {};
    const auto event_handler = p::serialized::DefaultEventHandler<::tyr::GroundTag, decltype(brfs_solver)>::create();
    serialized_options.event_handler = event_handler;
    serialized_options.subgoal_strategy = p::SerializedGoalStrategy<::tyr::GroundTag>::create(*serialized_context.task);
    serialized_options.goal_strategy = p::ConjunctiveGoalStrategy<::tyr::GroundTag>::create(*serialized_context.task);

    const auto serialized_result = p::serialized::find_solution(brfs_solver, serialized_options);

    ASSERT_EQ(direct_result.status, p::SearchStatus::SOLVED);
    ASSERT_TRUE(direct_result.plan);
    ASSERT_EQ(serialized_result.status, p::SearchStatus::SOLVED);
    ASSERT_TRUE(serialized_result.plan);

    EXPECT_FALSE(serialized_result.plan->empty());
    EXPECT_EQ(serialized_result.plan->get_length(), direct_result.plan->get_length());
    EXPECT_EQ(serialized_result.plan->get_cost(), direct_result.plan->get_cost());
    EXPECT_GE(event_handler->get_statistics().get_num_subsearches(), 1);
    EXPECT_EQ(event_handler->get_statistics().get_search_statistics().size(), event_handler->get_statistics().get_num_subsearches());
    EXPECT_EQ(event_handler->get_statistics().get_solver_statistics().size(), event_handler->get_statistics().get_num_subsearches());
}

TEST(TyrPlanningSerialized, DetectsRepeatedSubgoalState)
{
    auto context = create_gripper_context();
    auto start_node = context.successor_generator->get_initial_node(*context.state_repository, *context.axiom_evaluator);
    auto labeled_succ_nodes = p::LabeledNodeList<::tyr::GroundTag> {};
    context.successor_generator->get_labeled_successor_nodes(start_node, *context.state_repository, *context.axiom_evaluator, labeled_succ_nodes);
    ASSERT_FALSE(labeled_succ_nodes.empty());

    const auto first_labeled_succ_node_it =
        std::find_if(labeled_succ_nodes.begin(),
                     labeled_succ_nodes.end(),
                     [&](const auto& labeled_succ_node) { return labeled_succ_node.node.get_state().get_index() != start_node.get_state().get_index(); });
    ASSERT_NE(first_labeled_succ_node_it, labeled_succ_nodes.end());
    const auto first_labeled_succ_node = *first_labeled_succ_node_it;
    const auto first_goal_node = first_labeled_succ_node.node;
    const auto second_start_node = p::Node<::tyr::GroundTag>(first_goal_node.get_state(), 0);
    const auto repeated_start_node = p::Node<::tyr::GroundTag>(start_node.get_state(), 1);

    auto first_subresult = p::SearchResult<::tyr::GroundTag> {};
    first_subresult.status = p::SearchStatus::SOLVED;
    first_subresult.statistics.increment_num_generated_successors();
    first_subresult.statistics.increment_num_pruned();
    first_subresult.statistics.set_num_registered_states(20);
    first_subresult.statistics.set_state_storage_memory_usage(100);
    first_subresult.statistics.set_action_bindings_memory_usage(10);
    first_subresult.statistics.set_predicate_bindings_memory_usage(40);
    first_subresult.statistics.set_axiom_bindings_memory_usage(5);
    first_subresult.statistics.set_function_bindings_memory_usage(80);
    first_subresult.goal_node = first_goal_node;
    first_subresult.plan = p::Plan<::tyr::GroundTag>(start_node, p::LabeledNodeList<::tyr::GroundTag> { first_labeled_succ_node });

    auto second_subresult = p::SearchResult<::tyr::GroundTag> {};
    second_subresult.status = p::SearchStatus::SOLVED;
    second_subresult.statistics.increment_num_generated_successors();
    second_subresult.statistics.increment_num_expanded();
    second_subresult.statistics.set_num_registered_states(30);
    second_subresult.statistics.set_state_storage_memory_usage(90);
    second_subresult.statistics.set_action_bindings_memory_usage(15);
    second_subresult.statistics.set_predicate_bindings_memory_usage(35);
    second_subresult.statistics.set_axiom_bindings_memory_usage(7);
    second_subresult.statistics.set_function_bindings_memory_usage(70);
    second_subresult.goal_node = repeated_start_node;
    second_subresult.plan = p::Plan<::tyr::GroundTag>(second_start_node,
                                                      p::LabeledNodeList<::tyr::GroundTag> { p::LabeledNode<::tyr::GroundTag> {
                                                          first_labeled_succ_node.label,
                                                          repeated_start_node,
                                                      } });

    auto solver = ScriptedSolver(std::deque<p::SearchResult<::tyr::GroundTag>> { std::move(first_subresult), std::move(second_subresult) },
                                 context.task,
                                 context.state_repository,
                                 context.axiom_evaluator);

    auto options = p::serialized::Options<::tyr::GroundTag, ScriptedSolver> {};
    options.event_handler = p::serialized::DefaultEventHandler<::tyr::GroundTag, ScriptedSolver>::create();
    options.subgoal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.goal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();

    const auto result = p::serialized::find_solution(solver, options);

    ASSERT_EQ(result.status, p::SearchStatus::CYCLE);
    ASSERT_TRUE(result.plan);
    ASSERT_TRUE(result.cycle_range);
    EXPECT_FALSE(result.plan->empty());
    EXPECT_EQ(result.plan->get_length(), 2);
    EXPECT_EQ(result.plan->get_cost(), 2);
    EXPECT_EQ(result.cycle_range->first, 0);
    EXPECT_EQ(result.cycle_range->second, 2);
    EXPECT_EQ(result.statistics.get_num_generated_successors(), 2);
    EXPECT_EQ(result.statistics.get_num_expanded(), 1);
    EXPECT_EQ(result.statistics.get_num_pruned(), 1);
    EXPECT_EQ(result.statistics.get_num_registered_states(), 30);
    EXPECT_EQ(result.statistics.get_state_storage_memory_usage(), 100);
    EXPECT_EQ(result.statistics.get_action_bindings_memory_usage(), 15);
    EXPECT_EQ(result.statistics.get_predicate_bindings_memory_usage(), 40);
    EXPECT_EQ(result.statistics.get_axiom_bindings_memory_usage(), 7);
    EXPECT_EQ(result.statistics.get_function_bindings_memory_usage(), 80);
}

TEST(TyrPlanningSerialized, DetectsCycleUsingCanonicalSubplanStateIdentity)
{
    auto context = create_gripper_context();
    const auto initial_node = context.successor_generator->get_initial_node(*context.state_repository, *context.axiom_evaluator);
    const auto successors = context.successor_generator->get_labeled_successor_nodes(initial_node, *context.state_repository, *context.axiom_evaluator);
    const auto successor_it =
        std::ranges::find_if(successors, [&](const auto& successor) { return successor.node.get_state().get_index() != initial_node.get_state().get_index(); });
    ASSERT_NE(successor_it, successors.end());
    const auto& successor = *successor_it;

    auto foreign = make_worker_context(context);
    const auto foreign_start = foreign.successor_generator->get_initial_node(*foreign.state_repository, *foreign.axiom_evaluator);
    ASSERT_EQ(foreign_start.get_state().get_index(), initial_node.get_state().get_index());
    ASSERT_NE(foreign_start.get_state(), initial_node.get_state());
    const auto foreign_successor_state = p::materialize_state(successor.node.get_state(), *foreign.state_repository, *foreign.axiom_evaluator);
    ASSERT_NE(foreign_successor_state, successor.node.get_state());

    const auto first_goal = p::Node<::tyr::GroundTag>(foreign_successor_state, 1);
    auto first_subresult = p::SearchResult<::tyr::GroundTag> {};
    first_subresult.status = p::SearchStatus::SOLVED;
    first_subresult.goal_node = first_goal;
    first_subresult.plan =
        p::Plan<::tyr::GroundTag>(foreign_start, p::LabeledNodeList<::tyr::GroundTag> { p::LabeledNode<::tyr::GroundTag> { successor.label, first_goal } });

    const auto second_start = p::Node<::tyr::GroundTag>(first_goal.get_state(), 0);
    const auto second_goal = p::Node<::tyr::GroundTag>(foreign_start.get_state(), 1);
    auto second_subresult = p::SearchResult<::tyr::GroundTag> {};
    second_subresult.status = p::SearchStatus::SOLVED;
    second_subresult.goal_node = second_goal;
    second_subresult.plan =
        p::Plan<::tyr::GroundTag>(second_start, p::LabeledNodeList<::tyr::GroundTag> { p::LabeledNode<::tyr::GroundTag> { successor.label, second_goal } });

    auto solver = ScriptedSolver(std::deque<p::SearchResult<::tyr::GroundTag>> { std::move(first_subresult), std::move(second_subresult) },
                                 context.task,
                                 context.state_repository,
                                 context.axiom_evaluator);
    auto options = p::serialized::Options<::tyr::GroundTag, ScriptedSolver> {};
    options.start_node = foreign_start;
    options.subgoal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.goal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.max_num_subsearches = 2;

    const auto result = p::serialized::find_solution(solver, options);

    ASSERT_EQ(result.status, p::SearchStatus::CYCLE);
    ASSERT_TRUE(result.plan);
    ASSERT_TRUE(result.cycle_range);
    EXPECT_EQ(result.plan->get_start_node().get_state(), initial_node.get_state());
    EXPECT_EQ(result.plan->get_length(), 2);
    EXPECT_EQ(result.plan->get_labeled_succ_nodes()[0].node.get_state(), successor.node.get_state());
    EXPECT_EQ(result.plan->get_labeled_succ_nodes()[1].node.get_state(), initial_node.get_state());
    EXPECT_EQ(*result.cycle_range, (std::pair<size_t, size_t> { 0, 2 }));
}

}
