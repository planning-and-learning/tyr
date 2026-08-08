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

#include <filesystem>

namespace p = tyr::planning;

namespace tyr::tests
{
struct SiwExpectation
{
    std::optional<p::SearchStatus> status;
    std::optional<PlanStatistics> plan;
    std::optional<ygg::uint_t> maximum_effective_width;
    std::optional<p::ProgressStatistics::Snapshot> counters;
};

struct SiwCase
{
    std::string name;
    std::filesystem::path domain_file;
    std::filesystem::path task_file;
    ygg::uint_t max_arity;
    std::vector<std::pair<std::string, SiwExpectation>> configs;
};

void PrintTo(const SiwCase& test_case, std::ostream* os) { *os << test_case.name << " (" << test_case.task_file << ")"; }

SiwExpectation parse_expectation(const boost::json::object& object)
{
    auto result = SiwExpectation {};
    if (const auto value = ygg::common::find_string(object, "status", "case"))
        result.status = parse_search_status(*value);
    result.plan = parse_optional_plan(object);
    if (const auto value = ygg::common::find_uint_t(object, "maximum_effective_width", "case"))
        result.maximum_effective_width = *value;
    result.counters = parse_optional_counters(object);
    return result;
}

SiwCase parse_case(const boost::json::object& suite, const boost::json::object& object)
{
    auto result = SiwCase { ygg::common::as_string(object, "name", "case"),
                            ygg::common::resolve_path(std::filesystem::path(BENCHMARKS_DIR), ygg::common::as_string(object, "domain_file", "case")),
                            ygg::common::resolve_path(std::filesystem::path(BENCHMARKS_DIR), ygg::common::as_string(object, "task_file", "case")),
                            ygg::common::as_uint_t(suite, "max_arity", "suite"),
                            {} };
    for (const auto& [key, value] : object)
        if (value.is_object() && value.as_object().if_contains("status"))
            result.configs.emplace_back(std::string(key), parse_expectation(value.as_object()));
    return result;
}

std::vector<SiwCase> load_cases()
{
    const auto suite = ygg::common::load_json_file(ygg::common::root_path() / kStatisticsFixture);
    const auto& suite_object = ygg::common::as_object(suite, "suite");
    auto result = std::vector<SiwCase> {};
    for (const auto& case_value : ygg::common::as_array(suite_object, "cases", "suite"))
        result.push_back(parse_case(suite_object, ygg::common::as_object(case_value, "case")));
    return result;
}

void check_expectation(const SiwExpectation& expectation, const SiwCase& test_case)
{
    auto context = create_search_context<StatisticsTaskKind>(test_case.domain_file, test_case.task_file);
    auto brfs_solver = p::brfs::Solver<StatisticsTaskKind> { context.task,
                                                             context.state_repository,
                                                             context.axiom_evaluator,
                                                             context.successor_generator,
                                                             p::brfs::Options<StatisticsTaskKind> {} };
    brfs_solver.options.event_handler = p::brfs::DefaultEventHandler<StatisticsTaskKind>::create();

    auto iw_solver = p::iw::Solver<StatisticsTaskKind> { std::move(brfs_solver), test_case.max_arity, p::iw::Options<StatisticsTaskKind> {} };
    const auto event_handler = p::siw::DefaultEventHandler<StatisticsTaskKind>::create();

    auto options = p::siw::Options<StatisticsTaskKind> {};
    options.event_handler = event_handler;

    const auto result = p::siw::find_solution(iw_solver, options);

    const auto maximum_effective_width = event_handler->get_statistics().get_maximum_effective_width();
    const auto average_effective_width = event_handler->get_statistics().get_average_effective_width();
    const auto num_solved_subsearches = event_handler->get_statistics().get_num_solved_subsearches();

    if (maximum_effective_width)
    {
        EXPECT_TRUE(average_effective_width);
        EXPECT_GT(num_solved_subsearches, 0);
        EXPECT_LE(*average_effective_width, static_cast<double>(*maximum_effective_width));
    }
    else
    {
        EXPECT_FALSE(average_effective_width);
        EXPECT_EQ(num_solved_subsearches, 0);
    }

    if (expectation.status)
    {
        EXPECT_EQ(result.status, *expectation.status);
    }
    if (expectation.plan)
    {
        ASSERT_TRUE(result.plan);
        expect_plan(*expectation.plan, *result.plan);
    }
    if (expectation.maximum_effective_width)
    {
        ASSERT_TRUE(maximum_effective_width);
        EXPECT_EQ(*maximum_effective_width, *expectation.maximum_effective_width);
    }
    if (expectation.counters)
    {
        expect_counters(*expectation.counters, result.statistics);
    }
    expect_repository_statistics(context, result.statistics);
}

class SiwStatisticsTest : public ::testing::TestWithParam<SiwCase>
{
};

TEST_P(SiwStatisticsTest, MatchesExpectedOutcome)
{
    const auto& test_case = GetParam();

    for (const auto& [key, expectation] : test_case.configs)
    {
        SCOPED_TRACE(key);
        check_expectation(expectation, test_case);
    }
}

INSTANTIATE_TEST_SUITE_P(TyrPlanningSiwStatistics,
                         SiwStatisticsTest,
                         ::testing::ValuesIn(load_cases()),
                         [](const testing::TestParamInfo<SiwCase>& info) { return info.param.name; });

}
