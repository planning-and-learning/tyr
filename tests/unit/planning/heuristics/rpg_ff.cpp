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

#include "tyr/planning/heuristics/rpg_ff.hpp"

#include "heuristic.hpp"

namespace tyr::tests
{
TEST(TyrPlanningGroundFFRPGHeuristicTest, RelaxedPlanDescribesLatestEvaluation) { expect_relaxed_plan_reset_after_dead_end<GroundTag>(); }
TEST(TyrPlanningGroundFFRPGHeuristicTest, WorkerRelaxedPlanIsIndependent) { expect_worker_has_independent_relaxed_plan<GroundTag>(); }
}

namespace tyr::tests
{
namespace
{
class GroundHeuristicFixtureTest : public ::testing::TestWithParam<HeuristicCase>
{
};

TEST(TyrPlanningGroundHeuristicWorkerTest, SnapshotsConfiguredGoal) { expect_worker_snapshots_configured_goal<GroundTag, tyr::planning::FFRPGHeuristic>(); }

TEST(TyrPlanningGroundHeuristicEvaluationTest, AlternatingStatesReplacePreviousModel)
{ expect_alternating_states_replace_previous_model<GroundTag, tyr::planning::FFRPGHeuristic>(); }

TEST_P(GroundHeuristicFixtureTest, InitialStateMatchesFixture) { check_heuristic_case<GroundTag, tyr::planning::FFRPGHeuristic>(GetParam()); }

INSTANTIATE_TEST_SUITE_P(TyrPlanningGroundHeuristicFixture,
                         GroundHeuristicFixtureTest,
                         ::testing::ValuesIn(load_cases("tests/fixtures/planning/heuristics/ground/rpg_ff.json")),
                         [](const testing::TestParamInfo<HeuristicCase>& param_info) { return param_info.param.name; });
}
}

namespace tyr::tests
{
TEST(TyrPlanningLiftedFFRPGHeuristicTest, RelaxedPlanDescribesLatestEvaluation) { expect_relaxed_plan_reset_after_dead_end<LiftedTag>(); }
TEST(TyrPlanningLiftedFFRPGHeuristicTest, WorkerRelaxedPlanIsIndependent) { expect_worker_has_independent_relaxed_plan<LiftedTag>(); }
}

namespace tyr::tests
{
namespace
{
class LiftedHeuristicFixtureTest : public ::testing::TestWithParam<HeuristicCase>
{
};

TEST(TyrPlanningLiftedHeuristicWorkerTest, SnapshotsConfiguredGoal) { expect_worker_snapshots_configured_goal<LiftedTag, tyr::planning::FFRPGHeuristic>(); }

TEST(TyrPlanningLiftedHeuristicEvaluationTest, AlternatingStatesReplacePreviousModel)
{ expect_alternating_states_replace_previous_model<LiftedTag, tyr::planning::FFRPGHeuristic>(); }

TEST_P(LiftedHeuristicFixtureTest, InitialStateMatchesFixture) { check_heuristic_case<LiftedTag, tyr::planning::FFRPGHeuristic>(GetParam()); }

INSTANTIATE_TEST_SUITE_P(TyrPlanningLiftedHeuristicFixture,
                         LiftedHeuristicFixtureTest,
                         ::testing::ValuesIn(load_cases("tests/fixtures/planning/heuristics/lifted/rpg_ff.json")),
                         [](const testing::TestParamInfo<HeuristicCase>& param_info) { return param_info.param.name; });
}
}
