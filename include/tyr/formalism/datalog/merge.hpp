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

template<FactKind F>
std::pair<PredicateView<F>, bool> merge_d2d(PredicateView<F> element, MergeContext& context);

template<FactKind F>
std::pair<AtomView<::tyr::LiftedTag, F>, bool> merge_d2d(AtomView<::tyr::LiftedTag, F> element, MergeContext& context);

template<FactKind F>
std::pair<PredicateBindingView<F>, bool> merge_d2d(PredicateBindingView<F> element, MergeContext& context);

template<FactKind F>
std::pair<AtomView<::tyr::GroundTag, F>, bool> merge_d2d(AtomView<::tyr::GroundTag, F> element, MergeContext& context);

template<::tyr::TaskKind T, FactKind F>
std::pair<LiteralView<T, F>, bool> merge_d2d(LiteralView<T, F> element, MergeContext& context);

// Numeric

template<FactKind F>
std::pair<FunctionView<F>, bool> merge_d2d(FunctionView<F> element, MergeContext& context);

template<FactKind F>
std::pair<FunctionTermView<::tyr::LiftedTag, F>, bool> merge_d2d(FunctionTermView<::tyr::LiftedTag, F> element, MergeContext& context);

template<FactKind F>
std::pair<FunctionBindingView<F>, bool> merge_d2d(FunctionBindingView<F> element, MergeContext& context);

template<FactKind F>
std::pair<FunctionTermView<::tyr::GroundTag, F>, bool> merge_d2d(FunctionTermView<::tyr::GroundTag, F> element, MergeContext& context);

template<FactKind F>
std::pair<FunctionTermValueView<::tyr::GroundTag, F>, bool> merge_d2d(FunctionTermValueView<::tyr::GroundTag, F> element, MergeContext& context);

template<::tyr::TaskKind T>
FunctionExpressionView<T> merge_d2d(FunctionExpressionView<T> element, MergeContext& context);

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

template<::tyr::TaskKind T, FactKind F>
std::pair<NumericEffectView<T, F>, bool> merge_d2d(NumericEffectView<T, F> element, MergeContext& context);

template<::tyr::TaskKind T, FactKind F>
NumericEffectOperatorView<T, F> merge_d2d(NumericEffectOperatorView<T, F> element, MergeContext& context);

std::pair<ConjunctiveConditionView<::tyr::LiftedTag>, bool> merge_d2d(ConjunctiveConditionView<::tyr::LiftedTag> element, MergeContext& context);

std::pair<ConjunctiveConditionView<::tyr::GroundTag>, bool> merge_d2d(ConjunctiveConditionView<::tyr::GroundTag> element, MergeContext& context);

std::pair<MetricView, bool> merge_d2d(MetricView element, MergeContext& context);

template<RelationKind R>
std::pair<RuleView<::tyr::LiftedTag, R>, bool> merge_d2d(RuleView<::tyr::LiftedTag, R> element, MergeContext& context);

template<RelationKind R>
std::pair<RuleBindingView<R>, bool> merge_d2d(RuleBindingView<R> element, MergeContext& context);

template<RelationKind R>
std::pair<RuleView<::tyr::GroundTag, R>, bool> merge_d2d(RuleView<::tyr::GroundTag, R> element, MergeContext& context);

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

template<FactKind F>
std::pair<PredicateView<F>, bool> merge_d2d(PredicateView<F> element, MergeContext& context)
{
    auto predicate = ::tyr::formalism::datalog::checkout<Predicate<F>>(context.builder);

    predicate->name = element.get_name();
    predicate->arity = element.get_arity();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *predicate);
}

template<FactKind F>
std::pair<AtomView<::tyr::LiftedTag, F>, bool> merge_d2d(AtomView<::tyr::LiftedTag, F> element, MergeContext& context)
{
    auto atom = ::tyr::formalism::datalog::checkout<Atom<::tyr::LiftedTag, F>>(context.builder);

    atom->predicate = merge_d2d(element.get_predicate(), context).first.get_index();
    for (const auto term : element.get_terms())
        atom->terms.push_back(merge_d2d(term, context).get_data());

    return ::tyr::formalism::datalog::get_or_create(context.destination, *atom);
}

template<FactKind F>
std::pair<PredicateBindingView<F>, bool> merge_d2d(PredicateBindingView<F> element, MergeContext& context)
{
    auto binding = ::tyr::formalism::datalog::checkout<RelationBinding<Predicate<F>>>(context.builder);

    binding->relation = merge_d2d(element.get_relation(), context).first.get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(merge_d2d(object, context).first.get_index());

    return ::tyr::formalism::datalog::get_or_create(context.destination, *binding);
}

template<FactKind F>
std::pair<AtomView<::tyr::GroundTag, F>, bool> merge_d2d(AtomView<::tyr::GroundTag, F> element, MergeContext& context)
{
    auto atom = ::tyr::formalism::datalog::checkout<Atom<::tyr::GroundTag, F>>(context.builder);

    atom->binding = merge_d2d(element.get_row(), context).first.get_index();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *atom);
}

template<::tyr::TaskKind T, FactKind F>
std::pair<LiteralView<T, F>, bool> merge_d2d(LiteralView<T, F> element, MergeContext& context)
{
    auto literal = ::tyr::formalism::datalog::checkout<Literal<T, F>>(context.builder);

    literal->polarity = element.get_polarity();
    literal->atom = merge_d2d(element.get_atom(), context).first.get_index();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *literal);
}

// Numeric

template<FactKind F>
std::pair<FunctionView<F>, bool> merge_d2d(FunctionView<F> element, MergeContext& context)
{
    auto function = ::tyr::formalism::datalog::checkout<Function<F>>(context.builder);

    function->name = element.get_name();
    function->arity = element.get_arity();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *function);
}

template<FactKind F>
std::pair<FunctionTermView<::tyr::LiftedTag, F>, bool> merge_d2d(FunctionTermView<::tyr::LiftedTag, F> element, MergeContext& context)
{
    auto fterm = ::tyr::formalism::datalog::checkout<FunctionTerm<::tyr::LiftedTag, F>>(context.builder);

    fterm->function = merge_d2d(element.get_function(), context).first.get_index();
    for (const auto term : element.get_terms())
        fterm->terms.push_back(merge_d2d(term, context).get_data());

    return ::tyr::formalism::datalog::get_or_create(context.destination, *fterm);
}

template<FactKind F>
std::pair<FunctionBindingView<F>, bool> merge_d2d(FunctionBindingView<F> element, MergeContext& context)
{
    auto binding = ::tyr::formalism::datalog::checkout<RelationBinding<Function<F>>>(context.builder);

    binding->relation = merge_d2d(element.get_relation(), context).first.get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(merge_d2d(object, context).first.get_index());

    return ::tyr::formalism::datalog::get_or_create(context.destination, *binding);
}

template<FactKind F>
std::pair<FunctionTermView<::tyr::GroundTag, F>, bool> merge_d2d(FunctionTermView<::tyr::GroundTag, F> element, MergeContext& context)
{
    auto fterm = ::tyr::formalism::datalog::checkout<FunctionTerm<::tyr::GroundTag, F>>(context.builder);

    fterm->binding = merge_d2d(element.get_row(), context).first.get_index();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *fterm);
}

template<FactKind F>
std::pair<FunctionTermValueView<::tyr::GroundTag, F>, bool> merge_d2d(FunctionTermValueView<::tyr::GroundTag, F> element, MergeContext& context)
{
    auto fterm_value = ::tyr::formalism::datalog::checkout<FunctionTermValue<::tyr::GroundTag, F>>(context.builder);

    fterm_value->fterm = merge_d2d(element.get_fterm(), context).first.get_index();
    fterm_value->value = element.get_value();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *fterm_value);
}

template<::tyr::TaskKind T>
FunctionExpressionView<T> merge_d2d(FunctionExpressionView<T> element, MergeContext& context)
{
    const auto data = visit(
        [&](auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return ygg::Data<FunctionExpression<T>>(arg);
            else if constexpr (std::is_same_v<Alternative, ArithmeticOperatorView<ygg::Data<FunctionExpression<T>>>>)
                return ygg::Data<FunctionExpression<T>>(merge_d2d(arg, context).get_data());
            else
                return ygg::Data<FunctionExpression<T>>(merge_d2d(arg, context).first.get_index());
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

template<::tyr::TaskKind T, FactKind F>
std::pair<NumericEffectView<T, F>, bool> merge_d2d(NumericEffectView<T, F> element, MergeContext& context)
{
    auto numeric_effect = ::tyr::formalism::datalog::checkout<NumericEffect<T, F>>(context.builder);

    numeric_effect->operator_kind = element.get_operator();
    numeric_effect->fterm = merge_d2d(element.get_fterm(), context).first.get_index();
    numeric_effect->fexpr = merge_d2d(element.get_fexpr(), context).get_data();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *numeric_effect);
}

template<::tyr::TaskKind T, FactKind F>
NumericEffectOperatorView<T, F> merge_d2d(NumericEffectOperatorView<T, F> element, MergeContext& context)
{
    const auto data = visit([&](auto&& arg) { return ygg::Data<NumericEffectOperator<T, F>>(arg.get_operator(), merge_d2d(arg, context).first.get_index()); },
                            element.get_variant());
    return ygg::make_view(data, context.destination);
}

inline std::pair<ConjunctiveConditionView<::tyr::LiftedTag>, bool> merge_d2d(ConjunctiveConditionView<::tyr::LiftedTag> element, MergeContext& context)
{
    auto conj_cond = ::tyr::formalism::datalog::checkout<ConjunctiveCondition<::tyr::LiftedTag>>(context.builder);

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

inline std::pair<ConjunctiveConditionView<::tyr::GroundTag>, bool> merge_d2d(ConjunctiveConditionView<::tyr::GroundTag> element, MergeContext& context)
{
    auto conj_cond = ::tyr::formalism::datalog::checkout<ConjunctiveCondition<::tyr::GroundTag>>(context.builder);

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

template<::tyr::TaskKind T>
auto merge_rule_head(AtomView<T, FluentTag> head, MergeContext& context)
{
    return merge_d2d(head, context).first.get_index();
}

template<::tyr::TaskKind T>
auto merge_rule_head(NumericEffectOperatorView<T, FluentTag> head, MergeContext& context)
{
    return merge_d2d(head, context).get_data();
}

template<RelationKind R>
std::pair<RuleView<::tyr::LiftedTag, R>, bool> merge_d2d(RuleView<::tyr::LiftedTag, R> element, MergeContext& context)
{
    auto rule = ::tyr::formalism::datalog::checkout<Rule<::tyr::LiftedTag, R>>(context.builder);

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
    auto binding = ::tyr::formalism::datalog::checkout<RelationBinding<Rule<::tyr::LiftedTag, R>>>(context.builder);

    binding->relation = merge_d2d(element.get_relation(), context).first.get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(merge_d2d(object, context).first.get_index());

    return ::tyr::formalism::datalog::get_or_create(context.destination, *binding);
}

template<RelationKind R>
std::pair<RuleView<::tyr::GroundTag, R>, bool> merge_d2d(RuleView<::tyr::GroundTag, R> element, MergeContext& context)
{
    auto rule = ::tyr::formalism::datalog::checkout<Rule<::tyr::GroundTag, R>>(context.builder);

    rule->binding = merge_d2d(element.get_row(), context).first.get_index();
    rule->body = merge_d2d(element.get_body(), context).first.get_index();
    rule->head = merge_rule_head(element.get_head(), context);
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

extern template std::pair<AtomView<::tyr::LiftedTag, StaticTag>, bool> merge_d2d(AtomView<::tyr::LiftedTag, StaticTag> element, MergeContext& context);
extern template std::pair<AtomView<::tyr::LiftedTag, FluentTag>, bool> merge_d2d(AtomView<::tyr::LiftedTag, FluentTag> element, MergeContext& context);

extern template std::pair<PredicateBindingView<StaticTag>, bool> merge_d2d(PredicateBindingView<StaticTag> element, MergeContext& context);
extern template std::pair<PredicateBindingView<FluentTag>, bool> merge_d2d(PredicateBindingView<FluentTag> element, MergeContext& context);

extern template std::pair<AtomView<::tyr::GroundTag, StaticTag>, bool> merge_d2d(AtomView<::tyr::GroundTag, StaticTag> element, MergeContext& context);
extern template std::pair<AtomView<::tyr::GroundTag, FluentTag>, bool> merge_d2d(AtomView<::tyr::GroundTag, FluentTag> element, MergeContext& context);

extern template std::pair<LiteralView<::tyr::LiftedTag, StaticTag>, bool> merge_d2d(LiteralView<::tyr::LiftedTag, StaticTag> element, MergeContext& context);
extern template std::pair<LiteralView<::tyr::LiftedTag, FluentTag>, bool> merge_d2d(LiteralView<::tyr::LiftedTag, FluentTag> element, MergeContext& context);

extern template std::pair<LiteralView<::tyr::GroundTag, StaticTag>, bool> merge_d2d(LiteralView<::tyr::GroundTag, StaticTag> element, MergeContext& context);
extern template std::pair<LiteralView<::tyr::GroundTag, FluentTag>, bool> merge_d2d(LiteralView<::tyr::GroundTag, FluentTag> element, MergeContext& context);

extern template std::pair<FunctionView<StaticTag>, bool> merge_d2d(FunctionView<StaticTag> element, MergeContext& context);
extern template std::pair<FunctionView<FluentTag>, bool> merge_d2d(FunctionView<FluentTag> element, MergeContext& context);

extern template std::pair<FunctionTermView<::tyr::LiftedTag, StaticTag>, bool> merge_d2d(FunctionTermView<::tyr::LiftedTag, StaticTag> element,
                                                                                         MergeContext& context);
extern template std::pair<FunctionTermView<::tyr::LiftedTag, FluentTag>, bool> merge_d2d(FunctionTermView<::tyr::LiftedTag, FluentTag> element,
                                                                                         MergeContext& context);

extern template std::pair<FunctionBindingView<StaticTag>, bool> merge_d2d(FunctionBindingView<StaticTag> element, MergeContext& context);
extern template std::pair<FunctionBindingView<FluentTag>, bool> merge_d2d(FunctionBindingView<FluentTag> element, MergeContext& context);

extern template std::pair<FunctionTermView<::tyr::GroundTag, StaticTag>, bool> merge_d2d(FunctionTermView<::tyr::GroundTag, StaticTag> element,
                                                                                         MergeContext& context);
extern template std::pair<FunctionTermView<::tyr::GroundTag, FluentTag>, bool> merge_d2d(FunctionTermView<::tyr::GroundTag, FluentTag> element,
                                                                                         MergeContext& context);

extern template std::pair<FunctionTermValueView<::tyr::GroundTag, StaticTag>, bool> merge_d2d(FunctionTermValueView<::tyr::GroundTag, StaticTag> element,
                                                                                              MergeContext& context);
extern template std::pair<FunctionTermValueView<::tyr::GroundTag, FluentTag>, bool> merge_d2d(FunctionTermValueView<::tyr::GroundTag, FluentTag> element,
                                                                                              MergeContext& context);

extern template std::pair<UnaryOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>, bool>
merge_d2d(UnaryOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeContext& context);
extern template std::pair<UnaryOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>>, bool>
merge_d2d(UnaryOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeContext& context);

extern template std::pair<BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression<::tyr::LiftedTag>>>, bool>
merge_d2d(BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeContext& context);
extern template std::pair<BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression<::tyr::LiftedTag>>>, bool>
merge_d2d(BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeContext& context);
extern template std::pair<BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression<::tyr::GroundTag>>>, bool>
merge_d2d(BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeContext& context);
extern template std::pair<BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression<::tyr::GroundTag>>>, bool>
merge_d2d(BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeContext& context);

extern template std::pair<MultiOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>, bool>
merge_d2d(MultiOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeContext& context);
extern template std::pair<MultiOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>>, bool>
merge_d2d(MultiOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeContext& context);

extern template ArithmeticOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>
merge_d2d(ArithmeticOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeContext& context);
extern template ArithmeticOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>>
merge_d2d(ArithmeticOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeContext& context);

extern template std::pair<NumericEffectView<::tyr::LiftedTag, FluentTag>, bool> merge_d2d(NumericEffectView<::tyr::LiftedTag, FluentTag> element,
                                                                                          MergeContext& context);
extern template NumericEffectOperatorView<::tyr::LiftedTag, FluentTag> merge_d2d(NumericEffectOperatorView<::tyr::LiftedTag, FluentTag> element,
                                                                                 MergeContext& context);

extern template std::pair<NumericEffectView<::tyr::GroundTag, FluentTag>, bool> merge_d2d(NumericEffectView<::tyr::GroundTag, FluentTag> element,
                                                                                          MergeContext& context);
extern template NumericEffectOperatorView<::tyr::GroundTag, FluentTag> merge_d2d(NumericEffectOperatorView<::tyr::GroundTag, FluentTag> element,
                                                                                 MergeContext& context);

extern template std::pair<RuleView<::tyr::LiftedTag, PredicateTag>, bool> merge_d2d(RuleView<::tyr::LiftedTag, PredicateTag> element, MergeContext& context);
extern template std::pair<RuleView<::tyr::LiftedTag, FunctionTag>, bool> merge_d2d(RuleView<::tyr::LiftedTag, FunctionTag> element, MergeContext& context);

extern template std::pair<RuleBindingView<PredicateTag>, bool> merge_d2d(RuleBindingView<PredicateTag> element, MergeContext& context);
extern template std::pair<RuleBindingView<FunctionTag>, bool> merge_d2d(RuleBindingView<FunctionTag> element, MergeContext& context);

extern template std::pair<RuleView<::tyr::GroundTag, PredicateTag>, bool> merge_d2d(RuleView<::tyr::GroundTag, PredicateTag> element, MergeContext& context);
extern template std::pair<RuleView<::tyr::GroundTag, FunctionTag>, bool> merge_d2d(RuleView<::tyr::GroundTag, FunctionTag> element, MergeContext& context);
}

#endif

#endif
