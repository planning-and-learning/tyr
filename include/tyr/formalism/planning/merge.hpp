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

#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>
#include <yggdrasil/containers/tuple.hpp>
#include "tyr/formalism/planning/builder.hpp"
#include "tyr/formalism/planning/canonicalization.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/merge_decl.hpp"
#include "tyr/formalism/planning/repository.hpp"

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

template<OpKind O, typename T>
std::pair<UnaryOperatorView<O, T>, bool> merge_p2p(UnaryOperatorView<O, T> element, MergeContext& context);

template<OpKind O, typename T>
std::pair<BinaryOperatorView<O, T>, bool> merge_p2p(BinaryOperatorView<O, T> element, MergeContext& context);

template<OpKind O, typename T>
std::pair<MultiOperatorView<O, T>, bool> merge_p2p(MultiOperatorView<O, T> element, MergeContext& context);

template<typename T>
ArithmeticOperatorView<T> merge_p2p(ArithmeticOperatorView<T> element, MergeContext& context);

template<typename T>
BooleanOperatorView<T> merge_p2p(BooleanOperatorView<T> element, MergeContext& context);

template<NumericEffectOpKind O, FactKind T>
std::pair<NumericEffectView<O, T>, bool> merge_p2p(NumericEffectView<O, T> element, MergeContext& context);

template<FactKind T>
NumericEffectOperatorView<T> merge_p2p(NumericEffectOperatorView<T> element, MergeContext& context);

template<NumericEffectOpKind O, FactKind T>
std::pair<GroundNumericEffectView<O, T>, bool> merge_p2p(GroundNumericEffectView<O, T> element, MergeContext& context);

template<FactKind T>
GroundNumericEffectOperatorView<T> merge_p2p(GroundNumericEffectOperatorView<T> element, MergeContext& context);

// Composite

std::pair<ConjunctiveConditionView, bool> merge_p2p(ConjunctiveConditionView element, MergeContext& context);

std::pair<AxiomView, bool> merge_p2p(AxiomView element, MergeContext& context);

std::pair<MetricView, bool> merge_p2p(MetricView element, MergeContext& context);

// Common

inline std::pair<VariableView, bool> merge_p2p(VariableView element, MergeContext& context)
{
    auto variable_ptr = context.builder.template get_builder<Variable>();
    auto& variable = *variable_ptr;
    variable.clear();

    variable.name = element.get_name();

    canonicalize(variable);
    return context.destination.get_or_create(variable);
}

inline std::pair<ObjectView, bool> merge_p2p(ObjectView element, MergeContext& context)
{
    auto object_ptr = context.builder.template get_builder<Object>();
    auto& object = *object_ptr;
    object.clear();

    object.name = element.get_name();

    canonicalize(object);
    return context.destination.get_or_create(object);
}

template<FactKind T>
std::pair<PredicateBindingView<T>, bool> merge_p2p(PredicateBindingView<T> element, MergeContext& context)
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
std::pair<FunctionBindingView<T>, bool> merge_p2p(FunctionBindingView<T> element, MergeContext& context)
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

inline std::pair<ActionBindingView, bool> merge_p2p(ActionBindingView element, MergeContext& context)
{
    auto binding_ptr = context.builder.template get_builder<RelationBinding<Action>>();
    auto& binding = *binding_ptr;
    binding.clear();

    binding.relation = element.get_relation().get_index();
    for (const auto object : element.get_objects())
        binding.objects.push_back(object.get_index());

    canonicalize(binding);
    return context.destination.get_or_create(binding);
}

inline std::pair<AxiomBindingView, bool> merge_p2p(AxiomBindingView element, MergeContext& context)
{
    auto binding_ptr = context.builder.template get_builder<RelationBinding<Axiom>>();
    auto& binding = *binding_ptr;
    binding.clear();

    binding.relation = element.get_relation().get_index();
    for (const auto object : element.get_objects())
        binding.objects.push_back(object.get_index());

    canonicalize(binding);
    return context.destination.get_or_create(binding);
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
    auto predicate_ptr = context.builder.template get_builder<Predicate<T>>();
    auto& predicate = *predicate_ptr;
    predicate.clear();

    predicate.name = element.get_name();
    predicate.arity = element.get_arity();

    canonicalize(predicate);
    return context.destination.get_or_create(predicate);
}

template<FactKind T>
std::pair<AtomView<T>, bool> merge_p2p(AtomView<T> element, MergeContext& context)
{
    auto atom_ptr = context.builder.template get_builder<Atom<T>>();
    auto& atom = *atom_ptr;
    atom.clear();

    atom.predicate = merge_p2p(element.get_predicate(), context).first.get_index();
    for (const auto term : element.get_terms())
        atom.terms.push_back(merge_p2p(term, context).get_data());

    canonicalize(atom);
    return context.destination.get_or_create(atom);
}

template<FactKind T>
std::pair<GroundAtomView<T>, bool> merge_p2p(GroundAtomView<T> element, MergeContext& context)
{
    auto atom_ptr = context.builder.template get_builder<GroundAtom<T>>();
    auto& atom = *atom_ptr;
    atom.clear();

    atom.binding = merge_p2p(element.get_row(), context).first.get_index();

    canonicalize(atom);
    return context.destination.get_or_create(atom);
}

template<FactKind T>
std::pair<LiteralView<T>, bool> merge_p2p(LiteralView<T> element, MergeContext& context)
{
    auto literal_ptr = context.builder.template get_builder<Literal<T>>();
    auto& literal = *literal_ptr;
    literal.clear();

    literal.polarity = element.get_polarity();
    literal.atom = merge_p2p(element.get_atom(), context).first.get_index();

    canonicalize(literal);
    return context.destination.get_or_create(literal);
}

template<FactKind T>
std::pair<GroundLiteralView<T>, bool> merge_p2p(GroundLiteralView<T> element, MergeContext& context)
{
    auto literal_ptr = context.builder.template get_builder<GroundLiteral<T>>();
    auto& literal = *literal_ptr;
    literal.clear();

    literal.polarity = element.get_polarity();
    literal.atom = merge_p2p(element.get_atom(), context).first.get_index();

    canonicalize(literal);
    return context.destination.get_or_create(literal);
}

// Numeric

template<FactKind T>
std::pair<FunctionView<T>, bool> merge_p2p(FunctionView<T> element, MergeContext& context)
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
std::pair<FunctionTermView<T>, bool> merge_p2p(FunctionTermView<T> element, MergeContext& context)
{
    auto fterm_ptr = context.builder.template get_builder<FunctionTerm<T>>();
    auto& fterm = *fterm_ptr;
    fterm.clear();

    fterm.function = element.get_function().get_index();
    for (const auto term : element.get_terms())
        fterm.terms.push_back(merge_p2p(term, context).get_data());

    canonicalize(fterm);
    return context.destination.get_or_create(fterm);
}

template<FactKind T>
std::pair<GroundFunctionTermView<T>, bool> merge_p2p(GroundFunctionTermView<T> element, MergeContext& context)
{
    auto fterm_ptr = context.builder.template get_builder<GroundFunctionTerm<T>>();
    auto& fterm = *fterm_ptr;
    fterm.clear();

    fterm.binding = merge_p2p(element.get_row(), context).first.get_index();

    canonicalize(fterm);
    return context.destination.get_or_create(fterm);
}

template<FactKind T>
std::pair<GroundFunctionTermValueView<T>, bool> merge_p2p(GroundFunctionTermValueView<T> element, MergeContext& context)
{
    auto fterm_value_ptr = context.builder.template get_builder<GroundFunctionTermValue<T>>();
    auto& fterm_value = *fterm_value_ptr;
    fterm_value.clear();

    fterm_value.fterm = merge_p2p(element.get_fterm(), context).first.get_index();
    fterm_value.value = element.get_value();

    canonicalize(fterm_value);
    return context.destination.get_or_create(fterm_value);
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

template<OpKind O, typename T>
std::pair<UnaryOperatorView<O, T>, bool> merge_p2p(UnaryOperatorView<O, T> element, MergeContext& context)
{
    auto unary_ptr = context.builder.template get_builder<UnaryOperator<O, T>>();
    auto& unary = *unary_ptr;
    unary.clear();

    unary.arg = merge_p2p(element.get_arg(), context).get_data();

    canonicalize(unary);
    return context.destination.get_or_create(unary);
}

template<OpKind O, typename T>
std::pair<BinaryOperatorView<O, T>, bool> merge_p2p(BinaryOperatorView<O, T> element, MergeContext& context)
{
    auto binary_ptr = context.builder.template get_builder<BinaryOperator<O, T>>();
    auto& binary = *binary_ptr;
    binary.clear();

    binary.lhs = merge_p2p(element.get_lhs(), context).get_data();
    binary.rhs = merge_p2p(element.get_rhs(), context).get_data();

    canonicalize(binary);
    return context.destination.get_or_create(binary);
}

template<OpKind O, typename T>
std::pair<MultiOperatorView<O, T>, bool> merge_p2p(MultiOperatorView<O, T> element, MergeContext& context)
{
    auto multi_ptr = context.builder.template get_builder<MultiOperator<O, T>>();
    auto& multi = *multi_ptr;
    multi.clear();

    for (const auto arg : element.get_args())
        multi.args.push_back(merge_p2p(arg, context).get_data());

    canonicalize(multi);
    return context.destination.get_or_create(multi);
}

template<typename T>
ArithmeticOperatorView<T> merge_p2p(ArithmeticOperatorView<T> element, MergeContext& context)
{
    const auto data =
        visit([&](auto&& arg) { return ygg::Data<ArithmeticOperator<T>>(merge_p2p(arg, context).first.get_index()); }, element.get_variant());
    return ygg::make_view(data, context.destination);
}

template<typename T>
BooleanOperatorView<T> merge_p2p(BooleanOperatorView<T> element, MergeContext& context)
{
    const auto data = visit([&](auto&& arg) { return ygg::Data<BooleanOperator<T>>(merge_p2p(arg, context).first.get_index()); }, element.get_variant());
    return ygg::make_view(data, context.destination);
}

template<NumericEffectOpKind O, FactKind T>
std::pair<NumericEffectView<O, T>, bool> merge_p2p(NumericEffectView<O, T> element, MergeContext& context)
{
    auto numeric_effect_ptr = context.builder.template get_builder<NumericEffect<O, T>>();
    auto& numeric_effect = *numeric_effect_ptr;
    numeric_effect.clear();

    numeric_effect.fterm = merge_p2p(element.get_fterm(), context).first.get_index();
    numeric_effect.fexpr = merge_p2p(element.get_fexpr(), context).get_data();

    canonicalize(numeric_effect);
    return context.destination.get_or_create(numeric_effect);
}

template<FactKind T>
NumericEffectOperatorView<T> merge_p2p(NumericEffectOperatorView<T> element, MergeContext& context)
{
    const auto data =
        visit([&](auto&& arg) { return ygg::Data<NumericEffectOperator<T>>(merge_p2p(arg, context).first.get_index()); }, element.get_variant());
    return ygg::make_view(data, context.destination);
}

template<NumericEffectOpKind O, FactKind T>
std::pair<GroundNumericEffectView<O, T>, bool> merge_p2p(GroundNumericEffectView<O, T> element, MergeContext& context)
{
    auto numeric_effect_ptr = context.builder.template get_builder<GroundNumericEffect<O, T>>();
    auto& numeric_effect = *numeric_effect_ptr;
    numeric_effect.clear();

    numeric_effect.fterm = merge_p2p(element.get_fterm(), context).first.get_index();
    numeric_effect.fexpr = merge_p2p(element.get_fexpr(), context).get_data();

    canonicalize(numeric_effect);
    return context.destination.get_or_create(numeric_effect);
}

template<FactKind T>
GroundNumericEffectOperatorView<T> merge_p2p(GroundNumericEffectOperatorView<T> element, MergeContext& context)
{
    const auto data =
        visit([&](auto&& arg) { return ygg::Data<GroundNumericEffectOperator<T>>(merge_p2p(arg, context).first.get_index()); }, element.get_variant());
    return ygg::make_view(data, context.destination);
}

// Composite

inline std::pair<ConjunctiveConditionView, bool> merge_p2p(ConjunctiveConditionView element, MergeContext& context)
{
    auto conj_cond_ptr = context.builder.template get_builder<ConjunctiveCondition>();
    auto& conj_cond = *conj_cond_ptr;
    conj_cond.clear();

    for (const auto variable : element.get_variables())
        conj_cond.variables.push_back(merge_p2p(variable, context).first.get_index());
    for (const auto literal : element.template get_literals<StaticTag>())
        conj_cond.static_literals.push_back(merge_p2p(literal, context).first.get_index());
    for (const auto literal : element.template get_literals<FluentTag>())
        conj_cond.fluent_literals.push_back(merge_p2p(literal, context).first.get_index());
    for (const auto literal : element.template get_literals<DerivedTag>())
        conj_cond.derived_literals.push_back(merge_p2p(literal, context).first.get_index());
    for (const auto numeric_constraint : element.get_numeric_constraints())
        conj_cond.numeric_constraints.push_back(merge_p2p(numeric_constraint, context).get_data());

    canonicalize(conj_cond);
    return context.destination.get_or_create(conj_cond);
}

inline std::pair<AxiomView, bool> merge_p2p(AxiomView element, MergeContext& context)
{
    auto axiom_ptr = context.builder.template get_builder<Axiom>();
    auto& axiom = *axiom_ptr;
    axiom.clear();

    for (const auto variable : element.get_variables())
        axiom.variables.push_back(merge_p2p(variable, context).first.get_index());
    axiom.body = merge_p2p(element.get_body(), context).first.get_index();
    axiom.head = merge_p2p(element.get_head(), context).first.get_index();

    canonicalize(axiom);
    return context.destination.get_or_create(axiom);
}

inline std::pair<MetricView, bool> merge_p2p(MetricView element, MergeContext& context)
{
    auto metric_ptr = context.builder.template get_builder<Metric>();
    auto& metric = *metric_ptr;
    metric.clear();

    metric.objective = element.get_objective();
    metric.fexpr = merge_p2p(element.get_fexpr(), context).get_data();

    canonicalize(metric);
    return context.destination.get_or_create(metric);
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

extern template std::pair<UnaryOperatorView<Sub, ygg::Data<FunctionExpression>>, bool> merge_p2p(UnaryOperatorView<Sub, ygg::Data<FunctionExpression>> element,
                                                                                            MergeContext& context);
extern template std::pair<UnaryOperatorView<Sub, ygg::Data<GroundFunctionExpression>>, bool>
merge_p2p(UnaryOperatorView<Sub, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);

extern template std::pair<BinaryOperatorView<Eq, ygg::Data<FunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Eq, ygg::Data<FunctionExpression>> element,
                                                                                            MergeContext& context);
extern template std::pair<BinaryOperatorView<Ne, ygg::Data<FunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Ne, ygg::Data<FunctionExpression>> element,
                                                                                            MergeContext& context);
extern template std::pair<BinaryOperatorView<Ge, ygg::Data<FunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Ge, ygg::Data<FunctionExpression>> element,
                                                                                            MergeContext& context);
extern template std::pair<BinaryOperatorView<Gt, ygg::Data<FunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Gt, ygg::Data<FunctionExpression>> element,
                                                                                            MergeContext& context);
extern template std::pair<BinaryOperatorView<Le, ygg::Data<FunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Le, ygg::Data<FunctionExpression>> element,
                                                                                            MergeContext& context);
extern template std::pair<BinaryOperatorView<Lt, ygg::Data<FunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Lt, ygg::Data<FunctionExpression>> element,
                                                                                            MergeContext& context);
extern template std::pair<BinaryOperatorView<Add, ygg::Data<FunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Add, ygg::Data<FunctionExpression>> element,
                                                                                             MergeContext& context);
extern template std::pair<BinaryOperatorView<Sub, ygg::Data<FunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Sub, ygg::Data<FunctionExpression>> element,
                                                                                             MergeContext& context);
extern template std::pair<BinaryOperatorView<Mul, ygg::Data<FunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Mul, ygg::Data<FunctionExpression>> element,
                                                                                             MergeContext& context);
extern template std::pair<BinaryOperatorView<Div, ygg::Data<FunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Div, ygg::Data<FunctionExpression>> element,
                                                                                             MergeContext& context);
extern template std::pair<BinaryOperatorView<Eq, ygg::Data<GroundFunctionExpression>>, bool>
merge_p2p(BinaryOperatorView<Eq, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);
extern template std::pair<BinaryOperatorView<Ne, ygg::Data<GroundFunctionExpression>>, bool>
merge_p2p(BinaryOperatorView<Ne, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);
extern template std::pair<BinaryOperatorView<Ge, ygg::Data<GroundFunctionExpression>>, bool>
merge_p2p(BinaryOperatorView<Ge, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);
extern template std::pair<BinaryOperatorView<Gt, ygg::Data<GroundFunctionExpression>>, bool>
merge_p2p(BinaryOperatorView<Gt, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);
extern template std::pair<BinaryOperatorView<Le, ygg::Data<GroundFunctionExpression>>, bool>
merge_p2p(BinaryOperatorView<Le, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);
extern template std::pair<BinaryOperatorView<Lt, ygg::Data<GroundFunctionExpression>>, bool>
merge_p2p(BinaryOperatorView<Lt, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);
extern template std::pair<BinaryOperatorView<Add, ygg::Data<GroundFunctionExpression>>, bool>
merge_p2p(BinaryOperatorView<Add, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);
extern template std::pair<BinaryOperatorView<Sub, ygg::Data<GroundFunctionExpression>>, bool>
merge_p2p(BinaryOperatorView<Sub, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);
extern template std::pair<BinaryOperatorView<Mul, ygg::Data<GroundFunctionExpression>>, bool>
merge_p2p(BinaryOperatorView<Mul, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);
extern template std::pair<BinaryOperatorView<Div, ygg::Data<GroundFunctionExpression>>, bool>
merge_p2p(BinaryOperatorView<Div, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);

extern template std::pair<MultiOperatorView<Add, ygg::Data<FunctionExpression>>, bool> merge_p2p(MultiOperatorView<Add, ygg::Data<FunctionExpression>> element,
                                                                                            MergeContext& context);
extern template std::pair<MultiOperatorView<Mul, ygg::Data<FunctionExpression>>, bool> merge_p2p(MultiOperatorView<Mul, ygg::Data<FunctionExpression>> element,
                                                                                            MergeContext& context);
extern template std::pair<MultiOperatorView<Add, ygg::Data<GroundFunctionExpression>>, bool>
merge_p2p(MultiOperatorView<Add, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);
extern template std::pair<MultiOperatorView<Mul, ygg::Data<GroundFunctionExpression>>, bool>
merge_p2p(MultiOperatorView<Mul, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);

extern template ArithmeticOperatorView<ygg::Data<FunctionExpression>> merge_p2p(ArithmeticOperatorView<ygg::Data<FunctionExpression>> element, MergeContext& context);
extern template ArithmeticOperatorView<ygg::Data<GroundFunctionExpression>> merge_p2p(ArithmeticOperatorView<ygg::Data<GroundFunctionExpression>> element,
                                                                                       MergeContext& context);

extern template BooleanOperatorView<ygg::Data<FunctionExpression>> merge_p2p(BooleanOperatorView<ygg::Data<FunctionExpression>> element, MergeContext& context);
extern template BooleanOperatorView<ygg::Data<GroundFunctionExpression>> merge_p2p(BooleanOperatorView<ygg::Data<GroundFunctionExpression>> element,
                                                                                    MergeContext& context);

extern template std::pair<NumericEffectView<Assign, FluentTag>, bool> merge_p2p(NumericEffectView<Assign, FluentTag> element, MergeContext& context);
extern template std::pair<NumericEffectView<Increase, FluentTag>, bool> merge_p2p(NumericEffectView<Increase, FluentTag> element, MergeContext& context);
extern template std::pair<NumericEffectView<Decrease, FluentTag>, bool> merge_p2p(NumericEffectView<Decrease, FluentTag> element, MergeContext& context);
extern template std::pair<NumericEffectView<ScaleUp, FluentTag>, bool> merge_p2p(NumericEffectView<ScaleUp, FluentTag> element, MergeContext& context);
extern template std::pair<NumericEffectView<ScaleDown, FluentTag>, bool> merge_p2p(NumericEffectView<ScaleDown, FluentTag> element, MergeContext& context);

extern template std::pair<NumericEffectView<Increase, AuxiliaryTag>, bool> merge_p2p(NumericEffectView<Increase, AuxiliaryTag> element, MergeContext& context);

extern template NumericEffectOperatorView<FluentTag> merge_p2p(NumericEffectOperatorView<FluentTag> element, MergeContext& context);
extern template NumericEffectOperatorView<AuxiliaryTag> merge_p2p(NumericEffectOperatorView<AuxiliaryTag> element, MergeContext& context);

extern template std::pair<GroundNumericEffectView<Assign, FluentTag>, bool> merge_p2p(GroundNumericEffectView<Assign, FluentTag> element,
                                                                                      MergeContext& context);
extern template std::pair<GroundNumericEffectView<Increase, FluentTag>, bool> merge_p2p(GroundNumericEffectView<Increase, FluentTag> element,
                                                                                        MergeContext& context);
extern template std::pair<GroundNumericEffectView<Decrease, FluentTag>, bool> merge_p2p(GroundNumericEffectView<Decrease, FluentTag> element,
                                                                                        MergeContext& context);
extern template std::pair<GroundNumericEffectView<ScaleUp, FluentTag>, bool> merge_p2p(GroundNumericEffectView<ScaleUp, FluentTag> element,
                                                                                       MergeContext& context);
extern template std::pair<GroundNumericEffectView<ScaleDown, FluentTag>, bool> merge_p2p(GroundNumericEffectView<ScaleDown, FluentTag> element,
                                                                                         MergeContext& context);

extern template std::pair<GroundNumericEffectView<Increase, AuxiliaryTag>, bool> merge_p2p(GroundNumericEffectView<Increase, AuxiliaryTag> element,
                                                                                           MergeContext& context);

extern template GroundNumericEffectOperatorView<FluentTag> merge_p2p(GroundNumericEffectOperatorView<FluentTag> element, MergeContext& context);
extern template GroundNumericEffectOperatorView<AuxiliaryTag> merge_p2p(GroundNumericEffectOperatorView<AuxiliaryTag> element, MergeContext& context);
}

#endif

#endif
