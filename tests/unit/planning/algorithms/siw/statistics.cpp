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

#include "statistics.hpp"

#include <gtest/gtest.h>
#include <tyr/planning/declarations.hpp>

namespace tyr::tests
{
namespace
{
class GroundSiwStatisticsTest : public ::testing::TestWithParam<SiwCase>
{
};

TEST_P(GroundSiwStatisticsTest, MatchesExpectedOutcome) { check_siw_case<GroundTag>(GetParam()); }

INSTANTIATE_TEST_SUITE_P(TyrPlanningGroundSiwStatistics,
                         GroundSiwStatisticsTest,
                         ::testing::ValuesIn(load_cases("tests/support/data/planning/algorithms/ground/siw.json")),
                         [](const testing::TestParamInfo<SiwCase>& param_info) { return param_info.param.name; });
}
}

namespace tyr::tests
{
namespace
{
class LiftedSiwStatisticsTest : public ::testing::TestWithParam<SiwCase>
{
};

TEST_P(LiftedSiwStatisticsTest, MatchesExpectedOutcome) { check_siw_case<LiftedTag>(GetParam()); }

INSTANTIATE_TEST_SUITE_P(TyrPlanningLiftedSiwStatistics,
                         LiftedSiwStatisticsTest,
                         ::testing::ValuesIn(load_cases("tests/support/data/planning/algorithms/lifted/siw.json")),
                         [](const testing::TestParamInfo<SiwCase>& param_info) { return param_info.param.name; });
}
}
