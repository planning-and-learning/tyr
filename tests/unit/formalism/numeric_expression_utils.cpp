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

#include "tyr/formalism/numeric_expression_utils.hpp"

#include <gtest/gtest.h>
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
    Operator op;
    NumericLeaf lhs;
    NumericLeaf rhs;

    [[nodiscard]] auto get_operator() const noexcept { return op; }
    [[nodiscard]] auto get_lhs() const noexcept { return lhs; }
    [[nodiscard]] auto get_rhs() const noexcept { return rhs; }
};

struct ReferenceBooleanExpression : BinaryExpression<formalism::BooleanOperatorKind>
{
    [[nodiscard]] const auto& get_operator() const noexcept { return op; }
};

struct MultiExpression
{
    formalism::ArithmeticOperatorKind op;
    std::vector<NumericLeaf> args;

    [[nodiscard]] auto get_operator() const noexcept { return op; }
    [[nodiscard]] const auto& get_args() const noexcept { return args; }
};

struct NumericEffect
{
    formalism::NumericEffectOperatorKind op;
    NumericLeaf target;
    NumericLeaf rhs;

    [[nodiscard]] auto get_operator() const noexcept { return op; }
    [[nodiscard]] auto get_fterm() const noexcept { return target; }
    [[nodiscard]] auto get_fexpr() const noexcept { return rhs; }
};

}

TEST(TyrDatalogNumericUtilsTest, TraversesExpressionsInDeterministicSupportOrder)
{
    auto order = std::vector<int> {};
    const auto resolve = [&](const NumericLeaf& leaf)
    {
        order.push_back(leaf.id);
        return leaf.value;
    };

    const auto arithmetic =
        BinaryExpression<formalism::ArithmeticOperatorKind> { formalism::ArithmeticOperatorKind::Sub, { 1, Interval(8, 8) }, { 2, Interval(3, 3) } };
    EXPECT_EQ(formalism::evaluate_numeric_expression(arithmetic, resolve), Interval(5, 5));
    EXPECT_EQ(order, (std::vector<int> { 2, 1 }));

    order.clear();
    const auto boolean = BinaryExpression<formalism::BooleanOperatorKind> { formalism::BooleanOperatorKind::Gt, { 1, Interval(8, 8) }, { 2, Interval(3, 3) } };
    EXPECT_TRUE(formalism::evaluate_numeric_expression(boolean, resolve));
    EXPECT_EQ(order, (std::vector<int> { 2, 1 }));

    order.clear();
    const auto reference_boolean = ReferenceBooleanExpression { { formalism::BooleanOperatorKind::Eq, { 1, Interval(1, 3) }, { 2, Interval(2, 4) } } };
    EXPECT_TRUE(formalism::evaluate_numeric_expression(reference_boolean, resolve));
    EXPECT_EQ(order, (std::vector<int> { 2, 1 }));

    order.clear();
    const auto multi = MultiExpression { formalism::ArithmeticOperatorKind::Add, { { 1, Interval(1, 1) }, { 2, Interval(2, 2) }, { 3, Interval(3, 3) } } };
    EXPECT_EQ(formalism::evaluate_numeric_expression(multi, resolve), Interval(6, 6));
    EXPECT_EQ(order, (std::vector<int> { 1, 2, 3 }));

    order.clear();
    EXPECT_TRUE(empty(formalism::evaluate_numeric_expression(MultiExpression { formalism::ArithmeticOperatorKind::Add, {} }, resolve)));
    EXPECT_TRUE(order.empty());
}

TEST(TyrDatalogNumericUtilsTest, EvaluatesNumericEffectsLazilyRhsFirst)
{
    auto order = std::vector<int> {};
    const auto resolve = [&](const NumericLeaf& leaf)
    {
        order.push_back(leaf.id);
        return leaf.value;
    };

    const auto assign = NumericEffect { formalism::NumericEffectOperatorKind::Assign, { 1, Interval(2, 2) }, { 2, Interval(5, 5) } };
    EXPECT_EQ(formalism::evaluate_numeric_effect(assign, resolve), Interval(5, 5));
    EXPECT_EQ(order, (std::vector<int> { 2 }));

    order.clear();
    const auto increase = NumericEffect { formalism::NumericEffectOperatorKind::Increase, { 1, Interval(2, 2) }, { 2, Interval(5, 5) } };
    EXPECT_EQ(formalism::evaluate_numeric_effect(increase, resolve), Interval(7, 7));
    EXPECT_EQ(order, (std::vector<int> { 2, 1 }));

    order.clear();
    const auto unsupported = NumericEffect { formalism::NumericEffectOperatorKind::Increase, { 1, Interval(2, 2) }, { 2, {} } };
    EXPECT_TRUE(empty(formalism::evaluate_numeric_effect(unsupported, resolve)));
    EXPECT_EQ(order, (std::vector<int> { 2 }));
}

}
