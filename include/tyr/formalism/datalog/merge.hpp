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

#include "tyr/formalism/datalog/builder.hpp"
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

std::pair<ConjunctiveConditionView, bool> merge_d2d(ConjunctiveConditionView element, MergeContext& context);

std::pair<GroundConjunctiveConditionView, bool> merge_d2d(GroundConjunctiveConditionView element, MergeContext& context);

template<RelationKind R>
std::pair<RuleView<R>, bool> merge_d2d(RuleView<R> element, MergeContext& context);

// Common

inline std::pair<VariableView, bool> merge_d2d(VariableView element, MergeContext& context)
{
    auto variable_ptr = context.builder.template get_builder<Variable>();
    auto& variable = *variable_ptr;
    variable.clear();

    variable.name = element.get_name();

    canonicalize(variable);
    return context.destination.get_or_create(variable);
}

inline std::pair<ObjectView, bool> merge_d2d(ObjectView element, MergeContext& context)
{
    auto object_ptr = context.builder.template get_builder<Object>();
    auto& object = *object_ptr;
    object.clear();

    object.name = element.get_name();

    canonicalize(object);
    return context.destination.get_or_create(object);
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
    auto predicate_ptr = context.builder.template get_builder<Predicate<T>>();
    auto& predicate = *predicate_ptr;
    predicate.clear();

    predicate.name = element.get_name();
    predicate.arity = element.get_arity();

    canonicalize(predicate);
    return context.destination.get_or_create(predicate);
}

template<FactKind T>
std::pair<AtomView<T>, bool> merge_d2d(AtomView<T> element, MergeContext& context)
{
    auto atom_ptr = context.builder.template get_builder<Atom<T>>();
    auto& atom = *atom_ptr;
    atom.clear();

    atom.predicate = element.get_predicate().get_index();
    for (const auto term : element.get_terms())
        atom.terms.push_back(merge_d2d(term, context).get_data());

    canonicalize(atom);
    return context.destination.get_or_create(atom);
}

template<FactKind T>
std::pair<PredicateBindingView<T>, bool> merge_d2d(PredicateBindingView<T> element, MergeContext& context)
{
    auto binding_ptr = context.builder.template get_builder<RelationBinding<Predicate<T>>>();
    auto& binding = *binding_ptr;
    binding.clear();

    binding.relation = element.get_relation().get_index();
    for (const auto object : element.get_objects())
        binding.objects.push_back(object.get_index());

    canonicalize(binding);
    return context.destination.get_or_create(binding);
}

template<FactKind T>
std::pair<GroundAtomView<T>, bool> merge_d2d(GroundAtomView<T> element, MergeContext& context)
{
    auto atom_ptr = context.builder.template get_builder<GroundAtom<T>>();
    auto& atom = *atom_ptr;
    atom.clear();

    atom.binding = merge_d2d(element.get_row(), context).first.get_index();

    canonicalize(atom);
    return context.destination.get_or_create(atom);
}

template<FactKind T>
std::pair<LiteralView<T>, bool> merge_d2d(LiteralView<T> element, MergeContext& context)
{
    auto literal_ptr = context.builder.template get_builder<Literal<T>>();
    auto& literal = *literal_ptr;
    literal.clear();

    literal.polarity = element.get_polarity();
    literal.atom = merge_d2d(element.get_atom(), context).first.get_index();

    canonicalize(literal);
    return context.destination.get_or_create(literal);
}

template<FactKind T>
std::pair<GroundLiteralView<T>, bool> merge_d2d(GroundLiteralView<T> element, MergeContext& context)
{
    auto literal_ptr = context.builder.template get_builder<GroundLiteral<T>>();
    auto& literal = *literal_ptr;
    literal.clear();

    literal.polarity = element.get_polarity();
    literal.atom = merge_d2d(element.get_atom(), context).first.get_index();

    canonicalize(literal);
    return context.destination.get_or_create(literal);
}

// Numeric

template<FactKind T>
std::pair<FunctionView<T>, bool> merge_d2d(FunctionView<T> element, MergeContext& context)
{
    auto function_ptr = context.builder.template get_builder<Function<T>>();
    auto& function = *function_ptr;
    function.clear();

    function.name = element.get_name();
    function.arity = element.get_arity();

    canonicalize(function);
    return context.destination.get_or_create(function);
}

template<FactKind T>
std::pair<FunctionTermView<T>, bool> merge_d2d(FunctionTermView<T> element, MergeContext& context)
{
    auto fterm_ptr = context.builder.template get_builder<FunctionTerm<T>>();
    auto& fterm = *fterm_ptr;
    fterm.clear();

    fterm.function = element.get_function().get_index();
    for (const auto term : element.get_terms())
        fterm.terms.push_back(merge_d2d(term, context).get_data());

    canonicalize(fterm);
    return context.destination.get_or_create(fterm);
}

template<FactKind T>
std::pair<FunctionBindingView<T>, bool> merge_d2d(FunctionBindingView<T> element, MergeContext& context)
{
    auto binding_ptr = context.builder.template get_builder<RelationBinding<Function<T>>>();
    auto& binding = *binding_ptr;
    binding.clear();

    binding.relation = element.get_relation().get_index();
    for (const auto object : element.get_objects())
        binding.objects.push_back(object.get_index());

    canonicalize(binding);
    return context.destination.get_or_create(binding);
}

template<FactKind T>
std::pair<GroundFunctionTermView<T>, bool> merge_d2d(GroundFunctionTermView<T> element, MergeContext& context)
{
    auto fterm_ptr = context.builder.template get_builder<GroundFunctionTerm<T>>();
    auto& fterm = *fterm_ptr;
    fterm.clear();

    fterm.binding = merge_d2d(element.get_row(), context).first.get_index();

    canonicalize(fterm);
    return context.destination.get_or_create(fterm);
}

template<FactKind T>
std::pair<GroundFunctionTermValueView<T>, bool> merge_d2d(GroundFunctionTermValueView<T> element, MergeContext& context)
{
    auto fterm_value_ptr = context.builder.template get_builder<GroundFunctionTermValue<T>>();
    auto& fterm_value = *fterm_value_ptr;
    fterm_value.clear();

    fterm_value.fterm = merge_d2d(element.get_fterm(), context).first.get_index();
    fterm_value.value = element.get_value();

    canonicalize(fterm_value);
    return context.destination.get_or_create(fterm_value);
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
    auto unary_ptr = context.builder.template get_builder<UnaryOperator<T>>();
    auto& unary = *unary_ptr;
    unary.clear();

    unary.operator_kind = element.get_operator();
    unary.arg = merge_d2d(element.get_arg(), context).get_data();

    canonicalize(unary);
    return context.destination.get_or_create(unary);
}

template<BinaryOperatorKind O, typename T>
std::pair<BinaryOperatorView<O, T>, bool> merge_d2d(BinaryOperatorView<O, T> element, MergeContext& context)
{
    auto binary_ptr = context.builder.template get_builder<BinaryOperator<O, T>>();
    auto& binary = *binary_ptr;
    binary.clear();

    binary.operator_kind = element.get_operator();
    binary.lhs = merge_d2d(element.get_lhs(), context).get_data();
    binary.rhs = merge_d2d(element.get_rhs(), context).get_data();

    canonicalize(binary);
    return context.destination.get_or_create(binary);
}

template<typename T>
std::pair<MultiOperatorView<T>, bool> merge_d2d(MultiOperatorView<T> element, MergeContext& context)
{
    auto multi_ptr = context.builder.template get_builder<MultiOperator<T>>();
    auto& multi = *multi_ptr;
    multi.clear();

    multi.operator_kind = element.get_operator();
    for (const auto arg : element.get_args())
        multi.args.push_back(merge_d2d(arg, context).get_data());

    canonicalize(multi);
    return context.destination.get_or_create(multi);
}

template<typename T>
ArithmeticOperatorView<T> merge_d2d(ArithmeticOperatorView<T> element, MergeContext& context)
{
    const auto data = visit(
        [&](auto&& arg) { return ygg::Data<ArithmeticOperator<T>>(arg.get_operator(), merge_d2d(arg, context).first.get_index()); }, element.get_variant());
    return ygg::make_view(data, context.destination);
}

template<typename T>
BooleanOperatorView<T> merge_d2d(BooleanOperatorView<T> element, MergeContext& context)
{
    const auto data = visit(
        [&](auto&& arg) { return ygg::Data<BooleanOperator<T>>(arg.get_operator(), merge_d2d(arg, context).first.get_index()); }, element.get_variant());
    return ygg::make_view(data, context.destination);
}

template<FactKind T>
std::pair<NumericEffectView<T>, bool> merge_d2d(NumericEffectView<T> element, MergeContext& context)
{
    auto numeric_effect_ptr = context.builder.template get_builder<NumericEffect<T>>();
    auto& numeric_effect = *numeric_effect_ptr;
    numeric_effect.clear();

    numeric_effect.operator_kind = element.get_operator();
    numeric_effect.fterm = merge_d2d(element.get_fterm(), context).first.get_index();
    numeric_effect.fexpr = merge_d2d(element.get_fexpr(), context).get_data();

    canonicalize(numeric_effect);
    return context.destination.get_or_create(numeric_effect);
}

template<FactKind T>
NumericEffectOperatorView<T> merge_d2d(NumericEffectOperatorView<T> element, MergeContext& context)
{
    const auto data = visit(
        [&](auto&& arg) { return ygg::Data<NumericEffectOperator<T>>(arg.get_operator(), merge_d2d(arg, context).first.get_index()); }, element.get_variant());
    return ygg::make_view(data, context.destination);
}

inline std::pair<ConjunctiveConditionView, bool> merge_d2d(ConjunctiveConditionView element, MergeContext& context)
{
    auto conj_cond_ptr = context.builder.template get_builder<ConjunctiveCondition>();
    auto& conj_cond = *conj_cond_ptr;
    conj_cond.clear();

    for (const auto literal : element.template get_literals<StaticTag>())
        conj_cond.static_literals.push_back(merge_d2d(literal, context).first.get_index());
    for (const auto literal : element.template get_literals<FluentTag>())
        conj_cond.fluent_literals.push_back(merge_d2d(literal, context).first.get_index());
    for (const auto numeric_constraint : element.get_numeric_constraints())
        conj_cond.numeric_constraints.push_back(merge_d2d(numeric_constraint, context).get_data());

    canonicalize(conj_cond);
    return context.destination.get_or_create(conj_cond);
}

inline std::pair<GroundConjunctiveConditionView, bool> merge_d2d(GroundConjunctiveConditionView element, MergeContext& context)
{
    auto conj_cond_ptr = context.builder.template get_builder<GroundConjunctiveCondition>();
    auto& conj_cond = *conj_cond_ptr;
    conj_cond.clear();

    for (const auto literal : element.template get_literals<StaticTag>())
        conj_cond.static_literals.push_back(merge_d2d(literal, context).first.get_index());
    for (const auto literal : element.template get_literals<FluentTag>())
        conj_cond.fluent_literals.push_back(merge_d2d(literal, context).first.get_index());
    for (const auto numeric_constraint : element.get_numeric_constraints())
        conj_cond.numeric_constraints.push_back(merge_d2d(numeric_constraint, context).get_data());

    canonicalize(conj_cond);
    return context.destination.get_or_create(conj_cond);
}

inline auto merge_rule_head(AtomView<FluentTag> head, MergeContext& context) { return merge_d2d(head, context).first.get_index(); }

inline auto merge_rule_head(NumericEffectOperatorView<FluentTag> head, MergeContext& context) { return merge_d2d(head, context).get_data(); }

template<RelationKind R>
std::pair<RuleView<R>, bool> merge_d2d(RuleView<R> element, MergeContext& context)
{
    auto rule_ptr = context.builder.template get_builder<Rule<R>>();
    auto& rule = *rule_ptr;
    rule.clear();

    for (const auto variable : element.get_variables())
        rule.variables.push_back(merge_d2d(variable, context).first.get_index());
    rule.body = merge_d2d(element.get_body(), context).first.get_index();
    rule.head = merge_rule_head(element.get_head(), context);
    for (const auto metric_effect : element.get_metric_effects())
        rule.metric_effects.push_back(merge_d2d(metric_effect, context).get_data());

    canonicalize(rule);
    return context.destination.get_or_create(rule);
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

extern template std::pair<RuleView<PredicateTag>, bool> merge_d2d(RuleView<PredicateTag> element, MergeContext& context);
extern template std::pair<RuleView<FunctionTag>, bool> merge_d2d(RuleView<FunctionTag> element, MergeContext& context);
}

#endif

#endif
