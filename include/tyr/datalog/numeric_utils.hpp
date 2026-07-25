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

#ifndef TYR_DATALOG_NUMERIC_UTILS_HPP_
#define TYR_DATALOG_NUMERIC_UTILS_HPP_

#include "tyr/datalog/policies/aggregation.hpp"
#include "tyr/formalism/declarations.hpp"

#include <limits>
#include <optional>
#include <type_traits>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/core/types.hpp>

namespace tyr::datalog
{

/// Interval semantics of applying a numeric effect operator.
template<::tyr::formalism::NumericEffectOpKind Op>
ygg::ClosedInterval<ygg::float_t> apply_numeric_effect(Op, ygg::ClosedInterval<ygg::float_t> lhs, ygg::ClosedInterval<ygg::float_t> rhs)
{
    if constexpr (std::is_same_v<Op, ::tyr::formalism::Assign>)
        return rhs;
    else if constexpr (std::is_same_v<Op, ::tyr::formalism::Increase>)
        return lhs + rhs;
    else if constexpr (std::is_same_v<Op, ::tyr::formalism::Decrease>)
        return lhs - rhs;
    else if constexpr (std::is_same_v<Op, ::tyr::formalism::ScaleUp>)
        return lhs * rhs;
    else if constexpr (std::is_same_v<Op, ::tyr::formalism::ScaleDown>)
        return lhs / rhs;
    else
        static_assert(ygg::dependent_false<Op>::value, "Missing case");
}

/// Nonnegative cost that a single application of a metric effect adds. Operands are evaluated lazily,
/// so evaluation side effects (e.g., support selection) only occur for operands the operator needs.
/// Returns nullopt when an operand is unsupported.
template<::tyr::formalism::NumericEffectOpKind Op, typename EvalLhs, typename EvalRhs>
std::optional<Cost> metric_effect_delta(Op, EvalLhs&& eval_lhs, EvalRhs&& eval_rhs)
{
    const auto rhs = eval_rhs();
    if (empty(rhs))
        return std::nullopt;

    if constexpr (std::is_same_v<Op, ::tyr::formalism::Increase>)
    {
        return clamp_metric_delta(lower(rhs));
    }
    else if constexpr (std::is_same_v<Op, ::tyr::formalism::Decrease>)
    {
        return Cost(0);
    }
    else
    {
        const auto lhs = eval_lhs();
        if (empty(lhs))
            return std::nullopt;
        const auto next = apply_numeric_effect(Op {}, lhs, rhs);
        if (empty(next))
            return std::nullopt;
        return clamp_metric_delta(lower(next) - upper(lhs));
    }
}

/// Closure of repeated free growth: a zero-cost rule that strictly grows a bound beyond `current` can be
/// repeated for free (fact intervals are hull-monotone), so that bound diverges. Widening it to its exact
/// limit immediately keeps cost-ordered fixpoints terminating without changing any derivable cost.
inline ygg::ClosedInterval<ygg::float_t> widen_free_growth(ygg::ClosedInterval<ygg::float_t> next, ygg::ClosedInterval<ygg::float_t> current)
{
    if (empty(current))
        return next;

    constexpr auto inf = std::numeric_limits<ygg::float_t>::infinity();
    return ygg::ClosedInterval<ygg::float_t>(lower(next) < lower(current) ? -inf : lower(next), upper(next) > upper(current) ? inf : upper(next));
}

}

#endif
