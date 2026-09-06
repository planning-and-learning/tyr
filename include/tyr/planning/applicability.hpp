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

#ifndef TYR_PLANNING_APPLICABILITY_HPP_
#define TYR_PLANNING_APPLICABILITY_HPP_

#include "tyr/formalism/arithmetic_operator_utils.hpp"
#include "tyr/formalism/boolean_operator_utils.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/numeric_effect_operator_utils.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/applicability_decl.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/state_builder.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/state_builder.hpp"
#include "tyr/planning/lifted/task.hpp"

#include <algorithm>
#include <boost/dynamic_bitset.hpp>
#include <concepts>
#include <iterator>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <vector>
#include <yggdrasil/containers/dynamic_bitset.hpp>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::planning
{

/**
 * evaluate
 */

template<TaskKind Kind>
ygg::float_t evaluate(ygg::float_t element, const StateContext<Kind>& context);

template<TaskKind Kind>
ygg::float_t evaluate(::tyr::formalism::planning::GroundUnaryOperatorView element, const StateContext<Kind>& context);

template<TaskKind Kind>
ygg::float_t evaluate(::tyr::formalism::planning::GroundBinaryOperatorView<::tyr::formalism::ArithmeticOperatorKind> element,
                      const StateContext<Kind>& context);

template<TaskKind Kind>
bool evaluate(::tyr::formalism::planning::GroundBinaryOperatorView<::tyr::formalism::BooleanOperatorKind> element, const StateContext<Kind>& context);

template<TaskKind Kind>
ygg::float_t evaluate(::tyr::formalism::planning::GroundMultiOperatorView element, const StateContext<Kind>& context);

template<TaskKind Kind>
ygg::float_t evaluate(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::StaticTag> element, const StateContext<Kind>& context);

template<TaskKind Kind>
ygg::float_t evaluate(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::FluentTag> element, const StateContext<Kind>& context);

template<TaskKind Kind>
ygg::float_t evaluate(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag> element, const StateContext<Kind>& context);

template<TaskKind Kind>
ygg::float_t evaluate(::tyr::formalism::planning::FunctionExpressionView<::tyr::GroundTag> element, const StateContext<Kind>& context);

template<TaskKind Kind>
ygg::float_t evaluate(::tyr::formalism::planning::GroundArithmeticOperatorView element, const StateContext<Kind>& context);

template<TaskKind Kind>
bool evaluate(::tyr::formalism::planning::GroundBooleanOperatorView element, const StateContext<Kind>& context);

template<TaskKind Kind, ::tyr::formalism::FactKind T>
ygg::float_t evaluate(::tyr::formalism::planning::NumericEffectView<::tyr::GroundTag, T> element, const StateContext<Kind>& context);

template<TaskKind Kind, ::tyr::formalism::FactKind T>
ygg::float_t evaluate(::tyr::formalism::planning::NumericEffectOperatorView<::tyr::GroundTag, T> element, const StateContext<Kind>& context);

/**
 * is_applicable_if_fires
 */

template<TaskKind Kind>
bool is_applicable_if_fires(::tyr::formalism::planning::ConditionalEffectView<::tyr::GroundTag> element,
                            const StateContext<Kind>& context,
                            ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families);

template<TaskKind Kind>
bool is_applicable_if_fires(::tyr::formalism::planning::ConditionalEffectListView<::tyr::GroundTag> elements,
                            const StateContext<Kind>& context,
                            ::tyr::formalism::planning::EffectFamilyList& out_fluent_effect_families);

/**
 * is_applicable
 */

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::LiteralView<::tyr::GroundTag, ::tyr::formalism::StaticTag> element, const StateContext<Kind>& context);

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::LiteralView<::tyr::GroundTag, ::tyr::formalism::DerivedTag> element, const StateContext<Kind>& context);

template<TaskKind Kind, ::tyr::formalism::FactKind T>
bool is_applicable(::tyr::formalism::planning::LiteralListView<::tyr::GroundTag, T> elements, const StateContext<Kind>& context);

template<::tyr::formalism::PolarityKind P, TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::FDRFactView<::tyr::formalism::FluentTag> element, const StateContext<Kind>& context);

template<::tyr::formalism::PolarityKind P, TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::FDRFactListView<::tyr::formalism::FluentTag> elements, const StateContext<Kind>& context);

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::GroundBooleanOperatorView element, const StateContext<Kind>& context);

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::GroundBooleanOperatorListView elements, const StateContext<Kind>& context);

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::NumericEffectView<::tyr::GroundTag, ::tyr::formalism::FluentTag> element,
                   const StateContext<Kind>& context,
                   ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families);

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::NumericEffectOperatorView<::tyr::GroundTag, ::tyr::formalism::FluentTag> element,
                   const StateContext<Kind>& context,
                   ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families);

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::NumericEffectOperatorListView<::tyr::GroundTag, ::tyr::formalism::FluentTag> elements,
                   const StateContext<Kind>& context,
                   ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families);

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::NumericEffectView<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag> element, const StateContext<Kind>& context);

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::NumericEffectOperatorView<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag> element, const StateContext<Kind>& context);

// GroundConjunctiveCondition

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::ConjunctiveConditionView<::tyr::GroundTag> element, const StateContext<Kind>& context);

// GroundConjunctiveEffect

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::ConjunctiveEffectView<::tyr::GroundTag> element,
                   const StateContext<Kind>& context,
                   ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families);

// GroundAction

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::ActionView<::tyr::GroundTag> element,
                   const StateContext<Kind>& context,
                   ::tyr::formalism::planning::EffectFamilyList& out_fluent_effect_families);

// GroundAxiom

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::AxiomView<::tyr::GroundTag> element, const StateContext<Kind>& context);

/**
 * is_statically_applicable
 */

bool is_statically_applicable(::tyr::formalism::planning::LiteralView<::tyr::GroundTag, ::tyr::formalism::StaticTag> element, const boost::dynamic_bitset<>& static_atoms);

bool is_statically_applicable(::tyr::formalism::planning::LiteralListView<::tyr::GroundTag, ::tyr::formalism::StaticTag> elements,
                              const boost::dynamic_bitset<>& static_atoms);

// GroundConjunctiveCondition

bool is_statically_applicable(::tyr::formalism::planning::ConjunctiveConditionView<::tyr::GroundTag> element, const boost::dynamic_bitset<>& static_atoms);

// GroundAction

bool is_statically_applicable(::tyr::formalism::planning::ActionView<::tyr::GroundTag> element, const boost::dynamic_bitset<>& static_atoms);

// GroundAxiom

bool is_statically_applicable(::tyr::formalism::planning::AxiomView<::tyr::GroundTag> element, const boost::dynamic_bitset<>& static_atoms);

/**
 * is_dynamically_applicable
 */

// GroundConjunctiveCondition

template<TaskKind Kind>
bool is_dynamically_applicable(::tyr::formalism::planning::ConjunctiveConditionView<::tyr::GroundTag> element, const StateContext<Kind>& context);

/**
 * is_consistent
 */

// GroundConjunctiveCondition

bool is_consistent(
    ::tyr::formalism::planning::ConjunctiveConditionView<::tyr::GroundTag> element,
    ygg::UnorderedMap<ygg::Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>>, ::tyr::formalism::planning::FDRValue>& fluent_assign,
    ygg::UnorderedMap<ygg::Index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>>, bool>& derived_assign);

// GroundAction

bool is_consistent(::tyr::formalism::planning::ActionView<::tyr::GroundTag> element,
                   ygg::UnorderedMap<ygg::Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>>, ::tyr::formalism::planning::FDRValue>&
                       out_fluent_assign,
                   ygg::UnorderedMap<ygg::Index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>>, bool>& out_derived_assign);

// GroundAxiom

bool is_consistent(::tyr::formalism::planning::AxiomView<::tyr::GroundTag> element,
                   ygg::UnorderedMap<ygg::Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>>, ::tyr::formalism::planning::FDRValue>&
                       out_fluent_assign,
                   ygg::UnorderedMap<ygg::Index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>>, bool>& out_derived_assign);

/**
 * evaluate
 */

template<TaskKind Kind>
ygg::float_t evaluate(ygg::float_t element, const StateContext<Kind>&)
{
    return element;
}

template<TaskKind Kind>
ygg::float_t evaluate(::tyr::formalism::planning::GroundUnaryOperatorView element, const StateContext<Kind>& context)
{
    return ::tyr::formalism::apply(element.get_operator(), evaluate(element.get_arg(), context));
}

template<TaskKind Kind>
ygg::float_t evaluate(::tyr::formalism::planning::GroundBinaryOperatorView<::tyr::formalism::ArithmeticOperatorKind> element, const StateContext<Kind>& context)
{
    return ::tyr::formalism::apply(element.get_operator(), evaluate(element.get_lhs(), context), evaluate(element.get_rhs(), context));
}

template<TaskKind Kind>
bool evaluate(::tyr::formalism::planning::GroundBinaryOperatorView<::tyr::formalism::BooleanOperatorKind> element, const StateContext<Kind>& context)
{
    return ::tyr::formalism::apply(element.get_operator(), evaluate(element.get_lhs(), context), evaluate(element.get_rhs(), context));
}

template<TaskKind Kind>
ygg::float_t evaluate(::tyr::formalism::planning::GroundMultiOperatorView element, const StateContext<Kind>& context)
{
    const auto child_fexprs = element.get_args();

    return std::accumulate(std::next(child_fexprs.begin()),  // Start from the second expression
                           child_fexprs.end(),
                           evaluate(child_fexprs.front(), context),
                           [&](const auto& value, const auto& child_expr)
                           { return ::tyr::formalism::apply(element.get_operator(), value, evaluate(child_expr, context)); });
}

template<TaskKind Kind>
ygg::float_t evaluate(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::StaticTag> element, const StateContext<Kind>& context)
{
    return context.task.get(element.get_index());
}

template<TaskKind Kind>
ygg::float_t evaluate(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::FluentTag> element, const StateContext<Kind>& context)
{
    return context.state_builder.get(element.get_index());
}

template<TaskKind Kind>
ygg::float_t evaluate(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag>, const StateContext<Kind>& context)
{
    return context.auxiliary_value;
}

template<TaskKind Kind>
ygg::float_t evaluate(::tyr::formalism::planning::FunctionExpressionView<::tyr::GroundTag> element, const StateContext<Kind>& context)
{
    return visit([&](auto&& arg) { return evaluate(arg, context); }, element.get_variant());
}

template<TaskKind Kind>
ygg::float_t evaluate(::tyr::formalism::planning::GroundArithmeticOperatorView element, const StateContext<Kind>& context)
{
    return visit([&](auto&& arg) { return evaluate(arg, context); }, element.get_variant());
}

template<TaskKind Kind>
bool evaluate(::tyr::formalism::planning::GroundBooleanOperatorView element, const StateContext<Kind>& context)
{
    return visit([&](auto&& arg) { return evaluate(arg, context); }, element.get_variant());
}

template<TaskKind Kind, ::tyr::formalism::FactKind T>
ygg::float_t evaluate(::tyr::formalism::planning::NumericEffectView<::tyr::GroundTag, T> element, const StateContext<Kind>& context)
{
    return ::tyr::formalism::planning::apply(element.get_operator(), evaluate(element.get_fterm(), context), evaluate(element.get_fexpr(), context));
}

template<TaskKind Kind, ::tyr::formalism::FactKind T>
ygg::float_t evaluate(::tyr::formalism::planning::NumericEffectOperatorView<::tyr::GroundTag, T> element, const StateContext<Kind>& context)
{
    return visit([&](auto&& arg) { return evaluate(arg, context); }, element.get_variant());
}

/**
 * is_applicable_if_fires
 */

template<TaskKind Kind>
bool is_applicable_if_fires(::tyr::formalism::planning::ConditionalEffectView<::tyr::GroundTag> element,
                            const StateContext<Kind>& context,
                            ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families)
{
    const auto effect = element.get_effect();
    if (effect.get_numeric_effects().empty() && !effect.get_auxiliary_numeric_effect().has_value())
        return true;

    if (!is_applicable(element.get_condition(), context))
        return true;

    // Important: only modify effect families if condition is satisfied
    return is_applicable(effect, context, ref_fluent_effect_families);
}

template<TaskKind Kind>
bool is_applicable_if_fires(::tyr::formalism::planning::ConditionalEffectListView<::tyr::GroundTag> elements,
                            const StateContext<Kind>& context,
                            ::tyr::formalism::planning::EffectFamilyList& out_fluent_effect_families)
{
    out_fluent_effect_families.clear();

    return std::all_of(elements.begin(),
                       elements.end(),
                       [&](auto&& cond_effect) { return is_applicable_if_fires(cond_effect, context, out_fluent_effect_families); });
}

/**
 * is_applicable
 */

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::LiteralView<::tyr::GroundTag, ::tyr::formalism::StaticTag> element, const StateContext<Kind>& context)
{
    return context.task.test(element.get_atom().get_index()) == element.get_polarity();
}

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::LiteralView<::tyr::GroundTag, ::tyr::formalism::DerivedTag> element, const StateContext<Kind>& context)
{
    return context.state_builder.test(element.get_atom().get_index()) == element.get_polarity();
}

template<TaskKind Kind, ::tyr::formalism::FactKind T>
bool is_applicable(::tyr::formalism::planning::LiteralListView<::tyr::GroundTag, T> elements, const StateContext<Kind>& context)
{
    return std::all_of(elements.begin(), elements.end(), [&](auto&& arg) { return is_applicable(arg, context); });
}

template<::tyr::formalism::PolarityKind P, TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::FDRFactView<::tyr::formalism::FluentTag> element, const StateContext<Kind>& context)
{
    assert(element.has_value());

    const auto value = context.state_builder.get(element.get_variable().get_index());

    if constexpr (std::same_as<P, ::tyr::formalism::PositiveTag>)
        return value == element.get_value();
    else if constexpr (std::same_as<P, ::tyr::formalism::NegativeTag>)
        return value != element.get_value();
    else
        static_assert(ygg::dependent_false<P>::value, "Missing case");
}

template<::tyr::formalism::PolarityKind P, TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::FDRFactListView<::tyr::formalism::FluentTag> elements, const StateContext<Kind>& context)
{
    return std::all_of(elements.begin(), elements.end(), [&](auto&& arg) { return is_applicable<P>(arg, context); });
}

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::GroundBooleanOperatorView element, const StateContext<Kind>& context)
{
    return evaluate(element, context);
}

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::GroundBooleanOperatorListView elements, const StateContext<Kind>& context)
{
    return std::all_of(elements.begin(), elements.end(), [&](auto&& arg) { return is_applicable(arg, context); });
}

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::NumericEffectView<::tyr::GroundTag, ::tyr::formalism::FluentTag> element,
                   const StateContext<Kind>& context,
                   ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families)
{
    const auto fterm_index = element.get_fterm().get_index();
    if (ref_fluent_effect_families.size() <= fterm_index.get_value())
        ref_fluent_effect_families.resize(fterm_index.get_value() + 1, ::tyr::formalism::EffectFamily::None);

    // Check non-conflicting effects
    const auto family = ::tyr::formalism::effect_family(element.get_operator());
    if (!::tyr::formalism::planning::is_compatible_effect_family(family, ref_fluent_effect_families[fterm_index.get_value()]))
        return false;  /// incompatible effects

    ref_fluent_effect_families[fterm_index.get_value()] = family;

    // Check fterm is well-defined in context
    if (element.get_operator() != ::tyr::formalism::NumericEffectOperatorKind::Assign)
    {
        if (std::isnan(context.state_builder.get(fterm_index)))
            return false;  /// target function is undefined and operator is not assign
    }

    // Check fexpr is well-defined in context
    return !std::isnan(evaluate(element.get_fexpr(), context));
}

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::NumericEffectOperatorView<::tyr::GroundTag, ::tyr::formalism::FluentTag> element,
                   const StateContext<Kind>& context,
                   ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families)
{
    return visit([&](auto&& arg) { return is_applicable(arg, context, ref_fluent_effect_families); }, element.get_variant());
}

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::NumericEffectOperatorListView<::tyr::GroundTag, ::tyr::formalism::FluentTag> elements,
                   const StateContext<Kind>& context,
                   ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families)
{
    return std::all_of(elements.begin(), elements.end(), [&](auto&& arg) { return is_applicable(arg, context, ref_fluent_effect_families); });
}

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::NumericEffectView<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag> element, const StateContext<Kind>& context)
{
    // Check fexpr is well-defined in context
    return !std::isnan(evaluate(element.get_fexpr(), context));
}

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::NumericEffectOperatorView<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag> element, const StateContext<Kind>& context)
{
    return visit([&](auto&& arg) { return is_applicable(arg, context); }, element.get_variant());
}

// GroundConjunctiveCondition

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::ConjunctiveConditionView<::tyr::GroundTag> element, const StateContext<Kind>& context)
{
    return is_applicable(element.template get_literals<::tyr::formalism::StaticTag>(), context)                                   //
           && is_applicable<::tyr::formalism::PositiveTag>(element.template get_facts<::tyr::formalism::PositiveTag>(), context)  //
           && is_applicable<::tyr::formalism::NegativeTag>(element.template get_facts<::tyr::formalism::NegativeTag>(), context)  //
           && is_applicable(element.template get_literals<::tyr::formalism::DerivedTag>(), context)                               //
           && is_applicable(element.get_numeric_constraints(), context);
}

// GroundConjunctiveEffect

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::ConjunctiveEffectView<::tyr::GroundTag> element,
                   const StateContext<Kind>& context,
                   ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families)
{
    return is_applicable(element.get_numeric_effects(), context, ref_fluent_effect_families)
           && (!element.get_auxiliary_numeric_effect().has_value() || is_applicable(element.get_auxiliary_numeric_effect().value(), context));
}

// GroundAction

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::ActionView<::tyr::GroundTag> element,
                   const StateContext<Kind>& context,
                   ::tyr::formalism::planning::EffectFamilyList& out_fluent_effect_families)
{
    return is_applicable(element.get_condition(), context) && is_applicable_if_fires(element.get_effects(), context, out_fluent_effect_families);
}

// GroundAxiom

template<TaskKind Kind>
bool is_applicable(::tyr::formalism::planning::AxiomView<::tyr::GroundTag> element, const StateContext<Kind>& context)
{
    return is_applicable(element.get_body(), context);
}

/**
 * is_statically_applicable
 */

inline bool is_statically_applicable(::tyr::formalism::planning::LiteralView<::tyr::GroundTag, ::tyr::formalism::StaticTag> element,
                                     const boost::dynamic_bitset<>& static_atoms)
{
    return ygg::test(ygg::uint_t(element.get_atom().get_index()), static_atoms) == element.get_polarity();
}

inline bool is_statically_applicable(::tyr::formalism::planning::LiteralListView<::tyr::GroundTag, ::tyr::formalism::StaticTag> elements,
                                     const boost::dynamic_bitset<>& static_atoms)
{
    return std::all_of(elements.begin(), elements.end(), [&](auto&& arg) { return is_statically_applicable(arg, static_atoms); });
}

// GroundConjunctiveCondition

inline bool is_statically_applicable(::tyr::formalism::planning::ConjunctiveConditionView<::tyr::GroundTag> element, const boost::dynamic_bitset<>& static_atoms)
{
    return is_statically_applicable(element.template get_literals<::tyr::formalism::StaticTag>(), static_atoms);
}

// GroundAction

inline bool is_statically_applicable(::tyr::formalism::planning::ActionView<::tyr::GroundTag> element, const boost::dynamic_bitset<>& static_atoms)
{
    return is_statically_applicable(element.get_condition(), static_atoms);
}

// GroundAxiom

inline bool is_statically_applicable(::tyr::formalism::planning::AxiomView<::tyr::GroundTag> element, const boost::dynamic_bitset<>& static_atoms)
{
    return is_statically_applicable(element.get_body(), static_atoms);
}

/**
 * is_dynamically_applicable
 */

// GroundConjunctiveCondition

template<TaskKind Kind>
bool is_dynamically_applicable(::tyr::formalism::planning::ConjunctiveConditionView<::tyr::GroundTag> element, const StateContext<Kind>& context)
{
    return is_applicable<::tyr::formalism::PositiveTag>(element.template get_facts<::tyr::formalism::PositiveTag>(), context)     //
           && is_applicable<::tyr::formalism::NegativeTag>(element.template get_facts<::tyr::formalism::NegativeTag>(), context)  //
           && is_applicable(element.template get_literals<::tyr::formalism::DerivedTag>(), context)                               //
           && is_applicable(element.get_numeric_constraints(), context);
}

/**
 * is_consistent
 */

// GroundConjunctiveCondition

inline bool is_consistent(
    ::tyr::formalism::planning::ConjunctiveConditionView<::tyr::GroundTag> element,
    ygg::UnorderedMap<ygg::Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>>, ::tyr::formalism::planning::FDRValue>& fluent_assign,
    ygg::UnorderedMap<ygg::Index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>>, bool>& derived_assign)
{
    for (const auto fact : element.template get_facts<::tyr::formalism::PositiveTag>())
    {
        const auto var = fact.get_variable().get_index();
        const auto val = fact.get_value();

        if (const auto it = fluent_assign.find(var); it != fluent_assign.end())
        {
            if (it->second != val)
                return false;
        }
        else
        {
            fluent_assign.emplace(var, val);
        }
    }

    for (const auto fact : element.template get_facts<::tyr::formalism::NegativeTag>())
    {
        const auto var = fact.get_variable().get_index();
        const auto val = fact.get_value();

        if (const auto it = fluent_assign.find(var); it != fluent_assign.end())
        {
            if (it->second == val)
                return false;
        }
    }

    for (const auto literal : element.template get_literals<::tyr::formalism::DerivedTag>())
    {
        const auto atom = literal.get_atom().get_index();
        const auto pol = literal.get_polarity();

        if (const auto it = derived_assign.find(atom); it != derived_assign.end())
        {
            if (it->second != pol)
                return false;
        }
        else
        {
            derived_assign.emplace(atom, pol);
        }
    }

    return true;
}

// GroundAction

inline bool is_consistent(::tyr::formalism::planning::ActionView<::tyr::GroundTag> element,
                          ygg::UnorderedMap<ygg::Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>>,
                                            ::tyr::formalism::planning::FDRValue>& out_fluent_assign,
                          ygg::UnorderedMap<ygg::Index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>>, bool>& out_derived_assign)
{
    out_fluent_assign.clear();
    out_derived_assign.clear();
    return is_consistent(element.get_condition(), out_fluent_assign, out_derived_assign);
}

// GroundAxiom

inline bool is_consistent(::tyr::formalism::planning::AxiomView<::tyr::GroundTag> element,
                          ygg::UnorderedMap<ygg::Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>>,
                                            ::tyr::formalism::planning::FDRValue>& out_fluent_assign,
                          ygg::UnorderedMap<ygg::Index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>>, bool>& out_derived_assign)
{
    out_fluent_assign.clear();
    out_derived_assign.clear();
    return is_consistent(element.get_body(), out_fluent_assign, out_derived_assign);
}

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::planning
{
extern template ygg::float_t evaluate(ygg::float_t element, const StateContext<LiftedTag>& context);
extern template ygg::float_t evaluate(ygg::float_t element, const StateContext<GroundTag>& context);

extern template ygg::float_t evaluate(::tyr::formalism::planning::GroundUnaryOperatorView element, const StateContext<LiftedTag>& context);
extern template ygg::float_t evaluate(::tyr::formalism::planning::GroundUnaryOperatorView element, const StateContext<GroundTag>& context);

extern template ygg::float_t evaluate(::tyr::formalism::planning::GroundBinaryOperatorView<::tyr::formalism::ArithmeticOperatorKind> element,
                                      const StateContext<LiftedTag>& context);
extern template ygg::float_t evaluate(::tyr::formalism::planning::GroundBinaryOperatorView<::tyr::formalism::ArithmeticOperatorKind> element,
                                      const StateContext<GroundTag>& context);

extern template bool evaluate(::tyr::formalism::planning::GroundBinaryOperatorView<::tyr::formalism::BooleanOperatorKind> element,
                              const StateContext<LiftedTag>& context);
extern template bool evaluate(::tyr::formalism::planning::GroundBinaryOperatorView<::tyr::formalism::BooleanOperatorKind> element,
                              const StateContext<GroundTag>& context);

extern template ygg::float_t evaluate(::tyr::formalism::planning::GroundMultiOperatorView element, const StateContext<LiftedTag>& context);
extern template ygg::float_t evaluate(::tyr::formalism::planning::GroundMultiOperatorView element, const StateContext<GroundTag>& context);

extern template ygg::float_t evaluate(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::StaticTag> element,
                                      const StateContext<LiftedTag>& context);
extern template ygg::float_t evaluate(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::StaticTag> element,
                                      const StateContext<GroundTag>& context);

extern template ygg::float_t evaluate(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::FluentTag> element,
                                      const StateContext<LiftedTag>& context);
extern template ygg::float_t evaluate(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::FluentTag> element,
                                      const StateContext<GroundTag>& context);

extern template ygg::float_t evaluate(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag> element,
                                      const StateContext<LiftedTag>& context);
extern template ygg::float_t evaluate(::tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag> element,
                                      const StateContext<GroundTag>& context);

extern template ygg::float_t evaluate(::tyr::formalism::planning::FunctionExpressionView<::tyr::GroundTag> element, const StateContext<LiftedTag>& context);
extern template ygg::float_t evaluate(::tyr::formalism::planning::FunctionExpressionView<::tyr::GroundTag> element, const StateContext<GroundTag>& context);

extern template ygg::float_t evaluate(::tyr::formalism::planning::GroundArithmeticOperatorView element, const StateContext<LiftedTag>& context);
extern template ygg::float_t evaluate(::tyr::formalism::planning::GroundArithmeticOperatorView element, const StateContext<GroundTag>& context);

extern template bool is_applicable(::tyr::formalism::planning::GroundBooleanOperatorView element, const StateContext<LiftedTag>& context);
extern template bool is_applicable(::tyr::formalism::planning::GroundBooleanOperatorView element, const StateContext<GroundTag>& context);

extern template bool evaluate(::tyr::formalism::planning::GroundBooleanOperatorView element, const StateContext<LiftedTag>& context);
extern template bool evaluate(::tyr::formalism::planning::GroundBooleanOperatorView element, const StateContext<GroundTag>& context);

extern template ygg::float_t evaluate(::tyr::formalism::planning::NumericEffectView<::tyr::GroundTag, ::tyr::formalism::FluentTag> element,
                                      const StateContext<LiftedTag>& context);
extern template ygg::float_t evaluate(::tyr::formalism::planning::NumericEffectView<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag> element,
                                      const StateContext<LiftedTag>& context);
extern template ygg::float_t evaluate(::tyr::formalism::planning::NumericEffectView<::tyr::GroundTag, ::tyr::formalism::FluentTag> element,
                                      const StateContext<GroundTag>& context);
extern template ygg::float_t evaluate(::tyr::formalism::planning::NumericEffectView<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag> element,
                                      const StateContext<GroundTag>& context);

extern template ygg::float_t evaluate(::tyr::formalism::planning::NumericEffectOperatorView<::tyr::GroundTag, ::tyr::formalism::FluentTag> element,
                                      const StateContext<LiftedTag>& context);
extern template ygg::float_t evaluate(::tyr::formalism::planning::NumericEffectOperatorView<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag> element,
                                      const StateContext<LiftedTag>& context);
extern template ygg::float_t evaluate(::tyr::formalism::planning::NumericEffectOperatorView<::tyr::GroundTag, ::tyr::formalism::FluentTag> element,
                                      const StateContext<GroundTag>& context);
extern template ygg::float_t evaluate(::tyr::formalism::planning::NumericEffectOperatorView<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag> element,
                                      const StateContext<GroundTag>& context);

/**
 * is_applicable_if_fires
 */

extern template bool is_applicable_if_fires(::tyr::formalism::planning::ConditionalEffectView<::tyr::GroundTag> element,
                                            const StateContext<LiftedTag>& context,
                                            ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families);
extern template bool is_applicable_if_fires(::tyr::formalism::planning::ConditionalEffectView<::tyr::GroundTag> element,
                                            const StateContext<GroundTag>& context,
                                            ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families);

extern template bool is_applicable_if_fires(::tyr::formalism::planning::ConditionalEffectListView<::tyr::GroundTag> elements,
                                            const StateContext<LiftedTag>& context,
                                            ::tyr::formalism::planning::EffectFamilyList& out_fluent_effect_families);
extern template bool is_applicable_if_fires(::tyr::formalism::planning::ConditionalEffectListView<::tyr::GroundTag> elements,
                                            const StateContext<GroundTag>& context,
                                            ::tyr::formalism::planning::EffectFamilyList& out_fluent_effect_families);

/**
 * is_applicable
 */

extern template bool is_applicable(::tyr::formalism::planning::LiteralView<::tyr::GroundTag, ::tyr::formalism::StaticTag> element, const StateContext<LiftedTag>& context);
extern template bool is_applicable(::tyr::formalism::planning::LiteralView<::tyr::GroundTag, ::tyr::formalism::StaticTag> element, const StateContext<GroundTag>& context);

extern template bool is_applicable(::tyr::formalism::planning::LiteralView<::tyr::GroundTag, ::tyr::formalism::DerivedTag> element, const StateContext<LiftedTag>& context);
extern template bool is_applicable(::tyr::formalism::planning::LiteralView<::tyr::GroundTag, ::tyr::formalism::DerivedTag> element, const StateContext<GroundTag>& context);

extern template bool is_applicable(::tyr::formalism::planning::LiteralListView<::tyr::GroundTag, ::tyr::formalism::StaticTag> elements,
                                   const StateContext<LiftedTag>& context);
extern template bool is_applicable(::tyr::formalism::planning::LiteralListView<::tyr::GroundTag, ::tyr::formalism::DerivedTag> elements,
                                   const StateContext<LiftedTag>& context);
extern template bool is_applicable(::tyr::formalism::planning::LiteralListView<::tyr::GroundTag, ::tyr::formalism::StaticTag> elements,
                                   const StateContext<GroundTag>& context);
extern template bool is_applicable(::tyr::formalism::planning::LiteralListView<::tyr::GroundTag, ::tyr::formalism::DerivedTag> elements,
                                   const StateContext<GroundTag>& context);

extern template bool is_applicable<::tyr::formalism::PositiveTag>(::tyr::formalism::planning::FDRFactView<::tyr::formalism::FluentTag> element,
                                                                  const StateContext<LiftedTag>& context);
extern template bool is_applicable<::tyr::formalism::NegativeTag>(::tyr::formalism::planning::FDRFactView<::tyr::formalism::FluentTag> element,
                                                                  const StateContext<LiftedTag>& context);
extern template bool is_applicable<::tyr::formalism::PositiveTag>(::tyr::formalism::planning::FDRFactView<::tyr::formalism::FluentTag> element,
                                                                  const StateContext<GroundTag>& context);
extern template bool is_applicable<::tyr::formalism::NegativeTag>(::tyr::formalism::planning::FDRFactView<::tyr::formalism::FluentTag> element,
                                                                  const StateContext<GroundTag>& context);

extern template bool is_applicable<::tyr::formalism::PositiveTag>(::tyr::formalism::planning::FDRFactListView<::tyr::formalism::FluentTag> elements,
                                                                  const StateContext<LiftedTag>& context);
extern template bool is_applicable<::tyr::formalism::NegativeTag>(::tyr::formalism::planning::FDRFactListView<::tyr::formalism::FluentTag> elements,
                                                                  const StateContext<LiftedTag>& context);
extern template bool is_applicable<::tyr::formalism::PositiveTag>(::tyr::formalism::planning::FDRFactListView<::tyr::formalism::FluentTag> elements,
                                                                  const StateContext<GroundTag>& context);
extern template bool is_applicable<::tyr::formalism::NegativeTag>(::tyr::formalism::planning::FDRFactListView<::tyr::formalism::FluentTag> elements,
                                                                  const StateContext<GroundTag>& context);

extern template bool is_applicable(::tyr::formalism::planning::GroundBooleanOperatorListView elements, const StateContext<LiftedTag>& context);
extern template bool is_applicable(::tyr::formalism::planning::GroundBooleanOperatorListView elements, const StateContext<GroundTag>& context);

extern template bool is_applicable(::tyr::formalism::planning::NumericEffectView<::tyr::GroundTag, ::tyr::formalism::FluentTag> element,
                                   const StateContext<LiftedTag>& context,
                                   ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families);
extern template bool is_applicable(::tyr::formalism::planning::NumericEffectView<::tyr::GroundTag, ::tyr::formalism::FluentTag> element,
                                   const StateContext<GroundTag>& context,
                                   ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families);

extern template bool is_applicable(::tyr::formalism::planning::NumericEffectOperatorView<::tyr::GroundTag, ::tyr::formalism::FluentTag> element,
                                   const StateContext<LiftedTag>& context,
                                   ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families);
extern template bool is_applicable(::tyr::formalism::planning::NumericEffectOperatorView<::tyr::GroundTag, ::tyr::formalism::FluentTag> element,
                                   const StateContext<GroundTag>& context,
                                   ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families);

extern template bool is_applicable(::tyr::formalism::planning::NumericEffectOperatorListView<::tyr::GroundTag, ::tyr::formalism::FluentTag> elements,
                                   const StateContext<LiftedTag>& context,
                                   ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families);
extern template bool is_applicable(::tyr::formalism::planning::NumericEffectOperatorListView<::tyr::GroundTag, ::tyr::formalism::FluentTag> elements,
                                   const StateContext<GroundTag>& context,
                                   ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families);

extern template bool is_applicable(::tyr::formalism::planning::NumericEffectView<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag> element,
                                   const StateContext<LiftedTag>& context);
extern template bool is_applicable(::tyr::formalism::planning::NumericEffectView<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag> element,
                                   const StateContext<GroundTag>& context);

extern template bool is_applicable(::tyr::formalism::planning::NumericEffectOperatorView<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag> element,
                                   const StateContext<LiftedTag>& context);
extern template bool is_applicable(::tyr::formalism::planning::NumericEffectOperatorView<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag> element,
                                   const StateContext<GroundTag>& context);

// GroundConjunctiveCondition

extern template bool is_applicable(::tyr::formalism::planning::ConjunctiveConditionView<::tyr::GroundTag> element, const StateContext<LiftedTag>& context);
extern template bool is_applicable(::tyr::formalism::planning::ConjunctiveConditionView<::tyr::GroundTag> element, const StateContext<GroundTag>& context);

// GroundConjunctiveEffect

extern template bool is_applicable(::tyr::formalism::planning::ConjunctiveEffectView<::tyr::GroundTag> element,
                                   const StateContext<LiftedTag>& context,
                                   ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families);
extern template bool is_applicable(::tyr::formalism::planning::ConjunctiveEffectView<::tyr::GroundTag> element,
                                   const StateContext<GroundTag>& context,
                                   ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families);

// GroundAction

extern template bool is_applicable(::tyr::formalism::planning::ActionView<::tyr::GroundTag> element,
                                   const StateContext<LiftedTag>& context,
                                   ::tyr::formalism::planning::EffectFamilyList& out_fluent_effect_families);
extern template bool is_applicable(::tyr::formalism::planning::ActionView<::tyr::GroundTag> element,
                                   const StateContext<GroundTag>& context,
                                   ::tyr::formalism::planning::EffectFamilyList& out_fluent_effect_families);

// GroundAxiom

extern template bool is_applicable(::tyr::formalism::planning::AxiomView<::tyr::GroundTag> element, const StateContext<LiftedTag>& context);
extern template bool is_applicable(::tyr::formalism::planning::AxiomView<::tyr::GroundTag> element, const StateContext<GroundTag>& context);

/**
 * is_dynamically_applicable
 */

// GroundConjunctiveCondition

extern template bool is_dynamically_applicable(::tyr::formalism::planning::ConjunctiveConditionView<::tyr::GroundTag> element, const StateContext<LiftedTag>& context);
extern template bool is_dynamically_applicable(::tyr::formalism::planning::ConjunctiveConditionView<::tyr::GroundTag> element, const StateContext<GroundTag>& context);
}

#endif

#endif
