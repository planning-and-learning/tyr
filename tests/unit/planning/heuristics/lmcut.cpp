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

#include "tyr/planning/heuristics/lmcut.hpp"

#include "heuristic.hpp"
#include "tyr/planning/ground/successor_generator.hpp"
#include "tyr/planning/lifted/successor_generator.hpp"

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <new>
#include <utility>

namespace
{
/// The allocations of this program while it counts them: its operator new does.
struct AllocationCounter
{
    std::atomic<std::size_t> allocations { 0 };
    std::atomic<bool> counting { false };
};

AllocationCounter& allocation_counter() noexcept
{
    static AllocationCounter counter;
    return counter;
}

void* counted(void* block)
{
    auto& counter = allocation_counter();
    if (counter.counting.load(std::memory_order_relaxed))
        counter.allocations.fetch_add(1, std::memory_order_relaxed);
    if (block == nullptr)
        throw std::bad_alloc();
    return block;
}
}

// NOLINTBEGIN(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc): the replaced allocation functions
void* operator new(std::size_t size) { return counted(std::malloc(size)); }
void* operator new(std::size_t size, std::align_val_t alignment) { return counted(std::aligned_alloc(static_cast<std::size_t>(alignment), size)); }
void operator delete(void* block) noexcept { std::free(block); }
void operator delete(void* block, std::size_t /*size*/) noexcept { std::free(block); }
void operator delete(void* block, std::align_val_t /*alignment*/) noexcept { std::free(block); }
void operator delete(void* block, std::size_t /*size*/, std::align_val_t /*alignment*/) noexcept { std::free(block); }
// NOLINTEND(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc)

namespace tyr::tests
{
namespace
{
/// An evaluation allocates nothing once its buffers have the size the state needs; the first does, which shows that
/// the allocations are counted.
template<TaskKind Kind>
void expect_evaluation_allocates_nothing_after_warmup(const HeuristicCase& test_case)
{
    auto context = create_heuristic_context<Kind>(test_case.domain_file, test_case.task_file);
    auto heuristic = tyr::planning::LMCutHeuristic<Kind>::create(context.task, context.execution_context);
    const auto state = context.successor_generator->get_initial_node(*context.state_repository, *context.axiom_evaluator).get_state();
    const auto evaluate = [&]
    {
        auto& counter = allocation_counter();
        counter.allocations = 0;
        counter.counting = true;
        const auto value = heuristic->evaluate(state);
        counter.counting = false;
        return std::pair { value, counter.allocations.load() };
    };

    const auto [value, first] = evaluate();
    EXPECT_GT(first, 0U);
    for (int warmup = 0; warmup < 3; ++warmup)
        EXPECT_EQ(evaluate().first, value);
    EXPECT_EQ(evaluate(), (std::pair { value, std::size_t { 0 } }));
}
}
}

TEST(TyrPlanningGroundLMCutHeuristicTest, GeneralCostModeThrows)
{ tyr::tests::expect_general_cost_mode_throws<tyr::GroundTag, tyr::planning::LMCutHeuristic>(); }

TEST(TyrPlanningGroundLMCutHeuristicTest, WeightedAlternativeCountsOneUnitAction)
{
    const auto fixture = ygg::common::root_path() / "tests/support/data/planning/heuristics/lmcut_weighted_alternative";
    auto context = tyr::tests::create_heuristic_context<tyr::GroundTag>(fixture / "domain.pddl", fixture / "problem.pddl");
    const auto state = context.successor_generator->get_initial_node(*context.state_repository, *context.axiom_evaluator).get_state();
    auto unit = tyr::planning::LMCutHeuristic<tyr::GroundTag>::create(context.task, context.execution_context, tyr::CostMode::UNIT);

    EXPECT_EQ(unit->evaluate(state), 1);
}

namespace tyr::tests
{
namespace
{
class GroundHeuristicFixtureTest : public ::testing::TestWithParam<HeuristicCase>
{
};

TEST(TyrPlanningGroundHeuristicWorkerTest, SnapshotsConfiguredGoal) { expect_worker_snapshots_configured_goal<GroundTag, tyr::planning::LMCutHeuristic>(); }

TEST(TyrPlanningGroundHeuristicEvaluationTest, AlternatingStatesReplacePreviousModel)
{ expect_alternating_states_replace_previous_model<GroundTag, tyr::planning::LMCutHeuristic>(); }

TEST_P(GroundHeuristicFixtureTest, InitialStateMatchesFixture) { check_heuristic_case<GroundTag, tyr::planning::LMCutHeuristic>(GetParam()); }

TEST_P(GroundHeuristicFixtureTest, EvaluationAllocatesNothingAfterWarmup) { expect_evaluation_allocates_nothing_after_warmup<GroundTag>(GetParam()); }

INSTANTIATE_TEST_SUITE_P(TyrPlanningGroundHeuristicFixture,
                         GroundHeuristicFixtureTest,
                         ::testing::ValuesIn(load_cases("tests/support/data/planning/heuristics/ground/lmcut.json")),
                         [](const testing::TestParamInfo<HeuristicCase>& param_info) { return param_info.param.name; });
}
}

TEST(TyrPlanningLiftedLMCutHeuristicTest, GeneralCostModeThrows)
{ tyr::tests::expect_general_cost_mode_throws<tyr::LiftedTag, tyr::planning::LMCutHeuristic>(); }

TEST(TyrPlanningLiftedLMCutHeuristicTest, RepeatedEvaluationReusesResetWorkspace)
{
    auto context = tyr::tests::create_preferred_action_reset_context<tyr::LiftedTag>();
    auto heuristic = tyr::planning::LMCutHeuristic<tyr::LiftedTag>::create(context.task, context.execution_context);
    const auto state = context.successor_generator->get_initial_node(*context.state_repository, *context.axiom_evaluator).get_state();

    const auto first = heuristic->evaluate(state);
    EXPECT_EQ(heuristic->evaluate(state), first);
}

TEST(TyrPlanningLiftedLMCutHeuristicTest, WeightedAlternativeCountsOneUnitAction)
{
    const auto fixture = ygg::common::root_path() / "tests/support/data/planning/heuristics/lmcut_weighted_alternative";
    auto context = tyr::tests::create_heuristic_context<tyr::LiftedTag>(fixture / "domain.pddl", fixture / "problem.pddl");
    const auto state = context.successor_generator->get_initial_node(*context.state_repository, *context.axiom_evaluator).get_state();
    auto unit = tyr::planning::LMCutHeuristic<tyr::LiftedTag>::create(context.task, context.execution_context, tyr::CostMode::UNIT);

    EXPECT_EQ(unit->evaluate(state), 1);
}

namespace tyr::tests
{
namespace
{
class LiftedHeuristicFixtureTest : public ::testing::TestWithParam<HeuristicCase>
{
};

TEST(TyrPlanningLiftedHeuristicWorkerTest, SnapshotsConfiguredGoal) { expect_worker_snapshots_configured_goal<LiftedTag, tyr::planning::LMCutHeuristic>(); }

TEST(TyrPlanningLiftedHeuristicEvaluationTest, AlternatingStatesReplacePreviousModel)
{ expect_alternating_states_replace_previous_model<LiftedTag, tyr::planning::LMCutHeuristic>(); }

TEST_P(LiftedHeuristicFixtureTest, InitialStateMatchesFixture) { check_heuristic_case<LiftedTag, tyr::planning::LMCutHeuristic>(GetParam()); }

TEST_P(LiftedHeuristicFixtureTest, EvaluationAllocatesNothingAfterWarmup) { expect_evaluation_allocates_nothing_after_warmup<LiftedTag>(GetParam()); }

INSTANTIATE_TEST_SUITE_P(TyrPlanningLiftedHeuristicFixture,
                         LiftedHeuristicFixtureTest,
                         ::testing::ValuesIn(load_cases("tests/support/data/planning/heuristics/lifted/lmcut.json")),
                         [](const testing::TestParamInfo<HeuristicCase>& param_info) { return param_info.param.name; });
}
}
