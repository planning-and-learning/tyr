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

#ifndef TYR_DATALOG_APPLICABILITY_LIFTED_HPP_
#define TYR_DATALOG_APPLICABILITY_LIFTED_HPP_

#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/applicability_lifted_decl.hpp"
#include "tyr/datalog/numeric_utils.hpp"
#include "tyr/formalism/arithmetic_operator_utils.hpp"
#include "tyr/formalism/boolean_operator_utils.hpp"
#include "tyr/formalism/datalog/grounder.hpp"
#include "tyr/formalism/datalog/views.hpp"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <yggdrasil/core/closed_interval.hpp>

namespace tyr::datalog
{

/**
 * evaluate
 */

ygg::ClosedInterval<ygg::float_t> evaluate(ygg::float_t element, const ApplicabilityContext& context);

ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::LiftedUnaryOperatorView element, const ApplicabilityContext& context);

ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::LiftedBinaryOperatorView<::tyr::formalism::ArithmeticOperatorKind> element,
                                           const ApplicabilityContext& context);

bool evaluate(::tyr::formalism::datalog::LiftedBinaryOperatorView<::tyr::formalism::BooleanOperatorKind> element, const ApplicabilityContext& context);

ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::LiftedMultiOperatorView element, const ApplicabilityContext& context);

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::FunctionTermView<T> element, const ApplicabilityContext& context);

ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::FunctionExpressionView element, const ApplicabilityContext& context);

ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::LiftedArithmeticOperatorView element, const ApplicabilityContext& context);

bool evaluate(::tyr::formalism::datalog::LiftedBooleanOperatorView element, const ApplicabilityContext& context);

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::NumericEffectView<T> element, const ApplicabilityContext& context);

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::NumericEffectOperatorView<T> element, const ApplicabilityContext& context);

/**
 * is_applicable
 */

template<::tyr::formalism::FactKind T>
bool is_applicable(::tyr::formalism::datalog::LiteralView<T> element, const ApplicabilityContext& context);

template<::tyr::formalism::FactKind T>
bool is_applicable(::tyr::formalism::datalog::LiteralListView<T> elements, const ApplicabilityContext& context);

bool is_applicable(::tyr::formalism::datalog::LiftedBooleanOperatorView element, const ApplicabilityContext& context);

bool is_applicable(::tyr::formalism::datalog::LiftedBooleanOperatorListView elements, const ApplicabilityContext& context);

bool is_applicable(::tyr::formalism::datalog::ConjunctiveConditionView element, const ApplicabilityContext& context);

template<::tyr::formalism::RelationKind R>
bool is_applicable(::tyr::formalism::datalog::RuleView<R> element, const ApplicabilityContext& context);

/**
 * evaluate
 */

inline ygg::ClosedInterval<ygg::float_t> evaluate(ygg::float_t element, const ApplicabilityContext&)
{
    return ygg::ClosedInterval<ygg::float_t>(element, element);
}

inline ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::LiftedUnaryOperatorView element, const ApplicabilityContext& context)
{
    return ::tyr::formalism::apply(element.get_operator(), evaluate(element.get_arg(), context));
}

inline ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::LiftedBinaryOperatorView<::tyr::formalism::ArithmeticOperatorKind> element,
                                                  const ApplicabilityContext& context)
{
    // Sequence the operand evaluations: grounding mutates the context, and function argument
    // evaluation order is unspecified (gcc and clang disagree), which would make repository index
    // assignment compiler-dependent. rhs-first preserves the historical order the fixtures were
    // generated with.
    const auto rhs = evaluate(element.get_rhs(), context);
    const auto lhs = evaluate(element.get_lhs(), context);
    return ::tyr::formalism::apply(element.get_operator(), lhs, rhs);
}

inline bool evaluate(::tyr::formalism::datalog::LiftedBinaryOperatorView<::tyr::formalism::BooleanOperatorKind> element, const ApplicabilityContext& context)
{
    // Sequenced for the same reason as the arithmetic binary operator above.
    const auto rhs = evaluate(element.get_rhs(), context);
    const auto lhs = evaluate(element.get_lhs(), context);
    return ::tyr::formalism::apply_existential(element.get_operator(), lhs, rhs);
}

inline ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::LiftedMultiOperatorView element, const ApplicabilityContext& context)
{
    const auto child_fexprs = element.get_args();

    return std::accumulate(std::next(child_fexprs.begin()),
                           child_fexprs.end(),
                           evaluate(child_fexprs.front(), context),
                           [&](const auto& value, const auto& child_expr)
                           { return ::tyr::formalism::apply(element.get_operator(), value, evaluate(child_expr, context)); });
}

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::FunctionTermView<T> element, const ApplicabilityContext& context)
{
    const auto binding = try_ground_binding(element, context.grounder);
    if (!binding)
        return {};

    return context.fact_sets.template get<T>().function[*binding];
}

inline ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::FunctionExpressionView element, const ApplicabilityContext& context)
{
    return visit([&](auto&& arg) { return evaluate(arg, context); }, element.get_variant());
}

inline ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::LiftedArithmeticOperatorView element, const ApplicabilityContext& context)
{
    return visit([&](auto&& arg) { return evaluate(arg, context); }, element.get_variant());
}

inline bool evaluate(::tyr::formalism::datalog::LiftedBooleanOperatorView element, const ApplicabilityContext& context)
{
    return visit([&](auto&& arg) { return evaluate(arg, context); }, element.get_variant());
}

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::NumericEffectView<T> element, const ApplicabilityContext& context)
{
    const auto rhs = evaluate(element.get_fexpr(), context);
    if (empty(rhs))
        return {};

    if (element.get_operator() == ::tyr::formalism::NumericEffectOperatorKind::Assign)
        return rhs;

    const auto lhs = evaluate(element.get_fterm(), context);
    if (empty(lhs))
        return {};

    return apply_numeric_effect(element.get_operator(), lhs, rhs);
}

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::NumericEffectOperatorView<T> element, const ApplicabilityContext& context)
{
    return visit([&](auto&& arg) { return evaluate(arg, context); }, element.get_variant());
}

/**
 * is_applicable
 */

template<::tyr::formalism::FactKind T>
bool is_applicable(::tyr::formalism::datalog::LiteralView<T> element, const ApplicabilityContext& context)
{
    const auto binding = try_ground_binding(element.get_atom(), context.grounder);
    if (!binding)
        return !element.get_polarity();

    return context.fact_sets.template get<T>().predicate.contains(*binding) == element.get_polarity();
}

template<::tyr::formalism::FactKind T>
bool is_applicable(::tyr::formalism::datalog::LiteralListView<T> elements, const ApplicabilityContext& context)
{
    return std::all_of(elements.begin(), elements.end(), [&](auto&& arg) { return is_applicable(arg, context); });
}

inline bool is_applicable(::tyr::formalism::datalog::LiftedBooleanOperatorView element, const ApplicabilityContext& context)
{
    return evaluate(element, context);
}

inline bool is_applicable(::tyr::formalism::datalog::LiftedBooleanOperatorListView elements, const ApplicabilityContext& context)
{
    return std::all_of(elements.begin(), elements.end(), [&](auto&& arg) { return is_applicable(arg, context); });
}

inline bool is_applicable(::tyr::formalism::datalog::ConjunctiveConditionView element, const ApplicabilityContext& context)
{
    return is_applicable(element.template get_literals<::tyr::formalism::StaticTag>(), context)     //
           && is_applicable(element.template get_literals<::tyr::formalism::FluentTag>(), context)  //
           && is_applicable(element.get_numeric_constraints(), context);
}

template<::tyr::formalism::RelationKind R>
bool is_applicable(::tyr::formalism::datalog::RuleView<R> element, const ApplicabilityContext& context)
{
    return is_applicable(element.get_body(), context);
}

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::datalog
{
extern template bool is_applicable(::tyr::formalism::datalog::LiteralView<::tyr::formalism::StaticTag> element, const ApplicabilityContext& context);
extern template bool is_applicable(::tyr::formalism::datalog::LiteralView<::tyr::formalism::FluentTag> element, const ApplicabilityContext& context);

extern template bool is_applicable(::tyr::formalism::datalog::LiteralListView<::tyr::formalism::StaticTag> elements, const ApplicabilityContext& context);
extern template bool is_applicable(::tyr::formalism::datalog::LiteralListView<::tyr::formalism::FluentTag> elements, const ApplicabilityContext& context);

extern template ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::NumericEffectView<::tyr::formalism::FluentTag> element,
                                                           const ApplicabilityContext& context);
extern template ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::NumericEffectOperatorView<::tyr::formalism::FluentTag> element,
                                                           const ApplicabilityContext& context);
}

#endif

#endif
