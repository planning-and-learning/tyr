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

#ifndef TYR_TESTS_UNIT_PLANNING_ALGORITHMS_STATISTICS_BRFS_HPP_
#define TYR_TESTS_UNIT_PLANNING_ALGORITHMS_STATISTICS_BRFS_HPP_

#include "statistics.hpp"

#include <tyr/planning/algorithms/brfs.hpp>
#include <tyr/planning/algorithms/brfs/event_handler.hpp>

namespace tyr::tests
{
namespace
{
template<TaskKind Kind>
void check_statistics(const SearchStatistics& expected, const SearchCase& test_case)
{
    auto context = create_search_context<Kind>(test_case.domain_file, test_case.task_file);
    auto event_handler = tyr::planning::brfs::DefaultEventHandler<Kind>::create();

    auto options = tyr::planning::brfs::Options<Kind>();
    options.event_handler = event_handler;
    const auto result =
        tyr::planning::brfs::find_solution(*context.task, *context.state_repository, *context.axiom_evaluator, *context.successor_generator, options);

    expect_statistics(expected, result.statistics, result);
    expect_repository_statistics(context, result.statistics);
}
}

/// Checks every configuration of one fixture case.
template<TaskKind Kind>
void check_brfs_case(const SearchCase& test_case)
{
    for (const auto& [key, expected] : test_case.configs)
    {
        SCOPED_TRACE(key);
        check_statistics<Kind>(expected, test_case);
    }
}

}

#endif
