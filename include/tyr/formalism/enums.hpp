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

#ifndef TYR_FORMALISM_ENUMS_HPP_
#define TYR_FORMALISM_ENUMS_HPP_

#include <cstdint>
#include <stdexcept>
#include <string_view>

namespace tyr::formalism
{

enum class BooleanOperatorKind : std::uint8_t
{
    Eq,
    Ne,
    Le,
    Lt,
    Ge,
    Gt,
};

enum class ArithmeticOperatorKind : std::uint8_t
{
    Add,
    Sub,
    Mul,
    Div,
};

enum class NumericEffectOperatorKind : std::uint8_t
{
    Assign,
    Increase,
    Decrease,
    ScaleUp,
    ScaleDown,
};

enum class EffectFamily : std::uint8_t
{
    None,
    Assign,
    IncreaseDecrease,
    ScaleUpScaleDown,
};

enum class OptimizationDirection : std::uint8_t
{
    Minimize,
    Maximize,
};

constexpr std::string_view to_string(BooleanOperatorKind op)
{
    switch (op)
    {
        case BooleanOperatorKind::Eq:
            return "=";
        case BooleanOperatorKind::Ne:
            return "!=";
        case BooleanOperatorKind::Le:
            return "<=";
        case BooleanOperatorKind::Lt:
            return "<";
        case BooleanOperatorKind::Ge:
            return ">=";
        case BooleanOperatorKind::Gt:
            return ">";
    }
    throw std::invalid_argument("invalid BooleanOperatorKind");
}

constexpr std::string_view to_string(ArithmeticOperatorKind op)
{
    switch (op)
    {
        case ArithmeticOperatorKind::Add:
            return "+";
        case ArithmeticOperatorKind::Sub:
            return "-";
        case ArithmeticOperatorKind::Mul:
            return "*";
        case ArithmeticOperatorKind::Div:
            return "/";
    }
    throw std::invalid_argument("invalid ArithmeticOperatorKind");
}

constexpr std::string_view to_string(NumericEffectOperatorKind op)
{
    switch (op)
    {
        case NumericEffectOperatorKind::Assign:
            return "assign";
        case NumericEffectOperatorKind::Increase:
            return "increase";
        case NumericEffectOperatorKind::Decrease:
            return "decrease";
        case NumericEffectOperatorKind::ScaleUp:
            return "scale-up";
        case NumericEffectOperatorKind::ScaleDown:
            return "scale-down";
    }
    throw std::invalid_argument("invalid NumericEffectOperatorKind");
}

constexpr std::string_view to_string(OptimizationDirection direction)
{
    switch (direction)
    {
        case OptimizationDirection::Minimize:
            return "minimize";
        case OptimizationDirection::Maximize:
            return "maximize";
    }
    throw std::invalid_argument("invalid OptimizationDirection");
}

constexpr bool is_unary(ArithmeticOperatorKind op) noexcept { return op == ArithmeticOperatorKind::Sub; }

constexpr bool is_multi(ArithmeticOperatorKind op) noexcept { return op == ArithmeticOperatorKind::Add || op == ArithmeticOperatorKind::Mul; }

constexpr EffectFamily effect_family(NumericEffectOperatorKind op)
{
    switch (op)
    {
        case NumericEffectOperatorKind::Assign:
            return EffectFamily::Assign;
        case NumericEffectOperatorKind::Increase:
        case NumericEffectOperatorKind::Decrease:
            return EffectFamily::IncreaseDecrease;
        case NumericEffectOperatorKind::ScaleUp:
        case NumericEffectOperatorKind::ScaleDown:
            return EffectFamily::ScaleUpScaleDown;
    }
    throw std::invalid_argument("invalid NumericEffectOperatorKind");
}

}  // namespace tyr::formalism

#endif
