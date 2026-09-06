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
#include "tyr/formalism/arithmetic_operator_utils.hpp"
#include "tyr/formalism/boolean_operator_utils.hpp"
#include "tyr/formalism/declarations.hpp"

#include <concepts>
#include <limits>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <yggdrasil/containers/variant.hpp>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/core/types.hpp>

namespace tyr::datalog
{

namespace numeric_evaluation_detail
{

template<typename Expression, typename ResolveTerm>
auto evaluate(Expression expression, ResolveTerm& resolve_term)
{
    using Type = std::remove_cvref_t<Expression>;

    if constexpr (std::same_as<Type, ygg::float_t>)
    {
        return ygg::ClosedInterval<ygg::float_t>(expression, expression);
    }
    else if constexpr (requires { expression.get_variant(); })
    {
        return ygg::visit([&](const auto child) { return evaluate(child, resolve_term); }, expression.get_variant());
    }
    else if constexpr (requires {
                           typename Type::OperatorType;
                           expression.get_lhs();
                           expression.get_rhs();
                       })
    {
        const auto rhs = evaluate(expression.get_rhs(), resolve_term);
        const auto lhs = evaluate(expression.get_lhs(), resolve_term);
        if constexpr (std::same_as<typename Type::OperatorType, formalism::BooleanOperatorKind>)
            return formalism::apply_existential(expression.get_operator(), lhs, rhs);
        else
            return formalism::apply(expression.get_operator(), lhs, rhs);
    }
    else if constexpr (requires { expression.get_arg(); })
    {
        return formalism::apply(expression.get_operator(), evaluate(expression.get_arg(), resolve_term));
    }
    else if constexpr (requires { expression.get_args(); })
    {
        const auto args = expression.get_args();
        auto it = args.begin();
        if (it == args.end())
            return ygg::ClosedInterval<ygg::float_t>();

        auto value = evaluate(*it, resolve_term);
        ++it;
        for (; it != args.end(); ++it)
            value = formalism::apply(expression.get_operator(), value, evaluate(*it, resolve_term));
        return value;
    }
    else
    {
        return resolve_term(expression);
    }
}

}

/// Evaluates a lifted or ground numeric expression. Term resolution is supplied by the caller so
/// fact lookup, lazy grounding, and support selection share one deterministic traversal.
template<typename Expression, typename ResolveTerm>
auto evaluate_numeric_expression(Expression expression, ResolveTerm&& resolve_term)
{
    return numeric_evaluation_detail::evaluate(expression, resolve_term);
}

/// Interval semantics of applying a numeric effect operator.
inline ygg::ClosedInterval<ygg::float_t>
apply_numeric_effect(formalism::NumericEffectOperatorKind op, ygg::ClosedInterval<ygg::float_t> lhs, ygg::ClosedInterval<ygg::float_t> rhs)
{
    using enum formalism::NumericEffectOperatorKind;
    switch (op)
    {
        case Assign:
            return rhs;
        case Increase:
            return lhs + rhs;
        case Decrease:
            return lhs - rhs;
        case ScaleUp:
            return lhs * rhs;
        case ScaleDown:
            return lhs / rhs;
    }
    throw std::invalid_argument("invalid NumericEffectOperatorKind");
}

/// Evaluates the right-hand side before resolving the target. Assign never resolves its target.
template<typename Effect, typename ResolveTerm>
ygg::ClosedInterval<ygg::float_t> evaluate_numeric_effect(Effect effect, ResolveTerm&& resolve_term)
{
    const auto rhs = numeric_evaluation_detail::evaluate(effect.get_fexpr(), resolve_term);
    if (empty(rhs))
        return {};

    if (effect.get_operator() == formalism::NumericEffectOperatorKind::Assign)
        return rhs;

    const auto lhs = numeric_evaluation_detail::evaluate(effect.get_fterm(), resolve_term);
    if (empty(lhs))
        return {};

    return apply_numeric_effect(effect.get_operator(), lhs, rhs);
}

/// Nonnegative cost that a single application of a metric effect adds. Operands are evaluated lazily,
/// so evaluation side effects (e.g., support selection) only occur for operands the operator needs.
/// Returns nullopt when an operand is unsupported.
template<typename EvalLhs, typename EvalRhs>
std::optional<Cost> metric_effect_delta(formalism::NumericEffectOperatorKind op, EvalLhs&& eval_lhs, EvalRhs&& eval_rhs)
{
    const auto rhs = eval_rhs();
    if (empty(rhs))
        return std::nullopt;

    if (op == formalism::NumericEffectOperatorKind::Increase)
        return clamp_metric_delta(lower(rhs));
    if (op == formalism::NumericEffectOperatorKind::Decrease)
        return Cost(0);

    const auto lhs = eval_lhs();
    if (empty(lhs))
        return std::nullopt;
    const auto next = apply_numeric_effect(op, lhs, rhs);
    if (empty(next))
        return std::nullopt;
    return clamp_metric_delta(lower(next) - upper(lhs));
}

template<typename Range, typename Evaluate>
std::optional<Cost> sum_metric_effect_deltas(Cost initial, const Range& effects, Evaluate&& evaluate)
{
    for (const auto& effect : effects)
    {
        const auto delta = evaluate(effect);
        if (!delta)
            return std::nullopt;
        initial += *delta;
    }
    return initial;
}

/// Conservative closure acceleration for detected label-preserving zero-edge bound growth.
/// This does not imply exact numeric closure or global termination.
inline ygg::ClosedInterval<ygg::float_t> widen_free_growth(ygg::ClosedInterval<ygg::float_t> next, ygg::ClosedInterval<ygg::float_t> current)
{
    if (empty(current))
        return next;

    constexpr auto inf = std::numeric_limits<ygg::float_t>::infinity();
    return ygg::ClosedInterval<ygg::float_t>(lower(next) < lower(current) ? -inf : lower(next), upper(next) > upper(current) ? inf : upper(next));
}

}

#endif
