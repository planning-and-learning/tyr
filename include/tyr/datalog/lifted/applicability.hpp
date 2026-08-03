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

#ifndef TYR_DATALOG_APPLICABILITY_HPP_
#define TYR_DATALOG_APPLICABILITY_HPP_

#include "tyr/datalog/fact_sets.hpp"
#include "tyr/formalism/arithmetic_operator_utils.hpp"
#include "tyr/formalism/boolean_operator_utils.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/declarations.hpp"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <stdexcept>
#include <yggdrasil/core/closed_interval.hpp>

namespace tyr::datalog
{

/**
 * evaluate
 */

ygg::ClosedInterval<ygg::float_t> evaluate(ygg::float_t element, const FactSets& fact_sets);

ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundUnaryOperatorView element, const FactSets& fact_sets);

ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundBinaryOperatorView<::tyr::formalism::ArithmeticOperatorKind> element,
                                           const FactSets& fact_sets);

bool evaluate(::tyr::formalism::datalog::GroundBinaryOperatorView<::tyr::formalism::BooleanOperatorKind> element, const FactSets& fact_sets);

ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundMultiOperatorView element, const FactSets& fact_sets);

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundFunctionTermView<T> element, const FactSets& fact_sets);

ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundFunctionExpressionView element, const FactSets& fact_sets);

ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundArithmeticOperatorView element, const FactSets& fact_sets);

bool evaluate(::tyr::formalism::datalog::GroundBooleanOperatorView element, const FactSets& fact_sets);

/**
 * is_applicable
 */

template<::tyr::formalism::FactKind T>
bool is_applicable(::tyr::formalism::datalog::GroundLiteralView<T> element, const FactSets& fact_sets);

template<::tyr::formalism::FactKind T>
bool is_applicable(::tyr::formalism::datalog::GroundLiteralListView<T> elements, const FactSets& fact_sets);

bool is_applicable(::tyr::formalism::datalog::GroundBooleanOperatorListView elements, const FactSets& fact_sets);

// GroundConjunctiveCondition

bool is_applicable(::tyr::formalism::datalog::GroundConjunctiveConditionView element, const FactSets& fact_sets);

// GroundRule

template<::tyr::formalism::RelationKind R>
bool is_applicable(::tyr::formalism::datalog::GroundRuleView<R> element, const FactSets& fact_sets);

/**
 * evaluate
 */

inline ygg::ClosedInterval<ygg::float_t> evaluate(ygg::float_t element, const FactSets& fact_sets)
{
    return ygg::ClosedInterval<ygg::float_t>(element, element);
}

inline ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundUnaryOperatorView element, const FactSets& fact_sets)
{
    return ::tyr::formalism::apply(element.get_operator(), evaluate(element.get_arg(), fact_sets));
}

inline ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundBinaryOperatorView<::tyr::formalism::ArithmeticOperatorKind> element,
                                                  const FactSets& fact_sets)
{
    return ::tyr::formalism::apply(element.get_operator(), evaluate(element.get_lhs(), fact_sets), evaluate(element.get_rhs(), fact_sets));
}

inline bool evaluate(::tyr::formalism::datalog::GroundBinaryOperatorView<::tyr::formalism::BooleanOperatorKind> element, const FactSets& fact_sets)
{
    return ::tyr::formalism::apply_existential(element.get_operator(), evaluate(element.get_lhs(), fact_sets), evaluate(element.get_rhs(), fact_sets));
}

inline ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundMultiOperatorView element, const FactSets& fact_sets)
{
    const auto child_fexprs = element.get_args();

    return std::accumulate(std::next(child_fexprs.begin()),  // Start from the second expression
                           child_fexprs.end(),
                           evaluate(child_fexprs.front(), fact_sets),
                           [&](const auto& value, const auto& child_expr)
                           { return ::tyr::formalism::apply(element.get_operator(), value, evaluate(child_expr, fact_sets)); });
}

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundFunctionTermView<T> element, const FactSets& fact_sets)
{
    return fact_sets.template get<T>().function[element];
}

inline ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::AuxiliaryTag>, const FactSets&)
{
    throw std::logic_error("Auxiliary function terms are not stored in datalog fact sets.");
}

inline ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundFunctionExpressionView element, const FactSets& fact_sets)
{
    return visit([&](auto&& arg) { return evaluate(arg, fact_sets); }, element.get_variant());
}

inline ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundArithmeticOperatorView element, const FactSets& fact_sets)
{
    return visit([&](auto&& arg) { return evaluate(arg, fact_sets); }, element.get_variant());
}

inline bool evaluate(::tyr::formalism::datalog::GroundBooleanOperatorView element, const FactSets& fact_sets)
{
    return visit([&](auto&& arg) { return evaluate(arg, fact_sets); }, element.get_variant());
}

/**
 * is_applicable
 */

template<::tyr::formalism::FactKind T>
bool is_applicable(::tyr::formalism::datalog::GroundLiteralView<T> element, const FactSets& fact_sets)
{
    return fact_sets.template get<T>().predicate.contains(element.get_atom().get_row()) == element.get_polarity();
}

template<::tyr::formalism::FactKind T>
bool is_applicable(::tyr::formalism::datalog::GroundLiteralListView<T> elements, const FactSets& fact_sets)
{
    return std::all_of(elements.begin(), elements.end(), [&](auto&& arg) { return is_applicable(arg, fact_sets); });
}

inline bool is_applicable(::tyr::formalism::datalog::GroundBooleanOperatorListView elements, const FactSets& fact_sets)
{
    return std::all_of(elements.begin(), elements.end(), [&](auto&& arg) { return evaluate(arg, fact_sets); });
}

// GroundConjunctiveCondition

inline bool is_applicable(::tyr::formalism::datalog::GroundConjunctiveConditionView element, const FactSets& fact_sets)
{
    return is_applicable(element.template get_literals<::tyr::formalism::StaticTag>(), fact_sets)     //
           && is_applicable(element.template get_literals<::tyr::formalism::FluentTag>(), fact_sets)  //
           && is_applicable(element.get_numeric_constraints(), fact_sets);
}

inline bool is_dynamically_applicable(::tyr::formalism::datalog::GroundConjunctiveConditionView element, const FactSets& fact_sets)
{
    return is_applicable(element.template get_literals<::tyr::formalism::FluentTag>(), fact_sets)
           && is_applicable(element.get_numeric_constraints(), fact_sets);
}

// GroundRule

template<::tyr::formalism::RelationKind R>
inline bool is_applicable(::tyr::formalism::datalog::GroundRuleView<R> element, const FactSets& fact_sets)
{
    return is_applicable(element.get_body(), fact_sets);
}

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::datalog
{
extern template ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::StaticTag> element,
                                                           const FactSets& fact_sets);
extern template ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::FluentTag> element,
                                                           const FactSets& fact_sets);

/**
 * is_applicable
 */

extern template bool is_applicable(::tyr::formalism::datalog::GroundLiteralView<::tyr::formalism::StaticTag> element, const FactSets& fact_sets);
extern template bool is_applicable(::tyr::formalism::datalog::GroundLiteralView<::tyr::formalism::FluentTag> element, const FactSets& fact_sets);

extern template bool is_applicable(::tyr::formalism::datalog::GroundLiteralListView<::tyr::formalism::StaticTag> elements, const FactSets& fact_sets);
extern template bool is_applicable(::tyr::formalism::datalog::GroundLiteralListView<::tyr::formalism::FluentTag> elements, const FactSets& fact_sets);

// GroundConjunctiveCondition

// GroundRule

}

#endif

#endif
