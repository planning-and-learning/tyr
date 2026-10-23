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

namespace tyr::tests
{
using HeuristicTaskKind = GroundTag;
template<TaskKind Kind>
using TestedHeuristic = planning::FFRPGHeuristic<Kind>;
inline constexpr const char* kHeuristicFixture = "tests/fixtures/planning/heuristics/ground/rpg_ff.json";
}

#include "../heuristic.hpp"

namespace tyr::tests
{
TEST(TyrPlanningGroundFFRPGHeuristicTest, RelaxedPlanDescribesLatestEvaluation) { expect_relaxed_plan_reset_after_dead_end<GroundTag>(); }
TEST(TyrPlanningGroundFFRPGHeuristicTest, WorkerRelaxedPlanIsIndependent) { expect_worker_has_independent_relaxed_plan<GroundTag>(); }
TEST(TyrPlanningGroundFFRPGHeuristicTest, TraversesExactNumericHeadSupports) { expect_ff_traverses_exact_numeric_head_supports<GroundTag>(); }
}
