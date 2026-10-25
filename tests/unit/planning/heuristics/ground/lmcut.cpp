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

#include "../heuristic.hpp"
#include "tyr/planning/ground/successor_generator.hpp"

TEST(TyrPlanningGroundLMCutHeuristicTest, GeneralCostModeThrows)
{ tyr::tests::expect_general_cost_mode_throws<tyr::GroundTag, tyr::planning::LMCutHeuristic>(); }

TEST(TyrPlanningGroundLMCutHeuristicTest, WeightedAlternativeCountsOneUnitAction)
{
    const auto fixture = ygg::common::root_path() / "tests/fixtures/planning/heuristics/lmcut_weighted_alternative";
    auto context = tyr::tests::create_heuristic_context<tyr::GroundTag>(fixture / "domain.pddl", fixture / "problem.pddl");
    const auto state = context.successor_generator->get_initial_node(*context.state_repository, *context.axiom_evaluator).get_state();
    auto unit = tyr::planning::LMCutHeuristic<tyr::GroundTag>::create(context.task, context.execution_context, tyr::CostMode::UNIT);

    EXPECT_EQ(unit->evaluate(state), 1);
}

namespace tyr::tests
{
namespace
{
class HeuristicFixtureTest : public ::testing::TestWithParam<HeuristicCase>
{
};

TEST(TyrPlanningHeuristicWorkerTest, SnapshotsConfiguredGoal) { expect_worker_snapshots_configured_goal<GroundTag, tyr::planning::LMCutHeuristic>(); }

TEST(TyrPlanningHeuristicEvaluationTest, AlternatingStatesReplacePreviousModel)
{ expect_alternating_states_replace_previous_model<GroundTag, tyr::planning::LMCutHeuristic>(); }

TEST_P(HeuristicFixtureTest, InitialStateMatchesFixture) { check_heuristic_case<GroundTag, tyr::planning::LMCutHeuristic>(GetParam()); }

INSTANTIATE_TEST_SUITE_P(TyrPlanningHeuristicFixture,
                         HeuristicFixtureTest,
                         ::testing::ValuesIn(load_cases("tests/fixtures/planning/heuristics/ground/lmcut.json")),
                         [](const testing::TestParamInfo<HeuristicCase>& param_info) { return param_info.param.name; });
}
}
