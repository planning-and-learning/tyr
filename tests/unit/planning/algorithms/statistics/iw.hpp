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
struct IwExpectation
{
    std::optional<p::SearchStatus> status;
    std::optional<PlanStatistics> plan;
    std::optional<ygg::uint_t> solution_arity;
    std::optional<p::ProgressStatistics::Snapshot> counters;
};

struct IwCase
{
    std::string name;
    std::filesystem::path domain_file;
    std::filesystem::path task_file;
    ygg::uint_t max_arity;
    std::vector<std::pair<std::string, IwExpectation>> configs;
};

void PrintTo(const IwCase& test_case, std::ostream* os) { *os << test_case.name << " (" << test_case.task_file << ")"; }

IwExpectation parse_expectation(const boost::json::object& object)
{
    auto result = IwExpectation {};
    if (const auto value = ygg::common::find_string(object, "status", "case"))
        result.status = parse_search_status(*value);
    result.plan = parse_optional_plan(object);
    if (const auto value = ygg::common::find_uint_t(object, "solution_arity", "case"))
        result.solution_arity = *value;
    result.counters = parse_optional_counters(object);
    return result;
}

IwCase parse_case(const boost::json::object& suite, const boost::json::object& object)
{
    auto result = IwCase { ygg::common::as_string(object, "name", "case"),
                           ygg::common::resolve_path(std::filesystem::path(BENCHMARKS_DIR), ygg::common::as_string(object, "domain_file", "case")),
                           ygg::common::resolve_path(std::filesystem::path(BENCHMARKS_DIR), ygg::common::as_string(object, "task_file", "case")),
                           ygg::common::as_uint_t(suite, "max_arity", "suite"),
                           {} };
    for (const auto& [key, value] : object)
        if (value.is_object() && value.as_object().if_contains("status"))
            result.configs.emplace_back(std::string(key), parse_expectation(value.as_object()));
    return result;
}

std::vector<IwCase> load_cases()
{
    const auto suite = ygg::common::load_json_file(ygg::common::root_path() / kStatisticsFixture);
    const auto& suite_object = ygg::common::as_object(suite, "suite");
    auto result = std::vector<IwCase> {};
    for (const auto& case_value : ygg::common::as_array(suite_object, "cases", "suite"))
        result.push_back(parse_case(suite_object, ygg::common::as_object(case_value, "case")));
    return result;
}

void check_expectation(const IwExpectation& expectation, const IwCase& test_case)
{
    auto context = create_search_context<StatisticsTaskKind>(test_case.domain_file, test_case.task_file);
    auto brfs_solver = p::brfs::Solver<StatisticsTaskKind> { context.task,
                                                             context.state_repository,
                                                             context.axiom_evaluator,
                                                             context.successor_generator,
                                                             p::brfs::Options<StatisticsTaskKind> {} };
    brfs_solver.options.event_handler = p::brfs::DefaultEventHandler<StatisticsTaskKind>::create();

    auto options = p::iw::Options<StatisticsTaskKind> {};
    const auto event_handler = p::iw::DefaultEventHandler<StatisticsTaskKind>::create();
    options.event_handler = event_handler;

    const auto result = p::iw::find_solution(brfs_solver, test_case.max_arity, options);

    const auto solution_arity = event_handler->get_statistics().get_solution_arity();
    if (result.status == p::SearchStatus::SOLVED)
    {
        EXPECT_TRUE(solution_arity);
        if (solution_arity)
        {
            EXPECT_LE(*solution_arity, test_case.max_arity);
        }
    }
    else
    {
        EXPECT_FALSE(solution_arity);
    }

    if (expectation.status)
    {
        EXPECT_EQ(result.status, *expectation.status);
    }
    if (expectation.plan)
    {
        ASSERT_TRUE(result.plan);
        expect_plan(*expectation.plan, *result.plan);
        EXPECT_TRUE(solution_arity);
    }
    if (expectation.solution_arity)
    {
        ASSERT_TRUE(solution_arity);
        EXPECT_EQ(*solution_arity, *expectation.solution_arity);
    }
    if (expectation.counters)
    {
        expect_counters(*expectation.counters, result.statistics);
    }
    expect_repository_statistics(context, result.statistics);
}

class IwStatisticsTest : public ::testing::TestWithParam<IwCase>
{
};

TEST_P(IwStatisticsTest, MatchesExpectedOutcome)
{
    const auto& test_case = GetParam();

    for (const auto& [key, expectation] : test_case.configs)
    {
        SCOPED_TRACE(key);
        check_expectation(expectation, test_case);
    }
}

INSTANTIATE_TEST_SUITE_P(TyrPlanningIwStatistics,
                         IwStatisticsTest,
                         ::testing::ValuesIn(load_cases()),
                         [](const testing::TestParamInfo<IwCase>& info) { return info.param.name; });

}
