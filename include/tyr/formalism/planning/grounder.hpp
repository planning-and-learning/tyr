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

#include "tyr/analysis/domains.hpp"
#include "tyr/formalism/planning/canonicalization.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/grounder_decl.hpp"
#include "tyr/formalism/planning/merge.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"

#include <yggdrasil/containers/tuple.hpp>

namespace tyr::formalism::planning
{

/**
 * ground
 */

template<FactKind T>
std::pair<FunctionBindingView<T>, bool> ground(TermListView terms, FunctionView<T> function, GrounderContext& context);

template<FactKind T>
std::pair<FunctionTermView<GroundTag, T>, bool> ground(FunctionTermView<LiftedTag, T> element, GrounderContext& context);

FunctionExpressionView<GroundTag> ground(FunctionExpressionView<LiftedTag> element, GrounderContext& context);

std::pair<UnaryOperatorView<GroundTag>, bool> ground(UnaryOperatorView<LiftedTag> element, GrounderContext& context);

template<BinaryOperatorKind O>
std::pair<BinaryOperatorView<GroundTag, O>, bool> ground(BinaryOperatorView<LiftedTag, O> element, GrounderContext& context);

std::pair<MultiOperatorView<GroundTag>, bool> ground(MultiOperatorView<LiftedTag> element, GrounderContext& context);

BooleanOperatorView<GroundTag> ground(BooleanOperatorView<LiftedTag> element, GrounderContext& context);

ArithmeticOperatorView<GroundTag> ground(ArithmeticOperatorView<LiftedTag> element, GrounderContext& context);

template<FactKind T>
std::pair<PredicateBindingView<T>, bool> ground(TermListView terms, PredicateView<T> predicate, GrounderContext& context);

template<FactKind T>
std::pair<AtomView<GroundTag, T>, bool> ground(AtomView<LiftedTag, T> element, GrounderContext& context);

ygg::Data<FDRFact<FluentTag>> ground(AtomView<LiftedTag, FluentTag> element, GrounderContext& context, FDRContext& fdr);

template<FactKind T>
std::pair<LiteralView<GroundTag, T>, bool> ground(LiteralView<LiftedTag, T> element, GrounderContext& context);

ygg::Data<FDRFact<FluentTag>> ground(LiteralView<LiftedTag, FluentTag> element, GrounderContext& context, FDRContext& fdr);

std::pair<ConjunctiveConditionView<GroundTag>, bool> ground(ConjunctiveConditionView<LiftedTag> element, GrounderContext& context, FDRContext& fdr);

template<FactKind T>
std::pair<NumericEffectView<GroundTag, T>, bool> ground(NumericEffectView<LiftedTag, T> element, GrounderContext& context);

template<FactKind T>
NumericEffectOperatorView<GroundTag, T> ground(NumericEffectOperatorView<LiftedTag, T> element, GrounderContext& context);

std::pair<ConjunctiveEffectView<GroundTag>, bool> ground(ConjunctiveEffectView<LiftedTag> element, GrounderContext& context, FDRContext& fdr);

std::pair<ConditionalEffectView<GroundTag>, bool> ground(ConditionalEffectView<LiftedTag> element, GrounderContext& context, FDRContext& fdr);

std::pair<ActionBindingView, bool> ground(ActionView<LiftedTag> action, GrounderContext& context);

std::pair<ActionView<GroundTag>, bool> ground(ActionView<LiftedTag> element,
                                              GrounderContext& context,
                                              const analysis::ActionDomain& action_domains,
                                              analysis::CompatibilityWorkspace& compatibility_workspace,
                                              FDRContext& fdr);

std::pair<AxiomBindingView, bool> ground(AxiomView<LiftedTag> axiom, GrounderContext& context);

std::pair<AxiomView<GroundTag>, bool>
ground(AxiomView<LiftedTag> element, GrounderContext& context, GrounderCacheEntry<Axiom<LiftedTag>>& cache, FDRContext& fdr);

/**
 * try_ground
 */

template<FactKind T>
std::optional<FunctionTermView<GroundTag, T>> try_ground(FunctionTermView<LiftedTag, T> element, GrounderContext& context);

template<FactKind T>
std::optional<AtomView<GroundTag, T>> try_ground(AtomView<LiftedTag, T> element, GrounderContext& context);

/**
 * ground
 */

template<FactKind T>
std::pair<FunctionBindingView<T>, bool> ground(TermListView terms, FunctionView<T> function, GrounderContext& context)
{
    auto binding = planning::checkout<RelationBinding<Function<T>>>(context.builder);

    binding->relation = function.get_index();
    for (const auto term : terms)
    {
        visit(
            [&](auto&& arg)
            {
                using Alternative = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<Alternative, ParameterIndex>)
                    binding->objects.push_back(context.binding[ygg::uint_t(arg)]);
                else if constexpr (std::is_same_v<Alternative, ObjectView>)
                    binding->objects.push_back(arg.get_index());
                else
                    static_assert(ygg::dependent_false<Alternative>::value, "Missing case");
            },
            term.get_variant());
    }

    // Canonicalize and Serialize
    return planning::get_or_create(context.destination, *binding);
}

template<FactKind T>
std::pair<FunctionTermView<GroundTag, T>, bool> ground(FunctionTermView<LiftedTag, T> element, GrounderContext& context)
{
    // Fetch and clear
    auto fterm = planning::checkout<FunctionTerm<GroundTag, T>>(context.builder);

    // Fill data
    fterm->binding = ground(element.get_terms(), element.get_function(), context).first.get_index();

    // Canonicalize and Serialize
    return planning::get_or_create(context.destination, *fterm);
}

inline FunctionExpressionView<GroundTag> ground(FunctionExpressionView<LiftedTag> element, GrounderContext& context)
{
    const auto data = visit(
        [&](auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return ygg::Data<FunctionExpression<GroundTag>>(arg);
            else if constexpr (std::is_same_v<Alternative, ArithmeticOperatorView<LiftedTag>>)
                return ygg::Data<FunctionExpression<GroundTag>>(ground(arg, context).get_data());
            else
                return ygg::Data<FunctionExpression<GroundTag>>(ground(arg, context).first.get_index());
        },
        element.get_variant());
    return ygg::make_view(data, context.destination);
}

inline std::pair<UnaryOperatorView<GroundTag>, bool> ground(UnaryOperatorView<LiftedTag> element, GrounderContext& context)
{
    // Fetch and clear
    auto unary = planning::checkout<UnaryOperator<GroundTag>>(context.builder);

    // Fill data
    unary->operator_kind = element.get_operator();
    unary->arg = ground(element.get_arg(), context).get_data();

    // Canonicalize and Serialize
    return planning::get_or_create(context.destination, *unary);
}

template<BinaryOperatorKind O>
std::pair<BinaryOperatorView<GroundTag, O>, bool> ground(BinaryOperatorView<LiftedTag, O> element, GrounderContext& context)
{
    // Fetch and clear
    auto binary = planning::checkout<BinaryOperator<GroundTag, O>>(context.builder);

    // Fill data
    binary->operator_kind = element.get_operator();
    binary->lhs = ground(element.get_lhs(), context).get_data();
    binary->rhs = ground(element.get_rhs(), context).get_data();

    // Canonicalize and Serialize
    return planning::get_or_create(context.destination, *binary);
}

inline std::pair<MultiOperatorView<GroundTag>, bool> ground(MultiOperatorView<LiftedTag> element, GrounderContext& context)
{
    // Fetch and clear
    auto multi = planning::checkout<MultiOperator<GroundTag>>(context.builder);

    // Fill data
    multi->operator_kind = element.get_operator();
    for (const auto arg : element.get_args())
        multi->args.push_back(ground(arg, context).get_data());

    // Canonicalize and Serialize
    return planning::get_or_create(context.destination, *multi);
}

inline BooleanOperatorView<GroundTag> ground(BooleanOperatorView<LiftedTag> element, GrounderContext& context)
{
    const auto data = visit([&](auto&& arg) { return ygg::Data<BooleanOperator<GroundTag>>(arg.get_operator(), ground(arg, context).first.get_index()); },
                            element.get_variant());
    return ygg::make_view(data, context.destination);
}

inline ArithmeticOperatorView<GroundTag> ground(ArithmeticOperatorView<LiftedTag> element, GrounderContext& context)
{
    const auto data = visit([&](auto&& arg) { return ygg::Data<ArithmeticOperator<GroundTag>>(arg.get_operator(), ground(arg, context).first.get_index()); },
                            element.get_variant());
    return ygg::make_view(data, context.destination);
}

template<FactKind T>
std::pair<PredicateBindingView<T>, bool> ground(TermListView terms, PredicateView<T> predicate, GrounderContext& context)
{
    auto binding = planning::checkout<RelationBinding<Predicate<T>>>(context.builder);

    binding->relation = predicate.get_index();
    for (const auto term : terms)
    {
        visit(
            [&](auto&& arg)
            {
                using Alternative = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<Alternative, ParameterIndex>)
                    binding->objects.push_back(context.binding[ygg::uint_t(arg)]);
                else if constexpr (std::is_same_v<Alternative, ObjectView>)
                    binding->objects.push_back(arg.get_index());
                else
                    static_assert(ygg::dependent_false<Alternative>::value, "Missing case");
            },
            term.get_variant());
    }

    // Canonicalize and Serialize
    return planning::get_or_create(context.destination, *binding);
}

template<FactKind T>
std::pair<AtomView<GroundTag, T>, bool> ground(AtomView<LiftedTag, T> element, GrounderContext& context)
{
    // Fetch and clear
    auto atom = planning::checkout<Atom<GroundTag, T>>(context.builder);

    // Fill data
    atom->binding = ground(element.get_terms(), element.get_predicate(), context).first.get_index();

    // Canonicalize and Serialize
    return planning::get_or_create(context.destination, *atom);
}

inline ygg::Data<FDRFact<FluentTag>> ground(AtomView<LiftedTag, FluentTag> element, GrounderContext& context, FDRContext& fdr)
{
    return fdr.get_fact(ground(element, context).first).get_data();
}

template<FactKind T>
std::pair<LiteralView<GroundTag, T>, bool> ground(LiteralView<LiftedTag, T> element, GrounderContext& context)
{
    // Fetch and clear
    auto ground_literal = planning::checkout<Literal<GroundTag, T>>(context.builder);

    // Fill data
    ground_literal->polarity = element.get_polarity();
    ground_literal->atom = ground(element.get_atom(), context).first.get_index();

    // Canonicalize and Serialize
    return planning::get_or_create(context.destination, *ground_literal);
}

inline ygg::Data<FDRFact<FluentTag>> ground(LiteralView<LiftedTag, FluentTag> element, GrounderContext& context, FDRContext& fdr)
{
    auto fact = ground(element.get_atom(), context, fdr);
    if (!element.get_polarity())
        fact.value = FDRValue::none();

    return fact;
}

inline std::pair<ConjunctiveConditionView<GroundTag>, bool> ground(ConjunctiveConditionView<LiftedTag> element, GrounderContext& context, FDRContext& fdr)
{
    // Fetch and clear
    auto conj_cond = planning::checkout<ConjunctiveCondition<GroundTag>>(context.builder);

    // Fill data
    for (const auto literal : element.template get_literals<StaticTag>())
        conj_cond->static_literals.push_back(ground(literal, context).first.get_index());
    for (const auto literal : element.template get_literals<FluentTag>())
    {
        if (literal.get_polarity())
            conj_cond->positive_facts.push_back(ground(literal.get_atom(), context, fdr));
        else
            conj_cond->negative_facts.push_back(ground(literal.get_atom(), context, fdr));
    }
    for (const auto literal : element.template get_literals<DerivedTag>())
        conj_cond->derived_literals.push_back(ground(literal, context).first.get_index());
    for (const auto numeric_constraint : element.get_numeric_constraints())
        conj_cond->numeric_constraints.push_back(ground(numeric_constraint, context).get_data());

    // Canonicalize and Serialize
    return planning::get_or_create(context.destination, *conj_cond);
}

template<FactKind T>
std::pair<NumericEffectView<GroundTag, T>, bool> ground(NumericEffectView<LiftedTag, T> element, GrounderContext& context)
{
    // Fetch and clear
    auto numeric_effect = planning::checkout<NumericEffect<GroundTag, T>>(context.builder);

    // Fill data
    numeric_effect->operator_kind = element.get_operator();
    numeric_effect->fterm = ground(element.get_fterm(), context).first.get_index();
    numeric_effect->fexpr = ground(element.get_fexpr(), context).get_data();

    // Canonicalize and Serialize
    return planning::get_or_create(context.destination, *numeric_effect);
}

template<FactKind T>
NumericEffectOperatorView<GroundTag, T> ground(NumericEffectOperatorView<LiftedTag, T> element, GrounderContext& context)
{
    const auto data =
        visit([&](auto&& arg) { return ygg::Data<NumericEffectOperator<GroundTag, T>>(arg.get_operator(), ground(arg, context).first.get_index()); },
              element.get_variant());
    return ygg::make_view(data, context.destination);
}

inline std::pair<ConjunctiveEffectView<GroundTag>, bool> ground(ConjunctiveEffectView<LiftedTag> element, GrounderContext& context, FDRContext& fdr)
{
    // Fetch and clear
    auto conj_eff = planning::checkout<ConjunctiveEffect<GroundTag>>(context.builder);

    for (const auto literal : element.get_literals())
    {
        const auto new_fact = ground(literal.get_atom(), context, fdr);
        if (literal.get_polarity())
            conj_eff->add_facts.push_back(new_fact);
        else
            conj_eff->del_facts.push_back(new_fact);
    }
    for (const auto numeric_effect : element.get_numeric_effects())
        conj_eff->numeric_effects.push_back(ground(numeric_effect, context).get_data());
    if (element.get_auxiliary_numeric_effect().has_value())
        conj_eff->auxiliary_numeric_effect = ground(element.get_auxiliary_numeric_effect().value(), context).get_data();

    // Canonicalize and Serialize
    return planning::get_or_create(context.destination, *conj_eff);
}

inline std::pair<ConditionalEffectView<GroundTag>, bool> ground(ConditionalEffectView<LiftedTag> element, GrounderContext& context, FDRContext& fdr)
{
    // Fetch and clear
    auto cond_effect = planning::checkout<ConditionalEffect<GroundTag>>(context.builder);

    // Fill data
    cond_effect->condition = ground(element.get_condition(), context, fdr).first.get_index();
    cond_effect->effect = ground(element.get_effect(), context, fdr).first.get_index();

    // Canonicalize and Serialize
    return planning::get_or_create(context.destination, *cond_effect);
}

inline std::pair<ActionBindingView, bool> ground(ActionView<LiftedTag> action, GrounderContext& context)
{
    auto binding = planning::checkout<RelationBinding<Action<LiftedTag>>>(context.builder);

    binding->relation = action.get_index();
    for (ygg::uint_t i = 0; i < action.get_arity(); ++i)
        binding->objects.push_back(context.binding[i]);

    // Canonicalize and Serialize
    return planning::get_or_create(context.destination, *binding);
}

inline std::pair<ActionView<GroundTag>, bool> ground(ActionView<LiftedTag> element,
                                                     GrounderContext& context,
                                                     const analysis::ActionDomain& action_domains,
                                                     analysis::CompatibilityWorkspace& compatibility_workspace,
                                                     FDRContext& fdr)
{
    const auto binding = ground(element, context).first.get_index();

    auto action = planning::checkout<Action<GroundTag>>(context.builder);

    action->binding = binding;
    action->condition = ground(element.get_condition(), context, fdr).first.get_index();

    const auto binding_size = context.binding.size();

    for (ygg::uint_t cond_effect_index = 0; cond_effect_index < element.get_effects().size(); ++cond_effect_index)
    {
        const auto cond_effect = element.get_effects()[cond_effect_index];
        const auto& effect_domain = action_domains.payload.effect_domains.at(cond_effect.get_index()).payload;
        context.binding.resize(binding_size);
        const auto prefix = std::span<const ygg::Index<Object>>(context.binding.data(), element.get_arity());

        analysis::for_each_compatible_extension(effect_domain,
                                                prefix,
                                                compatibility_workspace,
                                                [&](auto extension)
                                                {
                                                    context.binding.resize(binding_size);
                                                    context.binding.insert(context.binding.end(), extension.begin(), extension.end());
                                                    action->effects.push_back(ground(cond_effect, context, fdr).first.get_index());
                                                });
    }

    context.binding.resize(binding_size);

    return planning::get_or_create(context.destination, *action);
}

inline std::pair<AxiomBindingView, bool> ground(AxiomView<LiftedTag> axiom, GrounderContext& context)
{
    auto binding = planning::checkout<RelationBinding<Axiom<LiftedTag>>>(context.builder);

    binding->relation = axiom.get_index();
    for (ygg::uint_t i = 0; i < axiom.get_arity(); ++i)
        binding->objects.push_back(context.binding[i]);

    // Canonicalize and Serialize
    return planning::get_or_create(context.destination, *binding);
}

inline std::pair<AxiomView<GroundTag>, bool>
ground(AxiomView<LiftedTag> element, GrounderContext& context, GrounderCacheEntry<Axiom<LiftedTag>>& cache, FDRContext& fdr)
{
    const auto binding = ground(element, context).first.get_index();

    auto& axiom_cache = cache.container;
    if (auto it = axiom_cache.find(binding); it != axiom_cache.end())
        return { ygg::make_view(it->second, context.destination), false };

    auto axiom = planning::checkout<Axiom<GroundTag>>(context.builder);

    axiom->binding = binding;
    axiom->body = ground(element.get_body(), context, fdr).first.get_index();
    axiom->head = ground(element.get_head(), context).first.get_index();

    const auto result = planning::get_or_create(context.destination, *axiom);

    axiom_cache.emplace(binding, result.first.get_index());

    return result;
}

/**
 * try_ground
 */

template<FactKind T>
std::optional<FunctionTermView<GroundTag, T>> try_ground(FunctionTermView<LiftedTag, T> element, GrounderContext& context)
{
    auto binding = planning::checkout<RelationBinding<Function<T>>>(context.builder);

    binding->relation = element.get_function().get_index();
    for (const auto term : element.get_terms())
    {
        visit(
            [&](auto&& arg)
            {
                using Alternative = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<Alternative, ParameterIndex>)
                    binding->objects.push_back(context.binding[ygg::uint_t(arg)]);
                else if constexpr (std::is_same_v<Alternative, ObjectView>)
                    binding->objects.push_back(arg.get_index());
                else
                    static_assert(ygg::dependent_false<Alternative>::value, "Missing case");
            },
            term.get_variant());
    }

    canonicalize(*binding);
    const auto binding_or_nullopt = context.destination.find(*binding);
    if (!binding_or_nullopt.has_value())
        return std::nullopt;

    auto fterm = planning::checkout<FunctionTerm<GroundTag, T>>(context.builder);

    fterm->binding = binding_or_nullopt->get_index();

    return context.destination.find(*fterm);
}

template<FactKind T>
std::optional<AtomView<GroundTag, T>> try_ground(AtomView<LiftedTag, T> element, GrounderContext& context)
{
    auto binding = planning::checkout<RelationBinding<Predicate<T>>>(context.builder);

    binding->relation = element.get_predicate().get_index();
    for (const auto term : element.get_terms())
    {
        visit(
            [&](auto&& arg)
            {
                using Alternative = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<Alternative, ParameterIndex>)
                    binding->objects.push_back(context.binding[ygg::uint_t(arg)]);
                else if constexpr (std::is_same_v<Alternative, ObjectView>)
                    binding->objects.push_back(arg.get_index());
                else
                    static_assert(ygg::dependent_false<Alternative>::value, "Missing case");
            },
            term.get_variant());
    }

    canonicalize(*binding);
    const auto binding_or_nullopt = context.destination.find(*binding);
    if (!binding_or_nullopt.has_value())
        return std::nullopt;

    auto atom = planning::checkout<Atom<GroundTag, T>>(context.builder);

    atom->binding = binding_or_nullopt->get_index();

    return context.destination.find(*atom);
}

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::planning
{
extern template std::pair<FunctionBindingView<StaticTag>, bool> ground(TermListView terms, FunctionView<StaticTag> function, GrounderContext& context);
extern template std::pair<FunctionBindingView<FluentTag>, bool> ground(TermListView terms, FunctionView<FluentTag> function, GrounderContext& context);
extern template std::pair<FunctionBindingView<AuxiliaryTag>, bool> ground(TermListView terms, FunctionView<AuxiliaryTag> function, GrounderContext& context);

extern template std::pair<FunctionTermView<GroundTag, StaticTag>, bool> ground(FunctionTermView<LiftedTag, StaticTag> element, GrounderContext& context);
extern template std::pair<FunctionTermView<GroundTag, FluentTag>, bool> ground(FunctionTermView<LiftedTag, FluentTag> element, GrounderContext& context);
extern template std::pair<FunctionTermView<GroundTag, AuxiliaryTag>, bool> ground(FunctionTermView<LiftedTag, AuxiliaryTag> element, GrounderContext& context);

extern template std::pair<BinaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool> ground(BinaryOperatorView<LiftedTag, ArithmeticOperatorKind> element,
                                                                                              GrounderContext& context);
extern template std::pair<BinaryOperatorView<GroundTag, BooleanOperatorKind>, bool> ground(BinaryOperatorView<LiftedTag, BooleanOperatorKind> element,
                                                                                           GrounderContext& context);

extern template std::pair<PredicateBindingView<StaticTag>, bool> ground(TermListView terms, PredicateView<StaticTag> predicate, GrounderContext& context);
extern template std::pair<PredicateBindingView<FluentTag>, bool> ground(TermListView terms, PredicateView<FluentTag> predicate, GrounderContext& context);
extern template std::pair<PredicateBindingView<DerivedTag>, bool> ground(TermListView terms, PredicateView<DerivedTag> predicate, GrounderContext& context);

extern template std::pair<AtomView<GroundTag, StaticTag>, bool> ground(AtomView<LiftedTag, StaticTag> element, GrounderContext& grounder_context);
extern template std::pair<AtomView<GroundTag, FluentTag>, bool> ground(AtomView<LiftedTag, FluentTag> element, GrounderContext& grounder_context);
extern template std::pair<AtomView<GroundTag, DerivedTag>, bool> ground(AtomView<LiftedTag, DerivedTag> element, GrounderContext& grounder_context);

extern template std::pair<LiteralView<GroundTag, StaticTag>, bool> ground(LiteralView<LiftedTag, StaticTag> element, GrounderContext& context);
extern template std::pair<LiteralView<GroundTag, FluentTag>, bool> ground(LiteralView<LiftedTag, FluentTag> element, GrounderContext& context);
extern template std::pair<LiteralView<GroundTag, DerivedTag>, bool> ground(LiteralView<LiftedTag, DerivedTag> element, GrounderContext& context);

extern template std::pair<NumericEffectView<GroundTag, FluentTag>, bool> ground(NumericEffectView<LiftedTag, FluentTag> element, GrounderContext& context);
extern template std::pair<NumericEffectView<GroundTag, AuxiliaryTag>, bool> ground(NumericEffectView<LiftedTag, AuxiliaryTag> element,
                                                                                   GrounderContext& context);

extern template NumericEffectOperatorView<GroundTag, FluentTag> ground(NumericEffectOperatorView<LiftedTag, FluentTag> element, GrounderContext& context);
extern template NumericEffectOperatorView<GroundTag, AuxiliaryTag> ground(NumericEffectOperatorView<LiftedTag, AuxiliaryTag> element, GrounderContext& context);

extern template std::optional<FunctionTermView<GroundTag, StaticTag>> try_ground(FunctionTermView<LiftedTag, StaticTag> element, GrounderContext& context);
extern template std::optional<FunctionTermView<GroundTag, FluentTag>> try_ground(FunctionTermView<LiftedTag, FluentTag> element, GrounderContext& context);
extern template std::optional<FunctionTermView<GroundTag, AuxiliaryTag>> try_ground(FunctionTermView<LiftedTag, AuxiliaryTag> element,
                                                                                    GrounderContext& context);

extern template std::optional<AtomView<GroundTag, StaticTag>> try_ground(AtomView<LiftedTag, StaticTag> element, GrounderContext& context);
extern template std::optional<AtomView<GroundTag, FluentTag>> try_ground(AtomView<LiftedTag, FluentTag> element, GrounderContext& context);
extern template std::optional<AtomView<GroundTag, DerivedTag>> try_ground(AtomView<LiftedTag, DerivedTag> element, GrounderContext& context);
}

#endif

#endif
