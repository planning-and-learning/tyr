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

#include "tyr/planning/heuristics/rpg_max.hpp"

#include "../heuristic.hpp"

namespace tyr::tests
{
namespace
{
class HeuristicFixtureTest : public ::testing::TestWithParam<HeuristicCase>
{
};

TEST(TyrPlanningHeuristicWorkerTest, SnapshotsConfiguredGoal) { expect_worker_snapshots_configured_goal<GroundTag, tyr::planning::MaxRPGHeuristic>(); }

TEST(TyrPlanningHeuristicEvaluationTest, AlternatingStatesReplacePreviousModel)
{ expect_alternating_states_replace_previous_model<GroundTag, tyr::planning::MaxRPGHeuristic>(); }

TEST_P(HeuristicFixtureTest, InitialStateMatchesFixture) { check_heuristic_case<GroundTag, tyr::planning::MaxRPGHeuristic>(GetParam()); }

INSTANTIATE_TEST_SUITE_P(TyrPlanningHeuristicFixture,
                         HeuristicFixtureTest,
                         ::testing::ValuesIn(load_cases("tests/fixtures/planning/heuristics/ground/rpg_max.json")),
                         [](const testing::TestParamInfo<HeuristicCase>& info) { return info.param.name; });
}
}
