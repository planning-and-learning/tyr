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

#include "../brfs.hpp"

#include <gtest/gtest.h>
#include <tyr/planning/declarations.hpp>

namespace tyr::tests
{
namespace
{
class BrfsStatisticsTest : public ::testing::TestWithParam<SearchCase>
{
};

TEST_P(BrfsStatisticsTest, StatisticsMatchFixture) { check_brfs_case<LiftedTag>(GetParam()); }

INSTANTIATE_TEST_SUITE_P(TyrPlanningBrfsStatistics,
                         BrfsStatisticsTest,
                         ::testing::ValuesIn(load_search_cases("tests/fixtures/planning/algorithms/lifted/brfs.json")),
                         [](const testing::TestParamInfo<SearchCase>& info) { return info.param.name; });
}
}
