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

#ifndef TYR_FORMALISM_PLANNING_MERGE_HPP_
#define TYR_FORMALISM_PLANNING_MERGE_HPP_

#include "tyr/formalism/planning/canonicalization.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/merge_decl.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <yggdrasil/containers/tuple.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::formalism::planning
{

// Common

std::pair<VariableView, bool> merge_p2p(VariableView element, MergeContext& context);

std::pair<ObjectView, bool> merge_p2p(ObjectView element, MergeContext& context);

template<FactKind T>
std::pair<PredicateBindingView<T>, bool> merge_p2p(PredicateBindingView<T> element, MergeContext& context);

template<FactKind T>
std::pair<FunctionBindingView<T>, bool> merge_p2p(FunctionBindingView<T> element, MergeContext& context);

std::pair<ActionBindingView, bool> merge_p2p(ActionBindingView element, MergeContext& context);

std::pair<AxiomBindingView, bool> merge_p2p(AxiomBindingView element, MergeContext& context);

TermView merge_p2p(TermView element, MergeContext& context);

// Propositional

template<FactKind T>
std::pair<PredicateView<T>, bool> merge_p2p(PredicateView<T> element, MergeContext& context);

template<FactKind T>
std::pair<AtomView<T>, bool> merge_p2p(AtomView<T> element, MergeContext& context);

template<FactKind T>
std::pair<GroundAtomView<T>, bool> merge_p2p(GroundAtomView<T> element, MergeContext& context);

template<FactKind T>
std::pair<LiteralView<T>, bool> merge_p2p(LiteralView<T> element, MergeContext& context);

template<FactKind T>
std::pair<GroundLiteralView<T>, bool> merge_p2p(GroundLiteralView<T> element, MergeContext& context);

// Numeric

template<FactKind T>
std::pair<FunctionView<T>, bool> merge_p2p(FunctionView<T> element, MergeContext& context);

template<FactKind T>
std::pair<FunctionTermView<T>, bool> merge_p2p(FunctionTermView<T> element, MergeContext& context);

template<FactKind T>
std::pair<GroundFunctionTermView<T>, bool> merge_p2p(GroundFunctionTermView<T> element, MergeContext& context);

template<FactKind T>
std::pair<GroundFunctionTermValueView<T>, bool> merge_p2p(GroundFunctionTermValueView<T> element, MergeContext& context);

FunctionExpressionView merge_p2p(FunctionExpressionView element, MergeContext& context);

GroundFunctionExpressionView merge_p2p(GroundFunctionExpressionView element, MergeContext& context);

template<typename T>
std::pair<UnaryOperatorView<T>, bool> merge_p2p(UnaryOperatorView<T> element, MergeContext& context);

template<BinaryOperatorKind O, typename T>
std::pair<BinaryOperatorView<O, T>, bool> merge_p2p(BinaryOperatorView<O, T> element, MergeContext& context);

template<typename T>
std::pair<MultiOperatorView<T>, bool> merge_p2p(MultiOperatorView<T> element, MergeContext& context);

template<typename T>
ArithmeticOperatorView<T> merge_p2p(ArithmeticOperatorView<T> element, MergeContext& context);

template<typename T>
BooleanOperatorView<T> merge_p2p(BooleanOperatorView<T> element, MergeContext& context);

template<FactKind T>
std::pair<NumericEffectView<T>, bool> merge_p2p(NumericEffectView<T> element, MergeContext& context);

template<FactKind T>
NumericEffectOperatorView<T> merge_p2p(NumericEffectOperatorView<T> element, MergeContext& context);

template<FactKind T>
std::pair<GroundNumericEffectView<T>, bool> merge_p2p(GroundNumericEffectView<T> element, MergeContext& context);

template<FactKind T>
GroundNumericEffectOperatorView<T> merge_p2p(GroundNumericEffectOperatorView<T> element, MergeContext& context);

// Composite

std::pair<ConjunctiveConditionView, bool> merge_p2p(ConjunctiveConditionView element, MergeContext& context);

std::pair<AxiomView, bool> merge_p2p(AxiomView element, MergeContext& context);

std::pair<MetricView, bool> merge_p2p(MetricView element, MergeContext& context);

// Common

inline std::pair<VariableView, bool> merge_p2p(VariableView element, MergeContext& context)
{
    auto variable = ::tyr::formalism::planning::checkout<Variable>(context.builder);

    variable->name = element.get_name();

    return ::tyr::formalism::planning::get_or_create(context.destination, *variable);
}

inline std::pair<ObjectView, bool> merge_p2p(ObjectView element, MergeContext& context)
{
    auto object = ::tyr::formalism::planning::checkout<Object>(context.builder);

    object->name = element.get_name();

    return ::tyr::formalism::planning::get_or_create(context.destination, *object);
}

template<FactKind T>
std::pair<PredicateBindingView<T>, bool> merge_p2p(PredicateBindingView<T> element, MergeContext& context)
{
    auto binding = ::tyr::formalism::planning::checkout<RelationBinding<Predicate<T>>>(context.builder);

    binding->relation = element.get_relation().get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(object.get_index());

    return ::tyr::formalism::planning::get_or_create(context.destination, *binding);
}

template<FactKind T>
std::pair<FunctionBindingView<T>, bool> merge_p2p(FunctionBindingView<T> element, MergeContext& context)
{
    auto binding = ::tyr::formalism::planning::checkout<RelationBinding<Function<T>>>(context.builder);

    binding->relation = element.get_relation().get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(object.get_index());

    return ::tyr::formalism::planning::get_or_create(context.destination, *binding);
}

inline std::pair<ActionBindingView, bool> merge_p2p(ActionBindingView element, MergeContext& context)
{
    auto binding = ::tyr::formalism::planning::checkout<RelationBinding<Action>>(context.builder);

    binding->relation = element.get_relation().get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(object.get_index());

    return ::tyr::formalism::planning::get_or_create(context.destination, *binding);
}

inline std::pair<AxiomBindingView, bool> merge_p2p(AxiomBindingView element, MergeContext& context)
{
    auto binding = ::tyr::formalism::planning::checkout<RelationBinding<Axiom>>(context.builder);

    binding->relation = element.get_relation().get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(object.get_index());

    return ::tyr::formalism::planning::get_or_create(context.destination, *binding);
}

inline TermView merge_p2p(TermView element, MergeContext& context)
{
    const auto data = visit(
        [&](auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ParameterIndex>)
                return ygg::Data<Term>(arg);
            else if constexpr (std::is_same_v<Alternative, ObjectView>)
                return ygg::Data<Term>(merge_p2p(arg, context).first.get_index());
            else
                static_assert(ygg::dependent_false<Alternative>::value, "Missing case");
        },
        element.get_variant());
    return ygg::make_view(data, context.destination);
}

// Propositional

template<FactKind T>
std::pair<PredicateView<T>, bool> merge_p2p(PredicateView<T> element, MergeContext& context)
{
    auto predicate = ::tyr::formalism::planning::checkout<Predicate<T>>(context.builder);

    predicate->name = element.get_name();
    predicate->arity = element.get_arity();

    return ::tyr::formalism::planning::get_or_create(context.destination, *predicate);
}

template<FactKind T>
std::pair<AtomView<T>, bool> merge_p2p(AtomView<T> element, MergeContext& context)
{
    auto atom = ::tyr::formalism::planning::checkout<Atom<T>>(context.builder);

    atom->predicate = merge_p2p(element.get_predicate(), context).first.get_index();
    for (const auto term : element.get_terms())
        atom->terms.push_back(merge_p2p(term, context).get_data());

    return ::tyr::formalism::planning::get_or_create(context.destination, *atom);
}

template<FactKind T>
std::pair<GroundAtomView<T>, bool> merge_p2p(GroundAtomView<T> element, MergeContext& context)
{
    auto atom = ::tyr::formalism::planning::checkout<GroundAtom<T>>(context.builder);

    atom->binding = merge_p2p(element.get_row(), context).first.get_index();

    return ::tyr::formalism::planning::get_or_create(context.destination, *atom);
}

template<FactKind T>
std::pair<LiteralView<T>, bool> merge_p2p(LiteralView<T> element, MergeContext& context)
{
    auto literal = ::tyr::formalism::planning::checkout<Literal<T>>(context.builder);

    literal->polarity = element.get_polarity();
    literal->atom = merge_p2p(element.get_atom(), context).first.get_index();

    return ::tyr::formalism::planning::get_or_create(context.destination, *literal);
}

template<FactKind T>
std::pair<GroundLiteralView<T>, bool> merge_p2p(GroundLiteralView<T> element, MergeContext& context)
{
    auto literal = ::tyr::formalism::planning::checkout<GroundLiteral<T>>(context.builder);

    literal->polarity = element.get_polarity();
    literal->atom = merge_p2p(element.get_atom(), context).first.get_index();

    return ::tyr::formalism::planning::get_or_create(context.destination, *literal);
}

// Numeric

template<FactKind T>
std::pair<FunctionView<T>, bool> merge_p2p(FunctionView<T> element, MergeContext& context)
{
    auto function = ::tyr::formalism::planning::checkout<Function<T>>(context.builder);

    function->name = element.get_name();
    function->arity = element.get_arity();

    return ::tyr::formalism::planning::get_or_create(context.destination, *function);
}

template<FactKind T>
std::pair<FunctionTermView<T>, bool> merge_p2p(FunctionTermView<T> element, MergeContext& context)
{
    auto fterm = ::tyr::formalism::planning::checkout<FunctionTerm<T>>(context.builder);

    fterm->function = element.get_function().get_index();
    for (const auto term : element.get_terms())
        fterm->terms.push_back(merge_p2p(term, context).get_data());

    return ::tyr::formalism::planning::get_or_create(context.destination, *fterm);
}

template<FactKind T>
std::pair<GroundFunctionTermView<T>, bool> merge_p2p(GroundFunctionTermView<T> element, MergeContext& context)
{
    auto fterm = ::tyr::formalism::planning::checkout<GroundFunctionTerm<T>>(context.builder);

    fterm->binding = merge_p2p(element.get_row(), context).first.get_index();

    return ::tyr::formalism::planning::get_or_create(context.destination, *fterm);
}

template<FactKind T>
std::pair<GroundFunctionTermValueView<T>, bool> merge_p2p(GroundFunctionTermValueView<T> element, MergeContext& context)
{
    auto fterm_value = ::tyr::formalism::planning::checkout<GroundFunctionTermValue<T>>(context.builder);

    fterm_value->fterm = merge_p2p(element.get_fterm(), context).first.get_index();
    fterm_value->value = element.get_value();

    return ::tyr::formalism::planning::get_or_create(context.destination, *fterm_value);
}

inline FunctionExpressionView merge_p2p(FunctionExpressionView element, MergeContext& context)
{
    const auto data = visit(
        [&](auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return ygg::Data<FunctionExpression>(arg);
            else if constexpr (std::is_same_v<Alternative, LiftedArithmeticOperatorView>)
                return ygg::Data<FunctionExpression>(merge_p2p(arg, context).get_data());
            else
                return ygg::Data<FunctionExpression>(merge_p2p(arg, context).first.get_index());
        },
        element.get_variant());
    return ygg::make_view(data, context.destination);
}

inline GroundFunctionExpressionView merge_p2p(GroundFunctionExpressionView element, MergeContext& context)
{
    const auto data = visit(
        [&](auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return ygg::Data<GroundFunctionExpression>(arg);
            else if constexpr (std::is_same_v<Alternative, GroundArithmeticOperatorView>)
                return ygg::Data<GroundFunctionExpression>(merge_p2p(arg, context).get_data());
            else
                return ygg::Data<GroundFunctionExpression>(merge_p2p(arg, context).first.get_index());
        },
        element.get_variant());
    return ygg::make_view(data, context.destination);
}

template<typename T>
std::pair<UnaryOperatorView<T>, bool> merge_p2p(UnaryOperatorView<T> element, MergeContext& context)
{
    auto unary = ::tyr::formalism::planning::checkout<UnaryOperator<T>>(context.builder);

    unary->operator_kind = element.get_operator();
    unary->arg = merge_p2p(element.get_arg(), context).get_data();

    return ::tyr::formalism::planning::get_or_create(context.destination, *unary);
}

template<BinaryOperatorKind O, typename T>
std::pair<BinaryOperatorView<O, T>, bool> merge_p2p(BinaryOperatorView<O, T> element, MergeContext& context)
{
    auto binary = ::tyr::formalism::planning::checkout<BinaryOperator<O, T>>(context.builder);

    binary->operator_kind = element.get_operator();
    binary->lhs = merge_p2p(element.get_lhs(), context).get_data();
    binary->rhs = merge_p2p(element.get_rhs(), context).get_data();

    return ::tyr::formalism::planning::get_or_create(context.destination, *binary);
}

template<typename T>
std::pair<MultiOperatorView<T>, bool> merge_p2p(MultiOperatorView<T> element, MergeContext& context)
{
    auto multi = ::tyr::formalism::planning::checkout<MultiOperator<T>>(context.builder);

    multi->operator_kind = element.get_operator();
    for (const auto arg : element.get_args())
        multi->args.push_back(merge_p2p(arg, context).get_data());

    return ::tyr::formalism::planning::get_or_create(context.destination, *multi);
}

template<typename T>
ArithmeticOperatorView<T> merge_p2p(ArithmeticOperatorView<T> element, MergeContext& context)
{
    const auto data = visit([&](auto&& arg) { return ygg::Data<ArithmeticOperator<T>>(arg.get_operator(), merge_p2p(arg, context).first.get_index()); },
                            element.get_variant());
    return ygg::make_view(data, context.destination);
}

template<typename T>
BooleanOperatorView<T> merge_p2p(BooleanOperatorView<T> element, MergeContext& context)
{
    const auto data =
        visit([&](auto&& arg) { return ygg::Data<BooleanOperator<T>>(arg.get_operator(), merge_p2p(arg, context).first.get_index()); }, element.get_variant());
    return ygg::make_view(data, context.destination);
}

template<FactKind T>
std::pair<NumericEffectView<T>, bool> merge_p2p(NumericEffectView<T> element, MergeContext& context)
{
    auto numeric_effect = ::tyr::formalism::planning::checkout<NumericEffect<T>>(context.builder);

    numeric_effect->operator_kind = element.get_operator();
    numeric_effect->fterm = merge_p2p(element.get_fterm(), context).first.get_index();
    numeric_effect->fexpr = merge_p2p(element.get_fexpr(), context).get_data();

    return ::tyr::formalism::planning::get_or_create(context.destination, *numeric_effect);
}

template<FactKind T>
NumericEffectOperatorView<T> merge_p2p(NumericEffectOperatorView<T> element, MergeContext& context)
{
    const auto data = visit([&](auto&& arg) { return ygg::Data<NumericEffectOperator<T>>(arg.get_operator(), merge_p2p(arg, context).first.get_index()); },
                            element.get_variant());
    return ygg::make_view(data, context.destination);
}

template<FactKind T>
std::pair<GroundNumericEffectView<T>, bool> merge_p2p(GroundNumericEffectView<T> element, MergeContext& context)
{
    auto numeric_effect = ::tyr::formalism::planning::checkout<GroundNumericEffect<T>>(context.builder);

    numeric_effect->operator_kind = element.get_operator();
    numeric_effect->fterm = merge_p2p(element.get_fterm(), context).first.get_index();
    numeric_effect->fexpr = merge_p2p(element.get_fexpr(), context).get_data();

    return ::tyr::formalism::planning::get_or_create(context.destination, *numeric_effect);
}

template<FactKind T>
GroundNumericEffectOperatorView<T> merge_p2p(GroundNumericEffectOperatorView<T> element, MergeContext& context)
{
    const auto data =
        visit([&](auto&& arg) { return ygg::Data<GroundNumericEffectOperator<T>>(arg.get_operator(), merge_p2p(arg, context).first.get_index()); },
              element.get_variant());
    return ygg::make_view(data, context.destination);
}

// Composite

inline std::pair<ConjunctiveConditionView, bool> merge_p2p(ConjunctiveConditionView element, MergeContext& context)
{
    auto conj_cond = ::tyr::formalism::planning::checkout<ConjunctiveCondition>(context.builder);

    for (const auto variable : element.get_variables())
        conj_cond->variables.push_back(merge_p2p(variable, context).first.get_index());
    for (const auto literal : element.template get_literals<StaticTag>())
        conj_cond->static_literals.push_back(merge_p2p(literal, context).first.get_index());
    for (const auto literal : element.template get_literals<FluentTag>())
        conj_cond->fluent_literals.push_back(merge_p2p(literal, context).first.get_index());
    for (const auto literal : element.template get_literals<DerivedTag>())
        conj_cond->derived_literals.push_back(merge_p2p(literal, context).first.get_index());
    for (const auto numeric_constraint : element.get_numeric_constraints())
        conj_cond->numeric_constraints.push_back(merge_p2p(numeric_constraint, context).get_data());

    return ::tyr::formalism::planning::get_or_create(context.destination, *conj_cond);
}

inline std::pair<AxiomView, bool> merge_p2p(AxiomView element, MergeContext& context)
{
    auto axiom = ::tyr::formalism::planning::checkout<Axiom>(context.builder);

    for (const auto variable : element.get_variables())
        axiom->variables.push_back(merge_p2p(variable, context).first.get_index());
    axiom->body = merge_p2p(element.get_body(), context).first.get_index();
    axiom->head = merge_p2p(element.get_head(), context).first.get_index();

    return ::tyr::formalism::planning::get_or_create(context.destination, *axiom);
}

inline std::pair<MetricView, bool> merge_p2p(MetricView element, MergeContext& context)
{
    auto metric = ::tyr::formalism::planning::checkout<Metric>(context.builder);

    metric->optimization_direction = element.get_optimization_direction();
    metric->fexpr = merge_p2p(element.get_fexpr(), context).get_data();

    return ::tyr::formalism::planning::get_or_create(context.destination, *metric);
}

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::planning
{
extern template std::pair<PredicateBindingView<StaticTag>, bool> merge_p2p(PredicateBindingView<StaticTag> element, MergeContext& context);
extern template std::pair<PredicateBindingView<FluentTag>, bool> merge_p2p(PredicateBindingView<FluentTag> element, MergeContext& context);
extern template std::pair<PredicateBindingView<DerivedTag>, bool> merge_p2p(PredicateBindingView<DerivedTag> element, MergeContext& context);

extern template std::pair<FunctionBindingView<StaticTag>, bool> merge_p2p(FunctionBindingView<StaticTag> element, MergeContext& context);
extern template std::pair<FunctionBindingView<FluentTag>, bool> merge_p2p(FunctionBindingView<FluentTag> element, MergeContext& context);
extern template std::pair<FunctionBindingView<AuxiliaryTag>, bool> merge_p2p(FunctionBindingView<AuxiliaryTag> element, MergeContext& context);

extern template std::pair<PredicateView<StaticTag>, bool> merge_p2p(PredicateView<StaticTag> element, MergeContext& context);
extern template std::pair<PredicateView<FluentTag>, bool> merge_p2p(PredicateView<FluentTag> element, MergeContext& context);
extern template std::pair<PredicateView<DerivedTag>, bool> merge_p2p(PredicateView<DerivedTag> element, MergeContext& context);

extern template std::pair<AtomView<StaticTag>, bool> merge_p2p(AtomView<StaticTag> element, MergeContext& context);
extern template std::pair<AtomView<FluentTag>, bool> merge_p2p(AtomView<FluentTag> element, MergeContext& context);
extern template std::pair<AtomView<DerivedTag>, bool> merge_p2p(AtomView<DerivedTag> element, MergeContext& context);

extern template std::pair<GroundAtomView<StaticTag>, bool> merge_p2p(GroundAtomView<StaticTag> element, MergeContext& context);
extern template std::pair<GroundAtomView<FluentTag>, bool> merge_p2p(GroundAtomView<FluentTag> element, MergeContext& context);
extern template std::pair<GroundAtomView<DerivedTag>, bool> merge_p2p(GroundAtomView<DerivedTag> element, MergeContext& context);

extern template std::pair<LiteralView<StaticTag>, bool> merge_p2p(LiteralView<StaticTag> element, MergeContext& context);
extern template std::pair<LiteralView<FluentTag>, bool> merge_p2p(LiteralView<FluentTag> element, MergeContext& context);
extern template std::pair<LiteralView<DerivedTag>, bool> merge_p2p(LiteralView<DerivedTag> element, MergeContext& context);

extern template std::pair<GroundLiteralView<StaticTag>, bool> merge_p2p(GroundLiteralView<StaticTag> element, MergeContext& context);
extern template std::pair<GroundLiteralView<FluentTag>, bool> merge_p2p(GroundLiteralView<FluentTag> element, MergeContext& context);
extern template std::pair<GroundLiteralView<DerivedTag>, bool> merge_p2p(GroundLiteralView<DerivedTag> element, MergeContext& context);

extern template std::pair<FunctionView<StaticTag>, bool> merge_p2p(FunctionView<StaticTag> element, MergeContext& context);
extern template std::pair<FunctionView<FluentTag>, bool> merge_p2p(FunctionView<FluentTag> element, MergeContext& context);
extern template std::pair<FunctionView<AuxiliaryTag>, bool> merge_p2p(FunctionView<AuxiliaryTag> element, MergeContext& context);

extern template std::pair<FunctionTermView<StaticTag>, bool> merge_p2p(FunctionTermView<StaticTag> element, MergeContext& context);
extern template std::pair<FunctionTermView<FluentTag>, bool> merge_p2p(FunctionTermView<FluentTag> element, MergeContext& context);
extern template std::pair<FunctionTermView<AuxiliaryTag>, bool> merge_p2p(FunctionTermView<AuxiliaryTag> element, MergeContext& context);

extern template std::pair<GroundFunctionTermView<StaticTag>, bool> merge_p2p(GroundFunctionTermView<StaticTag> element, MergeContext& context);
extern template std::pair<GroundFunctionTermView<FluentTag>, bool> merge_p2p(GroundFunctionTermView<FluentTag> element, MergeContext& context);
extern template std::pair<GroundFunctionTermView<AuxiliaryTag>, bool> merge_p2p(GroundFunctionTermView<AuxiliaryTag> element, MergeContext& context);

extern template std::pair<GroundFunctionTermValueView<StaticTag>, bool> merge_p2p(GroundFunctionTermValueView<StaticTag> element, MergeContext& context);
extern template std::pair<GroundFunctionTermValueView<FluentTag>, bool> merge_p2p(GroundFunctionTermValueView<FluentTag> element, MergeContext& context);
extern template std::pair<GroundFunctionTermValueView<AuxiliaryTag>, bool> merge_p2p(GroundFunctionTermValueView<AuxiliaryTag> element, MergeContext& context);

extern template std::pair<UnaryOperatorView<ygg::Data<FunctionExpression>>, bool> merge_p2p(UnaryOperatorView<ygg::Data<FunctionExpression>> element,
                                                                                            MergeContext& context);
extern template std::pair<UnaryOperatorView<ygg::Data<GroundFunctionExpression>>, bool>
merge_p2p(UnaryOperatorView<ygg::Data<GroundFunctionExpression>> element, MergeContext& context);

extern template std::pair<BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression>>, bool>
merge_p2p(BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression>> element, MergeContext& context);
extern template std::pair<BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression>>, bool>
merge_p2p(BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression>> element, MergeContext& context);
extern template std::pair<BinaryOperatorView<BooleanOperatorKind, ygg::Data<GroundFunctionExpression>>, bool>
merge_p2p(BinaryOperatorView<BooleanOperatorKind, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);
extern template std::pair<BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<GroundFunctionExpression>>, bool>
merge_p2p(BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);

extern template std::pair<MultiOperatorView<ygg::Data<FunctionExpression>>, bool> merge_p2p(MultiOperatorView<ygg::Data<FunctionExpression>> element,
                                                                                            MergeContext& context);
extern template std::pair<MultiOperatorView<ygg::Data<GroundFunctionExpression>>, bool>
merge_p2p(MultiOperatorView<ygg::Data<GroundFunctionExpression>> element, MergeContext& context);

extern template ArithmeticOperatorView<ygg::Data<FunctionExpression>> merge_p2p(ArithmeticOperatorView<ygg::Data<FunctionExpression>> element,
                                                                                MergeContext& context);
extern template ArithmeticOperatorView<ygg::Data<GroundFunctionExpression>> merge_p2p(ArithmeticOperatorView<ygg::Data<GroundFunctionExpression>> element,
                                                                                      MergeContext& context);

extern template BooleanOperatorView<ygg::Data<FunctionExpression>> merge_p2p(BooleanOperatorView<ygg::Data<FunctionExpression>> element, MergeContext& context);
extern template BooleanOperatorView<ygg::Data<GroundFunctionExpression>> merge_p2p(BooleanOperatorView<ygg::Data<GroundFunctionExpression>> element,
                                                                                   MergeContext& context);

extern template std::pair<NumericEffectView<FluentTag>, bool> merge_p2p(NumericEffectView<FluentTag> element, MergeContext& context);

extern template std::pair<NumericEffectView<AuxiliaryTag>, bool> merge_p2p(NumericEffectView<AuxiliaryTag> element, MergeContext& context);

extern template NumericEffectOperatorView<FluentTag> merge_p2p(NumericEffectOperatorView<FluentTag> element, MergeContext& context);
extern template NumericEffectOperatorView<AuxiliaryTag> merge_p2p(NumericEffectOperatorView<AuxiliaryTag> element, MergeContext& context);

extern template std::pair<GroundNumericEffectView<FluentTag>, bool> merge_p2p(GroundNumericEffectView<FluentTag> element, MergeContext& context);

extern template std::pair<GroundNumericEffectView<AuxiliaryTag>, bool> merge_p2p(GroundNumericEffectView<AuxiliaryTag> element, MergeContext& context);

extern template GroundNumericEffectOperatorView<FluentTag> merge_p2p(GroundNumericEffectOperatorView<FluentTag> element, MergeContext& context);
extern template GroundNumericEffectOperatorView<AuxiliaryTag> merge_p2p(GroundNumericEffectOperatorView<AuxiliaryTag> element, MergeContext& context);
}

#endif

#endif
