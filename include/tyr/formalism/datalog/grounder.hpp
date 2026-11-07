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
#include "tyr/formalism/write_context.hpp"
#include "tyr/formalism/binding_index.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/datalog/interning.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/atom_view.hpp"
#include "tyr/formalism/datalog/binary_operator_view.hpp"
#include "tyr/formalism/datalog/conditional_effect_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_view.hpp"
#include "tyr/formalism/datalog/function_term_value_view.hpp"
#include "tyr/formalism/datalog/function_term_view.hpp"
#include "tyr/formalism/datalog/literal_view.hpp"
#include "tyr/formalism/datalog/multi_operator_view.hpp"
#include "tyr/formalism/datalog/numeric_effect_view.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/datalog/unary_operator_view.hpp"
#include <yggdrasil/formalism/binding_view.hpp>
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/variable_view.hpp"


namespace tyr::formalism::datalog
{

/**
 * ground
 */

template<FactKind F>
std::pair<FunctionBindingView<F>, bool> ground(TermListView terms, FunctionView<F> function, WriteContext<Builder, Repository>& context);

template<FactKind F>
std::pair<FunctionTermView<GroundTag, F>, bool> ground(FunctionTermView<LiftedTag, F> element, WriteContext<Builder, Repository>& context);

FunctionExpressionView<GroundTag> ground(FunctionExpressionView<LiftedTag> element, WriteContext<Builder, Repository>& context);

std::pair<UnaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool> ground(UnaryOperatorView<LiftedTag, ArithmeticOperatorKind> element, WriteContext<Builder, Repository>& context);

template<OperatorKind O>
std::pair<BinaryOperatorView<GroundTag, O>, bool> ground(BinaryOperatorView<LiftedTag, O> element, WriteContext<Builder, Repository>& context);

std::pair<MultiOperatorView<GroundTag, ArithmeticOperatorKind>, bool> ground(MultiOperatorView<LiftedTag, ArithmeticOperatorKind> element, WriteContext<Builder, Repository>& context);

template<FactKind F>
std::pair<PredicateBindingView<F>, bool> ground(TermListView terms, PredicateView<F> predicate, WriteContext<Builder, Repository>& context);

template<FactKind F>
std::pair<AtomView<GroundTag, F>, bool> ground(AtomView<LiftedTag, F> element, WriteContext<Builder, Repository>& context);

template<FactKind F>
std::pair<LiteralView<GroundTag, F>, bool> ground(LiteralView<LiftedTag, F> element, WriteContext<Builder, Repository>& context);

template<FactKind F>
std::pair<NumericEffectView<GroundTag, F>, bool> ground(NumericEffectView<LiftedTag, F> element, WriteContext<Builder, Repository>& context);

std::pair<ConjunctiveConditionView<GroundTag>, bool> ground(ConjunctiveConditionView<LiftedTag> element, WriteContext<Builder, Repository>& context);

template<RelationKind R>
std::pair<RuleView<GroundTag, R>, bool> ground(RuleView<LiftedTag, R> element, WriteContext<Builder, Repository>& context);

/**
 * ground_binding
 */

template<FactKind F>
std::pair<PredicateBindingView<F>, bool> ground_binding(AtomView<LiftedTag, F> element, WriteContext<Builder, Repository>& context);

/// Grounds the binding without interning the enclosing GroundFunctionTerm symbol.
template<FactKind F>
std::pair<FunctionBindingView<F>, bool> ground_binding(FunctionTermView<LiftedTag, F> element, WriteContext<Builder, Repository>& context);

template<RelationKind R>
std::pair<RuleBindingView<R>, bool> ground_binding(RuleView<LiftedTag, R> element, WriteContext<Builder, Repository>& context);

/**
 * try_ground
 */

template<FactKind F>
std::optional<FunctionBindingView<F>> try_ground_binding(FunctionTermView<LiftedTag, F> element, WriteContext<Builder, Repository>& context);

template<FactKind F>
std::optional<PredicateBindingView<F>> try_ground_binding(AtomView<LiftedTag, F> element, WriteContext<Builder, Repository>& context);

/**
 * ground
 */

/// Substitute terms into retained object storage without interning a binding.
inline void ground(TermListView terms, const WriteContext<Builder, Repository>& context, urd::flat_vector<urd::index<Object>>& objects)
{
    objects.clear();
    for (const auto term : terms)
        visit(
            [&]<typename Alternative>(Alternative arg)
            {
                if constexpr (std::same_as<Alternative, ParameterIndex>)
                    objects.push_back(context.binding()[arg.value]);
                else if constexpr (std::same_as<Alternative, ObjectView>)
                    objects.push_back(arg.index());
                else
                    static_assert(urd::repr::dependent_false<Alternative>::value, "Missing case");
            },
            term);
}

template<FactKind F>
std::pair<FunctionBindingView<F>, bool> ground(TermListView terms, FunctionView<F> function, WriteContext<Builder, Repository>& context)
{
    auto binding = datalog::checkout<RelationBinding<Function<F>>>(context.builder());

    binding->relation = function.index();
    ground(terms, context, binding->objects);

    // Canonicalize and Serialize
    return datalog::insert(context.destination(), *binding);
}

template<FactKind F>
std::pair<FunctionTermView<GroundTag, F>, bool> ground(FunctionTermView<LiftedTag, F> element, WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto fterm = datalog::checkout<FunctionTerm<GroundTag, F>>(context.builder());

    // Fill data
    fterm->binding = ground(element.get_terms(), element.get_function(), context).first.index();

    // Canonicalize and Serialize
    return datalog::insert(context.destination(), *fterm);
}

inline FunctionExpressionView<GroundTag> ground(FunctionExpressionView<LiftedTag> element, WriteContext<Builder, Repository>& context)
{
    return std::visit(
        [&]<typename Alternative>(const Alternative& arg) -> FunctionExpressionView<GroundTag>
        {
            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return arg;
            else
                return ground(arg, context).first;
        },
        element);
}

inline std::pair<UnaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool> ground(UnaryOperatorView<LiftedTag, ArithmeticOperatorKind> element, WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto unary = datalog::checkout<UnaryOperator<GroundTag, ArithmeticOperatorKind>>(context.builder());

    // Fill data
    unary->operator_kind = element.get_operator();
    unary->arg = urd::to_stored(ground(element.get_arg(), context));

    // Canonicalize and Serialize
    return datalog::insert(context.destination(), *unary);
}

template<OperatorKind O>
std::pair<BinaryOperatorView<GroundTag, O>, bool> ground(BinaryOperatorView<LiftedTag, O> element, WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto binary = datalog::checkout<BinaryOperator<GroundTag, O>>(context.builder());

    // Fill data
    binary->operator_kind = element.get_operator();
    binary->lhs = urd::to_stored(ground(element.get_lhs(), context));
    binary->rhs = urd::to_stored(ground(element.get_rhs(), context));

    // Canonicalize and Serialize
    return datalog::insert(context.destination(), *binary);
}

inline std::pair<MultiOperatorView<GroundTag, ArithmeticOperatorKind>, bool> ground(MultiOperatorView<LiftedTag, ArithmeticOperatorKind> element, WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto multi = datalog::checkout<MultiOperator<GroundTag, ArithmeticOperatorKind>>(context.builder());

    // Fill data
    multi->operator_kind = element.get_operator();
    for (const auto arg : element.get_args())
        multi->args.push_back(urd::to_stored(ground(arg, context)));

    // Canonicalize and Serialize
    return datalog::insert(context.destination(), *multi);
}

template<FactKind F>
std::pair<PredicateBindingView<F>, bool> ground(TermListView terms, PredicateView<F> predicate, WriteContext<Builder, Repository>& context)
{
    auto binding = datalog::checkout<RelationBinding<Predicate<F>>>(context.builder());

    binding->relation = predicate.index();
    ground(terms, context, binding->objects);

    // Canonicalize and Serialize
    return datalog::insert(context.destination(), *binding);
}

template<FactKind F>
std::pair<AtomView<GroundTag, F>, bool> ground(AtomView<LiftedTag, F> element, WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto atom = datalog::checkout<Atom<GroundTag, F>>(context.builder());

    // Fill data
    atom->binding = ground(element.get_terms(), element.get_predicate(), context).first.index();

    // Canonicalize and Serialize
    return datalog::insert(context.destination(), *atom);
}

template<FactKind F>
std::pair<LiteralView<GroundTag, F>, bool> ground(LiteralView<LiftedTag, F> element, WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto ground_literal = datalog::checkout<Literal<GroundTag, F>>(context.builder());

    // Fill data
    ground_literal->polarity = element.get_polarity();
    ground_literal->atom = ground(element.get_atom(), context).first.index();

    // Canonicalize and Serialize
    return datalog::insert(context.destination(), *ground_literal);
}

template<FactKind F>
std::pair<NumericEffectView<GroundTag, F>, bool> ground(NumericEffectView<LiftedTag, F> element, WriteContext<Builder, Repository>& context)
{
    auto numeric_effect = datalog::checkout<NumericEffect<GroundTag, F>>(context.builder());

    numeric_effect->operator_kind = element.get_operator();
    numeric_effect->fterm = ground(element.get_fterm(), context).first.index();
    numeric_effect->fexpr = urd::to_stored(ground(element.get_fexpr(), context));

    return datalog::insert(context.destination(), *numeric_effect);
}

inline std::pair<ConjunctiveConditionView<GroundTag>, bool> ground(ConjunctiveConditionView<LiftedTag> element, WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto conj_cond = datalog::checkout<ConjunctiveCondition<GroundTag>>(context.builder());

    // Fill data
    for (const auto literal : element.template get_literals<StaticTag>())
        conj_cond->static_literals.push_back(ground(literal, context).first.index());
    for (const auto literal : element.template get_literals<FluentTag>())
        conj_cond->fluent_literals.push_back(ground(literal, context).first.index());
    for (const auto numeric_constraint : element.get_numeric_constraints())
        conj_cond->numeric_constraints.push_back(ground(numeric_constraint, context).first.index());

    // Canonicalize and Serialize
    return datalog::insert(context.destination(), *conj_cond);
}

template<RelationKind R>
std::pair<RuleView<GroundTag, R>, bool> ground(RuleView<LiftedTag, R> element, WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto rule = datalog::checkout<Rule<GroundTag, R>>(context.builder());

    // Fill data
    rule->binding = ground_binding(element, context).first.index();
    rule->body = ground(element.get_body(), context).first.index();
    if constexpr (std::same_as<R, PredicateTag>)
        for (const auto head : element.get_heads())
            rule->heads.push_back(ground(head, context).first.index());
    else
        rule->head = ground(element.get_head(), context).first.index();
    rule->cost = element.get_cost();

    // Canonicalize and Serialize
    return datalog::insert(context.destination(), *rule);
}

/**
 * ground_binding
 */

template<FactKind F>
std::pair<PredicateBindingView<F>, bool> ground_binding(AtomView<LiftedTag, F> element, WriteContext<Builder, Repository>& context)
{
    auto binding = datalog::checkout<RelationBinding<Predicate<F>>>(context.builder());

    binding->relation = element.get_predicate().index();
    ground(element.get_terms(), context, binding->objects);

    // Canonicalize and Serialize
    return datalog::insert(context.destination(), *binding);
}

template<FactKind F>
std::pair<FunctionBindingView<F>, bool> ground_binding(FunctionTermView<LiftedTag, F> element, WriteContext<Builder, Repository>& context)
{ return ground(element.get_terms(), element.get_function(), context); }

template<RelationKind R>
std::pair<RuleBindingView<R>, bool> ground_binding(RuleView<LiftedTag, R> element, WriteContext<Builder, Repository>& context)
{
    auto binding = datalog::checkout<RelationBinding<Rule<LiftedTag, R>>>(context.builder());

    binding->relation = element.index();
    for (const auto object : context.binding())
        binding->objects.push_back(object);

    return datalog::insert(context.destination(), *binding);
}

/**
 * try_ground
 */

template<FactKind F>
std::optional<FunctionBindingView<F>> try_ground_binding(FunctionTermView<LiftedTag, F> element, WriteContext<Builder, Repository>& context)
{
    auto binding = datalog::checkout<RelationBinding<Function<F>>>(context.builder());

    binding->relation = element.get_function().index();
    ground(element.get_terms(), context, binding->objects);

    return context.destination().find(*binding);
}

template<FactKind F>
std::optional<PredicateBindingView<F>> try_ground_binding(AtomView<LiftedTag, F> element, WriteContext<Builder, Repository>& context)
{
    auto binding = datalog::checkout<RelationBinding<Predicate<F>>>(context.builder());

    binding->relation = element.get_predicate().index();
    ground(element.get_terms(), context, binding->objects);

    return context.destination().find(*binding);
}
}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::datalog
{
extern template std::pair<FunctionBindingView<StaticTag>, bool> ground(TermListView terms, FunctionView<StaticTag> function, WriteContext<Builder, Repository>& context);
extern template std::pair<FunctionBindingView<FluentTag>, bool> ground(TermListView terms, FunctionView<FluentTag> function, WriteContext<Builder, Repository>& context);

extern template std::pair<FunctionTermView<GroundTag, StaticTag>, bool> ground(FunctionTermView<LiftedTag, StaticTag> element, WriteContext<Builder, Repository>& context);
extern template std::pair<FunctionTermView<GroundTag, FluentTag>, bool> ground(FunctionTermView<LiftedTag, FluentTag> element, WriteContext<Builder, Repository>& context);

extern template std::pair<BinaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool> ground(BinaryOperatorView<LiftedTag, ArithmeticOperatorKind> element,
                                                                                              WriteContext<Builder, Repository>& context);
extern template std::pair<BinaryOperatorView<GroundTag, BooleanOperatorKind>, bool> ground(BinaryOperatorView<LiftedTag, BooleanOperatorKind> element,
                                                                                           WriteContext<Builder, Repository>& context);

extern template std::pair<PredicateBindingView<StaticTag>, bool> ground(TermListView terms, PredicateView<StaticTag> predicate, WriteContext<Builder, Repository>& context);
extern template std::pair<PredicateBindingView<FluentTag>, bool> ground(TermListView terms, PredicateView<FluentTag> predicate, WriteContext<Builder, Repository>& context);

extern template std::pair<AtomView<GroundTag, StaticTag>, bool> ground(AtomView<LiftedTag, StaticTag> element, WriteContext<Builder, Repository>& context);
extern template std::pair<AtomView<GroundTag, FluentTag>, bool> ground(AtomView<LiftedTag, FluentTag> element, WriteContext<Builder, Repository>& context);

extern template std::pair<LiteralView<GroundTag, StaticTag>, bool> ground(LiteralView<LiftedTag, StaticTag> element, WriteContext<Builder, Repository>& context);
extern template std::pair<LiteralView<GroundTag, FluentTag>, bool> ground(LiteralView<LiftedTag, FluentTag> element, WriteContext<Builder, Repository>& context);

extern template std::pair<NumericEffectView<GroundTag, FluentTag>, bool> ground(NumericEffectView<LiftedTag, FluentTag> element, WriteContext<Builder, Repository>& context);

extern template std::pair<PredicateBindingView<StaticTag>, bool> ground_binding(AtomView<LiftedTag, StaticTag> element, WriteContext<Builder, Repository>& context);
extern template std::pair<PredicateBindingView<FluentTag>, bool> ground_binding(AtomView<LiftedTag, FluentTag> element, WriteContext<Builder, Repository>& context);

extern template std::pair<FunctionBindingView<StaticTag>, bool> ground_binding(FunctionTermView<LiftedTag, StaticTag> element, WriteContext<Builder, Repository>& context);
extern template std::pair<FunctionBindingView<FluentTag>, bool> ground_binding(FunctionTermView<LiftedTag, FluentTag> element, WriteContext<Builder, Repository>& context);

extern template std::optional<FunctionBindingView<StaticTag>> try_ground_binding(FunctionTermView<LiftedTag, StaticTag> element, WriteContext<Builder, Repository>& context);
extern template std::optional<FunctionBindingView<FluentTag>> try_ground_binding(FunctionTermView<LiftedTag, FluentTag> element, WriteContext<Builder, Repository>& context);

extern template std::optional<PredicateBindingView<StaticTag>> try_ground_binding(AtomView<LiftedTag, StaticTag> element, WriteContext<Builder, Repository>& context);
extern template std::optional<PredicateBindingView<FluentTag>> try_ground_binding(AtomView<LiftedTag, FluentTag> element, WriteContext<Builder, Repository>& context);

extern template std::pair<RuleView<GroundTag, PredicateTag>, bool> ground(RuleView<LiftedTag, PredicateTag> element, WriteContext<Builder, Repository>& context);
extern template std::pair<RuleView<GroundTag, FunctionTag>, bool> ground(RuleView<LiftedTag, FunctionTag> element, WriteContext<Builder, Repository>& context);

extern template std::pair<RuleBindingView<PredicateTag>, bool> ground_binding(RuleView<LiftedTag, PredicateTag> element, WriteContext<Builder, Repository>& context);
extern template std::pair<RuleBindingView<FunctionTag>, bool> ground_binding(RuleView<LiftedTag, FunctionTag> element, WriteContext<Builder, Repository>& context);
}

#endif

#endif
