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

#ifndef TYR_FORMALISM_PLANNING_GROUND_NUMERIC_EFFECT_OPERATOR_UTILS_HPP_
#define TYR_FORMALISM_PLANNING_GROUND_NUMERIC_EFFECT_OPERATOR_UTILS_HPP_

#include "tyr/formalism/planning/declarations.hpp"

namespace tyr::formalism::planning
{
/**
 * Float
 */

template<typename T>
inline T apply(NumericEffectOperatorKind op, T lhs, T rhs)
{
    switch (op)
    {
        case NumericEffectOperatorKind::Assign:
            return rhs;
        case NumericEffectOperatorKind::Increase:
            return lhs + rhs;
        case NumericEffectOperatorKind::Decrease:
            return lhs - rhs;
        case NumericEffectOperatorKind::ScaleUp:
            return lhs * rhs;
        case NumericEffectOperatorKind::ScaleDown:
            return lhs / rhs;
    }
    throw std::invalid_argument("invalid NumericEffectOperatorKind");
}
}

#endif
