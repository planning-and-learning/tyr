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

#include "tyr/datalog/numeric_utils.hpp"

#include "tyr/datalog/policies/annotation_types.hpp"

#include <array>
#include <gtest/gtest.h>
#include <optional>

namespace tyr::tests
{

TEST(TyrDatalogNumericUtilsTest, SumsMetricEffectDeltasFromInitialCost)
{
    const auto effects = std::array { datalog::Cost(2), datalog::Cost(3) };
    const auto result = datalog::sum_metric_effect_deltas(datalog::Cost(5), effects, [](const auto effect) { return std::optional(effect); });

    ASSERT_TRUE(result);
    EXPECT_EQ(*result, 10);
}

TEST(TyrDatalogNumericUtilsTest, StopsAtUnsupportedMetricEffect)
{
    const auto effects = std::array { datalog::Cost(1), datalog::Cost(2), datalog::Cost(3) };
    auto num_evaluated = 0;
    const auto result = datalog::sum_metric_effect_deltas(datalog::Cost(0),
                                                          effects,
                                                          [&](const auto effect) -> std::optional<datalog::Cost>
                                                          {
                                                              ++num_evaluated;
                                                              return effect == datalog::Cost(2) ? std::nullopt : std::optional(effect);
                                                          });

    EXPECT_FALSE(result);
    EXPECT_EQ(num_evaluated, 2);
}

TEST(TyrDatalogNumericUtilsTest, EmptyMetricEffectRangeKeepsInitialCost)
{
    const auto effects = std::array<datalog::Cost, 0> {};
    const auto result = datalog::sum_metric_effect_deltas(datalog::Cost(7), effects, [](const auto) { return std::optional<datalog::Cost> {}; });

    ASSERT_TRUE(result);
    EXPECT_EQ(*result, 7);
}

TEST(TyrDatalogNumericUtilsTest, AddsMetricDeltaToIntervals)
{
    const auto empty_metric = ygg::ClosedInterval<ygg::float_t> {};
    const auto metric = ygg::ClosedInterval<ygg::float_t>(2, 4);

    EXPECT_EQ(datalog::add_metric_delta(empty_metric, datalog::Cost(0)), empty_metric);
    EXPECT_EQ(datalog::add_metric_delta(empty_metric, datalog::Cost(3)), ygg::ClosedInterval<ygg::float_t>(3, 3));
    EXPECT_EQ(datalog::add_metric_delta(metric, datalog::Cost(0)), metric);
    EXPECT_EQ(datalog::add_metric_delta(metric, datalog::Cost(3)), ygg::ClosedInterval<ygg::float_t>(5, 7));
}

}
