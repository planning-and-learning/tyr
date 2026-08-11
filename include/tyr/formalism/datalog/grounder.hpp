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

#ifndef TYR_FORMALISM_DATALOG_GROUNDER_HPP_
#define TYR_FORMALISM_DATALOG_GROUNDER_HPP_

#include "tyr/analysis/declarations.hpp"
#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/grounder_decl.hpp"
#include "tyr/formalism/datalog/indices.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/declarations.hpp"

#include <yggdrasil/containers/tuple.hpp>
#include <yggdrasil/core/itertools.hpp>

namespace tyr::formalism::datalog
{

/**
 * ground
 */

template<FactKind T>
std::pair<FunctionBindingView<T>, bool> ground(TermListView terms, FunctionView<T> function, GrounderContext& context);

template<FactKind T>
std::pair<GroundFunctionTermView<T>, bool> ground(FunctionTermView<T> element, GrounderContext& context);

GroundFunctionExpressionView ground(FunctionExpressionView element, GrounderContext& context);

std::pair<GroundUnaryOperatorView, bool> ground(LiftedUnaryOperatorView element, GrounderContext& context);

template<BinaryOperatorKind O>
std::pair<GroundBinaryOperatorView<O>, bool> ground(LiftedBinaryOperatorView<O> element, GrounderContext& context);

std::pair<GroundMultiOperatorView, bool> ground(LiftedMultiOperatorView element, GrounderContext& context);

GroundBooleanOperatorView ground(LiftedBooleanOperatorView element, GrounderContext& context);

GroundArithmeticOperatorView ground(LiftedArithmeticOperatorView element, GrounderContext& context);

template<FactKind T>
std::pair<PredicateBindingView<T>, bool> ground(TermListView terms, PredicateView<T> predicate, GrounderContext& context);

template<FactKind T>
std::pair<GroundAtomView<T>, bool> ground(AtomView<T> element, GrounderContext& context);

template<FactKind T>
std::pair<GroundLiteralView<T>, bool> ground(LiteralView<T> element, GrounderContext& context);

template<FactKind T>
std::pair<GroundNumericEffectView<T>, bool> ground(NumericEffectView<T> element, GrounderContext& context);

template<FactKind T>
GroundNumericEffectOperatorView<T> ground(NumericEffectOperatorView<T> element, GrounderContext& context);

std::pair<GroundConjunctiveConditionView, bool> ground(ConjunctiveConditionView element, GrounderContext& context);

template<RelationKind R>
std::pair<GroundRuleView<R>, bool> ground(RuleView<R> element, GrounderContext& context);

/**
 * ground_binding
 */

template<FactKind T>
std::pair<PredicateBindingView<T>, bool> ground_binding(AtomView<T> element, GrounderContext& context);

/// Grounds the binding without interning the enclosing GroundFunctionTerm symbol.
template<FactKind T>
std::pair<FunctionBindingView<T>, bool> ground_binding(FunctionTermView<T> element, GrounderContext& context);

template<RelationKind R>
std::pair<RuleBindingView<R>, bool> ground_binding(RuleView<R> element, GrounderContext& context);

/**
 * try_ground
 */

template<FactKind T>
std::optional<FunctionBindingView<T>> try_ground_binding(FunctionTermView<T> element, GrounderContext& context);

template<FactKind T>
std::optional<PredicateBindingView<T>> try_ground_binding(AtomView<T> element, GrounderContext& context);

/**
 * ground
 */

template<FactKind T>
std::pair<FunctionBindingView<T>, bool> ground(TermListView terms, FunctionView<T> function, GrounderContext& context)
{
    auto binding = ::tyr::formalism::datalog::checkout<RelationBinding<Function<T>>>(context.builder);

    binding->relation = function.get_index();
    for (const auto term : terms)
    {
        visit(
            [&](auto&& arg)
            {
                using Alternative = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<Alternative, ParameterIndex>)
                    binding->objects.push_back(context.binding[ygg::uint_t(arg)]);
                else if constexpr (std::is_same_v<Alternative, ObjectView>)
                    binding->objects.push_back(arg.get_index());
                else
                    static_assert(ygg::dependent_false<Alternative>::value, "Missing case");
            },
            term.get_variant());
    }

    // Canonicalize and Serialize
    return ::tyr::formalism::datalog::get_or_create(context.destination, *binding);
}

template<FactKind T>
std::pair<GroundFunctionTermView<T>, bool> ground(FunctionTermView<T> element, GrounderContext& context)
{
    // Fetch and clear
    auto fterm = ::tyr::formalism::datalog::checkout<GroundFunctionTerm<T>>(context.builder);

    // Fill data
    fterm->binding = ground(element.get_terms(), element.get_function(), context).first.get_index();

    // Canonicalize and Serialize
    return ::tyr::formalism::datalog::get_or_create(context.destination, *fterm);
}

inline GroundFunctionExpressionView ground(FunctionExpressionView element, GrounderContext& context)
{
    const auto data = visit(
        [&](auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return ygg::Data<GroundFunctionExpression>(arg);
            else if constexpr (std::is_same_v<Alternative, LiftedArithmeticOperatorView>)
                return ygg::Data<GroundFunctionExpression>(ground(arg, context).get_data());
            else
                return ygg::Data<GroundFunctionExpression>(ground(arg, context).first.get_index());
        },
        element.get_variant());
    return ygg::make_view(data, context.destination);
}

inline std::pair<GroundUnaryOperatorView, bool> ground(LiftedUnaryOperatorView element, GrounderContext& context)
{
    // Fetch and clear
    auto unary = ::tyr::formalism::datalog::checkout<UnaryOperator<ygg::Data<GroundFunctionExpression>>>(context.builder);

    // Fill data
    unary->operator_kind = element.get_operator();
    unary->arg = ground(element.get_arg(), context).get_data();

    // Canonicalize and Serialize
    return ::tyr::formalism::datalog::get_or_create(context.destination, *unary);
}

template<BinaryOperatorKind O>
std::pair<GroundBinaryOperatorView<O>, bool> ground(LiftedBinaryOperatorView<O> element, GrounderContext& context)
{
    // Fetch and clear
    auto binary = ::tyr::formalism::datalog::checkout<BinaryOperator<O, ygg::Data<GroundFunctionExpression>>>(context.builder);

    // Fill data
    binary->operator_kind = element.get_operator();
    binary->lhs = ground(element.get_lhs(), context).get_data();
    binary->rhs = ground(element.get_rhs(), context).get_data();

    // Canonicalize and Serialize
    return ::tyr::formalism::datalog::get_or_create(context.destination, *binary);
}

inline std::pair<GroundMultiOperatorView, bool> ground(LiftedMultiOperatorView element, GrounderContext& context)
{
    // Fetch and clear
    auto multi = ::tyr::formalism::datalog::checkout<MultiOperator<ygg::Data<GroundFunctionExpression>>>(context.builder);

    // Fill data
    multi->operator_kind = element.get_operator();
    for (const auto arg : element.get_args())
        multi->args.push_back(ground(arg, context).get_data());

    // Canonicalize and Serialize
    return ::tyr::formalism::datalog::get_or_create(context.destination, *multi);
}

inline GroundBooleanOperatorView ground(LiftedBooleanOperatorView element, GrounderContext& context)
{
    const auto data = visit(
        [&](auto&& arg) { return ygg::Data<BooleanOperator<ygg::Data<GroundFunctionExpression>>>(arg.get_operator(), ground(arg, context).first.get_index()); },
        element.get_variant());
    return ygg::make_view(data, context.destination);
}

inline GroundArithmeticOperatorView ground(LiftedArithmeticOperatorView element, GrounderContext& context)
{
    const auto data =
        visit([&](auto&& arg)
              { return ygg::Data<ArithmeticOperator<ygg::Data<GroundFunctionExpression>>>(arg.get_operator(), ground(arg, context).first.get_index()); },
              element.get_variant());
    return ygg::make_view(data, context.destination);
}

template<FactKind T>
std::pair<PredicateBindingView<T>, bool> ground(TermListView terms, PredicateView<T> predicate, GrounderContext& context)
{
    auto binding = ::tyr::formalism::datalog::checkout<RelationBinding<Predicate<T>>>(context.builder);

    binding->relation = predicate.get_index();
    for (const auto term : terms)
    {
        visit(
            [&](auto&& arg)
            {
                using Alternative = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<Alternative, ParameterIndex>)
                    binding->objects.push_back(context.binding[ygg::uint_t(arg)]);
                else if constexpr (std::is_same_v<Alternative, ObjectView>)
                    binding->objects.push_back(arg.get_index());
                else
                    static_assert(ygg::dependent_false<Alternative>::value, "Missing case");
            },
            term.get_variant());
    }

    // Canonicalize and Serialize
    return ::tyr::formalism::datalog::get_or_create(context.destination, *binding);
}

template<FactKind T>
std::pair<GroundAtomView<T>, bool> ground(AtomView<T> element, GrounderContext& context)
{
    // Fetch and clear
    auto atom = ::tyr::formalism::datalog::checkout<GroundAtom<T>>(context.builder);

    // Fill data
    atom->binding = ground(element.get_terms(), element.get_predicate(), context).first.get_index();

    // Canonicalize and Serialize
    return ::tyr::formalism::datalog::get_or_create(context.destination, *atom);
}

template<FactKind T>
std::pair<GroundLiteralView<T>, bool> ground(LiteralView<T> element, GrounderContext& context)
{
    // Fetch and clear
    auto ground_literal = ::tyr::formalism::datalog::checkout<GroundLiteral<T>>(context.builder);

    // Fill data
    ground_literal->polarity = element.get_polarity();
    ground_literal->atom = ground(element.get_atom(), context).first.get_index();

    // Canonicalize and Serialize
    return ::tyr::formalism::datalog::get_or_create(context.destination, *ground_literal);
}

template<FactKind T>
std::pair<GroundNumericEffectView<T>, bool> ground(NumericEffectView<T> element, GrounderContext& context)
{
    auto numeric_effect = ::tyr::formalism::datalog::checkout<GroundNumericEffect<T>>(context.builder);

    numeric_effect->operator_kind = element.get_operator();
    numeric_effect->fterm = ground(element.get_fterm(), context).first.get_index();
    numeric_effect->fexpr = ground(element.get_fexpr(), context).get_data();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *numeric_effect);
}

template<FactKind T>
GroundNumericEffectOperatorView<T> ground(NumericEffectOperatorView<T> element, GrounderContext& context)
{
    const auto data = visit([&](auto&& arg) { return ygg::Data<GroundNumericEffectOperator<T>>(arg.get_operator(), ground(arg, context).first.get_index()); },
                            element.get_variant());
    return ygg::make_view(data, context.destination);
}

inline std::pair<GroundConjunctiveConditionView, bool> ground(ConjunctiveConditionView element, GrounderContext& context)
{
    // Fetch and clear
    auto conj_cond = ::tyr::formalism::datalog::checkout<GroundConjunctiveCondition>(context.builder);

    // Fill data
    for (const auto literal : element.template get_literals<StaticTag>())
        conj_cond->static_literals.push_back(ground(literal, context).first.get_index());
    for (const auto literal : element.template get_literals<FluentTag>())
        conj_cond->fluent_literals.push_back(ground(literal, context).first.get_index());
    for (const auto numeric_constraint : element.get_numeric_constraints())
        conj_cond->numeric_constraints.push_back(ground(numeric_constraint, context).get_data());

    // Canonicalize and Serialize
    return ::tyr::formalism::datalog::get_or_create(context.destination, *conj_cond);
}

template<RelationKind R>
std::pair<GroundRuleView<R>, bool> ground(RuleView<R> element, GrounderContext& context)
{
    // Fetch and clear
    auto rule = ::tyr::formalism::datalog::checkout<GroundRule<R>>(context.builder);

    // Fill data
    rule->binding = ground_binding(element, context).first.get_index();
    rule->body = ground(element.get_body(), context).first.get_index();
    if constexpr (std::same_as<R, PredicateTag>)
        rule->head = ground(element.get_head(), context).first.get_index();
    else
        rule->head = ground(element.get_head(), context).get_data();
    for (const auto metric_effect : element.get_metric_effects())
        rule->metric_effects.push_back(ground(metric_effect, context).get_data());

    // Canonicalize and Serialize
    return ::tyr::formalism::datalog::get_or_create(context.destination, *rule);
}

/**
 * ground_binding
 */

template<FactKind T>
std::pair<PredicateBindingView<T>, bool> ground_binding(AtomView<T> element, GrounderContext& context)
{
    auto binding = ::tyr::formalism::datalog::checkout<RelationBinding<Predicate<T>>>(context.builder);

    binding->relation = element.get_predicate().get_index();
    for (const auto term : element.get_terms())
    {
        visit(
            [&](auto&& arg)
            {
                using Alternative = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<Alternative, ParameterIndex>)
                    binding->objects.push_back(context.binding[ygg::uint_t(arg)]);
                else if constexpr (std::is_same_v<Alternative, ObjectView>)
                    binding->objects.push_back(arg.get_index());
                else
                    static_assert(ygg::dependent_false<Alternative>::value, "Missing case");
            },
            term.get_variant());
    }

    // Canonicalize and Serialize
    return ::tyr::formalism::datalog::get_or_create(context.destination, *binding);
}

template<FactKind T>
std::pair<FunctionBindingView<T>, bool> ground_binding(FunctionTermView<T> element, GrounderContext& context)
{
    return ground(element.get_terms(), element.get_function(), context);
}

template<RelationKind R>
std::pair<RuleBindingView<R>, bool> ground_binding(RuleView<R> element, GrounderContext& context)
{
    auto binding = ::tyr::formalism::datalog::checkout<RelationBinding<Rule<R>>>(context.builder);

    binding->relation = element.get_index();
    for (const auto object : context.binding)
        binding->objects.push_back(object);

    return ::tyr::formalism::datalog::get_or_create(context.destination, *binding);
}

/**
 * try_ground
 */

template<FactKind T>
std::optional<FunctionBindingView<T>> try_ground_binding(FunctionTermView<T> element, GrounderContext& context)
{
    auto binding = ::tyr::formalism::datalog::checkout<RelationBinding<Function<T>>>(context.builder);

    binding->relation = element.get_function().get_index();
    for (const auto term : element.get_terms())
    {
        visit(
            [&](auto&& arg)
            {
                using Alternative = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<Alternative, ParameterIndex>)
                    binding->objects.push_back(context.binding[ygg::uint_t(arg)]);
                else if constexpr (std::is_same_v<Alternative, ObjectView>)
                    binding->objects.push_back(arg.get_index());
                else
                    static_assert(ygg::dependent_false<Alternative>::value, "Missing case");
            },
            term.get_variant());
    }

    canonicalize(*binding);
    return context.destination.find(*binding);
}

template<FactKind T>
std::optional<PredicateBindingView<T>> try_ground_binding(AtomView<T> element, GrounderContext& context)
{
    auto binding = ::tyr::formalism::datalog::checkout<RelationBinding<Predicate<T>>>(context.builder);

    binding->relation = element.get_predicate().get_index();
    for (const auto term : element.get_terms())
    {
        visit(
            [&](auto&& arg)
            {
                using Alternative = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<Alternative, ParameterIndex>)
                    binding->objects.push_back(context.binding[ygg::uint_t(arg)]);
                else if constexpr (std::is_same_v<Alternative, ObjectView>)
                    binding->objects.push_back(arg.get_index());
                else
                    static_assert(ygg::dependent_false<Alternative>::value, "Missing case");
            },
            term.get_variant());
    }

    canonicalize(*binding);
    return context.destination.find(*binding);
}
}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::datalog
{
extern template std::pair<FunctionBindingView<StaticTag>, bool> ground(TermListView terms, FunctionView<StaticTag> function, GrounderContext& context);
extern template std::pair<FunctionBindingView<FluentTag>, bool> ground(TermListView terms, FunctionView<FluentTag> function, GrounderContext& context);

extern template std::pair<GroundFunctionTermView<StaticTag>, bool> ground(FunctionTermView<StaticTag> element, GrounderContext& context);
extern template std::pair<GroundFunctionTermView<FluentTag>, bool> ground(FunctionTermView<FluentTag> element, GrounderContext& context);

extern template std::pair<GroundBinaryOperatorView<ArithmeticOperatorKind>, bool> ground(LiftedBinaryOperatorView<ArithmeticOperatorKind> element,
                                                                                         GrounderContext& context);
extern template std::pair<GroundBinaryOperatorView<BooleanOperatorKind>, bool> ground(LiftedBinaryOperatorView<BooleanOperatorKind> element,
                                                                                      GrounderContext& context);

extern template std::pair<PredicateBindingView<StaticTag>, bool> ground(TermListView terms, PredicateView<StaticTag> predicate, GrounderContext& context);
extern template std::pair<PredicateBindingView<FluentTag>, bool> ground(TermListView terms, PredicateView<FluentTag> predicate, GrounderContext& context);

extern template std::pair<GroundAtomView<StaticTag>, bool> ground(AtomView<StaticTag> element, GrounderContext& context);
extern template std::pair<GroundAtomView<FluentTag>, bool> ground(AtomView<FluentTag> element, GrounderContext& context);

extern template std::pair<GroundLiteralView<StaticTag>, bool> ground(LiteralView<StaticTag> element, GrounderContext& context);
extern template std::pair<GroundLiteralView<FluentTag>, bool> ground(LiteralView<FluentTag> element, GrounderContext& context);

extern template std::pair<GroundNumericEffectView<FluentTag>, bool> ground(NumericEffectView<FluentTag> element, GrounderContext& context);
extern template GroundNumericEffectOperatorView<FluentTag> ground(NumericEffectOperatorView<FluentTag> element, GrounderContext& context);

extern template std::pair<PredicateBindingView<StaticTag>, bool> ground_binding(AtomView<StaticTag> element, GrounderContext& context);
extern template std::pair<PredicateBindingView<FluentTag>, bool> ground_binding(AtomView<FluentTag> element, GrounderContext& context);

extern template std::pair<FunctionBindingView<StaticTag>, bool> ground_binding(FunctionTermView<StaticTag> element, GrounderContext& context);
extern template std::pair<FunctionBindingView<FluentTag>, bool> ground_binding(FunctionTermView<FluentTag> element, GrounderContext& context);

extern template std::optional<FunctionBindingView<StaticTag>> try_ground_binding(::tyr::formalism::datalog::FunctionTermView<StaticTag> element,
                                                                                 ::tyr::formalism::datalog::GrounderContext& context);
extern template std::optional<FunctionBindingView<FluentTag>> try_ground_binding(::tyr::formalism::datalog::FunctionTermView<FluentTag> element,
                                                                                 ::tyr::formalism::datalog::GrounderContext& context);

extern template std::optional<PredicateBindingView<StaticTag>> try_ground_binding(::tyr::formalism::datalog::AtomView<StaticTag> element,
                                                                                  ::tyr::formalism::datalog::GrounderContext& context);
extern template std::optional<PredicateBindingView<FluentTag>> try_ground_binding(::tyr::formalism::datalog::AtomView<FluentTag> element,
                                                                                  ::tyr::formalism::datalog::GrounderContext& context);

extern template std::pair<GroundRuleView<PredicateTag>, bool> ground(RuleView<PredicateTag> element, GrounderContext& context);
extern template std::pair<GroundRuleView<FunctionTag>, bool> ground(RuleView<FunctionTag> element, GrounderContext& context);

extern template std::pair<RuleBindingView<PredicateTag>, bool> ground_binding(RuleView<PredicateTag> element, GrounderContext& context);
extern template std::pair<RuleBindingView<FunctionTag>, bool> ground_binding(RuleView<FunctionTag> element, GrounderContext& context);
}

#endif

#endif
