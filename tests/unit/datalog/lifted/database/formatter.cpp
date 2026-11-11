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

#include "tyr/datalog/lifted/database/syntax/formatter.hpp"

#include "query_helpers.hpp"
#include "yggdrasil/formatting/formatter.hpp"

#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <initializer_list>
#include <iterator>
#include <string>

namespace tyr::datalog::tests
{

TEST(YggdrasilTests, DatabaseQueryPlanFormatterPreservesExplanation)
{
    using Column = urd::index<ygg::Column>;
    auto repository = qb::repository();
    const std::array<ygg::ColumnLayout, 2> columns { ygg::ColumnLayout { Column(3), ygg::type_id<double> },
                                                     ygg::ColumnLayout { Column(8), ygg::type_id<std::int32_t> } };
    const auto input = qb::input(repository, 7, columns);
    const auto nullary = qb::input(repository, 2, std::initializer_list<Column> {});
    const auto join = qb::generic_join(repository, { input, nullary }, { Column(3), Column(8) }, { Column(8), Column(3) });
    const auto plan = tyr::datalog::database::compile({ join, input });
    const std::string expected = "0: input() columns=[3:5,8:2] slot=7\n"
                                 "1: input() columns=[] slot=2\n"
                                 "2: generic_join(0,1) columns=[8:2,3:5] variables=[3,8]\n"
                                 "roots=[2,0]\n";
    EXPECT_EQ(fmt::format("{}", plan), expected);
    EXPECT_EQ(ygg::to_string(plan), expected);
    EXPECT_EQ(database::explain(plan), expected);
    std::string appended;
    fmt::format_to(std::back_inserter(appended), "before{}after", plan);
    EXPECT_EQ(appended, "before" + expected + "after");
}

TEST(YggdrasilTests, DatabaseQueryPlanFormatterHandlesEmptyPlans)
{
    const database::QueryPlan plan;
    EXPECT_EQ(fmt::format("{}", plan), "roots=[]\n");
    EXPECT_EQ(database::explain(plan), "roots=[]\n");
}

}  // namespace tyr::datalog::tests
