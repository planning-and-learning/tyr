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

#ifndef TYR_FORMALISM_DATALOG_COPY_HPP_
#define TYR_FORMALISM_DATALOG_COPY_HPP_

#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/copy_decl.hpp"
#include "tyr/formalism/datalog/indices.hpp"
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

std::pair<VariableView, bool> copy(VariableView element, CopyContext& context);

std::pair<ObjectView, bool> copy(ObjectView element, CopyContext& context);

TermView copy(TermView element, CopyContext& context);

// Propositional

template<FactKind F>
std::pair<PredicateView<F>, bool> copy(PredicateView<F> element, CopyContext& context);

template<FactKind F>
std::pair<AtomView<LiftedTag, F>, bool> copy(AtomView<LiftedTag, F> element, CopyContext& context);

template<FactKind F>
std::pair<PredicateBindingView<F>, bool> copy(PredicateBindingView<F> element, CopyContext& context);

template<FactKind F>
std::pair<AtomView<GroundTag, F>, bool> copy(AtomView<GroundTag, F> element, CopyContext& context);

template<TaskKind T, FactKind F>
std::pair<LiteralView<T, F>, bool> copy(LiteralView<T, F> element, CopyContext& context);

// Numeric

template<FactKind F>
std::pair<FunctionView<F>, bool> copy(FunctionView<F> element, CopyContext& context);

template<FactKind F>
std::pair<FunctionTermView<LiftedTag, F>, bool> copy(FunctionTermView<LiftedTag, F> element, CopyContext& context);

template<FactKind F>
std::pair<FunctionBindingView<F>, bool> copy(FunctionBindingView<F> element, CopyContext& context);

template<FactKind F>
std::pair<FunctionTermView<GroundTag, F>, bool> copy(FunctionTermView<GroundTag, F> element, CopyContext& context);

template<FactKind F>
std::pair<FunctionTermValueView<GroundTag, F>, bool> copy(FunctionTermValueView<GroundTag, F> element, CopyContext& context);

template<TaskKind T>
FunctionExpressionView<T> copy(FunctionExpressionView<T> element, CopyContext& context);

template<TaskKind T>
std::pair<UnaryOperatorView<T>, bool> copy(UnaryOperatorView<T> element, CopyContext& context);

template<TaskKind T, BinaryOperatorKind O>
std::pair<BinaryOperatorView<T, O>, bool> copy(BinaryOperatorView<T, O> element, CopyContext& context);

template<TaskKind T>
std::pair<MultiOperatorView<T>, bool> copy(MultiOperatorView<T> element, CopyContext& context);

template<TaskKind T>
ArithmeticOperatorView<T> copy(ArithmeticOperatorView<T> element, CopyContext& context);

template<TaskKind T>
BooleanOperatorView<T> copy(BooleanOperatorView<T> element, CopyContext& context);

template<TaskKind T, FactKind F>
std::pair<NumericEffectView<T, F>, bool> copy(NumericEffectView<T, F> element, CopyContext& context);

template<TaskKind T, FactKind F>
NumericEffectOperatorView<T, F> copy(NumericEffectOperatorView<T, F> element, CopyContext& context);

std::pair<ConjunctiveConditionView<LiftedTag>, bool> copy(ConjunctiveConditionView<LiftedTag> element, CopyContext& context);

std::pair<ConjunctiveConditionView<GroundTag>, bool> copy(ConjunctiveConditionView<GroundTag> element, CopyContext& context);

std::pair<MetricView, bool> copy(MetricView element, CopyContext& context);

template<RelationKind R>
std::pair<RuleView<LiftedTag, R>, bool> copy(RuleView<LiftedTag, R> element, CopyContext& context);

template<RelationKind R>
std::pair<RuleBindingView<R>, bool> copy(RuleBindingView<R> element, CopyContext& context);

template<RelationKind R>
std::pair<RuleView<GroundTag, R>, bool> copy(RuleView<GroundTag, R> element, CopyContext& context);

// Common

inline std::pair<VariableView, bool> copy(VariableView element, CopyContext& context)
{
    auto variable = datalog::checkout<Variable>(context.builder);

    variable->name = element.get_name();

    return datalog::insert(context.destination, *variable);
}

inline std::pair<ObjectView, bool> copy(ObjectView element, CopyContext& context)
{
    auto object = datalog::checkout<Object>(context.builder);

    object->name = element.get_name();

    return datalog::insert(context.destination, *object);
}

inline TermView copy(TermView element, CopyContext& context)
{
    const auto data = visit(
        [&](auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ParameterIndex>)
                return ygg::Data<Term>(arg);
            else if constexpr (std::is_same_v<Alternative, ObjectView>)
                return ygg::Data<Term>(copy(arg, context).first.get_index());
            else
                static_assert(ygg::dependent_false<Alternative>::value, "Missing case");
        },
        element.get_variant());
    return ygg::make_view(data, context.destination);
}

// Propositional

template<FactKind F>
std::pair<PredicateView<F>, bool> copy(PredicateView<F> element, CopyContext& context)
{
    auto predicate = datalog::checkout<Predicate<F>>(context.builder);

    predicate->name = element.get_name();
    predicate->arity = element.get_arity();

    return datalog::insert(context.destination, *predicate);
}

template<FactKind F>
std::pair<AtomView<LiftedTag, F>, bool> copy(AtomView<LiftedTag, F> element, CopyContext& context)
{
    auto atom = datalog::checkout<Atom<LiftedTag, F>>(context.builder);

    atom->predicate = copy(element.get_predicate(), context).first.get_index();
    for (const auto term : element.get_terms())
        atom->terms.push_back(copy(term, context).get_data());

    return datalog::insert(context.destination, *atom);
}

template<FactKind F>
std::pair<PredicateBindingView<F>, bool> copy(PredicateBindingView<F> element, CopyContext& context)
{
    auto binding = datalog::checkout<RelationBinding<Predicate<F>>>(context.builder);

    binding->relation = copy(element.get_relation(), context).first.get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(copy(object, context).first.get_index());

    return datalog::insert(context.destination, *binding);
}

template<FactKind F>
std::pair<AtomView<GroundTag, F>, bool> copy(AtomView<GroundTag, F> element, CopyContext& context)
{
    auto atom = datalog::checkout<Atom<GroundTag, F>>(context.builder);

    atom->binding = copy(element.get_row(), context).first.get_index();

    return datalog::insert(context.destination, *atom);
}

template<TaskKind T, FactKind F>
std::pair<LiteralView<T, F>, bool> copy(LiteralView<T, F> element, CopyContext& context)
{
    auto literal = datalog::checkout<Literal<T, F>>(context.builder);

    literal->polarity = element.get_polarity();
    literal->atom = copy(element.get_atom(), context).first.get_index();

    return datalog::insert(context.destination, *literal);
}

// Numeric

template<FactKind F>
std::pair<FunctionView<F>, bool> copy(FunctionView<F> element, CopyContext& context)
{
    auto function = datalog::checkout<Function<F>>(context.builder);

    function->name = element.get_name();
    function->arity = element.get_arity();

    return datalog::insert(context.destination, *function);
}

template<FactKind F>
std::pair<FunctionTermView<LiftedTag, F>, bool> copy(FunctionTermView<LiftedTag, F> element, CopyContext& context)
{
    auto fterm = datalog::checkout<FunctionTerm<LiftedTag, F>>(context.builder);

    fterm->function = copy(element.get_function(), context).first.get_index();
    for (const auto term : element.get_terms())
        fterm->terms.push_back(copy(term, context).get_data());

    return datalog::insert(context.destination, *fterm);
}

template<FactKind F>
std::pair<FunctionBindingView<F>, bool> copy(FunctionBindingView<F> element, CopyContext& context)
{
    auto binding = datalog::checkout<RelationBinding<Function<F>>>(context.builder);

    binding->relation = copy(element.get_relation(), context).first.get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(copy(object, context).first.get_index());

    return datalog::insert(context.destination, *binding);
}

template<FactKind F>
std::pair<FunctionTermView<GroundTag, F>, bool> copy(FunctionTermView<GroundTag, F> element, CopyContext& context)
{
    auto fterm = datalog::checkout<FunctionTerm<GroundTag, F>>(context.builder);

    fterm->binding = copy(element.get_row(), context).first.get_index();

    return datalog::insert(context.destination, *fterm);
}

template<FactKind F>
std::pair<FunctionTermValueView<GroundTag, F>, bool> copy(FunctionTermValueView<GroundTag, F> element, CopyContext& context)
{
    auto fterm_value = datalog::checkout<FunctionTermValue<GroundTag, F>>(context.builder);

    fterm_value->fterm = copy(element.get_fterm(), context).first.get_index();
    fterm_value->value = element.get_value();

    return datalog::insert(context.destination, *fterm_value);
}

template<TaskKind T>
FunctionExpressionView<T> copy(FunctionExpressionView<T> element, CopyContext& context)
{
    const auto data = visit(
        [&](auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return ygg::Data<FunctionExpression<T>>(arg);
            else if constexpr (std::is_same_v<Alternative, ArithmeticOperatorView<T>>)
                return ygg::Data<FunctionExpression<T>>(copy(arg, context).get_data());
            else
                return ygg::Data<FunctionExpression<T>>(copy(arg, context).first.get_index());
        },
        element.get_variant());
    return ygg::make_view(data, context.destination);
}

template<TaskKind T>
std::pair<UnaryOperatorView<T>, bool> copy(UnaryOperatorView<T> element, CopyContext& context)
{
    auto unary = datalog::checkout<UnaryOperator<T>>(context.builder);

    unary->operator_kind = element.get_operator();
    unary->arg = copy(element.get_arg(), context).get_data();

    return datalog::insert(context.destination, *unary);
}

template<TaskKind T, BinaryOperatorKind O>
std::pair<BinaryOperatorView<T, O>, bool> copy(BinaryOperatorView<T, O> element, CopyContext& context)
{
    auto binary = datalog::checkout<BinaryOperator<T, O>>(context.builder);

    binary->operator_kind = element.get_operator();
    binary->lhs = copy(element.get_lhs(), context).get_data();
    binary->rhs = copy(element.get_rhs(), context).get_data();

    return datalog::insert(context.destination, *binary);
}

template<TaskKind T>
std::pair<MultiOperatorView<T>, bool> copy(MultiOperatorView<T> element, CopyContext& context)
{
    auto multi = datalog::checkout<MultiOperator<T>>(context.builder);

    multi->operator_kind = element.get_operator();
    for (const auto arg : element.get_args())
        multi->args.push_back(copy(arg, context).get_data());

    return datalog::insert(context.destination, *multi);
}

template<TaskKind T>
ArithmeticOperatorView<T> copy(ArithmeticOperatorView<T> element, CopyContext& context)
{
    const auto data =
        visit([&](auto&& arg) { return ygg::Data<ArithmeticOperator<T>>(arg.get_operator(), copy(arg, context).first.get_index()); }, element.get_variant());
    return ygg::make_view(data, context.destination);
}

template<TaskKind T>
BooleanOperatorView<T> copy(BooleanOperatorView<T> element, CopyContext& context)
{
    const auto data =
        visit([&](auto&& arg) { return ygg::Data<BooleanOperator<T>>(arg.get_operator(), copy(arg, context).first.get_index()); }, element.get_variant());
    return ygg::make_view(data, context.destination);
}

template<TaskKind T, FactKind F>
std::pair<NumericEffectView<T, F>, bool> copy(NumericEffectView<T, F> element, CopyContext& context)
{
    auto numeric_effect = datalog::checkout<NumericEffect<T, F>>(context.builder);

    numeric_effect->operator_kind = element.get_operator();
    numeric_effect->fterm = copy(element.get_fterm(), context).first.get_index();
    numeric_effect->fexpr = copy(element.get_fexpr(), context).get_data();

    return datalog::insert(context.destination, *numeric_effect);
}

template<TaskKind T, FactKind F>
NumericEffectOperatorView<T, F> copy(NumericEffectOperatorView<T, F> element, CopyContext& context)
{
    const auto data = visit([&](auto&& arg) { return ygg::Data<NumericEffectOperator<T, F>>(arg.get_operator(), copy(arg, context).first.get_index()); },
                            element.get_variant());
    return ygg::make_view(data, context.destination);
}

inline std::pair<ConjunctiveConditionView<LiftedTag>, bool> copy(ConjunctiveConditionView<LiftedTag> element, CopyContext& context)
{
    auto conj_cond = datalog::checkout<ConjunctiveCondition<LiftedTag>>(context.builder);

    for (const auto variable : element.get_variables())
        conj_cond->variables.push_back(copy(variable, context).first.get_index());
    for (const auto literal : element.template get_literals<StaticTag>())
        conj_cond->static_literals.push_back(copy(literal, context).first.get_index());
    for (const auto literal : element.template get_literals<FluentTag>())
        conj_cond->fluent_literals.push_back(copy(literal, context).first.get_index());
    for (const auto numeric_constraint : element.get_numeric_constraints())
        conj_cond->numeric_constraints.push_back(copy(numeric_constraint, context).get_data());

    return datalog::insert(context.destination, *conj_cond);
}

inline std::pair<ConjunctiveConditionView<GroundTag>, bool> copy(ConjunctiveConditionView<GroundTag> element, CopyContext& context)
{
    auto conj_cond = datalog::checkout<ConjunctiveCondition<GroundTag>>(context.builder);

    for (const auto literal : element.template get_literals<StaticTag>())
        conj_cond->static_literals.push_back(copy(literal, context).first.get_index());
    for (const auto literal : element.template get_literals<FluentTag>())
        conj_cond->fluent_literals.push_back(copy(literal, context).first.get_index());
    for (const auto numeric_constraint : element.get_numeric_constraints())
        conj_cond->numeric_constraints.push_back(copy(numeric_constraint, context).get_data());

    return datalog::insert(context.destination, *conj_cond);
}

inline std::pair<MetricView, bool> copy(MetricView element, CopyContext& context)
{
    auto metric = datalog::checkout<Metric>(context.builder);

    metric->fexpr = copy(element.get_fexpr(), context).get_data();

    return datalog::insert(context.destination, *metric);
}

template<TaskKind T>
auto merge_rule_head(AtomView<T, FluentTag> head, CopyContext& context)
{
    return copy(head, context).first.get_index();
}

template<TaskKind T>
auto merge_rule_head(NumericEffectOperatorView<T, FluentTag> head, CopyContext& context)
{
    return copy(head, context).get_data();
}

template<RelationKind R>
std::pair<RuleView<LiftedTag, R>, bool> copy(RuleView<LiftedTag, R> element, CopyContext& context)
{
    auto rule = datalog::checkout<Rule<LiftedTag, R>>(context.builder);

    for (const auto variable : element.get_variables())
        rule->variables.push_back(copy(variable, context).first.get_index());
    rule->body = copy(element.get_body(), context).first.get_index();
    rule->head = merge_rule_head(element.get_head(), context);
    for (const auto metric_effect : element.get_metric_effects())
        rule->metric_effects.push_back(copy(metric_effect, context).get_data());

    return datalog::insert(context.destination, *rule);
}

template<RelationKind R>
std::pair<RuleBindingView<R>, bool> copy(RuleBindingView<R> element, CopyContext& context)
{
    auto binding = datalog::checkout<RelationBinding<Rule<LiftedTag, R>>>(context.builder);

    binding->relation = copy(element.get_relation(), context).first.get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(copy(object, context).first.get_index());

    return datalog::insert(context.destination, *binding);
}

template<RelationKind R>
std::pair<RuleView<GroundTag, R>, bool> copy(RuleView<GroundTag, R> element, CopyContext& context)
{
    auto rule = datalog::checkout<Rule<GroundTag, R>>(context.builder);

    rule->binding = copy(element.get_row(), context).first.get_index();
    rule->body = copy(element.get_body(), context).first.get_index();
    rule->head = merge_rule_head(element.get_head(), context);
    for (const auto metric_effect : element.get_metric_effects())
        rule->metric_effects.push_back(copy(metric_effect, context).get_data());

    return datalog::insert(context.destination, *rule);
}

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::datalog
{
extern template std::pair<PredicateView<StaticTag>, bool> copy(PredicateView<StaticTag> element, CopyContext& context);
extern template std::pair<PredicateView<FluentTag>, bool> copy(PredicateView<FluentTag> element, CopyContext& context);

extern template std::pair<AtomView<LiftedTag, StaticTag>, bool> copy(AtomView<LiftedTag, StaticTag> element, CopyContext& context);
extern template std::pair<AtomView<LiftedTag, FluentTag>, bool> copy(AtomView<LiftedTag, FluentTag> element, CopyContext& context);

extern template std::pair<PredicateBindingView<StaticTag>, bool> copy(PredicateBindingView<StaticTag> element, CopyContext& context);
extern template std::pair<PredicateBindingView<FluentTag>, bool> copy(PredicateBindingView<FluentTag> element, CopyContext& context);

extern template std::pair<AtomView<GroundTag, StaticTag>, bool> copy(AtomView<GroundTag, StaticTag> element, CopyContext& context);
extern template std::pair<AtomView<GroundTag, FluentTag>, bool> copy(AtomView<GroundTag, FluentTag> element, CopyContext& context);

extern template std::pair<LiteralView<LiftedTag, StaticTag>, bool> copy(LiteralView<LiftedTag, StaticTag> element, CopyContext& context);
extern template std::pair<LiteralView<LiftedTag, FluentTag>, bool> copy(LiteralView<LiftedTag, FluentTag> element, CopyContext& context);

extern template std::pair<LiteralView<GroundTag, StaticTag>, bool> copy(LiteralView<GroundTag, StaticTag> element, CopyContext& context);
extern template std::pair<LiteralView<GroundTag, FluentTag>, bool> copy(LiteralView<GroundTag, FluentTag> element, CopyContext& context);

extern template std::pair<FunctionView<StaticTag>, bool> copy(FunctionView<StaticTag> element, CopyContext& context);
extern template std::pair<FunctionView<FluentTag>, bool> copy(FunctionView<FluentTag> element, CopyContext& context);

extern template std::pair<FunctionTermView<LiftedTag, StaticTag>, bool> copy(FunctionTermView<LiftedTag, StaticTag> element, CopyContext& context);
extern template std::pair<FunctionTermView<LiftedTag, FluentTag>, bool> copy(FunctionTermView<LiftedTag, FluentTag> element, CopyContext& context);

extern template std::pair<FunctionBindingView<StaticTag>, bool> copy(FunctionBindingView<StaticTag> element, CopyContext& context);
extern template std::pair<FunctionBindingView<FluentTag>, bool> copy(FunctionBindingView<FluentTag> element, CopyContext& context);

extern template std::pair<FunctionTermView<GroundTag, StaticTag>, bool> copy(FunctionTermView<GroundTag, StaticTag> element, CopyContext& context);
extern template std::pair<FunctionTermView<GroundTag, FluentTag>, bool> copy(FunctionTermView<GroundTag, FluentTag> element, CopyContext& context);

extern template std::pair<FunctionTermValueView<GroundTag, StaticTag>, bool> copy(FunctionTermValueView<GroundTag, StaticTag> element, CopyContext& context);
extern template std::pair<FunctionTermValueView<GroundTag, FluentTag>, bool> copy(FunctionTermValueView<GroundTag, FluentTag> element, CopyContext& context);

extern template std::pair<UnaryOperatorView<LiftedTag>, bool> copy(UnaryOperatorView<LiftedTag> element, CopyContext& context);
extern template std::pair<UnaryOperatorView<GroundTag>, bool> copy(UnaryOperatorView<GroundTag> element, CopyContext& context);

extern template std::pair<BinaryOperatorView<LiftedTag, BooleanOperatorKind>, bool> copy(BinaryOperatorView<LiftedTag, BooleanOperatorKind> element,
                                                                                         CopyContext& context);
extern template std::pair<BinaryOperatorView<LiftedTag, ArithmeticOperatorKind>, bool> copy(BinaryOperatorView<LiftedTag, ArithmeticOperatorKind> element,
                                                                                            CopyContext& context);
extern template std::pair<BinaryOperatorView<GroundTag, BooleanOperatorKind>, bool> copy(BinaryOperatorView<GroundTag, BooleanOperatorKind> element,
                                                                                         CopyContext& context);
extern template std::pair<BinaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool> copy(BinaryOperatorView<GroundTag, ArithmeticOperatorKind> element,
                                                                                            CopyContext& context);

extern template std::pair<MultiOperatorView<LiftedTag>, bool> copy(MultiOperatorView<LiftedTag> element, CopyContext& context);
extern template std::pair<MultiOperatorView<GroundTag>, bool> copy(MultiOperatorView<GroundTag> element, CopyContext& context);

extern template ArithmeticOperatorView<LiftedTag> copy(ArithmeticOperatorView<LiftedTag> element, CopyContext& context);
extern template ArithmeticOperatorView<GroundTag> copy(ArithmeticOperatorView<GroundTag> element, CopyContext& context);

extern template std::pair<NumericEffectView<LiftedTag, FluentTag>, bool> copy(NumericEffectView<LiftedTag, FluentTag> element, CopyContext& context);
extern template NumericEffectOperatorView<LiftedTag, FluentTag> copy(NumericEffectOperatorView<LiftedTag, FluentTag> element, CopyContext& context);

extern template std::pair<NumericEffectView<GroundTag, FluentTag>, bool> copy(NumericEffectView<GroundTag, FluentTag> element, CopyContext& context);
extern template NumericEffectOperatorView<GroundTag, FluentTag> copy(NumericEffectOperatorView<GroundTag, FluentTag> element, CopyContext& context);

extern template std::pair<RuleView<LiftedTag, PredicateTag>, bool> copy(RuleView<LiftedTag, PredicateTag> element, CopyContext& context);
extern template std::pair<RuleView<LiftedTag, FunctionTag>, bool> copy(RuleView<LiftedTag, FunctionTag> element, CopyContext& context);

extern template std::pair<RuleBindingView<PredicateTag>, bool> copy(RuleBindingView<PredicateTag> element, CopyContext& context);
extern template std::pair<RuleBindingView<FunctionTag>, bool> copy(RuleBindingView<FunctionTag> element, CopyContext& context);

extern template std::pair<RuleView<GroundTag, PredicateTag>, bool> copy(RuleView<GroundTag, PredicateTag> element, CopyContext& context);
extern template std::pair<RuleView<GroundTag, FunctionTag>, bool> copy(RuleView<GroundTag, FunctionTag> element, CopyContext& context);
}

#endif

#endif
