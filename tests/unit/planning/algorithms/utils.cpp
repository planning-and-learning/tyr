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

#include <gtest/gtest.h>
#include <stdexcept>
#include <string_view>
#include <tyr/planning/algorithms/astar_eager.hpp>
#include <tyr/planning/algorithms/brfs.hpp>
#include <tyr/planning/algorithms/gbfs_lazy.hpp>
#include <tyr/planning/algorithms/iw.hpp>
#include <tyr/planning/algorithms/siw.hpp>
#include <tyr/planning/algorithms/utils.hpp>
#include <tyr/planning/heuristics/blind.hpp>
#include <yggdrasil/core/config.hpp>

namespace p = tyr::planning;

namespace tyr::tests
{
namespace
{
template<typename Callable>
void expect_invalid_argument_message(Callable&& callable, std::string_view expected_message)
{
    try
    {
        callable();
        FAIL() << "Expected std::invalid_argument.";
    }
    catch (const std::invalid_argument& exception)
    {
        EXPECT_NE(std::string_view(exception.what()).find(expected_message), std::string_view::npos) << exception.what();
    }
}
}

TEST(PlanningAlgorithmUtilsTest, ComputeSuccessorGValueUsesUnitActionCosts) { EXPECT_EQ(p::compute_successor_g_value(3.0, 9.0, ::tyr::CostMode::UNIT), 4.0); }

TEST(PlanningAlgorithmUtilsTest, ComputeSuccessorGValueUsesGeneratedMetricWithGeneralActionCosts)
{
    EXPECT_EQ(p::compute_successor_g_value(3.0, 9.0, ::tyr::CostMode::GENERAL), 9.0);
}

TEST(PlanningAlgorithmUtilsTest, ComputeSuccessorGValueRejectsUnknownCostMode)
{
    EXPECT_THROW((void) p::compute_successor_g_value(3.0, 9.0, static_cast<::tyr::CostMode>(255)), std::runtime_error);
}

TEST(PlanningAlgorithmUtilsTest, BlindHeuristicHasNoPreferredActionsByDefault)
{
    auto heuristic = p::BlindHeuristic<::tyr::GroundTag> {};

    EXPECT_TRUE(heuristic.get_preferred_actions().empty());
}

TEST(PlanningAlgorithmUtilsTest, DefaultConstructedBasicSolverAdaptersRejectMissingRequiredMembers)
{
    auto astar_solver = p::astar_eager::Solver<::tyr::GroundTag> {};
    auto brfs_solver = p::brfs::Solver<::tyr::GroundTag> {};
    auto gbfs_solver = p::gbfs_lazy::Solver<::tyr::GroundTag> {};
    auto iw_solver = p::iw::Solver<::tyr::GroundTag> {};
    auto siw_solver = p::siw::Solver<::tyr::GroundTag> {};

    expect_invalid_argument_message([&]() { (void) astar_solver.solve(); }, "astar_eager::Solver::solve(): task is required");
    expect_invalid_argument_message([&]() { (void) brfs_solver.solve(); }, "brfs::Solver::solve(): task is required");
    expect_invalid_argument_message([&]() { (void) gbfs_solver.solve(); }, "gbfs_lazy::Solver::solve(): task is required");
    expect_invalid_argument_message([&]() { (void) iw_solver.solve(); }, "iw::find_solution(...): BRFS task is required");
    expect_invalid_argument_message([&]() { (void) siw_solver.solve(); }, "siw::find_solution(...): IW BRFS task is required");
}

TEST(PlanningAlgorithmUtilsTest, DefaultConstructedWidthSolverAdaptersExposeNestedDefaults)
{
    auto iw_solver = p::iw::Solver<::tyr::GroundTag> {};

    EXPECT_EQ(iw_solver.brfs_solver.task, nullptr);
    EXPECT_EQ(iw_solver.brfs_solver.state_repository, nullptr);
    EXPECT_EQ(iw_solver.brfs_solver.axiom_evaluator, nullptr);
    EXPECT_EQ(iw_solver.brfs_solver.successor_generator, nullptr);
    EXPECT_EQ(iw_solver.max_arity, p::iw::MaxArity);
    EXPECT_EQ(iw_solver.options.event_handler, nullptr);

    auto siw_solver = p::siw::Solver<::tyr::GroundTag> {};

    EXPECT_EQ(siw_solver.iw_solver.brfs_solver.task, nullptr);
    EXPECT_EQ(siw_solver.iw_solver.brfs_solver.state_repository, nullptr);
    EXPECT_EQ(siw_solver.iw_solver.brfs_solver.axiom_evaluator, nullptr);
    EXPECT_EQ(siw_solver.iw_solver.brfs_solver.successor_generator, nullptr);
    EXPECT_EQ(siw_solver.iw_solver.max_arity, p::iw::MaxArity);
    EXPECT_EQ(siw_solver.iw_solver.options.event_handler, nullptr);
    EXPECT_EQ(siw_solver.options.event_handler, nullptr);
}

}
