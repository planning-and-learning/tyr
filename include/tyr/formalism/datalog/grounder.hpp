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

template<FactKind F>
std::pair<FunctionBindingView<F>, bool> ground(TermListView terms, FunctionView<F> function, GrounderContext& context);

template<FactKind F>
std::pair<FunctionTermView<GroundTag, F>, bool> ground(FunctionTermView<LiftedTag, F> element, GrounderContext& context);

FunctionExpressionView<GroundTag> ground(FunctionExpressionView<LiftedTag> element, GrounderContext& context);

std::pair<UnaryOperatorView<GroundTag>, bool> ground(UnaryOperatorView<LiftedTag> element, GrounderContext& context);

template<BinaryOperatorKind O>
std::pair<BinaryOperatorView<GroundTag, O>, bool> ground(BinaryOperatorView<LiftedTag, O> element, GrounderContext& context);

std::pair<MultiOperatorView<GroundTag>, bool> ground(MultiOperatorView<LiftedTag> element, GrounderContext& context);

BooleanOperatorView<GroundTag> ground(BooleanOperatorView<LiftedTag> element, GrounderContext& context);

ArithmeticOperatorView<GroundTag> ground(ArithmeticOperatorView<LiftedTag> element, GrounderContext& context);

template<FactKind F>
std::pair<PredicateBindingView<F>, bool> ground(TermListView terms, PredicateView<F> predicate, GrounderContext& context);

template<FactKind F>
std::pair<AtomView<GroundTag, F>, bool> ground(AtomView<LiftedTag, F> element, GrounderContext& context);

template<FactKind F>
std::pair<LiteralView<GroundTag, F>, bool> ground(LiteralView<LiftedTag, F> element, GrounderContext& context);

template<FactKind F>
std::pair<NumericEffectView<GroundTag, F>, bool> ground(NumericEffectView<LiftedTag, F> element, GrounderContext& context);

template<FactKind F>
NumericEffectOperatorView<GroundTag, F> ground(NumericEffectOperatorView<LiftedTag, F> element, GrounderContext& context);

std::pair<ConjunctiveConditionView<GroundTag>, bool> ground(ConjunctiveConditionView<LiftedTag> element, GrounderContext& context);

template<RelationKind R>
std::pair<RuleView<GroundTag, R>, bool> ground(RuleView<LiftedTag, R> element, GrounderContext& context);

/**
 * ground_binding
 */

template<FactKind F>
std::pair<PredicateBindingView<F>, bool> ground_binding(AtomView<LiftedTag, F> element, GrounderContext& context);

/// Grounds the binding without interning the enclosing GroundFunctionTerm symbol.
template<FactKind F>
std::pair<FunctionBindingView<F>, bool> ground_binding(FunctionTermView<LiftedTag, F> element, GrounderContext& context);

template<RelationKind R>
std::pair<RuleBindingView<R>, bool> ground_binding(RuleView<LiftedTag, R> element, GrounderContext& context);

/**
 * try_ground
 */

template<FactKind F>
std::optional<FunctionBindingView<F>> try_ground_binding(FunctionTermView<LiftedTag, F> element, GrounderContext& context);

template<FactKind F>
std::optional<PredicateBindingView<F>> try_ground_binding(AtomView<LiftedTag, F> element, GrounderContext& context);

/**
 * ground
 */

template<FactKind F>
std::pair<FunctionBindingView<F>, bool> ground(TermListView terms, FunctionView<F> function, GrounderContext& context)
{
    auto binding = datalog::checkout<RelationBinding<Function<F>>>(context.builder);

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
    return datalog::get_or_create(context.destination, *binding);
}

template<FactKind F>
std::pair<FunctionTermView<GroundTag, F>, bool> ground(FunctionTermView<LiftedTag, F> element, GrounderContext& context)
{
    // Fetch and clear
    auto fterm = datalog::checkout<FunctionTerm<GroundTag, F>>(context.builder);

    // Fill data
    fterm->binding = ground(element.get_terms(), element.get_function(), context).first.get_index();

    // Canonicalize and Serialize
    return datalog::get_or_create(context.destination, *fterm);
}

inline FunctionExpressionView<GroundTag> ground(FunctionExpressionView<LiftedTag> element, GrounderContext& context)
{
    const auto data = visit(
        [&](auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return ygg::Data<FunctionExpression<GroundTag>>(arg);
            else if constexpr (std::is_same_v<Alternative, ArithmeticOperatorView<LiftedTag>>)
                return ygg::Data<FunctionExpression<GroundTag>>(ground(arg, context).get_data());
            else
                return ygg::Data<FunctionExpression<GroundTag>>(ground(arg, context).first.get_index());
        },
        element.get_variant());
    return ygg::make_view(data, context.destination);
}

inline std::pair<UnaryOperatorView<GroundTag>, bool> ground(UnaryOperatorView<LiftedTag> element, GrounderContext& context)
{
    // Fetch and clear
    auto unary = datalog::checkout<UnaryOperator<GroundTag>>(context.builder);

    // Fill data
    unary->operator_kind = element.get_operator();
    unary->arg = ground(element.get_arg(), context).get_data();

    // Canonicalize and Serialize
    return datalog::get_or_create(context.destination, *unary);
}

template<BinaryOperatorKind O>
std::pair<BinaryOperatorView<GroundTag, O>, bool> ground(BinaryOperatorView<LiftedTag, O> element, GrounderContext& context)
{
    // Fetch and clear
    auto binary = datalog::checkout<BinaryOperator<GroundTag, O>>(context.builder);

    // Fill data
    binary->operator_kind = element.get_operator();
    binary->lhs = ground(element.get_lhs(), context).get_data();
    binary->rhs = ground(element.get_rhs(), context).get_data();

    // Canonicalize and Serialize
    return datalog::get_or_create(context.destination, *binary);
}

inline std::pair<MultiOperatorView<GroundTag>, bool> ground(MultiOperatorView<LiftedTag> element, GrounderContext& context)
{
    // Fetch and clear
    auto multi = datalog::checkout<MultiOperator<GroundTag>>(context.builder);

    // Fill data
    multi->operator_kind = element.get_operator();
    for (const auto arg : element.get_args())
        multi->args.push_back(ground(arg, context).get_data());

    // Canonicalize and Serialize
    return datalog::get_or_create(context.destination, *multi);
}

inline BooleanOperatorView<GroundTag> ground(BooleanOperatorView<LiftedTag> element, GrounderContext& context)
{
    const auto data = visit([&](auto&& arg) { return ygg::Data<BooleanOperator<GroundTag>>(arg.get_operator(), ground(arg, context).first.get_index()); },
                            element.get_variant());
    return ygg::make_view(data, context.destination);
}

inline ArithmeticOperatorView<GroundTag> ground(ArithmeticOperatorView<LiftedTag> element, GrounderContext& context)
{
    const auto data = visit([&](auto&& arg) { return ygg::Data<ArithmeticOperator<GroundTag>>(arg.get_operator(), ground(arg, context).first.get_index()); },
                            element.get_variant());
    return ygg::make_view(data, context.destination);
}

template<FactKind F>
std::pair<PredicateBindingView<F>, bool> ground(TermListView terms, PredicateView<F> predicate, GrounderContext& context)
{
    auto binding = datalog::checkout<RelationBinding<Predicate<F>>>(context.builder);

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
    return datalog::get_or_create(context.destination, *binding);
}

template<FactKind F>
std::pair<AtomView<GroundTag, F>, bool> ground(AtomView<LiftedTag, F> element, GrounderContext& context)
{
    // Fetch and clear
    auto atom = datalog::checkout<Atom<GroundTag, F>>(context.builder);

    // Fill data
    atom->binding = ground(element.get_terms(), element.get_predicate(), context).first.get_index();

    // Canonicalize and Serialize
    return datalog::get_or_create(context.destination, *atom);
}

template<FactKind F>
std::pair<LiteralView<GroundTag, F>, bool> ground(LiteralView<LiftedTag, F> element, GrounderContext& context)
{
    // Fetch and clear
    auto ground_literal = datalog::checkout<Literal<GroundTag, F>>(context.builder);

    // Fill data
    ground_literal->polarity = element.get_polarity();
    ground_literal->atom = ground(element.get_atom(), context).first.get_index();

    // Canonicalize and Serialize
    return datalog::get_or_create(context.destination, *ground_literal);
}

template<FactKind F>
std::pair<NumericEffectView<GroundTag, F>, bool> ground(NumericEffectView<LiftedTag, F> element, GrounderContext& context)
{
    auto numeric_effect = datalog::checkout<NumericEffect<GroundTag, F>>(context.builder);

    numeric_effect->operator_kind = element.get_operator();
    numeric_effect->fterm = ground(element.get_fterm(), context).first.get_index();
    numeric_effect->fexpr = ground(element.get_fexpr(), context).get_data();

    return datalog::get_or_create(context.destination, *numeric_effect);
}

template<FactKind F>
NumericEffectOperatorView<GroundTag, F> ground(NumericEffectOperatorView<LiftedTag, F> element, GrounderContext& context)
{
    const auto data =
        visit([&](auto&& arg) { return ygg::Data<NumericEffectOperator<GroundTag, F>>(arg.get_operator(), ground(arg, context).first.get_index()); },
              element.get_variant());
    return ygg::make_view(data, context.destination);
}

inline std::pair<ConjunctiveConditionView<GroundTag>, bool> ground(ConjunctiveConditionView<LiftedTag> element, GrounderContext& context)
{
    // Fetch and clear
    auto conj_cond = datalog::checkout<ConjunctiveCondition<GroundTag>>(context.builder);

    // Fill data
    for (const auto literal : element.template get_literals<StaticTag>())
        conj_cond->static_literals.push_back(ground(literal, context).first.get_index());
    for (const auto literal : element.template get_literals<FluentTag>())
        conj_cond->fluent_literals.push_back(ground(literal, context).first.get_index());
    for (const auto numeric_constraint : element.get_numeric_constraints())
        conj_cond->numeric_constraints.push_back(ground(numeric_constraint, context).get_data());

    // Canonicalize and Serialize
    return datalog::get_or_create(context.destination, *conj_cond);
}

template<RelationKind R>
std::pair<RuleView<GroundTag, R>, bool> ground(RuleView<LiftedTag, R> element, GrounderContext& context)
{
    // Fetch and clear
    auto rule = datalog::checkout<Rule<GroundTag, R>>(context.builder);

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
    return datalog::get_or_create(context.destination, *rule);
}

/**
 * ground_binding
 */

template<FactKind F>
std::pair<PredicateBindingView<F>, bool> ground_binding(AtomView<LiftedTag, F> element, GrounderContext& context)
{
    auto binding = datalog::checkout<RelationBinding<Predicate<F>>>(context.builder);

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
    return datalog::get_or_create(context.destination, *binding);
}

template<FactKind F>
std::pair<FunctionBindingView<F>, bool> ground_binding(FunctionTermView<LiftedTag, F> element, GrounderContext& context)
{
    return ground(element.get_terms(), element.get_function(), context);
}

template<RelationKind R>
std::pair<RuleBindingView<R>, bool> ground_binding(RuleView<LiftedTag, R> element, GrounderContext& context)
{
    auto binding = datalog::checkout<RelationBinding<Rule<LiftedTag, R>>>(context.builder);

    binding->relation = element.get_index();
    for (const auto object : context.binding)
        binding->objects.push_back(object);

    return datalog::get_or_create(context.destination, *binding);
}

/**
 * try_ground
 */

template<FactKind F>
std::optional<FunctionBindingView<F>> try_ground_binding(FunctionTermView<LiftedTag, F> element, GrounderContext& context)
{
    auto binding = datalog::checkout<RelationBinding<Function<F>>>(context.builder);

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

template<FactKind F>
std::optional<PredicateBindingView<F>> try_ground_binding(AtomView<LiftedTag, F> element, GrounderContext& context)
{
    auto binding = datalog::checkout<RelationBinding<Predicate<F>>>(context.builder);

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

extern template std::pair<FunctionTermView<GroundTag, StaticTag>, bool> ground(FunctionTermView<LiftedTag, StaticTag> element, GrounderContext& context);
extern template std::pair<FunctionTermView<GroundTag, FluentTag>, bool> ground(FunctionTermView<LiftedTag, FluentTag> element, GrounderContext& context);

extern template std::pair<BinaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool> ground(BinaryOperatorView<LiftedTag, ArithmeticOperatorKind> element,
                                                                                              GrounderContext& context);
extern template std::pair<BinaryOperatorView<GroundTag, BooleanOperatorKind>, bool> ground(BinaryOperatorView<LiftedTag, BooleanOperatorKind> element,
                                                                                           GrounderContext& context);

extern template std::pair<PredicateBindingView<StaticTag>, bool> ground(TermListView terms, PredicateView<StaticTag> predicate, GrounderContext& context);
extern template std::pair<PredicateBindingView<FluentTag>, bool> ground(TermListView terms, PredicateView<FluentTag> predicate, GrounderContext& context);

extern template std::pair<AtomView<GroundTag, StaticTag>, bool> ground(AtomView<LiftedTag, StaticTag> element, GrounderContext& context);
extern template std::pair<AtomView<GroundTag, FluentTag>, bool> ground(AtomView<LiftedTag, FluentTag> element, GrounderContext& context);

extern template std::pair<LiteralView<GroundTag, StaticTag>, bool> ground(LiteralView<LiftedTag, StaticTag> element, GrounderContext& context);
extern template std::pair<LiteralView<GroundTag, FluentTag>, bool> ground(LiteralView<LiftedTag, FluentTag> element, GrounderContext& context);

extern template std::pair<NumericEffectView<GroundTag, FluentTag>, bool> ground(NumericEffectView<LiftedTag, FluentTag> element, GrounderContext& context);
extern template NumericEffectOperatorView<GroundTag, FluentTag> ground(NumericEffectOperatorView<LiftedTag, FluentTag> element, GrounderContext& context);

extern template std::pair<PredicateBindingView<StaticTag>, bool> ground_binding(AtomView<LiftedTag, StaticTag> element, GrounderContext& context);
extern template std::pair<PredicateBindingView<FluentTag>, bool> ground_binding(AtomView<LiftedTag, FluentTag> element, GrounderContext& context);

extern template std::pair<FunctionBindingView<StaticTag>, bool> ground_binding(FunctionTermView<LiftedTag, StaticTag> element, GrounderContext& context);
extern template std::pair<FunctionBindingView<FluentTag>, bool> ground_binding(FunctionTermView<LiftedTag, FluentTag> element, GrounderContext& context);

extern template std::optional<FunctionBindingView<StaticTag>> try_ground_binding(FunctionTermView<LiftedTag, StaticTag> element, GrounderContext& context);
extern template std::optional<FunctionBindingView<FluentTag>> try_ground_binding(FunctionTermView<LiftedTag, FluentTag> element, GrounderContext& context);

extern template std::optional<PredicateBindingView<StaticTag>> try_ground_binding(AtomView<LiftedTag, StaticTag> element, GrounderContext& context);
extern template std::optional<PredicateBindingView<FluentTag>> try_ground_binding(AtomView<LiftedTag, FluentTag> element, GrounderContext& context);

extern template std::pair<RuleView<GroundTag, PredicateTag>, bool> ground(RuleView<LiftedTag, PredicateTag> element, GrounderContext& context);
extern template std::pair<RuleView<GroundTag, FunctionTag>, bool> ground(RuleView<LiftedTag, FunctionTag> element, GrounderContext& context);

extern template std::pair<RuleBindingView<PredicateTag>, bool> ground_binding(RuleView<LiftedTag, PredicateTag> element, GrounderContext& context);
extern template std::pair<RuleBindingView<FunctionTag>, bool> ground_binding(RuleView<LiftedTag, FunctionTag> element, GrounderContext& context);
}

#endif

#endif
