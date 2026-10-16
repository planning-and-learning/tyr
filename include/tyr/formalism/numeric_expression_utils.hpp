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

#ifndef TYR_FORMALISM_NUMERIC_EXPRESSION_UTILS_HPP_
#define TYR_FORMALISM_NUMERIC_EXPRESSION_UTILS_HPP_

#include "tyr/formalism/arithmetic_operator_utils.hpp"
#include "tyr/formalism/boolean_operator_utils.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/numeric_effect_operator_utils.hpp"

#include <concepts>
#include <type_traits>
#include <yggdrasil/containers/variant.hpp>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/concepts.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/core/types.hpp>

namespace tyr::formalism
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
                           expression.get_operator();
                           expression.get_lhs();
                           expression.get_rhs();
                       })
    {
        const auto rhs = evaluate(expression.get_rhs(), resolve_term);
        const auto lhs = evaluate(expression.get_lhs(), resolve_term);
        if constexpr (requires {
                          { expression.get_operator() } -> ygg::SameAsIgnoringCvref<BooleanOperatorKind>;
                      })
            return apply_existential(expression.get_operator(), lhs, rhs);
        else
            return apply(expression.get_operator(), lhs, rhs);
    }
    else if constexpr (requires { expression.get_arg(); })
    {
        return apply(expression.get_operator(), evaluate(expression.get_arg(), resolve_term));
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
            value = apply(expression.get_operator(), value, evaluate(*it, resolve_term));
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

    return apply(effect.get_operator(), lhs, rhs);
}

}

#endif
