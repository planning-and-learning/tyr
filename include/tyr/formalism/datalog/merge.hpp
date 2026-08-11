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

#ifndef TYR_FORMALISM_DATALOG_MERGE_HPP_
#define TYR_FORMALISM_DATALOG_MERGE_HPP_

#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/indices.hpp"
#include "tyr/formalism/datalog/merge_decl.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/indices.hpp"
#include "tyr/formalism/views.hpp"

#include <yggdrasil/containers/tuple.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::formalism::datalog
{

// Common

std::pair<VariableView, bool> merge_d2d(VariableView element, MergeContext& context);

std::pair<ObjectView, bool> merge_d2d(ObjectView element, MergeContext& context);

TermView merge_d2d(TermView element, MergeContext& context);

// Propositional

template<FactKind T>
std::pair<PredicateView<T>, bool> merge_d2d(PredicateView<T> element, MergeContext& context);

template<FactKind T>
std::pair<AtomView<T>, bool> merge_d2d(AtomView<T> element, MergeContext& context);

template<FactKind T>
std::pair<PredicateBindingView<T>, bool> merge_d2d(PredicateBindingView<T> element, MergeContext& context);

template<FactKind T>
std::pair<GroundAtomView<T>, bool> merge_d2d(GroundAtomView<T> element, MergeContext& context);

template<FactKind T>
std::pair<LiteralView<T>, bool> merge_d2d(LiteralView<T> element, MergeContext& context);

template<FactKind T>
std::pair<GroundLiteralView<T>, bool> merge_d2d(GroundLiteralView<T> element, MergeContext& context);

// Numeric

template<FactKind T>
std::pair<FunctionView<T>, bool> merge_d2d(FunctionView<T> element, MergeContext& context);

template<FactKind T>
std::pair<FunctionTermView<T>, bool> merge_d2d(FunctionTermView<T> element, MergeContext& context);

template<FactKind T>
std::pair<FunctionBindingView<T>, bool> merge_d2d(FunctionBindingView<T> element, MergeContext& context);

template<FactKind T>
std::pair<GroundFunctionTermView<T>, bool> merge_d2d(GroundFunctionTermView<T> element, MergeContext& context);

template<FactKind T>
std::pair<GroundFunctionTermValueView<T>, bool> merge_d2d(GroundFunctionTermValueView<T> element, MergeContext& context);

FunctionExpressionView merge_d2d(FunctionExpressionView element, MergeContext& context);

GroundFunctionExpressionView merge_d2d(GroundFunctionExpressionView element, MergeContext& context);

template<typename T>
std::pair<UnaryOperatorView<T>, bool> merge_d2d(UnaryOperatorView<T> element, MergeContext& context);

template<BinaryOperatorKind O, typename T>
std::pair<BinaryOperatorView<O, T>, bool> merge_d2d(BinaryOperatorView<O, T> element, MergeContext& context);

template<typename T>
std::pair<MultiOperatorView<T>, bool> merge_d2d(MultiOperatorView<T> element, MergeContext& context);

template<typename T>
ArithmeticOperatorView<T> merge_d2d(ArithmeticOperatorView<T> element, MergeContext& context);

template<typename T>
BooleanOperatorView<T> merge_d2d(BooleanOperatorView<T> element, MergeContext& context);

template<FactKind T>
std::pair<NumericEffectView<T>, bool> merge_d2d(NumericEffectView<T> element, MergeContext& context);

template<FactKind T>
NumericEffectOperatorView<T> merge_d2d(NumericEffectOperatorView<T> element, MergeContext& context);

template<FactKind T>
std::pair<GroundNumericEffectView<T>, bool> merge_d2d(GroundNumericEffectView<T> element, MergeContext& context);

template<FactKind T>
GroundNumericEffectOperatorView<T> merge_d2d(GroundNumericEffectOperatorView<T> element, MergeContext& context);

std::pair<ConjunctiveConditionView, bool> merge_d2d(ConjunctiveConditionView element, MergeContext& context);

std::pair<GroundConjunctiveConditionView, bool> merge_d2d(GroundConjunctiveConditionView element, MergeContext& context);

std::pair<MetricView, bool> merge_d2d(MetricView element, MergeContext& context);

template<RelationKind R>
std::pair<RuleView<R>, bool> merge_d2d(RuleView<R> element, MergeContext& context);

template<RelationKind R>
std::pair<RuleBindingView<R>, bool> merge_d2d(RuleBindingView<R> element, MergeContext& context);

template<RelationKind R>
std::pair<GroundRuleView<R>, bool> merge_d2d(GroundRuleView<R> element, MergeContext& context);

// Common

inline std::pair<VariableView, bool> merge_d2d(VariableView element, MergeContext& context)
{
    auto variable = ::tyr::formalism::datalog::checkout<Variable>(context.builder);

    variable->name = element.get_name();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *variable);
}

inline std::pair<ObjectView, bool> merge_d2d(ObjectView element, MergeContext& context)
{
    auto object = ::tyr::formalism::datalog::checkout<Object>(context.builder);

    object->name = element.get_name();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *object);
}

inline TermView merge_d2d(TermView element, MergeContext& context)
{
    const auto data = visit(
        [&](auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ParameterIndex>)
                return ygg::Data<Term>(arg);
            else if constexpr (std::is_same_v<Alternative, ObjectView>)
                return ygg::Data<Term>(merge_d2d(arg, context).first.get_index());
            else
                static_assert(ygg::dependent_false<Alternative>::value, "Missing case");
        },
        element.get_variant());
    return ygg::make_view(data, context.destination);
}

// Propositional

template<FactKind T>
std::pair<PredicateView<T>, bool> merge_d2d(PredicateView<T> element, MergeContext& context)
{
    auto predicate = ::tyr::formalism::datalog::checkout<Predicate<T>>(context.builder);

    predicate->name = element.get_name();
    predicate->arity = element.get_arity();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *predicate);
}

template<FactKind T>
std::pair<AtomView<T>, bool> merge_d2d(AtomView<T> element, MergeContext& context)
{
    auto atom = ::tyr::formalism::datalog::checkout<Atom<T>>(context.builder);

    atom->predicate = merge_d2d(element.get_predicate(), context).first.get_index();
    for (const auto term : element.get_terms())
        atom->terms.push_back(merge_d2d(term, context).get_data());

    return ::tyr::formalism::datalog::get_or_create(context.destination, *atom);
}

template<FactKind T>
std::pair<PredicateBindingView<T>, bool> merge_d2d(PredicateBindingView<T> element, MergeContext& context)
{
    auto binding = ::tyr::formalism::datalog::checkout<RelationBinding<Predicate<T>>>(context.builder);

    binding->relation = merge_d2d(element.get_relation(), context).first.get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(merge_d2d(object, context).first.get_index());

    return ::tyr::formalism::datalog::get_or_create(context.destination, *binding);
}

template<FactKind T>
std::pair<GroundAtomView<T>, bool> merge_d2d(GroundAtomView<T> element, MergeContext& context)
{
    auto atom = ::tyr::formalism::datalog::checkout<GroundAtom<T>>(context.builder);

    atom->binding = merge_d2d(element.get_row(), context).first.get_index();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *atom);
}

template<FactKind T>
std::pair<LiteralView<T>, bool> merge_d2d(LiteralView<T> element, MergeContext& context)
{
    auto literal = ::tyr::formalism::datalog::checkout<Literal<T>>(context.builder);

    literal->polarity = element.get_polarity();
    literal->atom = merge_d2d(element.get_atom(), context).first.get_index();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *literal);
}

template<FactKind T>
std::pair<GroundLiteralView<T>, bool> merge_d2d(GroundLiteralView<T> element, MergeContext& context)
{
    auto literal = ::tyr::formalism::datalog::checkout<GroundLiteral<T>>(context.builder);

    literal->polarity = element.get_polarity();
    literal->atom = merge_d2d(element.get_atom(), context).first.get_index();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *literal);
}

// Numeric

template<FactKind T>
std::pair<FunctionView<T>, bool> merge_d2d(FunctionView<T> element, MergeContext& context)
{
    auto function = ::tyr::formalism::datalog::checkout<Function<T>>(context.builder);

    function->name = element.get_name();
    function->arity = element.get_arity();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *function);
}

template<FactKind T>
std::pair<FunctionTermView<T>, bool> merge_d2d(FunctionTermView<T> element, MergeContext& context)
{
    auto fterm = ::tyr::formalism::datalog::checkout<FunctionTerm<T>>(context.builder);

    fterm->function = merge_d2d(element.get_function(), context).first.get_index();
    for (const auto term : element.get_terms())
        fterm->terms.push_back(merge_d2d(term, context).get_data());

    return ::tyr::formalism::datalog::get_or_create(context.destination, *fterm);
}

template<FactKind T>
std::pair<FunctionBindingView<T>, bool> merge_d2d(FunctionBindingView<T> element, MergeContext& context)
{
    auto binding = ::tyr::formalism::datalog::checkout<RelationBinding<Function<T>>>(context.builder);

    binding->relation = merge_d2d(element.get_relation(), context).first.get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(merge_d2d(object, context).first.get_index());

    return ::tyr::formalism::datalog::get_or_create(context.destination, *binding);
}

template<FactKind T>
std::pair<GroundFunctionTermView<T>, bool> merge_d2d(GroundFunctionTermView<T> element, MergeContext& context)
{
    auto fterm = ::tyr::formalism::datalog::checkout<GroundFunctionTerm<T>>(context.builder);

    fterm->binding = merge_d2d(element.get_row(), context).first.get_index();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *fterm);
}

template<FactKind T>
std::pair<GroundFunctionTermValueView<T>, bool> merge_d2d(GroundFunctionTermValueView<T> element, MergeContext& context)
{
    auto fterm_value = ::tyr::formalism::datalog::checkout<GroundFunctionTermValue<T>>(context.builder);

    fterm_value->fterm = merge_d2d(element.get_fterm(), context).first.get_index();
    fterm_value->value = element.get_value();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *fterm_value);
}

inline FunctionExpressionView merge_d2d(FunctionExpressionView element, MergeContext& context)
{
    const auto data = visit(
        [&](auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return ygg::Data<FunctionExpression>(arg);
            else if constexpr (std::is_same_v<Alternative, LiftedArithmeticOperatorView>)
                return ygg::Data<FunctionExpression>(merge_d2d(arg, context).get_data());
            else
                return ygg::Data<FunctionExpression>(merge_d2d(arg, context).first.get_index());
        },
        element.get_variant());
    return ygg::make_view(data, context.destination);
}

inline GroundFunctionExpressionView merge_d2d(GroundFunctionExpressionView element, MergeContext& context)
{
    const auto data = visit(
        [&](auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return ygg::Data<GroundFunctionExpression>(arg);
            else if constexpr (std::is_same_v<Alternative, GroundArithmeticOperatorView>)
                return ygg::Data<GroundFunctionExpression>(merge_d2d(arg, context).get_data());
            else
                return ygg::Data<GroundFunctionExpression>(merge_d2d(arg, context).first.get_index());
        },
        element.get_variant());
    return ygg::make_view(data, context.destination);
}

template<typename T>
std::pair<UnaryOperatorView<T>, bool> merge_d2d(UnaryOperatorView<T> element, MergeContext& context)
{
    auto unary = ::tyr::formalism::datalog::checkout<UnaryOperator<T>>(context.builder);

    unary->operator_kind = element.get_operator();
    unary->arg = merge_d2d(element.get_arg(), context).get_data();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *unary);
}

template<BinaryOperatorKind O, typename T>
std::pair<BinaryOperatorView<O, T>, bool> merge_d2d(BinaryOperatorView<O, T> element, MergeContext& context)
{
    auto binary = ::tyr::formalism::datalog::checkout<BinaryOperator<O, T>>(context.builder);

    binary->operator_kind = element.get_operator();
    binary->lhs = merge_d2d(element.get_lhs(), context).get_data();
    binary->rhs = merge_d2d(element.get_rhs(), context).get_data();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *binary);
}

template<typename T>
std::pair<MultiOperatorView<T>, bool> merge_d2d(MultiOperatorView<T> element, MergeContext& context)
{
    auto multi = ::tyr::formalism::datalog::checkout<MultiOperator<T>>(context.builder);

    multi->operator_kind = element.get_operator();
    for (const auto arg : element.get_args())
        multi->args.push_back(merge_d2d(arg, context).get_data());

    return ::tyr::formalism::datalog::get_or_create(context.destination, *multi);
}

template<typename T>
ArithmeticOperatorView<T> merge_d2d(ArithmeticOperatorView<T> element, MergeContext& context)
{
    const auto data = visit([&](auto&& arg) { return ygg::Data<ArithmeticOperator<T>>(arg.get_operator(), merge_d2d(arg, context).first.get_index()); },
                            element.get_variant());
    return ygg::make_view(data, context.destination);
}

template<typename T>
BooleanOperatorView<T> merge_d2d(BooleanOperatorView<T> element, MergeContext& context)
{
    const auto data =
        visit([&](auto&& arg) { return ygg::Data<BooleanOperator<T>>(arg.get_operator(), merge_d2d(arg, context).first.get_index()); }, element.get_variant());
    return ygg::make_view(data, context.destination);
}

template<FactKind T>
std::pair<NumericEffectView<T>, bool> merge_d2d(NumericEffectView<T> element, MergeContext& context)
{
    auto numeric_effect = ::tyr::formalism::datalog::checkout<NumericEffect<T>>(context.builder);

    numeric_effect->operator_kind = element.get_operator();
    numeric_effect->fterm = merge_d2d(element.get_fterm(), context).first.get_index();
    numeric_effect->fexpr = merge_d2d(element.get_fexpr(), context).get_data();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *numeric_effect);
}

template<FactKind T>
NumericEffectOperatorView<T> merge_d2d(NumericEffectOperatorView<T> element, MergeContext& context)
{
    const auto data = visit([&](auto&& arg) { return ygg::Data<NumericEffectOperator<T>>(arg.get_operator(), merge_d2d(arg, context).first.get_index()); },
                            element.get_variant());
    return ygg::make_view(data, context.destination);
}

template<FactKind T>
std::pair<GroundNumericEffectView<T>, bool> merge_d2d(GroundNumericEffectView<T> element, MergeContext& context)
{
    auto numeric_effect = ::tyr::formalism::datalog::checkout<GroundNumericEffect<T>>(context.builder);

    numeric_effect->operator_kind = element.get_operator();
    numeric_effect->fterm = merge_d2d(element.get_fterm(), context).first.get_index();
    numeric_effect->fexpr = merge_d2d(element.get_fexpr(), context).get_data();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *numeric_effect);
}

template<FactKind T>
GroundNumericEffectOperatorView<T> merge_d2d(GroundNumericEffectOperatorView<T> element, MergeContext& context)
{
    const auto data =
        visit([&](auto&& arg) { return ygg::Data<GroundNumericEffectOperator<T>>(arg.get_operator(), merge_d2d(arg, context).first.get_index()); },
              element.get_variant());
    return ygg::make_view(data, context.destination);
}

inline std::pair<ConjunctiveConditionView, bool> merge_d2d(ConjunctiveConditionView element, MergeContext& context)
{
    auto conj_cond = ::tyr::formalism::datalog::checkout<ConjunctiveCondition>(context.builder);

    for (const auto variable : element.get_variables())
        conj_cond->variables.push_back(merge_d2d(variable, context).first.get_index());
    for (const auto literal : element.template get_literals<StaticTag>())
        conj_cond->static_literals.push_back(merge_d2d(literal, context).first.get_index());
    for (const auto literal : element.template get_literals<FluentTag>())
        conj_cond->fluent_literals.push_back(merge_d2d(literal, context).first.get_index());
    for (const auto numeric_constraint : element.get_numeric_constraints())
        conj_cond->numeric_constraints.push_back(merge_d2d(numeric_constraint, context).get_data());

    return ::tyr::formalism::datalog::get_or_create(context.destination, *conj_cond);
}

inline std::pair<GroundConjunctiveConditionView, bool> merge_d2d(GroundConjunctiveConditionView element, MergeContext& context)
{
    auto conj_cond = ::tyr::formalism::datalog::checkout<GroundConjunctiveCondition>(context.builder);

    for (const auto literal : element.template get_literals<StaticTag>())
        conj_cond->static_literals.push_back(merge_d2d(literal, context).first.get_index());
    for (const auto literal : element.template get_literals<FluentTag>())
        conj_cond->fluent_literals.push_back(merge_d2d(literal, context).first.get_index());
    for (const auto numeric_constraint : element.get_numeric_constraints())
        conj_cond->numeric_constraints.push_back(merge_d2d(numeric_constraint, context).get_data());

    return ::tyr::formalism::datalog::get_or_create(context.destination, *conj_cond);
}

inline std::pair<MetricView, bool> merge_d2d(MetricView element, MergeContext& context)
{
    auto metric = ::tyr::formalism::datalog::checkout<Metric>(context.builder);

    metric->fexpr = merge_d2d(element.get_fexpr(), context).get_data();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *metric);
}

inline auto merge_rule_head(AtomView<FluentTag> head, MergeContext& context) { return merge_d2d(head, context).first.get_index(); }

inline auto merge_rule_head(NumericEffectOperatorView<FluentTag> head, MergeContext& context) { return merge_d2d(head, context).get_data(); }

template<RelationKind R>
std::pair<RuleView<R>, bool> merge_d2d(RuleView<R> element, MergeContext& context)
{
    auto rule = ::tyr::formalism::datalog::checkout<Rule<R>>(context.builder);

    for (const auto variable : element.get_variables())
        rule->variables.push_back(merge_d2d(variable, context).first.get_index());
    rule->body = merge_d2d(element.get_body(), context).first.get_index();
    rule->head = merge_rule_head(element.get_head(), context);
    for (const auto metric_effect : element.get_metric_effects())
        rule->metric_effects.push_back(merge_d2d(metric_effect, context).get_data());

    return ::tyr::formalism::datalog::get_or_create(context.destination, *rule);
}

template<RelationKind R>
std::pair<RuleBindingView<R>, bool> merge_d2d(RuleBindingView<R> element, MergeContext& context)
{
    auto binding = ::tyr::formalism::datalog::checkout<RelationBinding<Rule<R>>>(context.builder);

    binding->relation = merge_d2d(element.get_relation(), context).first.get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(merge_d2d(object, context).first.get_index());

    return ::tyr::formalism::datalog::get_or_create(context.destination, *binding);
}

inline auto merge_ground_rule_head(GroundAtomView<FluentTag> head, MergeContext& context) { return merge_d2d(head, context).first.get_index(); }

inline auto merge_ground_rule_head(GroundNumericEffectOperatorView<FluentTag> head, MergeContext& context) { return merge_d2d(head, context).get_data(); }

template<RelationKind R>
std::pair<GroundRuleView<R>, bool> merge_d2d(GroundRuleView<R> element, MergeContext& context)
{
    auto rule = ::tyr::formalism::datalog::checkout<GroundRule<R>>(context.builder);

    rule->binding = merge_d2d(element.get_row(), context).first.get_index();
    rule->body = merge_d2d(element.get_body(), context).first.get_index();
    rule->head = merge_ground_rule_head(element.get_head(), context);
    for (const auto metric_effect : element.get_metric_effects())
        rule->metric_effects.push_back(merge_d2d(metric_effect, context).get_data());

    return ::tyr::formalism::datalog::get_or_create(context.destination, *rule);
}

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::datalog
{
extern template std::pair<PredicateView<StaticTag>, bool> merge_d2d(PredicateView<StaticTag> element, MergeContext& context);
extern template std::pair<PredicateView<FluentTag>, bool> merge_d2d(PredicateView<FluentTag> element, MergeContext& context);

extern template std::pair<AtomView<StaticTag>, bool> merge_d2d(AtomView<StaticTag> element, MergeContext& context);
extern template std::pair<AtomView<FluentTag>, bool> merge_d2d(AtomView<FluentTag> element, MergeContext& context);

extern template std::pair<PredicateBindingView<StaticTag>, bool> merge_d2d(PredicateBindingView<StaticTag> element, MergeContext& context);
extern template std::pair<PredicateBindingView<FluentTag>, bool> merge_d2d(PredicateBindingView<FluentTag> element, MergeContext& context);

extern template std::pair<GroundAtomView<StaticTag>, bool> merge_d2d(GroundAtomView<StaticTag> element, MergeContext& context);
extern template std::pair<GroundAtomView<FluentTag>, bool> merge_d2d(GroundAtomView<FluentTag> element, MergeContext& context);

extern template std::pair<LiteralView<StaticTag>, bool> merge_d2d(LiteralView<StaticTag> element, MergeContext& context);
extern template std::pair<LiteralView<FluentTag>, bool> merge_d2d(LiteralView<FluentTag> element, MergeContext& context);

extern template std::pair<GroundLiteralView<StaticTag>, bool> merge_d2d(GroundLiteralView<StaticTag> element, MergeContext& context);
extern template std::pair<GroundLiteralView<FluentTag>, bool> merge_d2d(GroundLiteralView<FluentTag> element, MergeContext& context);

extern template std::pair<FunctionView<StaticTag>, bool> merge_d2d(FunctionView<StaticTag> element, MergeContext& context);
extern template std::pair<FunctionView<FluentTag>, bool> merge_d2d(FunctionView<FluentTag> element, MergeContext& context);

extern template std::pair<FunctionTermView<StaticTag>, bool> merge_d2d(FunctionTermView<StaticTag> element, MergeContext& context);
extern template std::pair<FunctionTermView<FluentTag>, bool> merge_d2d(FunctionTermView<FluentTag> element, MergeContext& context);

extern template std::pair<FunctionBindingView<StaticTag>, bool> merge_d2d(FunctionBindingView<StaticTag> element, MergeContext& context);
extern template std::pair<FunctionBindingView<FluentTag>, bool> merge_d2d(FunctionBindingView<FluentTag> element, MergeContext& context);

extern template std::pair<GroundFunctionTermView<StaticTag>, bool> merge_d2d(GroundFunctionTermView<StaticTag> element, MergeContext& context);
extern template std::pair<GroundFunctionTermView<FluentTag>, bool> merge_d2d(GroundFunctionTermView<FluentTag> element, MergeContext& context);

extern template std::pair<GroundFunctionTermValueView<StaticTag>, bool> merge_d2d(GroundFunctionTermValueView<StaticTag> element, MergeContext& context);
extern template std::pair<GroundFunctionTermValueView<FluentTag>, bool> merge_d2d(GroundFunctionTermValueView<FluentTag> element, MergeContext& context);

extern template std::pair<UnaryOperatorView<ygg::Data<FunctionExpression>>, bool> merge_d2d(UnaryOperatorView<ygg::Data<FunctionExpression>> element,
                                                                                            MergeContext& context);
extern template std::pair<UnaryOperatorView<ygg::Data<GroundFunctionExpression>>, bool>
merge_d2d(UnaryOperatorView<ygg::Data<GroundFunctionExpression>> element, MergeContext& context);

extern template std::pair<BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression>>, bool>
merge_d2d(BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression>> element, MergeContext& context);
extern template std::pair<BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression>>, bool>
merge_d2d(BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression>> element, MergeContext& context);
extern template std::pair<BinaryOperatorView<BooleanOperatorKind, ygg::Data<GroundFunctionExpression>>, bool>
merge_d2d(BinaryOperatorView<BooleanOperatorKind, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);
extern template std::pair<BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<GroundFunctionExpression>>, bool>
merge_d2d(BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);

extern template std::pair<MultiOperatorView<ygg::Data<FunctionExpression>>, bool> merge_d2d(MultiOperatorView<ygg::Data<FunctionExpression>> element,
                                                                                            MergeContext& context);
extern template std::pair<MultiOperatorView<ygg::Data<GroundFunctionExpression>>, bool>
merge_d2d(MultiOperatorView<ygg::Data<GroundFunctionExpression>> element, MergeContext& context);

extern template ArithmeticOperatorView<ygg::Data<FunctionExpression>> merge_d2d(ArithmeticOperatorView<ygg::Data<FunctionExpression>> element,
                                                                                MergeContext& context);
extern template ArithmeticOperatorView<ygg::Data<GroundFunctionExpression>> merge_d2d(ArithmeticOperatorView<ygg::Data<GroundFunctionExpression>> element,
                                                                                      MergeContext& context);

extern template std::pair<NumericEffectView<FluentTag>, bool> merge_d2d(NumericEffectView<FluentTag> element, MergeContext& context);
extern template NumericEffectOperatorView<FluentTag> merge_d2d(NumericEffectOperatorView<FluentTag> element, MergeContext& context);

extern template std::pair<GroundNumericEffectView<FluentTag>, bool> merge_d2d(GroundNumericEffectView<FluentTag> element, MergeContext& context);
extern template GroundNumericEffectOperatorView<FluentTag> merge_d2d(GroundNumericEffectOperatorView<FluentTag> element, MergeContext& context);

extern template std::pair<RuleView<PredicateTag>, bool> merge_d2d(RuleView<PredicateTag> element, MergeContext& context);
extern template std::pair<RuleView<FunctionTag>, bool> merge_d2d(RuleView<FunctionTag> element, MergeContext& context);

extern template std::pair<RuleBindingView<PredicateTag>, bool> merge_d2d(RuleBindingView<PredicateTag> element, MergeContext& context);
extern template std::pair<RuleBindingView<FunctionTag>, bool> merge_d2d(RuleBindingView<FunctionTag> element, MergeContext& context);

extern template std::pair<GroundRuleView<PredicateTag>, bool> merge_d2d(GroundRuleView<PredicateTag> element, MergeContext& context);
extern template std::pair<GroundRuleView<FunctionTag>, bool> merge_d2d(GroundRuleView<FunctionTag> element, MergeContext& context);
}

#endif

#endif
