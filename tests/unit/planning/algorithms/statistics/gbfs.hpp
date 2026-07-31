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
void check_statistics(const SearchStatistics& expected, const SearchCase& test_case, const std::string& heuristic_name, ::tyr::CostMode cost_mode)
{
    auto context = create_search_context<StatisticsTaskKind>(test_case.domain_file, test_case.task_file);
    const auto num_ground_actions_before = context.task->get_repository()->template size<::tyr::formalism::planning::GroundAction>();
    auto heuristic = create_search_heuristic<StatisticsTaskKind>(heuristic_name, context, cost_mode);
    auto event_handler = p::gbfs_lazy::DefaultEventHandler<StatisticsTaskKind>::create();

    auto options = p::gbfs_lazy::Options<StatisticsTaskKind>();
    options.event_handler = event_handler;
    options.cost_mode = cost_mode;
    const auto result = p::gbfs_lazy::find_solution(*context.task, *context.successor_generator, *heuristic, options);

    expect_statistics(expected, event_handler->get_statistics(), result);
    if constexpr (std::is_same_v<StatisticsTaskKind, ::tyr::LiftedTag>)
    {
        EXPECT_EQ(context.task->get_repository()->template size<::tyr::formalism::planning::GroundAction>(), num_ground_actions_before);
    }
}
}

class GbfsStatisticsTest : public ::testing::TestWithParam<SearchCase>
{
};

TEST_P(GbfsStatisticsTest, StatisticsMatchFixture)
{
    const auto& test_case = GetParam();

    for (const auto& [key, expected] : test_case.configs)
    {
        SCOPED_TRACE(key);
        const auto [heuristic_name, cost_mode] = parse_costed_heuristic_key(key);
        check_statistics(expected, test_case, heuristic_name, cost_mode);
    }
}

INSTANTIATE_TEST_SUITE_P(TyrPlanningGbfsStatistics,
                         GbfsStatisticsTest,
                         ::testing::ValuesIn(load_search_cases(kStatisticsFixture)),
                         [](const testing::TestParamInfo<SearchCase>& info) { return info.param.name; });

}
