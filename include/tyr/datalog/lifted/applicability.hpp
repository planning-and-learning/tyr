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
#include "tyr/datalog/numeric_utils.hpp"
#include "tyr/formalism/arithmetic_operator_utils.hpp"
#include "tyr/formalism/boolean_operator_utils.hpp"
#include "tyr/formalism/datalog/builder.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/grounder.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/declarations.hpp"

#include <algorithm>
#include <concepts>
#include <iterator>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/closed_interval.hpp>

namespace tyr::datalog
{

/**
 * evaluate
 */

ygg::ClosedInterval<ygg::float_t> evaluate(ygg::float_t element, const FactSets& fact_sets);

template<::tyr::formalism::ArithmeticOpKind O>
ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundUnaryOperatorView<O> element, const FactSets& fact_sets);

template<::tyr::formalism::ArithmeticOpKind O>
ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundBinaryOperatorView<O> element, const FactSets& fact_sets);

template<::tyr::formalism::BooleanOpKind O>
bool evaluate(::tyr::formalism::datalog::GroundBinaryOperatorView<O> element, const FactSets& fact_sets);

template<::tyr::formalism::ArithmeticOpKind O>
ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundMultiOperatorView<O> element, const FactSets& fact_sets);

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

bool is_applicable(::tyr::formalism::datalog::GroundRuleView element, const FactSets& fact_sets);

/**
 * is_valid_binding
 */

ygg::ClosedInterval<ygg::float_t> is_valid_binding(ygg::float_t element, const FactSets&, ::tyr::formalism::datalog::GrounderContext&);

template<::tyr::formalism::ArithmeticOpKind O>
ygg::ClosedInterval<ygg::float_t>
is_valid_binding(::tyr::formalism::datalog::LiftedUnaryOperatorView<O> element, const FactSets& fact_sets, ::tyr::formalism::datalog::GrounderContext& context);

template<::tyr::formalism::ArithmeticOpKind O>
ygg::ClosedInterval<ygg::float_t> is_valid_binding(::tyr::formalism::datalog::LiftedBinaryOperatorView<O> element,
                                                   const FactSets& fact_sets,
                                                   ::tyr::formalism::datalog::GrounderContext& context);

template<::tyr::formalism::BooleanOpKind O>
bool is_valid_binding(::tyr::formalism::datalog::LiftedBinaryOperatorView<O> element,
                      const FactSets& fact_sets,
                      ::tyr::formalism::datalog::GrounderContext& context);

template<::tyr::formalism::ArithmeticOpKind O>
ygg::ClosedInterval<ygg::float_t>
is_valid_binding(::tyr::formalism::datalog::LiftedMultiOperatorView<O> element, const FactSets& fact_sets, ::tyr::formalism::datalog::GrounderContext& context);

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t>
is_valid_binding(::tyr::formalism::datalog::FunctionTermView<T> element, const FactSets& fact_sets, ::tyr::formalism::datalog::GrounderContext& context);

ygg::ClosedInterval<ygg::float_t>
is_valid_binding(::tyr::formalism::datalog::FunctionExpressionView element, const FactSets& fact_sets, ::tyr::formalism::datalog::GrounderContext& context);

ygg::ClosedInterval<ygg::float_t> is_valid_binding(::tyr::formalism::datalog::LiftedArithmeticOperatorView element,
                                                   const FactSets& fact_sets,
                                                   ::tyr::formalism::datalog::GrounderContext& context);

bool is_valid_binding(::tyr::formalism::datalog::LiftedBooleanOperatorView element,
                      const FactSets& fact_sets,
                      ::tyr::formalism::datalog::GrounderContext& context);

template<::tyr::formalism::FactKind T>
bool is_valid_binding(::tyr::formalism::datalog::LiteralView<T> element, const FactSets& fact_sets, ::tyr::formalism::datalog::GrounderContext& context);

template<::tyr::formalism::FactKind T>
bool is_valid_binding(::tyr::formalism::datalog::LiteralListView<T> elements, const FactSets& fact_sets, ::tyr::formalism::datalog::GrounderContext& context);

bool is_valid_binding(::tyr::formalism::datalog::LiftedBooleanOperatorListView elements,
                      const FactSets& fact_sets,
                      ::tyr::formalism::datalog::GrounderContext& context);

bool is_valid_binding(::tyr::formalism::datalog::ConjunctiveConditionView element,
                      const FactSets& fact_sets,
                      ::tyr::formalism::datalog::GrounderContext& context);

template<::tyr::formalism::NumericEffectOpKind Op, ::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t>
is_valid_binding(::tyr::formalism::datalog::NumericEffectView<Op, T> element, const FactSets& fact_sets, ::tyr::formalism::datalog::GrounderContext& context);

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> is_valid_binding(::tyr::formalism::datalog::NumericEffectOperatorView<T> element,
                                                   const FactSets& fact_sets,
                                                   ::tyr::formalism::datalog::GrounderContext& context);

/**
 * evaluate
 */

inline ygg::ClosedInterval<ygg::float_t> evaluate(ygg::float_t element, const FactSets& fact_sets)
{
    return ygg::ClosedInterval<ygg::float_t>(element, element);
}

template<::tyr::formalism::ArithmeticOpKind O>
ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundUnaryOperatorView<O> element, const FactSets& fact_sets)
{
    return ::tyr::formalism::apply(O {}, evaluate(element.get_arg(), fact_sets));
}

template<::tyr::formalism::ArithmeticOpKind O>
ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundBinaryOperatorView<O> element, const FactSets& fact_sets)
{
    return ::tyr::formalism::apply(O {}, evaluate(element.get_lhs(), fact_sets), evaluate(element.get_rhs(), fact_sets));
}

template<::tyr::formalism::BooleanOpKind O>
bool evaluate(::tyr::formalism::datalog::GroundBinaryOperatorView<O> element, const FactSets& fact_sets)
{
    return ::tyr::formalism::apply_existential(O {}, evaluate(element.get_lhs(), fact_sets), evaluate(element.get_rhs(), fact_sets));
}

template<::tyr::formalism::ArithmeticOpKind O>
ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundMultiOperatorView<O> element, const FactSets& fact_sets)
{
    const auto child_fexprs = element.get_args();

    return std::accumulate(std::next(child_fexprs.begin()),  // Start from the second expression
                           child_fexprs.end(),
                           evaluate(child_fexprs.front(), fact_sets),
                           [&](const auto& value, const auto& child_expr) { return ::tyr::formalism::apply(O {}, value, evaluate(child_expr, fact_sets)); });
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

inline bool is_applicable(::tyr::formalism::datalog::GroundRuleView element, const FactSets& fact_sets) { return is_applicable(element.get_body(), fact_sets); }

/**
 * is_valid_binding
 */

inline ygg::ClosedInterval<ygg::float_t> is_valid_binding(ygg::float_t element, const FactSets&, ::tyr::formalism::datalog::GrounderContext&)
{
    return ygg::ClosedInterval<ygg::float_t>(element, element);
}

template<::tyr::formalism::ArithmeticOpKind O>
ygg::ClosedInterval<ygg::float_t>
is_valid_binding(::tyr::formalism::datalog::LiftedUnaryOperatorView<O> element, const FactSets& fact_sets, ::tyr::formalism::datalog::GrounderContext& context)
{
    return ::tyr::formalism::apply(O {}, is_valid_binding(element.get_arg(), fact_sets, context));
}

template<::tyr::formalism::ArithmeticOpKind O>
ygg::ClosedInterval<ygg::float_t>
is_valid_binding(::tyr::formalism::datalog::LiftedBinaryOperatorView<O> element, const FactSets& fact_sets, ::tyr::formalism::datalog::GrounderContext& context)
{
    // Sequence the operand evaluations: grounding mutates the context, and function argument
    // evaluation order is unspecified (gcc and clang disagree), which would make repository index
    // assignment compiler-dependent. rhs-first preserves the historical order the fixtures were
    // generated with.
    const auto rhs = is_valid_binding(element.get_rhs(), fact_sets, context);
    const auto lhs = is_valid_binding(element.get_lhs(), fact_sets, context);
    return ::tyr::formalism::apply(O {}, lhs, rhs);
}

template<::tyr::formalism::BooleanOpKind O>
bool is_valid_binding(::tyr::formalism::datalog::LiftedBinaryOperatorView<O> element,
                      const FactSets& fact_sets,
                      ::tyr::formalism::datalog::GrounderContext& context)
{
    // Sequenced for the same reason as the arithmetic binary operator above.
    const auto rhs = is_valid_binding(element.get_rhs(), fact_sets, context);
    const auto lhs = is_valid_binding(element.get_lhs(), fact_sets, context);
    return ::tyr::formalism::apply_existential(O {}, lhs, rhs);
}

template<::tyr::formalism::ArithmeticOpKind O>
ygg::ClosedInterval<ygg::float_t>
is_valid_binding(::tyr::formalism::datalog::LiftedMultiOperatorView<O> element, const FactSets& fact_sets, ::tyr::formalism::datalog::GrounderContext& context)
{
    const auto child_fexprs = element.get_args();

    return std::accumulate(std::next(child_fexprs.begin()),  // Start from the second expression
                           child_fexprs.end(),
                           is_valid_binding(child_fexprs.front(), fact_sets, context),
                           [&](const auto& value, const auto& child_expr)
                           { return ::tyr::formalism::apply(O {}, value, is_valid_binding(child_expr, fact_sets, context)); });
}

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t>
is_valid_binding(::tyr::formalism::datalog::FunctionTermView<T> element, const FactSets& fact_sets, ::tyr::formalism::datalog::GrounderContext& context)
{
    auto binding_or_nullopt = try_ground_binding(element, context);
    if (!binding_or_nullopt)
        return {};

    return fact_sets.template get<T>().function[*binding_or_nullopt];
}

inline ygg::ClosedInterval<ygg::float_t>
is_valid_binding(::tyr::formalism::datalog::FunctionExpressionView element, const FactSets& fact_sets, ::tyr::formalism::datalog::GrounderContext& context)
{
    return visit([&](auto&& arg) { return is_valid_binding(arg, fact_sets, context); }, element.get_variant());
}

inline ygg::ClosedInterval<ygg::float_t> is_valid_binding(::tyr::formalism::datalog::LiftedArithmeticOperatorView element,
                                                          const FactSets& fact_sets,
                                                          ::tyr::formalism::datalog::GrounderContext& context)
{
    return visit([&](auto&& arg) { return is_valid_binding(arg, fact_sets, context); }, element.get_variant());
}

inline bool
is_valid_binding(::tyr::formalism::datalog::LiftedBooleanOperatorView element, const FactSets& fact_sets, ::tyr::formalism::datalog::GrounderContext& context)
{
    return visit([&](auto&& arg) { return is_valid_binding(arg, fact_sets, context); }, element.get_variant());
}

template<::tyr::formalism::FactKind T>
bool is_valid_binding(::tyr::formalism::datalog::LiteralView<T> element, const FactSets& fact_sets, ::tyr::formalism::datalog::GrounderContext& context)
{
    auto binding_or_nullopt = try_ground_binding(element.get_atom(), context);
    if (!binding_or_nullopt)
        return element.get_polarity() == false;

    return fact_sets.template get<T>().predicate.contains(*binding_or_nullopt) == element.get_polarity();
}

template<::tyr::formalism::FactKind T>
bool is_valid_binding(::tyr::formalism::datalog::LiteralListView<T> elements, const FactSets& fact_sets, ::tyr::formalism::datalog::GrounderContext& context)
{
    return std::all_of(elements.begin(), elements.end(), [&](auto&& arg) { return is_valid_binding(arg, fact_sets, context); });
}

inline bool is_valid_binding(::tyr::formalism::datalog::LiftedBooleanOperatorListView elements,
                             const FactSets& fact_sets,
                             ::tyr::formalism::datalog::GrounderContext& context)
{
    return std::all_of(elements.begin(), elements.end(), [&](auto&& arg) { return is_valid_binding(arg, fact_sets, context); });
}

inline bool
is_valid_binding(::tyr::formalism::datalog::ConjunctiveConditionView element, const FactSets& fact_sets, ::tyr::formalism::datalog::GrounderContext& context)
{
    return is_valid_binding(element.template get_literals<::tyr::formalism::StaticTag>(), fact_sets, context)     //
           && is_valid_binding(element.template get_literals<::tyr::formalism::FluentTag>(), fact_sets, context)  //
           && is_valid_binding(element.get_numeric_constraints(), fact_sets, context);
}

template<::tyr::formalism::NumericEffectOpKind Op, ::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t>
is_valid_binding(::tyr::formalism::datalog::NumericEffectView<Op, T> element, const FactSets& fact_sets, ::tyr::formalism::datalog::GrounderContext& context)
{
    const auto rhs = is_valid_binding(element.get_fexpr(), fact_sets, context);
    if (empty(rhs))
        return {};

    if constexpr (std::is_same_v<Op, ::tyr::formalism::Assign>)
    {
        return rhs;
    }
    else
    {
        const auto lhs = is_valid_binding(element.get_fterm(), fact_sets, context);
        if (empty(lhs))
            return {};

        return apply_numeric_effect(Op {}, lhs, rhs);
    }
}

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> is_valid_binding(::tyr::formalism::datalog::NumericEffectOperatorView<T> element,
                                                   const FactSets& fact_sets,
                                                   ::tyr::formalism::datalog::GrounderContext& context)
{
    return visit([&](auto&& arg) { return is_valid_binding(arg, fact_sets, context); }, element.get_variant());
}

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::datalog
{
/**
 * evaluate
 */

extern template ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundUnaryOperatorView<::tyr::formalism::Sub> element,
                                                           const FactSets& fact_sets);

extern template ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundBinaryOperatorView<::tyr::formalism::Add> element,
                                                           const FactSets& fact_sets);
extern template ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundBinaryOperatorView<::tyr::formalism::Sub> element,
                                                           const FactSets& fact_sets);
extern template ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundBinaryOperatorView<::tyr::formalism::Mul> element,
                                                           const FactSets& fact_sets);
extern template ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundBinaryOperatorView<::tyr::formalism::Div> element,
                                                           const FactSets& fact_sets);

extern template bool evaluate(::tyr::formalism::datalog::GroundBinaryOperatorView<::tyr::formalism::Eq> element, const FactSets& fact_sets);
extern template bool evaluate(::tyr::formalism::datalog::GroundBinaryOperatorView<::tyr::formalism::Ne> element, const FactSets& fact_sets);
extern template bool evaluate(::tyr::formalism::datalog::GroundBinaryOperatorView<::tyr::formalism::Ge> element, const FactSets& fact_sets);
extern template bool evaluate(::tyr::formalism::datalog::GroundBinaryOperatorView<::tyr::formalism::Gt> element, const FactSets& fact_sets);
extern template bool evaluate(::tyr::formalism::datalog::GroundBinaryOperatorView<::tyr::formalism::Le> element, const FactSets& fact_sets);
extern template bool evaluate(::tyr::formalism::datalog::GroundBinaryOperatorView<::tyr::formalism::Lt> element, const FactSets& fact_sets);

extern template ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundMultiOperatorView<::tyr::formalism::Add> element,
                                                           const FactSets& fact_sets);
extern template ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundMultiOperatorView<::tyr::formalism::Mul> element,
                                                           const FactSets& fact_sets);

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

/**
 * is_valid_binding
 */

extern template bool is_valid_binding(::tyr::formalism::datalog::LiteralView<::tyr::formalism::StaticTag> element,
                                      const FactSets& fact_sets,
                                      ::tyr::formalism::datalog::GrounderContext& context);
extern template bool is_valid_binding(::tyr::formalism::datalog::LiteralView<::tyr::formalism::FluentTag> element,
                                      const FactSets& fact_sets,
                                      ::tyr::formalism::datalog::GrounderContext& context);

extern template bool is_valid_binding(::tyr::formalism::datalog::LiteralListView<::tyr::formalism::StaticTag> elements,
                                      const FactSets& fact_sets,
                                      ::tyr::formalism::datalog::GrounderContext& context);
extern template bool is_valid_binding(::tyr::formalism::datalog::LiteralListView<::tyr::formalism::FluentTag> elements,
                                      const FactSets& fact_sets,
                                      ::tyr::formalism::datalog::GrounderContext& context);

extern template ygg::ClosedInterval<ygg::float_t>
is_valid_binding(::tyr::formalism::datalog::NumericEffectView<::tyr::formalism::Assign, ::tyr::formalism::FluentTag> element,
                 const FactSets& fact_sets,
                 ::tyr::formalism::datalog::GrounderContext& context);
extern template ygg::ClosedInterval<ygg::float_t>
is_valid_binding(::tyr::formalism::datalog::NumericEffectView<::tyr::formalism::Increase, ::tyr::formalism::FluentTag> element,
                 const FactSets& fact_sets,
                 ::tyr::formalism::datalog::GrounderContext& context);
extern template ygg::ClosedInterval<ygg::float_t>
is_valid_binding(::tyr::formalism::datalog::NumericEffectView<::tyr::formalism::Decrease, ::tyr::formalism::FluentTag> element,
                 const FactSets& fact_sets,
                 ::tyr::formalism::datalog::GrounderContext& context);
extern template ygg::ClosedInterval<ygg::float_t>
is_valid_binding(::tyr::formalism::datalog::NumericEffectView<::tyr::formalism::ScaleUp, ::tyr::formalism::FluentTag> element,
                 const FactSets& fact_sets,
                 ::tyr::formalism::datalog::GrounderContext& context);
extern template ygg::ClosedInterval<ygg::float_t>
is_valid_binding(::tyr::formalism::datalog::NumericEffectView<::tyr::formalism::ScaleDown, ::tyr::formalism::FluentTag> element,
                 const FactSets& fact_sets,
                 ::tyr::formalism::datalog::GrounderContext& context);
extern template ygg::ClosedInterval<ygg::float_t> is_valid_binding(::tyr::formalism::datalog::NumericEffectOperatorView<::tyr::formalism::FluentTag> element,
                                                                   const FactSets& fact_sets,
                                                                   ::tyr::formalism::datalog::GrounderContext& context);
}

#endif

#endif
