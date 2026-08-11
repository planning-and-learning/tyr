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

#include "tyr/planning/planning.hpp"

namespace tyr::tests
{
using HeuristicTaskKind = GroundTag;
template<TaskKind Kind>
using TestedHeuristic = ::tyr::planning::LMCutHeuristic<Kind>;
inline constexpr const char* kHeuristicFixture = "tests/fixtures/planning/heuristics/ground/lmcut.json";
}

#include "../heuristic.hpp"

TEST(TyrPlanningGroundLMCutHeuristicTest, GeneralMetricTargetsExactBinding) { tyr::tests::expect_general_metric_targets_exact_binding<tyr::GroundTag>(); }

TEST(TyrPlanningGroundLMCutHeuristicTest, WeightedAlternativeRemainsAdmissible)
{
    const auto fixture = ygg::common::root_path() / "tests/fixtures/planning/heuristics/lmcut_weighted_alternative";
    auto context = tyr::tests::create_heuristic_context<tyr::GroundTag>(fixture / "domain.pddl", fixture / "problem.pddl");
    const auto state = context.successor_generator->get_initial_node(*context.state_repository, *context.axiom_evaluator).get_state();
    auto general = tyr::planning::LMCutHeuristic<tyr::GroundTag>::create(context.task, context.execution_context, tyr::CostMode::GENERAL);
    auto unit = tyr::planning::LMCutHeuristic<tyr::GroundTag>::create(context.task, context.execution_context, tyr::CostMode::UNIT);

    EXPECT_EQ(general->evaluate(state), 50);
    EXPECT_EQ(unit->evaluate(state), 1);
}
