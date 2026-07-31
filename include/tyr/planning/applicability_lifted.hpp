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

#ifndef TYR_PLANNING_APPLICABILITY_LIFTED_HPP_
#define TYR_PLANNING_APPLICABILITY_LIFTED_HPP_

#include "tyr/analysis/declarations.hpp"
#include "tyr/formalism/arithmetic_operator_utils.hpp"
#include "tyr/formalism/boolean_operator_utils.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/ground_numeric_effect_operator_utils.hpp"
#include "tyr/formalism/planning/grounder.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/applicability_lifted_decl.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/lifted/state_builder.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/node.hpp"

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
#include <yggdrasil/core/itertools.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::planning
{

/**
 * evaluate
 */

ygg::float_t evaluate(ygg::float_t element, const ApplicabilityContext& context);

ygg::float_t evaluate(::tyr::formalism::planning::LiftedUnaryOperatorView element, const ApplicabilityContext& context);

ygg::float_t evaluate(::tyr::formalism::planning::LiftedBinaryOperatorView<::tyr::formalism::ArithmeticOperatorKind> element,
                      const ApplicabilityContext& context);

bool evaluate(::tyr::formalism::planning::LiftedBinaryOperatorView<::tyr::formalism::BooleanOperatorKind> element, const ApplicabilityContext& context);

ygg::float_t evaluate(::tyr::formalism::planning::LiftedMultiOperatorView element, const ApplicabilityContext& context);

ygg::float_t evaluate(::tyr::formalism::planning::FunctionTermView<::tyr::formalism::StaticTag> element, const ApplicabilityContext& context);

ygg::float_t evaluate(::tyr::formalism::planning::FunctionTermView<::tyr::formalism::FluentTag> element, const ApplicabilityContext& context);

ygg::float_t evaluate(::tyr::formalism::planning::FunctionTermView<::tyr::formalism::AuxiliaryTag> element, const ApplicabilityContext& context);

ygg::float_t evaluate(::tyr::formalism::planning::FunctionExpressionView element, const ApplicabilityContext& context);

ygg::float_t evaluate(::tyr::formalism::planning::LiftedArithmeticOperatorView element, const ApplicabilityContext& context);

bool evaluate(::tyr::formalism::planning::LiftedBooleanOperatorView element, const ApplicabilityContext& context);

template<::tyr::formalism::FactKind T>
ygg::float_t evaluate(::tyr::formalism::planning::NumericEffectView<T> element, const ApplicabilityContext& context);

template<::tyr::formalism::FactKind T>
ygg::float_t evaluate(::tyr::formalism::planning::NumericEffectOperatorView<T> element, const ApplicabilityContext& context);

/**
 * is_applicable_if_fires
 */

bool is_applicable_if_fires(::tyr::formalism::planning::ConditionalEffectView element,
                            const ApplicabilityContext& context,
                            ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families,
                            ygg::itertools::cartesian_set::Workspace<ygg::Index<::tyr::formalism::Object>>& cartesian_workspace,
                            const analysis::ConditionalEffectDomain& effect_domains);

bool is_applicable_if_fires(::tyr::formalism::planning::ConditionalEffectListView elements,
                            const ApplicabilityContext& context,
                            ::tyr::formalism::planning::EffectFamilyList& out_fluent_effect_families,
                            ygg::itertools::cartesian_set::Workspace<ygg::Index<::tyr::formalism::Object>>& cartesian_workspace,
                            const analysis::ActionDomain& action_domains);

/**
 * is_applicable
 */

bool is_applicable(::tyr::formalism::planning::LiteralView<::tyr::formalism::StaticTag> element, const ApplicabilityContext& context);

bool is_applicable(::tyr::formalism::planning::LiteralView<::tyr::formalism::FluentTag> element, const ApplicabilityContext& context);

bool is_applicable(::tyr::formalism::planning::LiteralView<::tyr::formalism::DerivedTag> element, const ApplicabilityContext& context);

template<::tyr::formalism::FactKind T>
bool is_applicable(::tyr::formalism::planning::LiteralListView<T> elements, const ApplicabilityContext& context);

bool is_applicable(::tyr::formalism::planning::LiftedBooleanOperatorView element, const ApplicabilityContext& context);

bool is_applicable(::tyr::formalism::planning::LiftedBooleanOperatorListView elements, const ApplicabilityContext& context);

bool is_applicable(::tyr::formalism::planning::NumericEffectView<::tyr::formalism::FluentTag> element,
                   const ApplicabilityContext& context,
                   ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families);

bool is_applicable(::tyr::formalism::planning::NumericEffectOperatorView<::tyr::formalism::FluentTag> element,
                   const ApplicabilityContext& context,
                   ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families);

bool is_applicable(::tyr::formalism::planning::NumericEffectOperatorListView<::tyr::formalism::FluentTag> elements,
                   const ApplicabilityContext& context,
                   ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families);

bool is_applicable(::tyr::formalism::planning::NumericEffectView<::tyr::formalism::AuxiliaryTag> element, const ApplicabilityContext& context);

bool is_applicable(::tyr::formalism::planning::NumericEffectOperatorView<::tyr::formalism::AuxiliaryTag> element, const ApplicabilityContext& context);

// ConjunctiveCondition

bool is_applicable(::tyr::formalism::planning::ConjunctiveConditionView element, const ApplicabilityContext& context);

// ConjunctiveEffect

bool is_applicable(::tyr::formalism::planning::ConjunctiveEffectView element,
                   const ApplicabilityContext& context,
                   ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families);

// Action

bool is_applicable(::tyr::formalism::planning::ActionView element,
                   const ApplicabilityContext& context,
                   ::tyr::formalism::planning::EffectFamilyList& out_fluent_effect_families,
                   ygg::itertools::cartesian_set::Workspace<ygg::Index<::tyr::formalism::Object>>& cartesian_workspace,
                   const analysis::ActionDomain& action_domains);

// Axiom

bool is_applicable(::tyr::formalism::planning::AxiomView element, const ApplicabilityContext& context);

/**
 * evaluate
 */

inline ygg::float_t evaluate(ygg::float_t element, const ApplicabilityContext& context) { return element; }

inline ygg::float_t evaluate(::tyr::formalism::planning::LiftedUnaryOperatorView element, const ApplicabilityContext& context)
{
    return ::tyr::formalism::apply(element.get_operator(), evaluate(element.get_arg(), context));
}

inline ygg::float_t evaluate(::tyr::formalism::planning::LiftedBinaryOperatorView<::tyr::formalism::ArithmeticOperatorKind> element,
                             const ApplicabilityContext& context)
{
    // Sequenced: evaluation grounds subterms (mutating the context), and argument evaluation order
    // is unspecified (gcc and clang disagree). rhs-first preserves the historical gcc order.
    const auto rhs = evaluate(element.get_rhs(), context);
    const auto lhs = evaluate(element.get_lhs(), context);
    return ::tyr::formalism::apply(element.get_operator(), lhs, rhs);
}

inline bool evaluate(::tyr::formalism::planning::LiftedBinaryOperatorView<::tyr::formalism::BooleanOperatorKind> element, const ApplicabilityContext& context)
{
    // Sequenced: evaluation grounds subterms (mutating the context), and argument evaluation order
    // is unspecified (gcc and clang disagree). rhs-first preserves the historical gcc order.
    const auto rhs = evaluate(element.get_rhs(), context);
    const auto lhs = evaluate(element.get_lhs(), context);
    return ::tyr::formalism::apply(element.get_operator(), lhs, rhs);
}

inline ygg::float_t evaluate(::tyr::formalism::planning::LiftedMultiOperatorView element, const ApplicabilityContext& context)
{
    const auto child_fexprs = element.get_args();

    return std::accumulate(std::next(child_fexprs.begin()),  // Start from the second expression
                           child_fexprs.end(),
                           evaluate(child_fexprs.front(), context),
                           [&](const auto& value, const auto& child_expr)
                           { return ::tyr::formalism::apply(element.get_operator(), value, evaluate(child_expr, context)); });
}

inline ygg::float_t evaluate(::tyr::formalism::planning::FunctionTermView<::tyr::formalism::StaticTag> element, const ApplicabilityContext& context)
{
    const auto fterm_or_nullopt = ::tyr::formalism::planning::try_ground(element, context.grounder);
    if (!fterm_or_nullopt.has_value())
        return std::numeric_limits<ygg::float_t>::quiet_NaN();

    return context.state.task.get(fterm_or_nullopt->get_index());
}

inline ygg::float_t evaluate(::tyr::formalism::planning::FunctionTermView<::tyr::formalism::FluentTag> element, const ApplicabilityContext& context)
{
    const auto fterm_or_nullopt = ::tyr::formalism::planning::try_ground(element, context.grounder);
    if (!fterm_or_nullopt.has_value())
        return std::numeric_limits<ygg::float_t>::quiet_NaN();

    return context.state.state_builder.get(fterm_or_nullopt->get_index());
}

inline ygg::float_t evaluate(::tyr::formalism::planning::FunctionTermView<::tyr::formalism::AuxiliaryTag> element, const ApplicabilityContext& context)
{
    return context.state.auxiliary_value;
}

inline ygg::float_t evaluate(::tyr::formalism::planning::FunctionExpressionView element, const ApplicabilityContext& context)
{
    return visit([&](auto&& arg) { return evaluate(arg, context); }, element.get_variant());
}

inline ygg::float_t evaluate(::tyr::formalism::planning::LiftedArithmeticOperatorView element, const ApplicabilityContext& context)
{
    return visit([&](auto&& arg) { return evaluate(arg, context); }, element.get_variant());
}

inline bool evaluate(::tyr::formalism::planning::LiftedBooleanOperatorView element, const ApplicabilityContext& context)
{
    return visit([&](auto&& arg) { return evaluate(arg, context); }, element.get_variant());
}

template<::tyr::formalism::FactKind T>
ygg::float_t evaluate(::tyr::formalism::planning::NumericEffectView<T> element, const ApplicabilityContext& context)
{
    // Sequenced for the same reason as the arithmetic binary operator above.
    const auto fexpr_value = evaluate(element.get_fexpr(), context);
    const auto fterm_value = evaluate(element.get_fterm(), context);
    return ::tyr::formalism::planning::apply(element.get_operator(), fterm_value, fexpr_value);
}

template<::tyr::formalism::FactKind T>
ygg::float_t evaluate(::tyr::formalism::planning::NumericEffectOperatorView<T> element, const ApplicabilityContext& context)
{
    return visit([&](auto&& arg) { return evaluate(arg, context); }, element.get_variant());
}

/**
 * is_applicable_if_fires
 */

inline bool is_applicable_if_fires(::tyr::formalism::planning::ConditionalEffectView element,
                                   const ApplicabilityContext& context,
                                   ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families,
                                   ygg::itertools::cartesian_set::Workspace<ygg::Index<::tyr::formalism::Object>>& cartesian_workspace,
                                   const analysis::ConditionalEffectDomain& effect_domains,
                                   size_t action_arity)
{
    const auto effect = element.get_effect();
    if (effect.get_numeric_effects().empty() && !effect.get_auxiliary_numeric_effect().has_value())
        return true;

    const auto& parameter_domains = effect_domains.payload.effect_domain.payload;
    const auto binding_size = context.grounder.binding.size();

    bool applicable = true;

    ygg::itertools::cartesian_set::for_each_element(
        parameter_domains.begin() + action_arity,
        parameter_domains.end(),
        cartesian_workspace,
        [&](auto&& binding_cond)
        {
            if (!applicable)
                return;

            context.grounder.binding.resize(binding_size);
            context.grounder.binding.insert(context.grounder.binding.end(), binding_cond.begin(), binding_cond.end());

            if (is_applicable(element.get_condition(), context) && !is_applicable(effect, context, ref_fluent_effect_families))
            {
                applicable = false;
                return;
            }
        });

    context.grounder.binding.resize(binding_size);

    return applicable;
}

inline bool is_applicable_if_fires(::tyr::formalism::planning::ConditionalEffectListView elements,
                                   const ApplicabilityContext& context,
                                   ::tyr::formalism::planning::EffectFamilyList& out_fluent_effect_families,
                                   ygg::itertools::cartesian_set::Workspace<ygg::Index<::tyr::formalism::Object>>& cartesian_workspace,
                                   const analysis::ActionDomain& action_domains)
{
    out_fluent_effect_families.clear();

    for (const auto cond_effect : elements)
    {
        if (!is_applicable_if_fires(cond_effect,
                                    context,
                                    out_fluent_effect_families,
                                    cartesian_workspace,
                                    action_domains.payload.effect_domains.at(cond_effect.get_index()),
                                    context.grounder.binding.size()))
            return false;
    }

    return true;
}

/**
 * is_applicable
 */

inline bool is_applicable(::tyr::formalism::planning::LiteralView<::tyr::formalism::StaticTag> element, const ApplicabilityContext& context)
{
    const auto atom_or_nullopt = ::tyr::formalism::planning::try_ground(element.get_atom(), context.grounder);
    if (!atom_or_nullopt.has_value())
        return !element.get_polarity();

    return context.state.task.test(atom_or_nullopt->get_index()) == element.get_polarity();
}

inline bool is_applicable(::tyr::formalism::planning::LiteralView<::tyr::formalism::FluentTag> element, const ApplicabilityContext& context)
{
    const auto atom_or_nullopt = ::tyr::formalism::planning::try_ground(element.get_atom(), context.grounder);
    if (!atom_or_nullopt.has_value())
        return !element.get_polarity();

    const auto fact_or_nullopt = context.fdr.get_fact(*atom_or_nullopt);

    if (!fact_or_nullopt.has_value())
        return !element.get_polarity();

    const auto& fact = *fact_or_nullopt;
    return (context.state.state_builder.get(fact.get_variable()) == fact.get_value()) == element.get_polarity();
}

inline bool is_applicable(::tyr::formalism::planning::LiteralView<::tyr::formalism::DerivedTag> element, const ApplicabilityContext& context)
{
    const auto atom_or_nullopt = ::tyr::formalism::planning::try_ground(element.get_atom(), context.grounder);
    if (!atom_or_nullopt.has_value())
        return !element.get_polarity();

    return context.state.state_builder.test(atom_or_nullopt->get_index()) == element.get_polarity();
}

template<::tyr::formalism::FactKind T>
bool is_applicable(::tyr::formalism::planning::LiteralListView<T> elements, const ApplicabilityContext& context)
{
    return std::all_of(elements.begin(), elements.end(), [&](auto&& arg) { return is_applicable(arg, context); });
}

inline bool is_applicable(::tyr::formalism::planning::LiftedBooleanOperatorView element, const ApplicabilityContext& context)
{
    return evaluate(element, context);
}

inline bool is_applicable(::tyr::formalism::planning::LiftedBooleanOperatorListView elements, const ApplicabilityContext& context)
{
    return std::all_of(elements.begin(), elements.end(), [&](auto&& arg) { return is_applicable(arg, context); });
}

inline bool is_applicable(::tyr::formalism::planning::NumericEffectView<::tyr::formalism::FluentTag> element,
                          const ApplicabilityContext& context,
                          ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families)
{
    auto fterm = ::tyr::formalism::planning::try_ground(element.get_fterm(), context.grounder);
    if (!fterm.has_value())
    {
        if (element.get_operator() == ::tyr::formalism::NumericEffectOperatorKind::Assign)
            fterm = ::tyr::formalism::planning::ground(element.get_fterm(), context.grounder).first;
        else
            return false;
    }

    const auto fterm_index = fterm->get_index();
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
        if (std::isnan(context.state.state_builder.get(fterm_index)))
            return false;  /// target function is undefined and operator is not assign
    }

    // Check fexpr is well-defined in context
    return !std::isnan(evaluate(element.get_fexpr(), context));
}

inline bool is_applicable(::tyr::formalism::planning::NumericEffectOperatorView<::tyr::formalism::FluentTag> element,
                          const ApplicabilityContext& context,
                          ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families)
{
    return visit([&](auto&& arg) { return is_applicable(arg, context, ref_fluent_effect_families); }, element.get_variant());
}

inline bool is_applicable(::tyr::formalism::planning::NumericEffectOperatorListView<::tyr::formalism::FluentTag> elements,
                          const ApplicabilityContext& context,
                          ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families)
{
    return std::all_of(elements.begin(), elements.end(), [&](auto&& arg) { return is_applicable(arg, context, ref_fluent_effect_families); });
}

inline bool is_applicable(::tyr::formalism::planning::NumericEffectView<::tyr::formalism::AuxiliaryTag> element, const ApplicabilityContext& context)
{
    // Check fexpr is well-defined in context
    return !std::isnan(evaluate(element.get_fexpr(), context));
}

inline bool is_applicable(::tyr::formalism::planning::NumericEffectOperatorView<::tyr::formalism::AuxiliaryTag> element, const ApplicabilityContext& context)
{
    return visit([&](auto&& arg) { return is_applicable(arg, context); }, element.get_variant());
}

// ConjunctiveCondition

inline bool is_applicable(::tyr::formalism::planning::ConjunctiveConditionView element, const ApplicabilityContext& context)
{
    return is_applicable(element.template get_literals<::tyr::formalism::StaticTag>(), context)  //
           && is_applicable(element.template get_literals<::tyr::formalism::FluentTag>(), context)
           && is_applicable(element.template get_literals<::tyr::formalism::DerivedTag>(), context)  //
           && is_applicable(element.get_numeric_constraints(), context);
}

// ConjunctiveEffect

inline bool is_applicable(::tyr::formalism::planning::ConjunctiveEffectView element,
                          const ApplicabilityContext& context,
                          ::tyr::formalism::planning::EffectFamilyList& ref_fluent_effect_families)
{
    return is_applicable(element.get_numeric_effects(), context, ref_fluent_effect_families)
           && (!element.get_auxiliary_numeric_effect().has_value() || is_applicable(element.get_auxiliary_numeric_effect().value(), context));
}

// Action

inline bool is_applicable(::tyr::formalism::planning::ActionView element,
                          const ApplicabilityContext& context,
                          ::tyr::formalism::planning::EffectFamilyList& out_fluent_effect_families,
                          ygg::itertools::cartesian_set::Workspace<ygg::Index<::tyr::formalism::Object>>& cartesian_workspace,
                          const analysis::ActionDomain& action_domains)
{
    return is_applicable(element.get_condition(), context)
           && is_applicable_if_fires(element.get_effects(), context, out_fluent_effect_families, cartesian_workspace, action_domains);
}

// Axiom

inline bool is_applicable(::tyr::formalism::planning::AxiomView element, const ApplicabilityContext& context)
{
    return is_applicable(element.get_body(), context);
}

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::planning
{
// NumericEffectView

extern template ygg::float_t evaluate(::tyr::formalism::planning::NumericEffectView<::tyr::formalism::FluentTag> element, const ApplicabilityContext& context);
extern template ygg::float_t evaluate(::tyr::formalism::planning::NumericEffectView<::tyr::formalism::AuxiliaryTag> element,
                                      const ApplicabilityContext& context);

// NumericEffectOperatorView

extern template ygg::float_t evaluate(::tyr::formalism::planning::NumericEffectOperatorView<::tyr::formalism::FluentTag> element,
                                      const ApplicabilityContext& context);
extern template ygg::float_t evaluate(::tyr::formalism::planning::NumericEffectOperatorView<::tyr::formalism::AuxiliaryTag> element,
                                      const ApplicabilityContext& context);

/**
 * is_applicable
 */

// LiteralListView

extern template bool is_applicable(::tyr::formalism::planning::LiteralListView<::tyr::formalism::StaticTag> elements, const ApplicabilityContext& context);
extern template bool is_applicable(::tyr::formalism::planning::LiteralListView<::tyr::formalism::FluentTag> elements, const ApplicabilityContext& context);
extern template bool is_applicable(::tyr::formalism::planning::LiteralListView<::tyr::formalism::DerivedTag> elements, const ApplicabilityContext& context);

}

#endif

#endif
