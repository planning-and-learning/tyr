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
#include <optional>
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

TaskPair parse_tasks(const std::filesystem::path& domain, const std::filesystem::path& problem)
{
    const auto fixture_root = std::filesystem::path(ROOT_DIR) / "tests/fixtures/planning/algorithms";
    return make_task_pair(p::Task<LiftedTag>::create(make_test_parser(fixture_root / domain).parse_task(fixture_root / problem)));
}

TaskPair parse_tasks(const std::filesystem::path& problem) { return parse_tasks("parallel_search_edge_cases_domain.pddl", problem); }

TaskPair parse_pruned_improvement_tasks()
{
    const auto fixture_root = std::filesystem::path(ROOT_DIR) / "tests/fixtures/planning/algorithms";
    const auto benchmark_root = std::filesystem::path(BENCHMARKS_DIR);
    return make_task_pair(p::Task<LiftedTag>::create(
        make_test_parser(benchmark_root / "classical/tests/transport/domain.pddl").parse_task(fixture_root / "parallel_astar_pruned_improvement.pddl")));
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
    using p::Heuristic<Kind>::evaluate;

    explicit GoalAsDeadEndHeuristic(p::TaskPtr<Kind> task) : m_task(std::move(task)), m_goal(m_task->get_task().get_goal()) {}

    void set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal) override { m_goal = goal; }

    ygg::float_t evaluate(const ygg::Builder<p::State<Kind>>& state) override
    {
        const auto context = p::StateContext<Kind>(*m_task, state, 0);
        return p::is_dynamically_applicable(m_goal, context) ? std::numeric_limits<ygg::float_t>::infinity() : 0;
    }

    p::HeuristicPtr<Kind> make_worker(ygg::ExecutionContextPtr) const override
    {
        auto worker = std::make_shared<GoalAsDeadEndHeuristic>(m_task);
        worker->set_goal(m_goal);
        return worker;
    }

private:
    p::TaskPtr<Kind> m_task;
    ::tyr::formalism::planning::GroundConjunctiveConditionView m_goal;
};

template<TaskKind Kind>
class NullWorkerHeuristic final : public p::Heuristic<Kind>
{
public:
    using p::Heuristic<Kind>::evaluate;

    void set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView) override {}
    ygg::float_t evaluate(const ygg::Builder<p::State<Kind>>&) override { return 0; }
    p::HeuristicPtr<Kind> make_worker(ygg::ExecutionContextPtr) const override { return nullptr; }
};

template<TaskKind Kind>
class ThrowingHeuristic final : public p::Heuristic<Kind>
{
public:
    using p::Heuristic<Kind>::evaluate;

    void set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView) override {}
    ygg::float_t evaluate(const ygg::Builder<p::State<Kind>>&) override { throw std::runtime_error("Unexpected heuristic evaluation."); }
    p::HeuristicPtr<Kind> make_worker(ygg::ExecutionContextPtr) const override { return std::make_shared<ThrowingHeuristic>(); }
};

template<TaskKind Kind>
class InfiniteHeuristic final : public p::Heuristic<Kind>
{
public:
    using p::Heuristic<Kind>::evaluate;

    void set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView) override {}
    ygg::float_t evaluate(const ygg::Builder<p::State<Kind>>&) override { return std::numeric_limits<ygg::float_t>::infinity(); }
    p::HeuristicPtr<Kind> make_worker(ygg::ExecutionContextPtr) const override { return std::make_shared<InfiniteHeuristic>(); }
};

template<TaskKind Kind>
class DuplicatePruningStrategy final : public p::PruningStrategy<Kind>
{
public:
    p::PruningStrategyPtr<Kind> make_worker(ygg::Index<p::Worker>) const override { return std::make_shared<DuplicatePruningStrategy>(); }

    bool should_prune_successor_state(const p::StateView<Kind>&, const p::StateView<Kind>&, bool is_new) override { return !is_new; }
};

template<TaskKind Kind>
class AlwaysPruningStrategy final : public p::PruningStrategy<Kind>
{
public:
    p::PruningStrategyPtr<Kind> make_worker(ygg::Index<p::Worker>) const override { return std::make_shared<AlwaysPruningStrategy>(); }
    bool should_prune_state(const p::StateView<Kind>&) override { return true; }
};

template<TaskKind Kind>
class AlwaysSuccessorPruningStrategy final : public p::PruningStrategy<Kind>
{
public:
    p::PruningStrategyPtr<Kind> make_worker(ygg::Index<p::Worker>) const override { return std::make_shared<AlwaysSuccessorPruningStrategy>(); }
    bool should_prune_successor_state(const p::StateView<Kind>&, const p::StateView<Kind>&, bool) override { return true; }
};

template<TaskKind Kind>
class AlwaysGoalStrategy final : public p::GoalStrategy<Kind>
{
public:
    p::GoalStrategyPtr<Kind> make_worker(ygg::Index<p::Worker>) const override { return std::make_shared<AlwaysGoalStrategy>(); }
    bool is_static_goal_satisfied(const p::Task<Kind>&) override { return true; }
    bool is_dynamic_goal_satisfied(const p::StateView<Kind>&, const ygg::Builder<p::State<Kind>>&) override { return true; }
};

template<TaskKind Kind>
class StaticallyUnsatisfiedGoalStrategy final : public p::GoalStrategy<Kind>
{
public:
    p::GoalStrategyPtr<Kind> make_worker(ygg::Index<p::Worker>) const override { return std::make_shared<StaticallyUnsatisfiedGoalStrategy>(); }
    bool is_static_goal_satisfied(const p::Task<Kind>&) override { return false; }
    bool is_dynamic_goal_satisfied(const p::StateView<Kind>&, const ygg::Builder<p::State<Kind>>&) override { return false; }
};

template<TaskKind Kind>
class NonRootGoalStrategy final : public p::GoalStrategy<Kind>
{
public:
    p::GoalStrategyPtr<Kind> make_worker(ygg::Index<p::Worker>) const override { return std::make_shared<NonRootGoalStrategy>(); }
    bool is_static_goal_satisfied(const p::Task<Kind>&) override { return true; }
    bool is_dynamic_goal_satisfied(const p::StateView<Kind>& seed_state, const ygg::Builder<p::State<Kind>>& state) override
    {
        return &seed_state.get_state_builder() != &state;
    }
};

template<TaskKind Kind>
class SeedRecordingGoalStrategy final : public p::GoalStrategy<Kind>
{
    using SeedIdentity = decltype(std::declval<p::StateView<Kind>>().identifying_members());

    struct Observations
    {
        explicit Observations(SeedIdentity expected_seed_) : expected_seed(std::move(expected_seed_)) {}

        SeedIdentity expected_seed;
        std::optional<SeedIdentity> root_seed;
        std::optional<SeedIdentity> successor_seed;
        size_t num_root_checks { 0 };
        size_t num_successor_checks { 0 };
    };

public:
    SeedRecordingGoalStrategy(const p::Task<Kind>& task, SeedIdentity expected_seed) :
        m_goal(task.get_task().get_goal()),
        m_observations(std::make_shared<Observations>(std::move(expected_seed)))
    {
    }

    p::GoalStrategyPtr<Kind> make_worker(ygg::Index<p::Worker>) const override
    {
        return p::GoalStrategyPtr<Kind>(new SeedRecordingGoalStrategy(m_goal, m_observations));
    }

    bool is_static_goal_satisfied(const p::Task<Kind>& task) override { return p::is_statically_applicable(m_goal, task.get_static_atoms_bitset()); }

    bool is_dynamic_goal_satisfied(const p::StateView<Kind>& seed_state, const ygg::Builder<p::State<Kind>>& state) override
    {
        const auto context = p::StateContext<Kind>(*seed_state.get_state_repository()->get_task(), state, 0);
        const auto is_goal = p::is_dynamically_applicable(m_goal, context);
        if (is_goal)
        {
            m_observations->successor_seed = seed_state.identifying_members();
            ++m_observations->num_successor_checks;
        }
        else
        {
            m_observations->root_seed = seed_state.identifying_members();
            ++m_observations->num_root_checks;
        }
        return is_goal;
    }

    bool observed_expected_seeds() const
    {
        return m_observations->root_seed && *m_observations->root_seed == m_observations->expected_seed && m_observations->successor_seed
               && *m_observations->successor_seed == m_observations->expected_seed;
    }

    size_t get_num_root_checks() const noexcept { return m_observations->num_root_checks; }
    size_t get_num_successor_checks() const noexcept { return m_observations->num_successor_checks; }

private:
    SeedRecordingGoalStrategy(::tyr::formalism::planning::GroundConjunctiveConditionView goal, std::shared_ptr<Observations> observations) :
        m_goal(goal),
        m_observations(std::move(observations))
    {
    }

    ::tyr::formalism::planning::GroundConjunctiveConditionView m_goal;
    std::shared_ptr<Observations> m_observations;
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
            auto heuristic = GoalAsDeadEndHeuristic<Kind>(task);
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
            EXPECT_EQ(result.plan->get_cost(), 101);
        });
}

template<TaskKind Kind>
void expect_one_step_goal_result(const p::SearchResult<Kind>& result)
{
    ASSERT_EQ(result.status, p::SearchStatus::SOLVED);
    ASSERT_TRUE(result.plan);
    EXPECT_EQ(result.plan->get_length(), 1);
    EXPECT_EQ(result.statistics.get_num_generated(), 1);
    EXPECT_EQ(result.statistics.get_num_pruned(), 0);
}

template<TaskKind Kind>
void expect_goals_bypass_successor_pruning(const p::TaskPtr<Kind>& task)
{
    for_each_execution_mode(
        [&](size_t num_workers, p::StateRepositoryMode mode)
        {
            {
                auto context = make_search_context(task);
                auto heuristic = p::BlindHeuristic<Kind>::create();
                auto options = p::astar_eager::Options<Kind> {};
                options.num_search_workers = num_workers;
                options.state_repository_mode = mode;
                options.pruning_strategy = std::make_shared<AlwaysSuccessorPruningStrategy<Kind>>();
                expect_one_step_goal_result(p::astar_eager::find_solution(*task, *context.successor_generator, *heuristic, options));
            }
            {
                auto context = make_search_context(task);
                auto options = p::brfs::Options<Kind> {};
                options.num_search_workers = num_workers;
                options.state_repository_mode = mode;
                options.pruning_strategy = std::make_shared<AlwaysSuccessorPruningStrategy<Kind>>();
                expect_one_step_goal_result(p::brfs::find_solution(*task, *context.successor_generator, options));
            }
            {
                auto context = make_search_context(task);
                auto heuristic = p::BlindHeuristic<Kind>::create();
                auto options = p::gbfs_lazy::Options<Kind> {};
                options.num_search_workers = num_workers;
                options.state_repository_mode = mode;
                options.pruning_strategy = std::make_shared<AlwaysSuccessorPruningStrategy<Kind>>();
                expect_one_step_goal_result(p::gbfs_lazy::find_solution(*task, *context.successor_generator, *heuristic, options));
            }
        });
}

template<TaskKind Kind>
void expect_iw_goal_bypasses_novelty_pruning(const p::TaskPtr<Kind>& task)
{
    for_each_execution_mode(
        [&](size_t num_workers, p::StateRepositoryMode mode)
        {
            auto context = make_search_context(task);
            auto brfs_options = p::brfs::Options<Kind> {};
            brfs_options.num_search_workers = num_workers;
            brfs_options.state_repository_mode = mode;
            auto solver = p::brfs::Solver<Kind> { task, context.successor_generator, std::move(brfs_options) };
            auto event_handler = p::iw::DefaultEventHandler<Kind>::create();
            auto options = p::iw::Options<Kind> {};
            options.event_handler = event_handler;

            const auto result = p::iw::find_solution(solver, 1, options);
            ASSERT_EQ(result.status, p::SearchStatus::SOLVED);
            ASSERT_TRUE(result.plan);
            EXPECT_EQ(result.plan->get_length(), 3);
            const auto solution_arity = event_handler->get_statistics().get_solution_arity();
            ASSERT_TRUE(solution_arity);
            EXPECT_EQ(*solution_arity, 1);
        });
}

template<TaskKind Kind>
void expect_parallel_goal_workers_receive_caller_seed(const p::TaskPtr<Kind>& task)
{
    for (const auto mode : { p::StateRepositoryMode::HASH_DISTRIBUTED, p::StateRepositoryMode::SHARED })
    {
        auto caller = make_search_context(task);
        auto foreign = make_search_context(task);
        const auto foreign_start = foreign.successor_generator->get_initial_node();
        const auto normalized_start = p::materialize_state(foreign_start.get_state(), *caller.repository);
        ASSERT_NE(normalized_start.identifying_members(), foreign_start.get_state().identifying_members());

        auto goal_strategy = std::make_shared<SeedRecordingGoalStrategy<Kind>>(*task, normalized_start.identifying_members());
        auto options = p::brfs::Options<Kind> {};
        options.start_node = foreign_start;
        options.goal_strategy = goal_strategy;
        options.num_search_workers = 2;
        options.state_repository_mode = mode;

        const auto result = p::brfs::find_solution(*task, *caller.successor_generator, options);
        ASSERT_EQ(result.status, p::SearchStatus::SOLVED);
        EXPECT_TRUE(goal_strategy->observed_expected_seeds());
        EXPECT_EQ(goal_strategy->get_num_root_checks(), 1);
        EXPECT_EQ(goal_strategy->get_num_successor_checks(), 1);
    }
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

            options.goal_strategy = std::make_shared<AlwaysGoalStrategy<Kind>>();
            const auto immediate_result = p::astar_eager::find_solution(*task, *caller.successor_generator, *heuristic, options);
            ASSERT_TRUE(immediate_result.plan);
            EXPECT_EQ(immediate_result.plan->get_start_node().get_state().get_state_repository(), caller.repository);
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

template<TaskKind Kind>
void expect_terminal_roots_skip_heuristic(const p::TaskPtr<Kind>& task)
{
    for_each_execution_mode(
        [&](size_t num_workers, p::StateRepositoryMode mode)
        {
            const auto check_astar = [&](auto configure, p::SearchStatus expected_status)
            {
                auto context = make_search_context(task);
                auto heuristic = ThrowingHeuristic<Kind> {};
                auto options = p::astar_eager::Options<Kind> {};
                options.num_search_workers = num_workers;
                options.state_repository_mode = mode;
                configure(options);

                auto result = p::SearchResult<Kind> {};
                ASSERT_NO_THROW(result = p::astar_eager::find_solution(*task, *context.successor_generator, heuristic, options));
                EXPECT_EQ(result.status, expected_status);
            };
            const auto check_gbfs = [&](auto configure, p::SearchStatus expected_status)
            {
                auto context = make_search_context(task);
                auto heuristic = ThrowingHeuristic<Kind> {};
                auto options = p::gbfs_lazy::Options<Kind> {};
                options.num_search_workers = num_workers;
                options.state_repository_mode = mode;
                configure(options);

                auto result = p::SearchResult<Kind> {};
                ASSERT_NO_THROW(result = p::gbfs_lazy::find_solution(*task, *context.successor_generator, heuristic, options));
                EXPECT_EQ(result.status, expected_status);
            };

            const auto dynamic_goal = [](auto& options) { options.goal_strategy = std::make_shared<AlwaysGoalStrategy<Kind>>(); };
            const auto static_failure = [](auto& options) { options.goal_strategy = std::make_shared<StaticallyUnsatisfiedGoalStrategy<Kind>>(); };
            const auto no_capacity = [](auto& options) { options.max_num_states = 0; };
            const auto root_pruned = [](auto& options) { options.pruning_strategy = std::make_shared<AlwaysPruningStrategy<Kind>>(); };

            check_astar(dynamic_goal, p::SearchStatus::SOLVED);
            check_astar(static_failure, p::SearchStatus::UNSOLVABLE);
            check_astar(no_capacity, p::SearchStatus::OUT_OF_STATES);
            check_astar(root_pruned, p::SearchStatus::EXHAUSTED);
            check_gbfs(dynamic_goal, p::SearchStatus::SOLVED);
            check_gbfs(static_failure, p::SearchStatus::UNSOLVABLE);
            check_gbfs(no_capacity, p::SearchStatus::OUT_OF_STATES);
            check_gbfs(root_pruned, p::SearchStatus::EXHAUSTED);
        });
}

template<TaskKind Kind>
void expect_non_finite_start_metrics_are_rejected(const p::TaskPtr<Kind>& task)
{
    for_each_execution_mode(
        [&](size_t num_workers, p::StateRepositoryMode mode)
        {
            for (const auto metric : { std::numeric_limits<ygg::float_t>::quiet_NaN(), std::numeric_limits<ygg::float_t>::infinity() })
            {
                {
                    auto context = make_search_context(task);
                    auto heuristic = p::BlindHeuristic<Kind>::create();
                    auto options = p::astar_eager::Options<Kind> {};
                    options.start_node = p::Node<Kind>(context.successor_generator->get_initial_node().get_state(), metric);
                    options.num_search_workers = num_workers;
                    options.state_repository_mode = mode;
                    options.goal_strategy = std::make_shared<AlwaysGoalStrategy<Kind>>();
                    EXPECT_THROW(p::astar_eager::find_solution(*task, *context.successor_generator, *heuristic, options), std::runtime_error);
                }
                {
                    auto context = make_search_context(task);
                    auto heuristic = p::BlindHeuristic<Kind>::create();
                    auto options = p::gbfs_lazy::Options<Kind> {};
                    options.start_node = p::Node<Kind>(context.successor_generator->get_initial_node().get_state(), metric);
                    options.num_search_workers = num_workers;
                    options.state_repository_mode = mode;
                    options.goal_strategy = std::make_shared<AlwaysGoalStrategy<Kind>>();
                    EXPECT_THROW(p::gbfs_lazy::find_solution(*task, *context.successor_generator, *heuristic, options), std::runtime_error);
                }
                {
                    auto context = make_search_context(task);
                    auto options = p::brfs::Options<Kind> {};
                    options.start_node = p::Node<Kind>(context.successor_generator->get_initial_node().get_state(), metric);
                    options.num_search_workers = num_workers;
                    options.state_repository_mode = mode;
                    options.goal_strategy = std::make_shared<AlwaysGoalStrategy<Kind>>();
                    EXPECT_THROW(p::brfs::find_solution(*task, *context.successor_generator, options), std::runtime_error);
                }
            }
        });
}

template<TaskKind Kind>
void expect_dead_end_statistics(const p::TaskPtr<Kind>& task)
{
    const auto expect_one_dead_end = [](const p::SearchResult<Kind>& result)
    {
        EXPECT_EQ(result.statistics.get_num_deadends(), 1);
        auto worker_total = uint64_t { 0 };
        for (const auto& statistics : result.worker_statistics)
            worker_total += statistics.get_num_deadends();
        EXPECT_EQ(worker_total, 1);
    };

    for_each_execution_mode(
        [&](size_t num_workers, p::StateRepositoryMode mode)
        {
            {
                auto context = make_search_context(task);
                auto heuristic = InfiniteHeuristic<Kind> {};
                auto options = p::astar_eager::Options<Kind> {};
                options.num_search_workers = num_workers;
                options.state_repository_mode = mode;
                options.goal_strategy = p::ExhaustiveGoalStrategy<Kind>::create();
                const auto result = p::astar_eager::find_solution(*task, *context.successor_generator, heuristic, options);
                EXPECT_EQ(result.status, p::SearchStatus::UNSOLVABLE);
                expect_one_dead_end(result);
            }
            {
                auto context = make_search_context(task);
                auto heuristic = InfiniteHeuristic<Kind> {};
                auto options = p::gbfs_lazy::Options<Kind> {};
                options.num_search_workers = num_workers;
                options.state_repository_mode = mode;
                options.goal_strategy = p::ExhaustiveGoalStrategy<Kind>::create();
                const auto result = p::gbfs_lazy::find_solution(*task, *context.successor_generator, heuristic, options);
                EXPECT_EQ(result.status, p::SearchStatus::UNSOLVABLE);
                expect_one_dead_end(result);
            }
            {
                auto context = make_search_context(task);
                auto heuristic = GoalAsDeadEndHeuristic<Kind>(task);
                auto options = p::astar_eager::Options<Kind> {};
                options.num_search_workers = num_workers;
                options.state_repository_mode = mode;
                options.goal_strategy = p::ExhaustiveGoalStrategy<Kind>::create();
                const auto result = p::astar_eager::find_solution(*task, *context.successor_generator, heuristic, options);
                EXPECT_EQ(result.status, p::SearchStatus::EXHAUSTED);
                expect_one_dead_end(result);
            }
            {
                auto context = make_search_context(task);
                auto heuristic = GoalAsDeadEndHeuristic<Kind>(task);
                auto options = p::gbfs_lazy::Options<Kind> {};
                options.num_search_workers = num_workers;
                options.state_repository_mode = mode;
                options.goal_strategy = p::ExhaustiveGoalStrategy<Kind>::create();
                const auto result = p::gbfs_lazy::find_solution(*task, *context.successor_generator, heuristic, options);
                EXPECT_EQ(result.status, p::SearchStatus::EXHAUSTED);
                expect_one_dead_end(result);
            }
        });
}

template<TaskKind Kind>
void expect_sequential_expansion_stops_materializing_after_goal(const p::TaskPtr<Kind>& task)
{
    auto context = make_search_context(task);
    auto options = p::brfs::Options<Kind> {};
    options.goal_strategy = std::make_shared<NonRootGoalStrategy<Kind>>();

    const auto result = p::brfs::find_solution(*task, *context.successor_generator, options);
    ASSERT_EQ(result.status, p::SearchStatus::SOLVED);
    ASSERT_TRUE(result.plan);
    EXPECT_EQ(result.plan->get_length(), 1);
    EXPECT_EQ(result.statistics.get_num_registered_states(), 2);
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
    const auto tasks = parse_pruned_improvement_tasks();
    expect_astar_pruned_improvement_preserves_existing_state(tasks.ground);
    expect_astar_pruned_improvement_preserves_existing_state(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, GoalsBypassSuccessorPruning)
{
    const auto tasks = parse_tasks("parallel_search_simple.pddl");
    expect_goals_bypass_successor_pruning(tasks.ground);
    expect_goals_bypass_successor_pruning(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, IwGoalsBypassNoveltyPruning)
{
    const auto tasks = parse_tasks("iw_goal_before_novelty_domain.pddl", "iw_goal_before_novelty_problem.pddl");
    expect_iw_goal_bypasses_novelty_pruning(tasks.ground);
    expect_iw_goal_bypasses_novelty_pruning(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, ParallelGoalWorkersReceiveCallerNormalizedSeed)
{
    const auto tasks = parse_tasks("parallel_search_simple.pddl");
    expect_parallel_goal_workers_receive_caller_seed(tasks.ground);
    expect_parallel_goal_workers_receive_caller_seed(tasks.lifted);
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

TEST(TyrPlanningSearchEngineTest, TerminalRootsSkipHeuristicEvaluation)
{
    const auto tasks = parse_tasks("parallel_search_simple.pddl");
    expect_terminal_roots_skip_heuristic(tasks.ground);
    expect_terminal_roots_skip_heuristic(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, RejectsNonFiniteStartMetricsBeforeRecognizingGoals)
{
    const auto tasks = parse_tasks("parallel_search_simple.pddl");
    expect_non_finite_start_metrics_are_rejected(tasks.ground);
    expect_non_finite_start_metrics_are_rejected(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, CountsEachDeadEndOnce)
{
    const auto tasks = parse_tasks("parallel_search_simple.pddl");
    expect_dead_end_statistics(tasks.ground);
    expect_dead_end_statistics(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, SequentialExpansionStopsMaterializingAfterGoal)
{
    const auto tasks = parse_tasks("parallel_search_diamond.pddl");
    expect_sequential_expansion_stops_materializing_after_goal(tasks.ground);
    expect_sequential_expansion_stops_materializing_after_goal(tasks.lifted);
}

}
}
