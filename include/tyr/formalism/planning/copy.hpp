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

#ifndef TYR_FORMALISM_PLANNING_COPY_HPP_
#define TYR_FORMALISM_PLANNING_COPY_HPP_

#include "tyr/formalism/planning/canonicalization.hpp"
#include "tyr/formalism/planning/copy_decl.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <yggdrasil/containers/tuple.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::formalism::planning
{

// Common

std::pair<VariableView, bool> copy(VariableView element, CopyContext& context);

std::pair<ObjectView, bool> copy(ObjectView element, CopyContext& context);

template<FactKind T>
std::pair<PredicateBindingView<T>, bool> copy(PredicateBindingView<T> element, CopyContext& context);

template<FactKind T>
std::pair<FunctionBindingView<T>, bool> copy(FunctionBindingView<T> element, CopyContext& context);

std::pair<ActionBindingView, bool> copy(ActionBindingView element, CopyContext& context);

std::pair<AxiomBindingView, bool> copy(AxiomBindingView element, CopyContext& context);

TermView copy(TermView element, CopyContext& context);

// Propositional

template<FactKind T>
std::pair<PredicateView<T>, bool> copy(PredicateView<T> element, CopyContext& context);

template<FactKind T>
std::pair<AtomView<LiftedTag, T>, bool> copy(AtomView<LiftedTag, T> element, CopyContext& context);

template<FactKind T>
std::pair<AtomView<GroundTag, T>, bool> copy(AtomView<GroundTag, T> element, CopyContext& context);

template<TaskKind T, FactKind F>
std::pair<LiteralView<T, F>, bool> copy(LiteralView<T, F> element, CopyContext& context);

// Numeric

template<FactKind T>
std::pair<FunctionView<T>, bool> copy(FunctionView<T> element, CopyContext& context);

template<FactKind T>
std::pair<FunctionTermView<LiftedTag, T>, bool> copy(FunctionTermView<LiftedTag, T> element, CopyContext& context);

template<FactKind T>
std::pair<FunctionTermView<GroundTag, T>, bool> copy(FunctionTermView<GroundTag, T> element, CopyContext& context);

template<FactKind T>
std::pair<FunctionTermValueView<GroundTag, T>, bool> copy(FunctionTermValueView<GroundTag, T> element, CopyContext& context);

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

// Composite

std::pair<ConjunctiveConditionView<LiftedTag>, bool> copy(ConjunctiveConditionView<LiftedTag> element, CopyContext& context);

std::pair<ConjunctiveEffectView<LiftedTag>, bool> copy(ConjunctiveEffectView<LiftedTag> element, CopyContext& context);

std::pair<ConditionalEffectView<LiftedTag>, bool> copy(ConditionalEffectView<LiftedTag> element, CopyContext& context);

std::pair<ActionView<LiftedTag>, bool> copy(ActionView<LiftedTag> element, CopyContext& context);

std::pair<AxiomView<LiftedTag>, bool> copy(AxiomView<LiftedTag> element, CopyContext& context);

std::pair<MetricView, bool> copy(MetricView element, CopyContext& context);

// Common

inline std::pair<VariableView, bool> copy(VariableView element, CopyContext& context)
{
    auto variable = planning::checkout<Variable>(context.builder);

    variable->name = element.get_name();

    return planning::insert(context.destination, *variable);
}

inline std::pair<ObjectView, bool> copy(ObjectView element, CopyContext& context)
{
    auto object = planning::checkout<Object>(context.builder);

    object->name = element.get_name();

    return planning::insert(context.destination, *object);
}

template<FactKind T>
std::pair<PredicateBindingView<T>, bool> copy(PredicateBindingView<T> element, CopyContext& context)
{
    auto binding = planning::checkout<RelationBinding<Predicate<T>>>(context.builder);

    binding->relation = copy(element.get_relation(), context).first.get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(copy(object, context).first.get_index());

    return planning::insert(context.destination, *binding);
}

template<FactKind T>
std::pair<FunctionBindingView<T>, bool> copy(FunctionBindingView<T> element, CopyContext& context)
{
    auto binding = planning::checkout<RelationBinding<Function<T>>>(context.builder);

    binding->relation = copy(element.get_relation(), context).first.get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(copy(object, context).first.get_index());

    return planning::insert(context.destination, *binding);
}

inline std::pair<ActionBindingView, bool> copy(ActionBindingView element, CopyContext& context)
{
    auto binding = planning::checkout<RelationBinding<Action<LiftedTag>>>(context.builder);

    binding->relation = copy(element.get_relation(), context).first.get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(copy(object, context).first.get_index());

    return planning::insert(context.destination, *binding);
}

inline std::pair<AxiomBindingView, bool> copy(AxiomBindingView element, CopyContext& context)
{
    auto binding = planning::checkout<RelationBinding<Axiom<LiftedTag>>>(context.builder);

    binding->relation = copy(element.get_relation(), context).first.get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(copy(object, context).first.get_index());

    return planning::insert(context.destination, *binding);
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

template<FactKind T>
std::pair<PredicateView<T>, bool> copy(PredicateView<T> element, CopyContext& context)
{
    auto predicate = planning::checkout<Predicate<T>>(context.builder);

    predicate->name = element.get_name();
    predicate->arity = element.get_arity();

    return planning::insert(context.destination, *predicate);
}

template<FactKind T>
std::pair<AtomView<LiftedTag, T>, bool> copy(AtomView<LiftedTag, T> element, CopyContext& context)
{
    auto atom = planning::checkout<Atom<LiftedTag, T>>(context.builder);

    atom->predicate = copy(element.get_predicate(), context).first.get_index();
    for (const auto term : element.get_terms())
        atom->terms.push_back(copy(term, context).get_data());

    return planning::insert(context.destination, *atom);
}

template<FactKind T>
std::pair<AtomView<GroundTag, T>, bool> copy(AtomView<GroundTag, T> element, CopyContext& context)
{
    auto atom = planning::checkout<Atom<GroundTag, T>>(context.builder);

    atom->binding = copy(element.get_row(), context).first.get_index();

    return planning::insert(context.destination, *atom);
}

template<TaskKind T, FactKind F>
std::pair<LiteralView<T, F>, bool> copy(LiteralView<T, F> element, CopyContext& context)
{
    auto literal = planning::checkout<Literal<T, F>>(context.builder);

    literal->polarity = element.get_polarity();
    literal->atom = copy(element.get_atom(), context).first.get_index();

    return planning::insert(context.destination, *literal);
}

// Numeric

template<FactKind T>
std::pair<FunctionView<T>, bool> copy(FunctionView<T> element, CopyContext& context)
{
    auto function = planning::checkout<Function<T>>(context.builder);

    function->name = element.get_name();
    function->arity = element.get_arity();

    return planning::insert(context.destination, *function);
}

template<FactKind T>
std::pair<FunctionTermView<LiftedTag, T>, bool> copy(FunctionTermView<LiftedTag, T> element, CopyContext& context)
{
    auto fterm = planning::checkout<FunctionTerm<LiftedTag, T>>(context.builder);

    fterm->function = copy(element.get_function(), context).first.get_index();
    for (const auto term : element.get_terms())
        fterm->terms.push_back(copy(term, context).get_data());

    return planning::insert(context.destination, *fterm);
}

template<FactKind T>
std::pair<FunctionTermView<GroundTag, T>, bool> copy(FunctionTermView<GroundTag, T> element, CopyContext& context)
{
    auto fterm = planning::checkout<FunctionTerm<GroundTag, T>>(context.builder);

    fterm->binding = copy(element.get_row(), context).first.get_index();

    return planning::insert(context.destination, *fterm);
}

template<FactKind T>
std::pair<FunctionTermValueView<GroundTag, T>, bool> copy(FunctionTermValueView<GroundTag, T> element, CopyContext& context)
{
    auto fterm_value = planning::checkout<FunctionTermValue<GroundTag, T>>(context.builder);

    fterm_value->fterm = copy(element.get_fterm(), context).first.get_index();
    fterm_value->value = element.get_value();

    return planning::insert(context.destination, *fterm_value);
}

template<TaskKind T>
inline FunctionExpressionView<T> copy(FunctionExpressionView<T> element, CopyContext& context)
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
    auto unary = planning::checkout<UnaryOperator<T>>(context.builder);

    unary->operator_kind = element.get_operator();
    unary->arg = copy(element.get_arg(), context).get_data();

    return planning::insert(context.destination, *unary);
}

template<TaskKind T, BinaryOperatorKind O>
std::pair<BinaryOperatorView<T, O>, bool> copy(BinaryOperatorView<T, O> element, CopyContext& context)
{
    auto binary = planning::checkout<BinaryOperator<T, O>>(context.builder);

    binary->operator_kind = element.get_operator();
    binary->lhs = copy(element.get_lhs(), context).get_data();
    binary->rhs = copy(element.get_rhs(), context).get_data();

    return planning::insert(context.destination, *binary);
}

template<TaskKind T>
std::pair<MultiOperatorView<T>, bool> copy(MultiOperatorView<T> element, CopyContext& context)
{
    auto multi = planning::checkout<MultiOperator<T>>(context.builder);

    multi->operator_kind = element.get_operator();
    for (const auto arg : element.get_args())
        multi->args.push_back(copy(arg, context).get_data());

    return planning::insert(context.destination, *multi);
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
    auto numeric_effect = planning::checkout<NumericEffect<T, F>>(context.builder);

    numeric_effect->operator_kind = element.get_operator();
    numeric_effect->fterm = copy(element.get_fterm(), context).first.get_index();
    numeric_effect->fexpr = copy(element.get_fexpr(), context).get_data();

    return planning::insert(context.destination, *numeric_effect);
}

template<TaskKind T, FactKind F>
NumericEffectOperatorView<T, F> copy(NumericEffectOperatorView<T, F> element, CopyContext& context)
{
    const auto data = visit([&](auto&& arg) { return ygg::Data<NumericEffectOperator<T, F>>(arg.get_operator(), copy(arg, context).first.get_index()); },
                            element.get_variant());
    return ygg::make_view(data, context.destination);
}

// Composite

inline std::pair<ConjunctiveConditionView<LiftedTag>, bool> copy(ConjunctiveConditionView<LiftedTag> element, CopyContext& context)
{
    auto conj_cond = planning::checkout<ConjunctiveCondition<LiftedTag>>(context.builder);

    for (const auto variable : element.get_variables())
        conj_cond->variables.push_back(copy(variable, context).first.get_index());
    for (const auto literal : element.template get_literals<StaticTag>())
        conj_cond->static_literals.push_back(copy(literal, context).first.get_index());
    for (const auto literal : element.template get_literals<FluentTag>())
        conj_cond->fluent_literals.push_back(copy(literal, context).first.get_index());
    for (const auto literal : element.template get_literals<DerivedTag>())
        conj_cond->derived_literals.push_back(copy(literal, context).first.get_index());
    for (const auto numeric_constraint : element.get_numeric_constraints())
        conj_cond->numeric_constraints.push_back(copy(numeric_constraint, context).get_data());

    return planning::insert(context.destination, *conj_cond);
}

inline std::pair<ConjunctiveEffectView<LiftedTag>, bool> copy(ConjunctiveEffectView<LiftedTag> element, CopyContext& context)
{
    auto effect = planning::checkout<ConjunctiveEffect<LiftedTag>>(context.builder);

    for (const auto literal : element.get_literals())
        effect->literals.push_back(copy(literal, context).first.get_index());
    for (const auto numeric_effect : element.get_numeric_effects())
        effect->numeric_effects.push_back(copy(numeric_effect, context).get_data());
    if (const auto auxiliary_effect = element.get_auxiliary_numeric_effect())
        effect->auxiliary_numeric_effect = copy(*auxiliary_effect, context).get_data();

    return planning::insert(context.destination, *effect);
}

inline std::pair<ConditionalEffectView<LiftedTag>, bool> copy(ConditionalEffectView<LiftedTag> element, CopyContext& context)
{
    auto effect = planning::checkout<ConditionalEffect<LiftedTag>>(context.builder);

    for (const auto variable : element.get_variables())
        effect->variables.push_back(copy(variable, context).first.get_index());
    effect->condition = copy(element.get_condition(), context).first.get_index();
    effect->effect = copy(element.get_effect(), context).first.get_index();

    return planning::insert(context.destination, *effect);
}

inline std::pair<ActionView<LiftedTag>, bool> copy(ActionView<LiftedTag> element, CopyContext& context)
{
    auto action = planning::checkout<Action<LiftedTag>>(context.builder);

    action->name = element.get_name();
    action->original_name = element.get_original_name();
    action->original_arity = element.get_original_arity();
    for (const auto variable : element.get_variables())
        action->variables.push_back(copy(variable, context).first.get_index());
    action->condition = copy(element.get_condition(), context).first.get_index();
    for (const auto effect : element.get_effects())
        action->effects.push_back(copy(effect, context).first.get_index());

    return planning::insert(context.destination, *action);
}

inline std::pair<AxiomView<LiftedTag>, bool> copy(AxiomView<LiftedTag> element, CopyContext& context)
{
    auto axiom = planning::checkout<Axiom<LiftedTag>>(context.builder);

    for (const auto variable : element.get_variables())
        axiom->variables.push_back(copy(variable, context).first.get_index());
    axiom->body = copy(element.get_body(), context).first.get_index();
    axiom->head = copy(element.get_head(), context).first.get_index();

    return planning::insert(context.destination, *axiom);
}

inline std::pair<MetricView, bool> copy(MetricView element, CopyContext& context)
{
    auto metric = planning::checkout<Metric>(context.builder);

    metric->optimization_direction = element.get_optimization_direction();
    metric->fexpr = copy(element.get_fexpr(), context).get_data();

    return planning::insert(context.destination, *metric);
}

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::planning
{
extern template std::pair<PredicateBindingView<StaticTag>, bool> copy(PredicateBindingView<StaticTag> element, CopyContext& context);
extern template std::pair<PredicateBindingView<FluentTag>, bool> copy(PredicateBindingView<FluentTag> element, CopyContext& context);
extern template std::pair<PredicateBindingView<DerivedTag>, bool> copy(PredicateBindingView<DerivedTag> element, CopyContext& context);

extern template std::pair<FunctionBindingView<StaticTag>, bool> copy(FunctionBindingView<StaticTag> element, CopyContext& context);
extern template std::pair<FunctionBindingView<FluentTag>, bool> copy(FunctionBindingView<FluentTag> element, CopyContext& context);
extern template std::pair<FunctionBindingView<AuxiliaryTag>, bool> copy(FunctionBindingView<AuxiliaryTag> element, CopyContext& context);

extern template std::pair<PredicateView<StaticTag>, bool> copy(PredicateView<StaticTag> element, CopyContext& context);
extern template std::pair<PredicateView<FluentTag>, bool> copy(PredicateView<FluentTag> element, CopyContext& context);
extern template std::pair<PredicateView<DerivedTag>, bool> copy(PredicateView<DerivedTag> element, CopyContext& context);

extern template std::pair<AtomView<LiftedTag, StaticTag>, bool> copy(AtomView<LiftedTag, StaticTag> element, CopyContext& context);
extern template std::pair<AtomView<LiftedTag, FluentTag>, bool> copy(AtomView<LiftedTag, FluentTag> element, CopyContext& context);
extern template std::pair<AtomView<LiftedTag, DerivedTag>, bool> copy(AtomView<LiftedTag, DerivedTag> element, CopyContext& context);

extern template std::pair<AtomView<GroundTag, StaticTag>, bool> copy(AtomView<GroundTag, StaticTag> element, CopyContext& context);
extern template std::pair<AtomView<GroundTag, FluentTag>, bool> copy(AtomView<GroundTag, FluentTag> element, CopyContext& context);
extern template std::pair<AtomView<GroundTag, DerivedTag>, bool> copy(AtomView<GroundTag, DerivedTag> element, CopyContext& context);

extern template std::pair<LiteralView<LiftedTag, StaticTag>, bool> copy(LiteralView<LiftedTag, StaticTag> element, CopyContext& context);
extern template std::pair<LiteralView<LiftedTag, FluentTag>, bool> copy(LiteralView<LiftedTag, FluentTag> element, CopyContext& context);
extern template std::pair<LiteralView<LiftedTag, DerivedTag>, bool> copy(LiteralView<LiftedTag, DerivedTag> element, CopyContext& context);

extern template std::pair<LiteralView<GroundTag, StaticTag>, bool> copy(LiteralView<GroundTag, StaticTag> element, CopyContext& context);
extern template std::pair<LiteralView<GroundTag, FluentTag>, bool> copy(LiteralView<GroundTag, FluentTag> element, CopyContext& context);
extern template std::pair<LiteralView<GroundTag, DerivedTag>, bool> copy(LiteralView<GroundTag, DerivedTag> element, CopyContext& context);

extern template std::pair<FunctionView<StaticTag>, bool> copy(FunctionView<StaticTag> element, CopyContext& context);
extern template std::pair<FunctionView<FluentTag>, bool> copy(FunctionView<FluentTag> element, CopyContext& context);
extern template std::pair<FunctionView<AuxiliaryTag>, bool> copy(FunctionView<AuxiliaryTag> element, CopyContext& context);

extern template std::pair<FunctionTermView<LiftedTag, StaticTag>, bool> copy(FunctionTermView<LiftedTag, StaticTag> element, CopyContext& context);
extern template std::pair<FunctionTermView<LiftedTag, FluentTag>, bool> copy(FunctionTermView<LiftedTag, FluentTag> element, CopyContext& context);
extern template std::pair<FunctionTermView<LiftedTag, AuxiliaryTag>, bool> copy(FunctionTermView<LiftedTag, AuxiliaryTag> element, CopyContext& context);

extern template std::pair<FunctionTermView<GroundTag, StaticTag>, bool> copy(FunctionTermView<GroundTag, StaticTag> element, CopyContext& context);
extern template std::pair<FunctionTermView<GroundTag, FluentTag>, bool> copy(FunctionTermView<GroundTag, FluentTag> element, CopyContext& context);
extern template std::pair<FunctionTermView<GroundTag, AuxiliaryTag>, bool> copy(FunctionTermView<GroundTag, AuxiliaryTag> element, CopyContext& context);

extern template std::pair<FunctionTermValueView<GroundTag, StaticTag>, bool> copy(FunctionTermValueView<GroundTag, StaticTag> element, CopyContext& context);
extern template std::pair<FunctionTermValueView<GroundTag, FluentTag>, bool> copy(FunctionTermValueView<GroundTag, FluentTag> element, CopyContext& context);
extern template std::pair<FunctionTermValueView<GroundTag, AuxiliaryTag>, bool> copy(FunctionTermValueView<GroundTag, AuxiliaryTag> element,
                                                                                     CopyContext& context);

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

extern template BooleanOperatorView<LiftedTag> copy(BooleanOperatorView<LiftedTag> element, CopyContext& context);
extern template BooleanOperatorView<GroundTag> copy(BooleanOperatorView<GroundTag> element, CopyContext& context);

extern template std::pair<NumericEffectView<LiftedTag, FluentTag>, bool> copy(NumericEffectView<LiftedTag, FluentTag> element, CopyContext& context);

extern template std::pair<NumericEffectView<LiftedTag, AuxiliaryTag>, bool> copy(NumericEffectView<LiftedTag, AuxiliaryTag> element, CopyContext& context);

extern template NumericEffectOperatorView<LiftedTag, FluentTag> copy(NumericEffectOperatorView<LiftedTag, FluentTag> element, CopyContext& context);
extern template NumericEffectOperatorView<LiftedTag, AuxiliaryTag> copy(NumericEffectOperatorView<LiftedTag, AuxiliaryTag> element, CopyContext& context);

extern template std::pair<NumericEffectView<GroundTag, FluentTag>, bool> copy(NumericEffectView<GroundTag, FluentTag> element, CopyContext& context);

extern template std::pair<NumericEffectView<GroundTag, AuxiliaryTag>, bool> copy(NumericEffectView<GroundTag, AuxiliaryTag> element, CopyContext& context);

extern template NumericEffectOperatorView<GroundTag, FluentTag> copy(NumericEffectOperatorView<GroundTag, FluentTag> element, CopyContext& context);
extern template NumericEffectOperatorView<GroundTag, AuxiliaryTag> copy(NumericEffectOperatorView<GroundTag, AuxiliaryTag> element, CopyContext& context);
}

#endif

#endif
