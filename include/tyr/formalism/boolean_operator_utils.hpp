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

#ifndef TYR_FORMALISM_BOOLEAN_OPERATOR_UTILS_HPP_
#define TYR_FORMALISM_BOOLEAN_OPERATOR_UTILS_HPP_

#include "tyr/formalism/declarations.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <type_traits>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::formalism
{

/// Exact comparisons; floating-point types compare within ygg::FloatTolerance (the specialization below).
template<typename T>
struct FloatCmp
{
    static bool eq(T a, T b) noexcept { return a == b; }
    static bool ne(T a, T b) noexcept { return a != b; }
    static bool le(T a, T b) noexcept { return a <= b; }
    static bool lt(T a, T b) noexcept { return a < b; }
    static bool ge(T a, T b) noexcept { return a >= b; }
    static bool gt(T a, T b) noexcept { return a > b; }
};

template<std::floating_point T>
struct FloatCmp<T>
{
    static T tol(T a, T b) noexcept { return ygg::FloatTolerance<T>::tolerance(a, b); }

    /// The relative tolerance is infinite for infinite operands, which would swallow every strict
    /// comparison against a free-growth widened bound, e.g. gt(inf, 0) becomes inf > inf.
    static bool exact(T a, T b) noexcept { return std::isinf(a) || std::isinf(b); }

    static bool eq(T a, T b) noexcept { return exact(a, b) ? a == b : std::abs(a - b) <= tol(a, b); }

    static bool ne(T a, T b) noexcept { return !eq(a, b); }

    static bool le(T a, T b) noexcept { return exact(a, b) ? a <= b : a <= b + tol(a, b); }

    static bool lt(T a, T b) noexcept { return exact(a, b) ? a < b : a < b - tol(a, b); }

    static bool ge(T a, T b) noexcept { return exact(a, b) ? a >= b : a + tol(a, b) >= b; }

    static bool gt(T a, T b) noexcept { return exact(a, b) ? a > b : a > b + tol(a, b); }
};

/**
 * Scalar comparisons
 */

template<typename T>
bool apply(BooleanOperatorKind op, T lhs, T rhs)
{
    switch (op)
    {
        case BooleanOperatorKind::Eq:
            return FloatCmp<T>::eq(lhs, rhs);
        case BooleanOperatorKind::Ne:
            return FloatCmp<T>::ne(lhs, rhs);
        case BooleanOperatorKind::Le:
            return FloatCmp<T>::le(lhs, rhs);
        case BooleanOperatorKind::Lt:
            return FloatCmp<T>::lt(lhs, rhs);
        case BooleanOperatorKind::Ge:
            return FloatCmp<T>::ge(lhs, rhs);
        case BooleanOperatorKind::Gt:
            return FloatCmp<T>::gt(lhs, rhs);
    }
    throw std::invalid_argument("invalid BooleanOperatorKind");
}

/**
 * Existential interval comparisons
 */

template<std::floating_point A>
bool apply_existential(BooleanOperatorKind op, const ygg::ClosedInterval<A>& lhs, const ygg::ClosedInterval<A>& rhs)
{
    if (empty(lhs) || empty(rhs))
        return false;

    switch (op)
    {
        case BooleanOperatorKind::Eq:
            return FloatCmp<A>::le(lower(lhs), upper(rhs)) && FloatCmp<A>::ge(upper(lhs), lower(rhs));
        case BooleanOperatorKind::Ne:
        {
            const bool lhs_is_point = FloatCmp<A>::eq(lower(lhs), upper(lhs));
            const bool rhs_is_point = FloatCmp<A>::eq(lower(rhs), upper(rhs));
            return !lhs_is_point || !rhs_is_point || FloatCmp<A>::ne(lower(lhs), lower(rhs));
        }
        case BooleanOperatorKind::Le:
            return FloatCmp<A>::le(lower(lhs), upper(rhs));
        case BooleanOperatorKind::Lt:
            return FloatCmp<A>::lt(lower(lhs), upper(rhs));
        case BooleanOperatorKind::Ge:
            return FloatCmp<A>::ge(upper(lhs), lower(rhs));
        case BooleanOperatorKind::Gt:
            return FloatCmp<A>::gt(upper(lhs), lower(rhs));
    }
    throw std::invalid_argument("invalid BooleanOperatorKind");
}

}

#endif
