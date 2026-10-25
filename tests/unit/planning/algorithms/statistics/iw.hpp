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

#ifndef TYR_TESTS_UNIT_PLANNING_ALGORITHMS_STATISTICS_IW_HPP_
#define TYR_TESTS_UNIT_PLANNING_ALGORITHMS_STATISTICS_IW_HPP_

#include "statistics.hpp"

#include <filesystem>
#include <tyr/planning/algorithms/brfs.hpp>
#include <tyr/planning/algorithms/brfs/event_handler.hpp>
#include <tyr/planning/algorithms/iw.hpp>
#include <tyr/planning/algorithms/iw/event_handler.hpp>

namespace tyr::tests
{
struct IwExpectation
{
    std::optional<tyr::planning::SearchStatus> status;
    std::optional<PlanStatistics> plan;
    std::optional<ygg::uint_t> solution_arity;
    std::optional<tyr::planning::ProgressStatistics::Snapshot> counters;
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
    result.solution_arity = ygg::common::find_uint_t(object, "solution_arity", "case");
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

std::vector<IwCase> load_cases(const std::filesystem::path& fixture)
{
    const auto suite = ygg::common::load_json_file(ygg::common::root_path() / fixture);
    const auto& suite_object = ygg::common::as_object(suite, "suite");
    auto result = std::vector<IwCase> {};
    for (const auto& case_value : ygg::common::as_array(suite_object, "cases", "suite"))
        result.push_back(parse_case(suite_object, ygg::common::as_object(case_value, "case")));
    return result;
}

template<TaskKind Kind>
void check_expectation(const IwExpectation& expectation, const IwCase& test_case)
{
    auto context = create_search_context<Kind>(test_case.domain_file, test_case.task_file);
    auto brfs_solver = tyr::planning::brfs::Solver<Kind> { context.task,
                                                           context.state_repository,
                                                           context.axiom_evaluator,
                                                           context.successor_generator,
                                                           tyr::planning::brfs::Options<Kind> {} };
    brfs_solver.options.event_handler = tyr::planning::brfs::DefaultEventHandler<Kind>::create();

    auto options = tyr::planning::iw::Options<Kind> {};
    const auto event_handler = tyr::planning::iw::DefaultEventHandler<Kind>::create();
    options.event_handler = event_handler;

    const auto result = tyr::planning::iw::find_solution(brfs_solver, test_case.max_arity, options);

    const auto solution_arity = event_handler->get_statistics().get_solution_arity();
    if (result.status == tyr::planning::SearchStatus::SOLVED)
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
        expect_plan(*expectation.plan, result.plan.value());
        EXPECT_TRUE(solution_arity);
    }
    if (expectation.solution_arity)
    {
        ASSERT_TRUE(solution_arity);
        EXPECT_EQ(solution_arity.value(), *expectation.solution_arity);
    }
    if (expectation.counters)
    {
        expect_counters(*expectation.counters, result.statistics);
    }
    expect_repository_statistics(context, result.statistics);
}

/// Checks every configuration of one fixture case.
template<TaskKind Kind>
void check_iw_case(const IwCase& test_case)
{
    for (const auto& [key, expectation] : test_case.configs)
    {
        SCOPED_TRACE(key);
        check_expectation<Kind>(expectation, test_case);
    }
}

}

#endif
