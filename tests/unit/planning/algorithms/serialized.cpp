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
#include <stdexcept>
#include <tyr/formalism/formalism.hpp>
#include <tyr/planning/planning.hpp>
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
    auto state_repository = p::StateRepositoryFactory<::tyr::GroundTag>().create(task, axiom_evaluator);
    auto successor_generator = p::SuccessorGeneratorFactory<::tyr::GroundTag>().create(task, execution_context, state_repository);

    return GroundSearchContext { std::move(task), std::move(successor_generator) };
}

class NeverSatisfiedGoalStrategy : public p::GoalStrategy<::tyr::GroundTag>
{
public:
    bool is_static_goal_satisfied(const p::Task<::tyr::GroundTag>& task) override
    {
        static_cast<void>(task);
        return true;
    }

    bool is_dynamic_goal_satisfied(const p::StateView<::tyr::GroundTag>& seed_state, const p::StateView<::tyr::GroundTag>& state) override
    {
        static_cast<void>(seed_state);
        static_cast<void>(state);
        return false;
    }
};

class ScriptedSolver
{
private:
    std::shared_ptr<std::deque<p::SearchResult<::tyr::GroundTag>>> m_results;

public:
    using EventHandlerType = p::brfs::EventHandler<::tyr::GroundTag>;

    p::brfs::Options<::tyr::GroundTag> options;

    explicit ScriptedSolver(std::deque<p::SearchResult<::tyr::GroundTag>> results) :
        m_results(std::make_shared<std::deque<p::SearchResult<::tyr::GroundTag>>>(std::move(results)))
    {
        options.event_handler = p::brfs::DefaultEventHandler<::tyr::GroundTag>::create();
    }

    p::SearchResult<::tyr::GroundTag> solve()
    {
        auto result = std::move(m_results->front());
        m_results->pop_front();
        return result;
    }
};
}

TEST(TyrPlanningSerialized, StatisticsClearResetsCountersAndProgressSnapshots)
{
    auto statistics = p::Statistics();
    statistics.increment_num_generated();
    statistics.increment_num_expanded();
    statistics.increment_num_deadends();
    statistics.increment_num_pruned();

    auto progress_statistics = p::ProgressStatistics();
    progress_statistics.add_snapshot(statistics);

    ASSERT_EQ(progress_statistics.size(), 1);
    ASSERT_FALSE(progress_statistics.empty());
    ASSERT_EQ(progress_statistics.get_snapshots().size(), 1);
    EXPECT_EQ(statistics.get_num_generated(), 1);
    EXPECT_EQ(statistics.get_num_expanded(), 1);
    EXPECT_EQ(statistics.get_num_deadends(), 1);
    EXPECT_EQ(statistics.get_num_pruned(), 1);

    statistics.clear();
    progress_statistics.clear();

    EXPECT_EQ(statistics.get_num_generated(), 0);
    EXPECT_EQ(statistics.get_num_expanded(), 0);
    EXPECT_EQ(statistics.get_num_deadends(), 0);
    EXPECT_EQ(statistics.get_num_pruned(), 0);
    EXPECT_TRUE(progress_statistics.empty());
    EXPECT_EQ(progress_statistics.size(), 0);
    EXPECT_TRUE(progress_statistics.get_snapshots().empty());
}

TEST(TyrPlanningSerialized, StatisticsAggregatesAndClearsDestinationLockMetrics)
{
    auto statistics = p::Statistics {};
    statistics.add_destination_lock_statistics(std::chrono::nanoseconds(3), std::chrono::nanoseconds(5));

    auto other = p::Statistics {};
    other.add_destination_lock_statistics(std::chrono::nanoseconds(7), std::chrono::nanoseconds(11));
    statistics.add(other);

    EXPECT_EQ(statistics.get_num_destination_lock_acquisitions(), 2);
    EXPECT_EQ(statistics.get_destination_lock_wait_time(), std::chrono::nanoseconds(10));
    EXPECT_EQ(statistics.get_destination_lock_hold_time(), std::chrono::nanoseconds(16));

    statistics.clear();
    EXPECT_EQ(statistics.get_num_destination_lock_acquisitions(), 0);
    EXPECT_EQ(statistics.get_destination_lock_wait_time(), std::chrono::nanoseconds(0));
    EXPECT_EQ(statistics.get_destination_lock_hold_time(), std::chrono::nanoseconds(0));
}

TEST(TyrPlanningSerialized, BrfsEventHandlerClearsProgressSnapshotsOnSearchStart)
{
    auto context = create_gripper_context();
    auto node = context.successor_generator->get_initial_node();
    auto event_handler = p::brfs::DefaultEventHandler<::tyr::GroundTag>(0);
    auto statistics = p::Statistics {};
    statistics.increment_num_generated();

    event_handler.on_start_search(node);
    event_handler.on_finish_layer(0, statistics);

    ASSERT_EQ(event_handler.get_progress_statistics().size(), 1);
    EXPECT_EQ(event_handler.get_progress_statistics().get_snapshots().front().get_num_generated(), 1);

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

TEST(TyrPlanningSerialized, ThrowsWhenZeroSubsearchesHaveNoStartNode)
{
    auto solver = ScriptedSolver({});
    auto options = p::serialized::Options<::tyr::GroundTag, ScriptedSolver> {};
    options.subgoal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.goal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.max_num_subsearches = 0;

    EXPECT_THROW(static_cast<void>(p::serialized::find_solution(solver, options)), std::invalid_argument);
}

TEST(TyrPlanningSerialized, ZeroSubsearchesReturnsEmptyPlanFromStartNode)
{
    auto context = create_gripper_context();
    auto start_node = context.successor_generator->get_initial_node();
    auto solver = ScriptedSolver({});

    auto options = p::serialized::Options<::tyr::GroundTag, ScriptedSolver> {};
    options.start_node = start_node;
    options.subgoal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.goal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.max_num_subsearches = 0;

    const auto result = p::serialized::find_solution(solver, options);

    ASSERT_EQ(result.status, p::SearchStatus::SOLVED);
    ASSERT_TRUE(result.plan);
    ASSERT_TRUE(result.goal_node);
    EXPECT_TRUE(result.plan->empty());
    EXPECT_EQ(result.plan->get_length(), 0);
    EXPECT_EQ(result.plan->get_cost(), 0);
    EXPECT_EQ(result.plan->get_start_node().get_state().get_index(), start_node.get_state().get_index());
    EXPECT_EQ(result.plan->get_start_node().get_metric(), start_node.get_metric());
    EXPECT_EQ(result.goal_node->get_state().get_index(), start_node.get_state().get_index());
    EXPECT_EQ(result.goal_node->get_metric(), start_node.get_metric());
}

TEST(TyrPlanningSerialized, BrfsSubsolverMatchesDirectBrfs)
{
    auto direct_context = create_gripper_context();
    auto serialized_context = create_gripper_context();

    auto direct_options = p::brfs::Options<::tyr::GroundTag> {};
    direct_options.event_handler = p::brfs::DefaultEventHandler<::tyr::GroundTag>::create();
    const auto direct_result = p::brfs::find_solution(*direct_context.task, *direct_context.successor_generator, direct_options);

    auto brfs_solver =
        p::brfs::Solver<::tyr::GroundTag> { serialized_context.task, serialized_context.successor_generator, p::brfs::Options<::tyr::GroundTag> {} };
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
    auto start_node = context.successor_generator->get_initial_node();
    auto labeled_succ_nodes = p::LabeledNodeList<::tyr::GroundTag> {};
    context.successor_generator->get_labeled_successor_nodes(start_node, labeled_succ_nodes);
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
    first_subresult.statistics.increment_num_generated();
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
    second_subresult.statistics.increment_num_generated();
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

    auto solver = ScriptedSolver(std::deque<p::SearchResult<::tyr::GroundTag>> { std::move(first_subresult), std::move(second_subresult) });

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
    EXPECT_EQ(result.statistics.get_num_generated(), 2);
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
    const auto initial_node = context.successor_generator->get_initial_node();
    const auto successors = context.successor_generator->get_labeled_successor_nodes(initial_node);
    const auto successor_it =
        std::ranges::find_if(successors, [&](const auto& successor) { return successor.node.get_state().get_index() != initial_node.get_state().get_index(); });
    ASSERT_NE(successor_it, successors.end());
    const auto& successor = *successor_it;

    auto foreign_generator = context.successor_generator->make_worker(ygg::ExecutionContext::create(1));
    ASSERT_NE(foreign_generator, nullptr);
    const auto foreign_start = foreign_generator->get_initial_node();
    ASSERT_EQ(foreign_start.get_state().get_index(), initial_node.get_state().get_index());
    ASSERT_NE(foreign_start.get_state(), initial_node.get_state());

    const auto canonical_start = p::Node<::tyr::GroundTag>(successor.node.get_state(), 0);
    const auto first_goal = p::Node<::tyr::GroundTag>(initial_node.get_state(), 1);
    auto first_subresult = p::SearchResult<::tyr::GroundTag> {};
    first_subresult.status = p::SearchStatus::SOLVED;
    first_subresult.goal_node = first_goal;
    first_subresult.plan =
        p::Plan<::tyr::GroundTag>(canonical_start, p::LabeledNodeList<::tyr::GroundTag> { p::LabeledNode<::tyr::GroundTag> { successor.label, first_goal } });

    const auto second_start = p::Node<::tyr::GroundTag>(first_goal.get_state(), 0);
    const auto second_goal = p::Node<::tyr::GroundTag>(canonical_start.get_state(), 1);
    auto second_subresult = p::SearchResult<::tyr::GroundTag> {};
    second_subresult.status = p::SearchStatus::SOLVED;
    second_subresult.goal_node = second_goal;
    second_subresult.plan =
        p::Plan<::tyr::GroundTag>(second_start, p::LabeledNodeList<::tyr::GroundTag> { p::LabeledNode<::tyr::GroundTag> { successor.label, second_goal } });

    auto solver = ScriptedSolver(std::deque<p::SearchResult<::tyr::GroundTag>> { std::move(first_subresult), std::move(second_subresult) });
    auto options = p::serialized::Options<::tyr::GroundTag, ScriptedSolver> {};
    options.start_node = foreign_start;
    options.subgoal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();
    options.goal_strategy = std::make_shared<NeverSatisfiedGoalStrategy>();

    const auto result = p::serialized::find_solution(solver, options);

    ASSERT_EQ(result.status, p::SearchStatus::CYCLE);
    ASSERT_TRUE(result.plan);
    ASSERT_TRUE(result.cycle_range);
    EXPECT_EQ(result.plan->get_start_node().get_state(), canonical_start.get_state());
    EXPECT_EQ(result.plan->get_length(), 2);
    EXPECT_EQ(*result.cycle_range, (std::pair<size_t, size_t> { 0, 2 }));
}

}
