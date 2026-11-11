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

#ifndef TYR_FORMALISM_PLANNING_GROUNDER_HPP_
#define TYR_FORMALISM_PLANNING_GROUNDER_HPP_

#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/copy.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/canonicalization.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/grounder_decl.hpp"
#include "tyr/formalism/planning/interning.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/formalism/write_context.hpp"

#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::formalism::planning
{

/**
 * ground
 */

template<FactKind T>
std::pair<FunctionRowView<T>, bool> ground(TermListView terms, FunctionView<T> function, WriteContext<Builder, Repository>& context);

template<FactKind T>
std::pair<FunctionTermView<GroundTag, T>, bool> ground(FunctionTermView<LiftedTag, T> element, WriteContext<Builder, Repository>& context);

FunctionExpressionView<GroundTag> ground(FunctionExpressionView<LiftedTag> element, WriteContext<Builder, Repository>& context);

std::pair<UnaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool> ground(UnaryOperatorView<LiftedTag, ArithmeticOperatorKind> element,
                                                                             WriteContext<Builder, Repository>& context);

template<OperatorKind O>
std::pair<BinaryOperatorView<GroundTag, O>, bool> ground(BinaryOperatorView<LiftedTag, O> element, WriteContext<Builder, Repository>& context);

std::pair<MultiOperatorView<GroundTag, ArithmeticOperatorKind>, bool> ground(MultiOperatorView<LiftedTag, ArithmeticOperatorKind> element,
                                                                             WriteContext<Builder, Repository>& context);

template<FactKind T>
std::pair<PredicateRowView<T>, bool> ground(TermListView terms, PredicateView<T> predicate, WriteContext<Builder, Repository>& context);

template<FactKind T>
std::pair<AtomView<GroundTag, T>, bool> ground(AtomView<LiftedTag, T> element, WriteContext<Builder, Repository>& context);

urd::data<FDRFact<FluentTag>> ground(AtomView<LiftedTag, FluentTag> element, WriteContext<Builder, Repository>& context, FDRContext& fdr);

template<FactKind T>
std::pair<LiteralView<GroundTag, T>, bool> ground(LiteralView<LiftedTag, T> element, WriteContext<Builder, Repository>& context);

urd::data<FDRFact<FluentTag>> ground(LiteralView<LiftedTag, FluentTag> element, WriteContext<Builder, Repository>& context, FDRContext& fdr);

std::pair<ConjunctiveConditionView<GroundTag>, bool>
ground(ConjunctiveConditionView<LiftedTag> element, WriteContext<Builder, Repository>& context, FDRContext& fdr);

template<FactKind T>
std::pair<NumericEffectView<GroundTag, T>, bool> ground(NumericEffectView<LiftedTag, T> element, WriteContext<Builder, Repository>& context);

std::pair<ConjunctiveEffectView<GroundTag>, bool> ground(ConjunctiveEffectView<LiftedTag> element, WriteContext<Builder, Repository>& context, FDRContext& fdr);

std::pair<ConditionalEffectView<GroundTag>, bool> ground(ConditionalEffectView<LiftedTag> element, WriteContext<Builder, Repository>& context, FDRContext& fdr);

std::pair<ActionRowView, bool> ground(ActionView<LiftedTag> action, WriteContext<Builder, Repository>& context);

/// Grounds the action binding in the context with its effects grounded for each completed binding that
/// `for_each_effect_binding(effect, callback)` puts into the context's binding before calling `callback`.
template<typename ForEachEffectBinding>
std::pair<ActionView<GroundTag>, bool>
ground(ActionView<LiftedTag> element, WriteContext<Builder, Repository>& context, FDRContext& fdr, ForEachEffectBinding for_each_effect_binding);

std::pair<AxiomRowView, bool> ground(AxiomView<LiftedTag> axiom, WriteContext<Builder, Repository>& context);

std::pair<AxiomView<GroundTag>, bool>
ground(AxiomView<LiftedTag> element, WriteContext<Builder, Repository>& context, GrounderCacheEntry<Axiom<LiftedTag>>& cache, FDRContext& fdr);

/**
 * try_ground
 */

template<FactKind T>
std::optional<FunctionTermView<GroundTag, T>> try_ground(FunctionTermView<LiftedTag, T> element, WriteContext<Builder, Repository>& context);

template<FactKind T>
std::optional<AtomView<GroundTag, T>> try_ground(AtomView<LiftedTag, T> element, WriteContext<Builder, Repository>& context);

/**
 * ground
 */

template<FactKind T>
std::pair<FunctionRowView<T>, bool> ground(TermListView terms, FunctionView<T> function, WriteContext<Builder, Repository>& context)
{
    auto binding = planning::checkout<ygg::Row<Function<T>>>(context.builder());

    binding->relation = function.index();
    for (const auto term : terms)
    {
        visit(
            [&]<typename Alternative>(const Alternative& arg)
            {
                if constexpr (std::is_same_v<Alternative, ParameterIndex>)
                    binding->values.push_back(context.binding()[arg.value].value);
                else if constexpr (std::is_same_v<Alternative, ObjectView>)
                    binding->values.push_back(arg.index().value);
                else
                    static_assert(false, "Missing case");
            },
            term);
    }

    // Canonicalize and Serialize
    return planning::insert(context.destination(), *binding);
}

template<FactKind T>
std::pair<FunctionTermView<GroundTag, T>, bool> ground(FunctionTermView<LiftedTag, T> element, WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto fterm = planning::checkout<FunctionTerm<GroundTag, T>>(context.builder());

    // Fill data
    fterm->row = ground(element.get_terms(), element.get_function(), context).first.data();

    // Canonicalize and Serialize
    return planning::insert(context.destination(), *fterm);
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

inline std::pair<UnaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool> ground(UnaryOperatorView<LiftedTag, ArithmeticOperatorKind> element,
                                                                                    WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto unary = planning::checkout<UnaryOperator<GroundTag, ArithmeticOperatorKind>>(context.builder());

    // Fill data
    unary->operator_kind = element.get_operator();
    unary->arg = urd::to_stored(ground(element.get_arg(), context));

    // Canonicalize and Serialize
    return planning::insert(context.destination(), *unary);
}

template<OperatorKind O>
std::pair<BinaryOperatorView<GroundTag, O>, bool> ground(BinaryOperatorView<LiftedTag, O> element, WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto binary = planning::checkout<BinaryOperator<GroundTag, O>>(context.builder());

    // Fill data
    binary->operator_kind = element.get_operator();
    binary->lhs = urd::to_stored(ground(element.get_lhs(), context));
    binary->rhs = urd::to_stored(ground(element.get_rhs(), context));

    // Canonicalize and Serialize
    return planning::insert(context.destination(), *binary);
}

inline std::pair<MultiOperatorView<GroundTag, ArithmeticOperatorKind>, bool> ground(MultiOperatorView<LiftedTag, ArithmeticOperatorKind> element,
                                                                                    WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto multi = planning::checkout<MultiOperator<GroundTag, ArithmeticOperatorKind>>(context.builder());

    // Fill data
    multi->operator_kind = element.get_operator();
    for (const auto arg : element.get_args())
        multi->args.push_back(urd::to_stored(ground(arg, context)));

    // Canonicalize and Serialize
    return planning::insert(context.destination(), *multi);
}

template<FactKind T>
std::pair<PredicateRowView<T>, bool> ground(TermListView terms, PredicateView<T> predicate, WriteContext<Builder, Repository>& context)
{
    auto binding = planning::checkout<ygg::Row<Predicate<T>>>(context.builder());

    binding->relation = predicate.index();
    for (const auto term : terms)
    {
        visit(
            [&]<typename Alternative>(const Alternative& arg)
            {
                if constexpr (std::is_same_v<Alternative, ParameterIndex>)
                    binding->values.push_back(context.binding()[arg.value].value);
                else if constexpr (std::is_same_v<Alternative, ObjectView>)
                    binding->values.push_back(arg.index().value);
                else
                    static_assert(false, "Missing case");
            },
            term);
    }

    // Canonicalize and Serialize
    return planning::insert(context.destination(), *binding);
}

template<FactKind T>
std::pair<AtomView<GroundTag, T>, bool> ground(AtomView<LiftedTag, T> element, WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto atom = planning::checkout<Atom<GroundTag, T>>(context.builder());

    // Fill data
    atom->row = ground(element.get_terms(), element.get_predicate(), context).first.data();

    // Canonicalize and Serialize
    return planning::insert(context.destination(), *atom);
}

inline urd::data<FDRFact<FluentTag>> ground(AtomView<LiftedTag, FluentTag> element, WriteContext<Builder, Repository>& context, FDRContext& fdr)
{ return fdr.get_fact(ground(element, context).first).data(); }

template<FactKind T>
std::pair<LiteralView<GroundTag, T>, bool> ground(LiteralView<LiftedTag, T> element, WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto ground_literal = planning::checkout<Literal<GroundTag, T>>(context.builder());

    // Fill data
    ground_literal->polarity = element.get_polarity();
    ground_literal->atom = ground(element.get_atom(), context).first.index();

    // Canonicalize and Serialize
    return planning::insert(context.destination(), *ground_literal);
}

inline urd::data<FDRFact<FluentTag>> ground(LiteralView<LiftedTag, FluentTag> element, WriteContext<Builder, Repository>& context, FDRContext& fdr)
{
    auto fact = ground(element.get_atom(), context, fdr);
    if (!element.get_polarity())
        fact.value = FDRValue::none();

    return fact;
}

inline std::pair<ConjunctiveConditionView<GroundTag>, bool>
ground(ConjunctiveConditionView<LiftedTag> element, WriteContext<Builder, Repository>& context, FDRContext& fdr)
{
    // Fetch and clear
    auto conj_cond = planning::checkout<ConjunctiveCondition<GroundTag>>(context.builder());

    // Fill data
    for (const auto literal : element.template get_literals<StaticTag>())
        conj_cond->static_literals.push_back(ground(literal, context).first.index());
    for (const auto literal : element.template get_literals<FluentTag>())
    {
        if (literal.get_polarity())
            conj_cond->positive_facts.push_back(ground(literal.get_atom(), context, fdr));
        else
            conj_cond->negative_facts.push_back(ground(literal.get_atom(), context, fdr));
    }
    for (const auto literal : element.template get_literals<DerivedTag>())
        conj_cond->derived_literals.push_back(ground(literal, context).first.index());
    for (const auto numeric_constraint : element.get_numeric_constraints())
        conj_cond->numeric_constraints.push_back(ground(numeric_constraint, context).first.index());

    // Canonicalize and Serialize
    return planning::insert(context.destination(), *conj_cond);
}

template<FactKind T>
std::pair<NumericEffectView<GroundTag, T>, bool> ground(NumericEffectView<LiftedTag, T> element, WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto numeric_effect = planning::checkout<NumericEffect<GroundTag, T>>(context.builder());

    // Fill data
    numeric_effect->operator_kind = element.get_operator();
    numeric_effect->fterm = ground(element.get_fterm(), context).first.index();
    numeric_effect->fexpr = urd::to_stored(ground(element.get_fexpr(), context));

    // Canonicalize and Serialize
    return planning::insert(context.destination(), *numeric_effect);
}

inline std::pair<ConjunctiveEffectView<GroundTag>, bool>
ground(ConjunctiveEffectView<LiftedTag> element, WriteContext<Builder, Repository>& context, FDRContext& fdr)
{
    // Fetch and clear
    auto conj_eff = planning::checkout<ConjunctiveEffect<GroundTag>>(context.builder());

    for (const auto literal : element.get_literals())
    {
        const auto new_fact = ground(literal.get_atom(), context, fdr);
        if (literal.get_polarity())
            conj_eff->add_facts.push_back(new_fact);
        else
            conj_eff->del_facts.push_back(new_fact);
    }
    for (const auto numeric_effect : element.get_numeric_effects())
        conj_eff->numeric_effects.push_back(ground(numeric_effect, context).first.index());
    if (element.get_auxiliary_numeric_effect().has_value())
        conj_eff->auxiliary_numeric_effect = ground(element.get_auxiliary_numeric_effect().value(), context).first.index();

    // Canonicalize and Serialize
    return planning::insert(context.destination(), *conj_eff);
}

inline std::pair<ConditionalEffectView<GroundTag>, bool>
ground(ConditionalEffectView<LiftedTag> element, WriteContext<Builder, Repository>& context, FDRContext& fdr)
{
    // Fetch and clear
    auto cond_effect = planning::checkout<ConditionalEffect<GroundTag>>(context.builder());

    // Fill data
    cond_effect->condition = ground(element.get_condition(), context, fdr).first.index();
    cond_effect->effect = ground(element.get_effect(), context, fdr).first.index();

    // Canonicalize and Serialize
    return planning::insert(context.destination(), *cond_effect);
}

inline std::pair<ActionRowView, bool> ground(ActionView<LiftedTag> action, WriteContext<Builder, Repository>& context)
{
    auto binding = planning::checkout<ygg::Row<Action<LiftedTag>>>(context.builder());

    binding->relation = action.index();
    for (ygg::uint_t i = 0; i < action.get_arity(); ++i)
        binding->values.push_back(context.binding()[i].value);

    // Canonicalize and Serialize
    return planning::insert(context.destination(), *binding);
}

template<typename ForEachEffectBinding>
std::pair<ActionView<GroundTag>, bool>
ground(ActionView<LiftedTag> element, WriteContext<Builder, Repository>& context, FDRContext& fdr, ForEachEffectBinding for_each_effect_binding)
{
    const auto binding = ground(element, context).first.data();

    auto action = planning::checkout<Action<GroundTag>>(context.builder());

    action->row = binding;
    action->condition = ground(element.get_condition(), context, fdr).first.index();

    for (const auto cond_effect : element.get_effects())
        for_each_effect_binding(cond_effect, [&] { action->effects.push_back(ground(cond_effect, context, fdr).first.index()); });

    return planning::insert(context.destination(), *action);
}

inline std::pair<AxiomRowView, bool> ground(AxiomView<LiftedTag> axiom, WriteContext<Builder, Repository>& context)
{
    auto binding = planning::checkout<ygg::Row<Axiom<LiftedTag>>>(context.builder());

    binding->relation = axiom.index();
    for (ygg::uint_t i = 0; i < axiom.get_arity(); ++i)
        binding->values.push_back(context.binding()[i].value);

    // Canonicalize and Serialize
    return planning::insert(context.destination(), *binding);
}

inline std::pair<AxiomView<GroundTag>, bool>
ground(AxiomView<LiftedTag> element, WriteContext<Builder, Repository>& context, GrounderCacheEntry<Axiom<LiftedTag>>& cache, FDRContext& fdr)
{
    const auto binding = ground(element, context).first.data();

    auto& axiom_cache = cache.container;
    if (auto it = axiom_cache.find(binding); it != axiom_cache.end())
        return { urd::resolve(it->second, context.destination()), false };

    auto axiom = planning::checkout<Axiom<GroundTag>>(context.builder());

    axiom->row = binding;
    axiom->body = ground(element.get_body(), context, fdr).first.index();
    axiom->head = ground(element.get_head(), context).first.index();

    const auto result = planning::insert(context.destination(), *axiom);

    axiom_cache.emplace(binding, result.first.index());

    return result;
}

/**
 * try_ground
 */

template<FactKind T>
std::optional<FunctionTermView<GroundTag, T>> try_ground(FunctionTermView<LiftedTag, T> element, WriteContext<Builder, Repository>& context)
{
    auto binding = planning::checkout<ygg::Row<Function<T>>>(context.builder());

    binding->relation = element.get_function().index();
    for (const auto term : element.get_terms())
    {
        visit(
            [&]<typename Alternative>(const Alternative& arg)
            {
                if constexpr (std::is_same_v<Alternative, ParameterIndex>)
                    binding->values.push_back(context.binding()[arg.value].value);
                else if constexpr (std::is_same_v<Alternative, ObjectView>)
                    binding->values.push_back(arg.index().value);
                else
                    static_assert(false, "Missing case");
            },
            term);
    }

    const auto binding_or_nullopt = context.destination().find(*binding);
    if (!binding_or_nullopt.has_value())
        return std::nullopt;

    auto fterm = planning::checkout<FunctionTerm<GroundTag, T>>(context.builder());

    fterm->row = binding_or_nullopt->data();

    return context.destination().find(*fterm);
}

template<FactKind T>
std::optional<AtomView<GroundTag, T>> try_ground(AtomView<LiftedTag, T> element, WriteContext<Builder, Repository>& context)
{
    auto binding = planning::checkout<ygg::Row<Predicate<T>>>(context.builder());

    binding->relation = element.get_predicate().index();
    for (const auto term : element.get_terms())
    {
        visit(
            [&]<typename Alternative>(const Alternative& arg)
            {
                if constexpr (std::is_same_v<Alternative, ParameterIndex>)
                    binding->values.push_back(context.binding()[arg.value].value);
                else if constexpr (std::is_same_v<Alternative, ObjectView>)
                    binding->values.push_back(arg.index().value);
                else
                    static_assert(false, "Missing case");
            },
            term);
    }

    const auto binding_or_nullopt = context.destination().find(*binding);
    if (!binding_or_nullopt.has_value())
        return std::nullopt;

    auto atom = planning::checkout<Atom<GroundTag, T>>(context.builder());

    atom->row = binding_or_nullopt->data();

    return context.destination().find(*atom);
}

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::planning
{
extern template std::pair<FunctionRowView<StaticTag>, bool>
ground(TermListView terms, FunctionView<StaticTag> function, WriteContext<Builder, Repository>& context);
extern template std::pair<FunctionRowView<FluentTag>, bool>
ground(TermListView terms, FunctionView<FluentTag> function, WriteContext<Builder, Repository>& context);
extern template std::pair<FunctionRowView<AuxiliaryTag>, bool>
ground(TermListView terms, FunctionView<AuxiliaryTag> function, WriteContext<Builder, Repository>& context);

extern template std::pair<FunctionTermView<GroundTag, StaticTag>, bool> ground(FunctionTermView<LiftedTag, StaticTag> element,
                                                                               WriteContext<Builder, Repository>& context);
extern template std::pair<FunctionTermView<GroundTag, FluentTag>, bool> ground(FunctionTermView<LiftedTag, FluentTag> element,
                                                                               WriteContext<Builder, Repository>& context);
extern template std::pair<FunctionTermView<GroundTag, AuxiliaryTag>, bool> ground(FunctionTermView<LiftedTag, AuxiliaryTag> element,
                                                                                  WriteContext<Builder, Repository>& context);

extern template std::pair<BinaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool> ground(BinaryOperatorView<LiftedTag, ArithmeticOperatorKind> element,
                                                                                              WriteContext<Builder, Repository>& context);
extern template std::pair<BinaryOperatorView<GroundTag, BooleanOperatorKind>, bool> ground(BinaryOperatorView<LiftedTag, BooleanOperatorKind> element,
                                                                                           WriteContext<Builder, Repository>& context);

extern template std::pair<PredicateRowView<StaticTag>, bool>
ground(TermListView terms, PredicateView<StaticTag> predicate, WriteContext<Builder, Repository>& context);
extern template std::pair<PredicateRowView<FluentTag>, bool>
ground(TermListView terms, PredicateView<FluentTag> predicate, WriteContext<Builder, Repository>& context);
extern template std::pair<PredicateRowView<DerivedTag>, bool>
ground(TermListView terms, PredicateView<DerivedTag> predicate, WriteContext<Builder, Repository>& context);

extern template std::pair<AtomView<GroundTag, StaticTag>, bool> ground(AtomView<LiftedTag, StaticTag> element,
                                                                       WriteContext<Builder, Repository>& grounder_context);
extern template std::pair<AtomView<GroundTag, FluentTag>, bool> ground(AtomView<LiftedTag, FluentTag> element,
                                                                       WriteContext<Builder, Repository>& grounder_context);
extern template std::pair<AtomView<GroundTag, DerivedTag>, bool> ground(AtomView<LiftedTag, DerivedTag> element,
                                                                        WriteContext<Builder, Repository>& grounder_context);

extern template std::pair<LiteralView<GroundTag, StaticTag>, bool> ground(LiteralView<LiftedTag, StaticTag> element,
                                                                          WriteContext<Builder, Repository>& context);
extern template std::pair<LiteralView<GroundTag, FluentTag>, bool> ground(LiteralView<LiftedTag, FluentTag> element,
                                                                          WriteContext<Builder, Repository>& context);
extern template std::pair<LiteralView<GroundTag, DerivedTag>, bool> ground(LiteralView<LiftedTag, DerivedTag> element,
                                                                           WriteContext<Builder, Repository>& context);

extern template std::pair<NumericEffectView<GroundTag, FluentTag>, bool> ground(NumericEffectView<LiftedTag, FluentTag> element,
                                                                                WriteContext<Builder, Repository>& context);
extern template std::pair<NumericEffectView<GroundTag, AuxiliaryTag>, bool> ground(NumericEffectView<LiftedTag, AuxiliaryTag> element,
                                                                                   WriteContext<Builder, Repository>& context);

extern template std::optional<FunctionTermView<GroundTag, StaticTag>> try_ground(FunctionTermView<LiftedTag, StaticTag> element,
                                                                                 WriteContext<Builder, Repository>& context);
extern template std::optional<FunctionTermView<GroundTag, FluentTag>> try_ground(FunctionTermView<LiftedTag, FluentTag> element,
                                                                                 WriteContext<Builder, Repository>& context);
extern template std::optional<FunctionTermView<GroundTag, AuxiliaryTag>> try_ground(FunctionTermView<LiftedTag, AuxiliaryTag> element,
                                                                                    WriteContext<Builder, Repository>& context);

extern template std::optional<AtomView<GroundTag, StaticTag>> try_ground(AtomView<LiftedTag, StaticTag> element, WriteContext<Builder, Repository>& context);
extern template std::optional<AtomView<GroundTag, FluentTag>> try_ground(AtomView<LiftedTag, FluentTag> element, WriteContext<Builder, Repository>& context);
extern template std::optional<AtomView<GroundTag, DerivedTag>> try_ground(AtomView<LiftedTag, DerivedTag> element, WriteContext<Builder, Repository>& context);
}

#endif

#endif
