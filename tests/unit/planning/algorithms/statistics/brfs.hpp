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

namespace p = tyr::planning;

namespace tyr::tests
{
namespace
{
void check_statistics(const SearchStatistics& expected, const SearchCase& test_case)
{
    auto context = create_search_context<StatisticsTaskKind>(test_case.domain_file, test_case.task_file);
    auto event_handler = p::brfs::DefaultEventHandler<StatisticsTaskKind>::create();

    auto options = p::brfs::Options<StatisticsTaskKind>();
    options.event_handler = event_handler;
    const auto result = p::brfs::find_solution(*context.task, *context.state_repository, *context.axiom_evaluator, *context.successor_generator, options);

    expect_statistics(expected, result.statistics, result);
    expect_repository_statistics(context, result.statistics);
}
}

class BrfsStatisticsTest : public ::testing::TestWithParam<SearchCase>
{
};

TEST_P(BrfsStatisticsTest, StatisticsMatchFixture)
{
    const auto& test_case = GetParam();

    for (const auto& [key, expected] : test_case.configs)
    {
        SCOPED_TRACE(key);
        check_statistics(expected, test_case);
    }
}

INSTANTIATE_TEST_SUITE_P(TyrPlanningBrfsStatistics,
                         BrfsStatisticsTest,
                         ::testing::ValuesIn(load_search_cases(kStatisticsFixture)),
                         [](const testing::TestParamInfo<SearchCase>& info) { return info.param.name; });

}
