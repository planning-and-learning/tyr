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
#include "tyr/formalism/conjunctive_condition_view.hpp"
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

namespace tyr::formalism
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
    auto binding = checkout<ygg::Row<Function<T>>>(context.builder());

    urd::store(binding->relation, schema_in_destination(function, context));
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
    return context.destination().insert(*binding, context.builder());
}

template<FactKind T>
std::pair<FunctionTermView<GroundTag, T>, bool> ground(FunctionTermView<LiftedTag, T> element, WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto fterm = checkout<FunctionTerm<GroundTag, T>>(context.builder());

    // Fill data
    urd::store(fterm->row, ground(element.get_terms(), element.get_function(), context).first);

    // Canonicalize and Serialize
    return context.destination().insert(*fterm, context.builder());
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
    auto unary = checkout<UnaryOperator<GroundTag, ArithmeticOperatorKind>>(context.builder());

    // Fill data
    unary->operation = element.get_operation();
    urd::store(unary->arg, ground(element.get_arg(), context));

    // Canonicalize and Serialize
    return context.destination().insert(*unary, context.builder());
}

template<OperatorKind O>
std::pair<BinaryOperatorView<GroundTag, O>, bool> ground(BinaryOperatorView<LiftedTag, O> element, WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto binary = checkout<BinaryOperator<GroundTag, O>>(context.builder());

    // Fill data
    binary->operation = element.get_operation();
    urd::store(binary->lhs, ground(element.get_lhs(), context));
    urd::store(binary->rhs, ground(element.get_rhs(), context));

    // Canonicalize and Serialize
    return context.destination().insert(*binary, context.builder());
}

inline std::pair<MultiOperatorView<GroundTag, ArithmeticOperatorKind>, bool> ground(MultiOperatorView<LiftedTag, ArithmeticOperatorKind> element,
                                                                                    WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto multi = checkout<MultiOperator<GroundTag, ArithmeticOperatorKind>>(context.builder());

    // Fill data
    multi->operation = element.get_operation();
    for (const auto arg : element.get_args())
        urd::append(multi->args, ground(arg, context));

    // Canonicalize and Serialize
    return context.destination().insert(*multi, context.builder());
}

template<FactKind T>
std::pair<PredicateRowView<T>, bool> ground(TermListView terms, PredicateView<T> predicate, WriteContext<Builder, Repository>& context)
{
    auto binding = checkout<ygg::Row<Predicate<T>>>(context.builder());

    urd::store(binding->relation, schema_in_destination(predicate, context));
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
    return context.destination().insert(*binding, context.builder());
}

template<FactKind T>
std::pair<AtomView<GroundTag, T>, bool> ground(AtomView<LiftedTag, T> element, WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto atom = checkout<Atom<GroundTag, T>>(context.builder());

    // Fill data
    urd::store(atom->row, ground(element.get_terms(), element.get_predicate(), context).first);

    // Canonicalize and Serialize
    return context.destination().insert(*atom, context.builder());
}

inline urd::data<FDRFact<FluentTag>> ground(AtomView<LiftedTag, FluentTag> element, WriteContext<Builder, Repository>& context, FDRContext& fdr)
{
    const auto fact = fdr.get_fact(ground(element, context).first);
    return urd::make<FDRFact<FluentTag>>(fact.get_variable(), fact.get_value());
}

template<FactKind T>
std::pair<LiteralView<GroundTag, T>, bool> ground(LiteralView<LiftedTag, T> element, WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto ground_literal = checkout<Literal<GroundTag, T>>(context.builder());

    // Fill data
    ground_literal->polarity = element.get_polarity();
    urd::store(ground_literal->atom, ground(element.get_atom(), context).first);

    // Canonicalize and Serialize
    return context.destination().insert(*ground_literal, context.builder());
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
    auto conj_cond = checkout<ConjunctiveCondition<GroundTag>>(context.builder());

    // Fill data
    for (const auto literal : element.template get_literals<StaticTag>())
        urd::append(conj_cond->static_literals, ground(literal, context).first);
    for (const auto literal : element.template get_literals<FluentTag>())
    {
        if (literal.get_polarity())
            urd::build_member(conj_cond->positive_facts.emplace_back(), ground(literal.get_atom(), context, fdr), context.destination());
        else
            urd::build_member(conj_cond->negative_facts.emplace_back(), ground(literal.get_atom(), context, fdr), context.destination());
    }
    for (const auto literal : element.template get_literals<DerivedTag>())
        urd::append(conj_cond->derived_literals, ground(literal, context).first);
    for (const auto numeric_constraint : element.get_numeric_constraints())
        urd::append(conj_cond->numeric_constraints, ground(numeric_constraint, context).first);

    // Canonicalize and Serialize
    return context.destination().insert(*conj_cond, context.builder());
}

template<FactKind T>
std::pair<NumericEffectView<GroundTag, T>, bool> ground(NumericEffectView<LiftedTag, T> element, WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto numeric_effect = checkout<NumericEffect<GroundTag, T>>(context.builder());

    // Fill data
    numeric_effect->operation = element.get_operation();
    urd::store(numeric_effect->fterm, ground(element.get_fterm(), context).first);
    urd::store(numeric_effect->fexpr, ground(element.get_fexpr(), context));

    // Canonicalize and Serialize
    return context.destination().insert(*numeric_effect, context.builder());
}

inline std::pair<ConjunctiveEffectView<GroundTag>, bool>
ground(ConjunctiveEffectView<LiftedTag> element, WriteContext<Builder, Repository>& context, FDRContext& fdr)
{
    // Fetch and clear
    auto conj_eff = checkout<ConjunctiveEffect<GroundTag>>(context.builder());

    for (const auto literal : element.get_literals())
    {
        const auto new_fact = ground(literal.get_atom(), context, fdr);
        if (literal.get_polarity())
            urd::build_member(conj_eff->add_facts.emplace_back(), new_fact, context.destination());
        else
            urd::build_member(conj_eff->del_facts.emplace_back(), new_fact, context.destination());
    }
    for (const auto numeric_effect : element.get_numeric_effects())
        urd::append(conj_eff->numeric_effects, ground(numeric_effect, context).first);
    if (element.get_auxiliary_numeric_effect().has_value())
        urd::store(conj_eff->auxiliary_numeric_effect, ground(element.get_auxiliary_numeric_effect().value(), context).first);

    // Canonicalize and Serialize
    return context.destination().insert(*conj_eff, context.builder());
}

inline std::pair<ConditionalEffectView<GroundTag>, bool>
ground(ConditionalEffectView<LiftedTag> element, WriteContext<Builder, Repository>& context, FDRContext& fdr)
{
    // Fetch and clear
    auto cond_effect = checkout<ConditionalEffect<GroundTag>>(context.builder());

    // Fill data
    urd::store(cond_effect->condition, ground(element.get_condition(), context, fdr).first);
    urd::store(cond_effect->effect, ground(element.get_effect(), context, fdr).first);

    // Canonicalize and Serialize
    return context.destination().insert(*cond_effect, context.builder());
}

inline std::pair<ActionRowView, bool> ground(ActionView<LiftedTag> action, WriteContext<Builder, Repository>& context)
{
    auto binding = checkout<ygg::Row<Action<LiftedTag>>>(context.builder());

    urd::store(binding->relation, schema_in_destination(action, context));
    for (ygg::uint_t i = 0; i < action.get_arity(); ++i)
        binding->values.push_back(context.binding()[i].value);

    // Canonicalize and Serialize
    return context.destination().insert(*binding, context.builder());
}

template<typename ForEachEffectBinding>
std::pair<ActionView<GroundTag>, bool>
ground(ActionView<LiftedTag> element, WriteContext<Builder, Repository>& context, FDRContext& fdr, ForEachEffectBinding for_each_effect_binding)
{
    const auto binding = ground(element, context).first.index();

    auto action = checkout<Action<GroundTag>>(context.builder());

    action->row = binding;
    urd::store(action->condition, ground(element.get_condition(), context, fdr).first);

    for (const auto cond_effect : element.get_effects())
        for_each_effect_binding(cond_effect, [&] { urd::append(action->effects, ground(cond_effect, context, fdr).first); });

    return context.destination().insert(*action, context.builder());
}

inline std::pair<AxiomRowView, bool> ground(AxiomView<LiftedTag> axiom, WriteContext<Builder, Repository>& context)
{
    auto binding = checkout<ygg::Row<Axiom<LiftedTag>>>(context.builder());

    urd::store(binding->relation, schema_in_destination(axiom, context));
    for (ygg::uint_t i = 0; i < axiom.get_body().get_arity(); ++i)
        binding->values.push_back(context.binding()[i].value);

    // Canonicalize and Serialize
    return context.destination().insert(*binding, context.builder());
}

inline std::pair<AxiomView<GroundTag>, bool>
ground(AxiomView<LiftedTag> element, WriteContext<Builder, Repository>& context, GrounderCacheEntry<Axiom<LiftedTag>>& cache, FDRContext& fdr)
{
    const auto binding = ground(element, context).first.index();

    auto& axiom_cache = cache.container;
    if (auto it = axiom_cache.find(binding); it != axiom_cache.end())
        return { urd::resolve(it->second, context.destination()), false };

    auto axiom = checkout<Axiom<GroundTag>>(context.builder());

    axiom->row = binding;
    urd::store(axiom->body, ground(element.get_body(), context, fdr).first);
    urd::store(axiom->head, ground(element.get_head(), context).first);

    const auto result = context.destination().insert(*axiom, context.builder());

    axiom_cache.emplace(binding, result.first.index());

    return result;
}

/**
 * try_ground
 */

template<FactKind T>
std::optional<FunctionTermView<GroundTag, T>> try_ground(FunctionTermView<LiftedTag, T> element, WriteContext<Builder, Repository>& context)
{
    auto binding = checkout<ygg::Row<Function<T>>>(context.builder());

    urd::store(binding->relation, schema_in_destination(element.get_function(), context));
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

    const auto binding_or_nullopt = context.destination().find(*binding, context.builder());
    if (!binding_or_nullopt.has_value())
        return std::nullopt;

    auto fterm = checkout<FunctionTerm<GroundTag, T>>(context.builder());

    fterm->row = binding_or_nullopt->index();

    return context.destination().find(*fterm, context.builder());
}

template<FactKind T>
std::optional<AtomView<GroundTag, T>> try_ground(AtomView<LiftedTag, T> element, WriteContext<Builder, Repository>& context)
{
    auto binding = checkout<ygg::Row<Predicate<T>>>(context.builder());

    urd::store(binding->relation, schema_in_destination(element.get_predicate(), context));
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

    const auto binding_or_nullopt = context.destination().find(*binding, context.builder());
    if (!binding_or_nullopt.has_value())
        return std::nullopt;

    auto atom = checkout<Atom<GroundTag, T>>(context.builder());

    atom->row = binding_or_nullopt->index();

    return context.destination().find(*atom, context.builder());
}

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism
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
