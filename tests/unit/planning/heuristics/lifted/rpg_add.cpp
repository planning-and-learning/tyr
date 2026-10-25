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

#include "tyr/planning/heuristics/rpg_add.hpp"

#include "../heuristic.hpp"

namespace tyr::tests
{
TEST(TyrPlanningLiftedAddRPGHeuristicTest, SetGoalReconfiguresEvaluator) { expect_builtin_set_goal_reconfigures_evaluator<LiftedTag>(); }
TEST(TyrPlanningLiftedAddRPGHeuristicTest, GeneralCostModeThrows) { expect_general_cost_mode_throws<LiftedTag, tyr::planning::AddRPGHeuristic>(); }
}

namespace tyr::tests
{
namespace
{
class HeuristicFixtureTest : public ::testing::TestWithParam<HeuristicCase>
{
};

TEST(TyrPlanningHeuristicWorkerTest, SnapshotsConfiguredGoal) { expect_worker_snapshots_configured_goal<LiftedTag, tyr::planning::AddRPGHeuristic>(); }

TEST(TyrPlanningHeuristicEvaluationTest, AlternatingStatesReplacePreviousModel)
{ expect_alternating_states_replace_previous_model<LiftedTag, tyr::planning::AddRPGHeuristic>(); }

TEST_P(HeuristicFixtureTest, InitialStateMatchesFixture) { check_heuristic_case<LiftedTag, tyr::planning::AddRPGHeuristic>(GetParam()); }

INSTANTIATE_TEST_SUITE_P(TyrPlanningHeuristicFixture,
                         HeuristicFixtureTest,
                         ::testing::ValuesIn(load_cases("tests/fixtures/planning/heuristics/lifted/rpg_add.json")),
                         [](const testing::TestParamInfo<HeuristicCase>& info) { return info.param.name; });
}
}
