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
std::pair<AtomView<LiftedTag, T>, bool> merge_p2p(AtomView<LiftedTag, T> element, MergeContext& context);

template<FactKind T>
std::pair<AtomView<GroundTag, T>, bool> merge_p2p(AtomView<GroundTag, T> element, MergeContext& context);

template<TaskKind T, FactKind F>
std::pair<LiteralView<T, F>, bool> merge_p2p(LiteralView<T, F> element, MergeContext& context);

// Numeric

template<FactKind T>
std::pair<FunctionView<T>, bool> merge_p2p(FunctionView<T> element, MergeContext& context);

template<FactKind T>
std::pair<FunctionTermView<LiftedTag, T>, bool> merge_p2p(FunctionTermView<LiftedTag, T> element, MergeContext& context);

template<FactKind T>
std::pair<FunctionTermView<GroundTag, T>, bool> merge_p2p(FunctionTermView<GroundTag, T> element, MergeContext& context);

template<FactKind T>
std::pair<FunctionTermValueView<GroundTag, T>, bool> merge_p2p(FunctionTermValueView<GroundTag, T> element, MergeContext& context);

template<TaskKind T>
FunctionExpressionView<T> merge_p2p(FunctionExpressionView<T> element, MergeContext& context);

template<TaskKind T>
std::pair<UnaryOperatorView<T>, bool> merge_p2p(UnaryOperatorView<T> element, MergeContext& context);

template<TaskKind T, BinaryOperatorKind O>
std::pair<BinaryOperatorView<T, O>, bool> merge_p2p(BinaryOperatorView<T, O> element, MergeContext& context);

template<TaskKind T>
std::pair<MultiOperatorView<T>, bool> merge_p2p(MultiOperatorView<T> element, MergeContext& context);

template<TaskKind T>
ArithmeticOperatorView<T> merge_p2p(ArithmeticOperatorView<T> element, MergeContext& context);

template<TaskKind T>
BooleanOperatorView<T> merge_p2p(BooleanOperatorView<T> element, MergeContext& context);

template<TaskKind T, FactKind F>
std::pair<NumericEffectView<T, F>, bool> merge_p2p(NumericEffectView<T, F> element, MergeContext& context);

template<TaskKind T, FactKind F>
NumericEffectOperatorView<T, F> merge_p2p(NumericEffectOperatorView<T, F> element, MergeContext& context);

// Composite

std::pair<ConjunctiveConditionView<LiftedTag>, bool> merge_p2p(ConjunctiveConditionView<LiftedTag> element, MergeContext& context);

std::pair<AxiomView<LiftedTag>, bool> merge_p2p(AxiomView<LiftedTag> element, MergeContext& context);

std::pair<MetricView, bool> merge_p2p(MetricView element, MergeContext& context);

// Common

inline std::pair<VariableView, bool> merge_p2p(VariableView element, MergeContext& context)
{
    auto variable = planning::checkout<Variable>(context.builder);

    variable->name = element.get_name();

    return planning::get_or_create(context.destination, *variable);
}

inline std::pair<ObjectView, bool> merge_p2p(ObjectView element, MergeContext& context)
{
    auto object = planning::checkout<Object>(context.builder);

    object->name = element.get_name();

    return planning::get_or_create(context.destination, *object);
}

template<FactKind T>
std::pair<PredicateBindingView<T>, bool> merge_p2p(PredicateBindingView<T> element, MergeContext& context)
{
    auto binding = planning::checkout<RelationBinding<Predicate<T>>>(context.builder);

    binding->relation = element.get_relation().get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(object.get_index());

    return planning::get_or_create(context.destination, *binding);
}

template<FactKind T>
std::pair<FunctionBindingView<T>, bool> merge_p2p(FunctionBindingView<T> element, MergeContext& context)
{
    auto binding = planning::checkout<RelationBinding<Function<T>>>(context.builder);

    binding->relation = element.get_relation().get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(object.get_index());

    return planning::get_or_create(context.destination, *binding);
}

inline std::pair<ActionBindingView, bool> merge_p2p(ActionBindingView element, MergeContext& context)
{
    auto binding = planning::checkout<RelationBinding<Action<LiftedTag>>>(context.builder);

    binding->relation = element.get_relation().get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(object.get_index());

    return planning::get_or_create(context.destination, *binding);
}

inline std::pair<AxiomBindingView, bool> merge_p2p(AxiomBindingView element, MergeContext& context)
{
    auto binding = planning::checkout<RelationBinding<Axiom<LiftedTag>>>(context.builder);

    binding->relation = element.get_relation().get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(object.get_index());

    return planning::get_or_create(context.destination, *binding);
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
    auto predicate = planning::checkout<Predicate<T>>(context.builder);

    predicate->name = element.get_name();
    predicate->arity = element.get_arity();

    return planning::get_or_create(context.destination, *predicate);
}

template<FactKind T>
std::pair<AtomView<LiftedTag, T>, bool> merge_p2p(AtomView<LiftedTag, T> element, MergeContext& context)
{
    auto atom = planning::checkout<Atom<LiftedTag, T>>(context.builder);

    atom->predicate = merge_p2p(element.get_predicate(), context).first.get_index();
    for (const auto term : element.get_terms())
        atom->terms.push_back(merge_p2p(term, context).get_data());

    return planning::get_or_create(context.destination, *atom);
}

template<FactKind T>
std::pair<AtomView<GroundTag, T>, bool> merge_p2p(AtomView<GroundTag, T> element, MergeContext& context)
{
    auto atom = planning::checkout<Atom<GroundTag, T>>(context.builder);

    atom->binding = merge_p2p(element.get_row(), context).first.get_index();

    return planning::get_or_create(context.destination, *atom);
}

template<TaskKind T, FactKind F>
std::pair<LiteralView<T, F>, bool> merge_p2p(LiteralView<T, F> element, MergeContext& context)
{
    auto literal = planning::checkout<Literal<T, F>>(context.builder);

    literal->polarity = element.get_polarity();
    literal->atom = merge_p2p(element.get_atom(), context).first.get_index();

    return planning::get_or_create(context.destination, *literal);
}

// Numeric

template<FactKind T>
std::pair<FunctionView<T>, bool> merge_p2p(FunctionView<T> element, MergeContext& context)
{
    auto function = planning::checkout<Function<T>>(context.builder);

    function->name = element.get_name();
    function->arity = element.get_arity();

    return planning::get_or_create(context.destination, *function);
}

template<FactKind T>
std::pair<FunctionTermView<LiftedTag, T>, bool> merge_p2p(FunctionTermView<LiftedTag, T> element, MergeContext& context)
{
    auto fterm = planning::checkout<FunctionTerm<LiftedTag, T>>(context.builder);

    fterm->function = element.get_function().get_index();
    for (const auto term : element.get_terms())
        fterm->terms.push_back(merge_p2p(term, context).get_data());

    return planning::get_or_create(context.destination, *fterm);
}

template<FactKind T>
std::pair<FunctionTermView<GroundTag, T>, bool> merge_p2p(FunctionTermView<GroundTag, T> element, MergeContext& context)
{
    auto fterm = planning::checkout<FunctionTerm<GroundTag, T>>(context.builder);

    fterm->binding = merge_p2p(element.get_row(), context).first.get_index();

    return planning::get_or_create(context.destination, *fterm);
}

template<FactKind T>
std::pair<FunctionTermValueView<GroundTag, T>, bool> merge_p2p(FunctionTermValueView<GroundTag, T> element, MergeContext& context)
{
    auto fterm_value = planning::checkout<FunctionTermValue<GroundTag, T>>(context.builder);

    fterm_value->fterm = merge_p2p(element.get_fterm(), context).first.get_index();
    fterm_value->value = element.get_value();

    return planning::get_or_create(context.destination, *fterm_value);
}

template<TaskKind T>
inline FunctionExpressionView<T> merge_p2p(FunctionExpressionView<T> element, MergeContext& context)
{
    const auto data = visit(
        [&](auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return ygg::Data<FunctionExpression<T>>(arg);
            else if constexpr (std::is_same_v<Alternative, ArithmeticOperatorView<T>>)
                return ygg::Data<FunctionExpression<T>>(merge_p2p(arg, context).get_data());
            else
                return ygg::Data<FunctionExpression<T>>(merge_p2p(arg, context).first.get_index());
        },
        element.get_variant());
    return ygg::make_view(data, context.destination);
}

template<TaskKind T>
std::pair<UnaryOperatorView<T>, bool> merge_p2p(UnaryOperatorView<T> element, MergeContext& context)
{
    auto unary = planning::checkout<UnaryOperator<T>>(context.builder);

    unary->operator_kind = element.get_operator();
    unary->arg = merge_p2p(element.get_arg(), context).get_data();

    return planning::get_or_create(context.destination, *unary);
}

template<TaskKind T, BinaryOperatorKind O>
std::pair<BinaryOperatorView<T, O>, bool> merge_p2p(BinaryOperatorView<T, O> element, MergeContext& context)
{
    auto binary = planning::checkout<BinaryOperator<T, O>>(context.builder);

    binary->operator_kind = element.get_operator();
    binary->lhs = merge_p2p(element.get_lhs(), context).get_data();
    binary->rhs = merge_p2p(element.get_rhs(), context).get_data();

    return planning::get_or_create(context.destination, *binary);
}

template<TaskKind T>
std::pair<MultiOperatorView<T>, bool> merge_p2p(MultiOperatorView<T> element, MergeContext& context)
{
    auto multi = planning::checkout<MultiOperator<T>>(context.builder);

    multi->operator_kind = element.get_operator();
    for (const auto arg : element.get_args())
        multi->args.push_back(merge_p2p(arg, context).get_data());

    return planning::get_or_create(context.destination, *multi);
}

template<TaskKind T>
ArithmeticOperatorView<T> merge_p2p(ArithmeticOperatorView<T> element, MergeContext& context)
{
    const auto data = visit([&](auto&& arg) { return ygg::Data<ArithmeticOperator<T>>(arg.get_operator(), merge_p2p(arg, context).first.get_index()); },
                            element.get_variant());
    return ygg::make_view(data, context.destination);
}

template<TaskKind T>
BooleanOperatorView<T> merge_p2p(BooleanOperatorView<T> element, MergeContext& context)
{
    const auto data =
        visit([&](auto&& arg) { return ygg::Data<BooleanOperator<T>>(arg.get_operator(), merge_p2p(arg, context).first.get_index()); }, element.get_variant());
    return ygg::make_view(data, context.destination);
}

template<TaskKind T, FactKind F>
std::pair<NumericEffectView<T, F>, bool> merge_p2p(NumericEffectView<T, F> element, MergeContext& context)
{
    auto numeric_effect = planning::checkout<NumericEffect<T, F>>(context.builder);

    numeric_effect->operator_kind = element.get_operator();
    numeric_effect->fterm = merge_p2p(element.get_fterm(), context).first.get_index();
    numeric_effect->fexpr = merge_p2p(element.get_fexpr(), context).get_data();

    return planning::get_or_create(context.destination, *numeric_effect);
}

template<TaskKind T, FactKind F>
NumericEffectOperatorView<T, F> merge_p2p(NumericEffectOperatorView<T, F> element, MergeContext& context)
{
    const auto data = visit([&](auto&& arg) { return ygg::Data<NumericEffectOperator<T, F>>(arg.get_operator(), merge_p2p(arg, context).first.get_index()); },
                            element.get_variant());
    return ygg::make_view(data, context.destination);
}

// Composite

inline std::pair<ConjunctiveConditionView<LiftedTag>, bool> merge_p2p(ConjunctiveConditionView<LiftedTag> element, MergeContext& context)
{
    auto conj_cond = planning::checkout<ConjunctiveCondition<LiftedTag>>(context.builder);

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

    return planning::get_or_create(context.destination, *conj_cond);
}

inline std::pair<AxiomView<LiftedTag>, bool> merge_p2p(AxiomView<LiftedTag> element, MergeContext& context)
{
    auto axiom = planning::checkout<Axiom<LiftedTag>>(context.builder);

    for (const auto variable : element.get_variables())
        axiom->variables.push_back(merge_p2p(variable, context).first.get_index());
    axiom->body = merge_p2p(element.get_body(), context).first.get_index();
    axiom->head = merge_p2p(element.get_head(), context).first.get_index();

    return planning::get_or_create(context.destination, *axiom);
}

inline std::pair<MetricView, bool> merge_p2p(MetricView element, MergeContext& context)
{
    auto metric = planning::checkout<Metric>(context.builder);

    metric->optimization_direction = element.get_optimization_direction();
    metric->fexpr = merge_p2p(element.get_fexpr(), context).get_data();

    return planning::get_or_create(context.destination, *metric);
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

extern template std::pair<AtomView<LiftedTag, StaticTag>, bool> merge_p2p(AtomView<LiftedTag, StaticTag> element, MergeContext& context);
extern template std::pair<AtomView<LiftedTag, FluentTag>, bool> merge_p2p(AtomView<LiftedTag, FluentTag> element, MergeContext& context);
extern template std::pair<AtomView<LiftedTag, DerivedTag>, bool> merge_p2p(AtomView<LiftedTag, DerivedTag> element, MergeContext& context);

extern template std::pair<AtomView<GroundTag, StaticTag>, bool> merge_p2p(AtomView<GroundTag, StaticTag> element, MergeContext& context);
extern template std::pair<AtomView<GroundTag, FluentTag>, bool> merge_p2p(AtomView<GroundTag, FluentTag> element, MergeContext& context);
extern template std::pair<AtomView<GroundTag, DerivedTag>, bool> merge_p2p(AtomView<GroundTag, DerivedTag> element, MergeContext& context);

extern template std::pair<LiteralView<LiftedTag, StaticTag>, bool> merge_p2p(LiteralView<LiftedTag, StaticTag> element, MergeContext& context);
extern template std::pair<LiteralView<LiftedTag, FluentTag>, bool> merge_p2p(LiteralView<LiftedTag, FluentTag> element, MergeContext& context);
extern template std::pair<LiteralView<LiftedTag, DerivedTag>, bool> merge_p2p(LiteralView<LiftedTag, DerivedTag> element, MergeContext& context);

extern template std::pair<LiteralView<GroundTag, StaticTag>, bool> merge_p2p(LiteralView<GroundTag, StaticTag> element, MergeContext& context);
extern template std::pair<LiteralView<GroundTag, FluentTag>, bool> merge_p2p(LiteralView<GroundTag, FluentTag> element, MergeContext& context);
extern template std::pair<LiteralView<GroundTag, DerivedTag>, bool> merge_p2p(LiteralView<GroundTag, DerivedTag> element, MergeContext& context);

extern template std::pair<FunctionView<StaticTag>, bool> merge_p2p(FunctionView<StaticTag> element, MergeContext& context);
extern template std::pair<FunctionView<FluentTag>, bool> merge_p2p(FunctionView<FluentTag> element, MergeContext& context);
extern template std::pair<FunctionView<AuxiliaryTag>, bool> merge_p2p(FunctionView<AuxiliaryTag> element, MergeContext& context);

extern template std::pair<FunctionTermView<LiftedTag, StaticTag>, bool> merge_p2p(FunctionTermView<LiftedTag, StaticTag> element, MergeContext& context);
extern template std::pair<FunctionTermView<LiftedTag, FluentTag>, bool> merge_p2p(FunctionTermView<LiftedTag, FluentTag> element, MergeContext& context);
extern template std::pair<FunctionTermView<LiftedTag, AuxiliaryTag>, bool> merge_p2p(FunctionTermView<LiftedTag, AuxiliaryTag> element, MergeContext& context);

extern template std::pair<FunctionTermView<GroundTag, StaticTag>, bool> merge_p2p(FunctionTermView<GroundTag, StaticTag> element, MergeContext& context);
extern template std::pair<FunctionTermView<GroundTag, FluentTag>, bool> merge_p2p(FunctionTermView<GroundTag, FluentTag> element, MergeContext& context);
extern template std::pair<FunctionTermView<GroundTag, AuxiliaryTag>, bool> merge_p2p(FunctionTermView<GroundTag, AuxiliaryTag> element, MergeContext& context);

extern template std::pair<FunctionTermValueView<GroundTag, StaticTag>, bool> merge_p2p(FunctionTermValueView<GroundTag, StaticTag> element,
                                                                                       MergeContext& context);
extern template std::pair<FunctionTermValueView<GroundTag, FluentTag>, bool> merge_p2p(FunctionTermValueView<GroundTag, FluentTag> element,
                                                                                       MergeContext& context);
extern template std::pair<FunctionTermValueView<GroundTag, AuxiliaryTag>, bool> merge_p2p(FunctionTermValueView<GroundTag, AuxiliaryTag> element,
                                                                                          MergeContext& context);

extern template std::pair<UnaryOperatorView<LiftedTag>, bool> merge_p2p(UnaryOperatorView<LiftedTag> element, MergeContext& context);
extern template std::pair<UnaryOperatorView<GroundTag>, bool> merge_p2p(UnaryOperatorView<GroundTag> element, MergeContext& context);

extern template std::pair<BinaryOperatorView<LiftedTag, BooleanOperatorKind>, bool> merge_p2p(BinaryOperatorView<LiftedTag, BooleanOperatorKind> element,
                                                                                              MergeContext& context);
extern template std::pair<BinaryOperatorView<LiftedTag, ArithmeticOperatorKind>, bool> merge_p2p(BinaryOperatorView<LiftedTag, ArithmeticOperatorKind> element,
                                                                                                 MergeContext& context);
extern template std::pair<BinaryOperatorView<GroundTag, BooleanOperatorKind>, bool> merge_p2p(BinaryOperatorView<GroundTag, BooleanOperatorKind> element,
                                                                                              MergeContext& context);
extern template std::pair<BinaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool> merge_p2p(BinaryOperatorView<GroundTag, ArithmeticOperatorKind> element,
                                                                                                 MergeContext& context);

extern template std::pair<MultiOperatorView<LiftedTag>, bool> merge_p2p(MultiOperatorView<LiftedTag> element, MergeContext& context);
extern template std::pair<MultiOperatorView<GroundTag>, bool> merge_p2p(MultiOperatorView<GroundTag> element, MergeContext& context);

extern template ArithmeticOperatorView<LiftedTag> merge_p2p(ArithmeticOperatorView<LiftedTag> element, MergeContext& context);
extern template ArithmeticOperatorView<GroundTag> merge_p2p(ArithmeticOperatorView<GroundTag> element, MergeContext& context);

extern template BooleanOperatorView<LiftedTag> merge_p2p(BooleanOperatorView<LiftedTag> element, MergeContext& context);
extern template BooleanOperatorView<GroundTag> merge_p2p(BooleanOperatorView<GroundTag> element, MergeContext& context);

extern template std::pair<NumericEffectView<LiftedTag, FluentTag>, bool> merge_p2p(NumericEffectView<LiftedTag, FluentTag> element, MergeContext& context);

extern template std::pair<NumericEffectView<LiftedTag, AuxiliaryTag>, bool> merge_p2p(NumericEffectView<LiftedTag, AuxiliaryTag> element,
                                                                                      MergeContext& context);

extern template NumericEffectOperatorView<LiftedTag, FluentTag> merge_p2p(NumericEffectOperatorView<LiftedTag, FluentTag> element, MergeContext& context);
extern template NumericEffectOperatorView<LiftedTag, AuxiliaryTag> merge_p2p(NumericEffectOperatorView<LiftedTag, AuxiliaryTag> element, MergeContext& context);

extern template std::pair<NumericEffectView<GroundTag, FluentTag>, bool> merge_p2p(NumericEffectView<GroundTag, FluentTag> element, MergeContext& context);

extern template std::pair<NumericEffectView<GroundTag, AuxiliaryTag>, bool> merge_p2p(NumericEffectView<GroundTag, AuxiliaryTag> element,
                                                                                      MergeContext& context);

extern template NumericEffectOperatorView<GroundTag, FluentTag> merge_p2p(NumericEffectOperatorView<GroundTag, FluentTag> element, MergeContext& context);
extern template NumericEffectOperatorView<GroundTag, AuxiliaryTag> merge_p2p(NumericEffectOperatorView<GroundTag, AuxiliaryTag> element, MergeContext& context);
}

#endif

#endif
