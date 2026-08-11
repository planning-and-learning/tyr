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
#include <vector>

namespace tyr::tests
{
namespace
{

using Interval = ygg::ClosedInterval<ygg::float_t>;

struct NumericLeaf
{
    int id;
    Interval value;
};

template<typename Operator>
struct BinaryExpression
{
    using OperatorType = Operator;

    Operator op;
    NumericLeaf lhs;
    NumericLeaf rhs;

    auto get_operator() const noexcept { return op; }
    auto get_lhs() const noexcept { return lhs; }
    auto get_rhs() const noexcept { return rhs; }
};

struct MultiExpression
{
    formalism::ArithmeticOperatorKind op;
    std::vector<NumericLeaf> args;

    auto get_operator() const noexcept { return op; }
    const auto& get_args() const noexcept { return args; }
};

struct NumericEffect
{
    formalism::NumericEffectOperatorKind op;
    NumericLeaf target;
    NumericLeaf rhs;

    auto get_operator() const noexcept { return op; }
    auto get_fterm() const noexcept { return target; }
    auto get_fexpr() const noexcept { return rhs; }
};

}

TEST(TyrDatalogNumericUtilsTest, TraversesExpressionsInDeterministicSupportOrder)
{
    auto order = std::vector<int> {};
    const auto resolve = [&](const NumericLeaf leaf)
    {
        order.push_back(leaf.id);
        return leaf.value;
    };

    const auto arithmetic =
        BinaryExpression<formalism::ArithmeticOperatorKind> { formalism::ArithmeticOperatorKind::Sub, { 1, Interval(8, 8) }, { 2, Interval(3, 3) } };
    EXPECT_EQ(datalog::evaluate_numeric_expression(arithmetic, resolve), Interval(5, 5));
    EXPECT_EQ(order, (std::vector<int> { 2, 1 }));

    order.clear();
    const auto boolean = BinaryExpression<formalism::BooleanOperatorKind> { formalism::BooleanOperatorKind::Gt, { 1, Interval(8, 8) }, { 2, Interval(3, 3) } };
    EXPECT_TRUE(datalog::evaluate_numeric_expression(boolean, resolve));
    EXPECT_EQ(order, (std::vector<int> { 2, 1 }));

    order.clear();
    const auto multi = MultiExpression { formalism::ArithmeticOperatorKind::Add, { { 1, Interval(1, 1) }, { 2, Interval(2, 2) }, { 3, Interval(3, 3) } } };
    EXPECT_EQ(datalog::evaluate_numeric_expression(multi, resolve), Interval(6, 6));
    EXPECT_EQ(order, (std::vector<int> { 1, 2, 3 }));

    order.clear();
    EXPECT_TRUE(empty(datalog::evaluate_numeric_expression(MultiExpression { formalism::ArithmeticOperatorKind::Add, {} }, resolve)));
    EXPECT_TRUE(order.empty());
}

TEST(TyrDatalogNumericUtilsTest, EvaluatesNumericEffectsLazilyRhsFirst)
{
    auto order = std::vector<int> {};
    const auto resolve = [&](const NumericLeaf leaf)
    {
        order.push_back(leaf.id);
        return leaf.value;
    };

    const auto assign = NumericEffect { formalism::NumericEffectOperatorKind::Assign, { 1, Interval(2, 2) }, { 2, Interval(5, 5) } };
    EXPECT_EQ(datalog::evaluate_numeric_effect(assign, resolve), Interval(5, 5));
    EXPECT_EQ(order, (std::vector<int> { 2 }));

    order.clear();
    const auto increase = NumericEffect { formalism::NumericEffectOperatorKind::Increase, { 1, Interval(2, 2) }, { 2, Interval(5, 5) } };
    EXPECT_EQ(datalog::evaluate_numeric_effect(increase, resolve), Interval(7, 7));
    EXPECT_EQ(order, (std::vector<int> { 2, 1 }));

    order.clear();
    const auto unsupported = NumericEffect { formalism::NumericEffectOperatorKind::Increase, { 1, Interval(2, 2) }, { 2, {} } };
    EXPECT_TRUE(empty(datalog::evaluate_numeric_effect(unsupported, resolve)));
    EXPECT_EQ(order, (std::vector<int> { 2 }));
}

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
