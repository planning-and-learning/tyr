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

#ifndef TYR_PLANNING_SUCCESSORS_APPLICABILITY_LIFTED_HPP_
#define TYR_PLANNING_SUCCESSORS_APPLICABILITY_LIFTED_HPP_

#include "tyr/analysis/domains.hpp"
#include "tyr/formalism/arithmetic_operator_utils.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/boolean_operator_utils.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/grounder.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_operator_utils.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/grounder.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/planning/successors/applicability_lifted_decl.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"
#include "tyr/planning/nodes/node_view.hpp"
#include "tyr/planning/states/state_builder.hpp"

#include <algorithm>
#include <concepts>
#include <iterator>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <urd/elements/equal_to.hpp>
#include <urd/elements/hash.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::planning
{

/**
 * evaluate
 */

ygg::float_t evaluate(ygg::float_t element, const ApplicabilityContext& context);

ygg::float_t evaluate(formalism::UnaryOperatorView<LiftedTag, formalism::ArithmeticOperatorKind> element, const ApplicabilityContext& context);

ygg::float_t evaluate(formalism::BinaryOperatorView<LiftedTag, formalism::ArithmeticOperatorKind> element, const ApplicabilityContext& context);

bool evaluate(formalism::BinaryOperatorView<LiftedTag, formalism::BooleanOperatorKind> element, const ApplicabilityContext& context);

ygg::float_t evaluate(formalism::MultiOperatorView<LiftedTag, formalism::ArithmeticOperatorKind> element, const ApplicabilityContext& context);

ygg::float_t evaluate(formalism::FunctionTermView<LiftedTag, formalism::StaticTag> element, const ApplicabilityContext& context);

ygg::float_t evaluate(formalism::FunctionTermView<LiftedTag, formalism::FluentTag> element, const ApplicabilityContext& context);

ygg::float_t evaluate(formalism::FunctionTermView<LiftedTag, formalism::AuxiliaryTag> element, const ApplicabilityContext& context);

ygg::float_t evaluate(formalism::FunctionExpressionView<LiftedTag> element, const ApplicabilityContext& context);

template<formalism::FactKind T>
ygg::float_t evaluate(formalism::NumericEffectView<LiftedTag, T> element, const ApplicabilityContext& context);

/**
 * is_applicable
 */

bool is_applicable(formalism::LiteralView<LiftedTag, formalism::StaticTag> element, const ApplicabilityContext& context);

bool is_applicable(formalism::LiteralView<LiftedTag, formalism::FluentTag> element, const ApplicabilityContext& context);

bool is_applicable(formalism::LiteralView<LiftedTag, formalism::DerivedTag> element, const ApplicabilityContext& context);

template<formalism::FactKind T>
bool is_applicable(formalism::LiteralListView<LiftedTag, T> elements, const ApplicabilityContext& context);

bool is_applicable(formalism::BooleanOperatorView<LiftedTag> element, const ApplicabilityContext& context);

bool is_applicable(formalism::BooleanOperatorListView<LiftedTag> elements, const ApplicabilityContext& context);

bool is_applicable(formalism::NumericEffectView<LiftedTag, formalism::FluentTag> element,
                   const ApplicabilityContext& context,
                   formalism::EffectFamilyList& ref_fluent_effect_families);

bool is_applicable(formalism::NumericEffectOperatorListView<LiftedTag, formalism::FluentTag> elements,
                   const ApplicabilityContext& context,
                   formalism::EffectFamilyList& ref_fluent_effect_families);

bool is_applicable(formalism::NumericEffectView<LiftedTag, formalism::AuxiliaryTag> element, const ApplicabilityContext& context);

// ConjunctiveCondition

bool is_applicable(formalism::ConjunctiveConditionView<LiftedTag> element, const ApplicabilityContext& context);

// ConjunctiveEffect

bool is_applicable(formalism::ConjunctiveEffectView<LiftedTag> element,
                   const ApplicabilityContext& context,
                   formalism::EffectFamilyList& ref_fluent_effect_families);

// Axiom

bool is_applicable(formalism::AxiomView<LiftedTag> element, const ApplicabilityContext& context);

/**
 * evaluate
 */

inline ygg::float_t evaluate(ygg::float_t element, const ApplicabilityContext&) { return element; }

inline ygg::float_t evaluate(formalism::UnaryOperatorView<LiftedTag, formalism::ArithmeticOperatorKind> element, const ApplicabilityContext& context)
{ return formalism::apply(element.get_operation(), evaluate(element.get_arg(), context)); }

inline ygg::float_t evaluate(formalism::BinaryOperatorView<LiftedTag, formalism::ArithmeticOperatorKind> element, const ApplicabilityContext& context)
{
    // Sequenced: evaluation grounds subterms (mutating the context), and argument evaluation order
    // is unspecified (gcc and clang disagree). rhs-first preserves the historical gcc order.
    const auto rhs = evaluate(element.get_rhs(), context);
    const auto lhs = evaluate(element.get_lhs(), context);
    return formalism::apply(element.get_operation(), lhs, rhs);
}

inline bool evaluate(formalism::BinaryOperatorView<LiftedTag, formalism::BooleanOperatorKind> element, const ApplicabilityContext& context)
{
    // Sequenced: evaluation grounds subterms (mutating the context), and argument evaluation order
    // is unspecified (gcc and clang disagree). rhs-first preserves the historical gcc order.
    const auto rhs = evaluate(element.get_rhs(), context);
    const auto lhs = evaluate(element.get_lhs(), context);
    return formalism::apply(element.get_operation(), lhs, rhs);
}

inline ygg::float_t evaluate(formalism::MultiOperatorView<LiftedTag, formalism::ArithmeticOperatorKind> element, const ApplicabilityContext& context)
{
    const auto child_fexprs = element.get_args();

    return std::accumulate(std::next(child_fexprs.begin()),  // Start from the second expression
                           child_fexprs.end(),
                           evaluate(child_fexprs.front(), context),
                           [&](const auto& value, const auto& child_expr)
                           { return formalism::apply(element.get_operation(), value, evaluate(child_expr, context)); });
}

inline ygg::float_t evaluate(formalism::FunctionTermView<LiftedTag, formalism::StaticTag> element, const ApplicabilityContext& context)
{
    const auto fterm_or_nullopt = formalism::try_ground(element, context.grounder());
    if (!fterm_or_nullopt.has_value())
        return std::numeric_limits<ygg::float_t>::quiet_NaN();

    return context.state().task().get_static_state().get(*fterm_or_nullopt);
}

inline ygg::float_t evaluate(formalism::FunctionTermView<LiftedTag, formalism::FluentTag> element, const ApplicabilityContext& context)
{
    const auto fterm_or_nullopt = formalism::try_ground(element, context.grounder());
    if (!fterm_or_nullopt.has_value())
        return std::numeric_limits<ygg::float_t>::quiet_NaN();

    return context.state().numeric_storage().get(fterm_or_nullopt->index());
}

inline ygg::float_t evaluate(formalism::FunctionTermView<LiftedTag, formalism::AuxiliaryTag>, const ApplicabilityContext& context)
{ return context.state().auxiliary_value(); }

inline ygg::float_t evaluate(formalism::FunctionExpressionView<LiftedTag> element, const ApplicabilityContext& context)
{
    return visit([&](auto&& arg) { return evaluate(arg, context); }, element);
}

template<formalism::FactKind T>
ygg::float_t evaluate(formalism::NumericEffectView<LiftedTag, T> element, const ApplicabilityContext& context)
{
    // Sequenced for the same reason as the arithmetic binary operator above.
    const auto fexpr_value = evaluate(element.get_fexpr(), context);
    const auto fterm_value = evaluate(element.get_fterm(), context);
    return formalism::apply(element.get_operation(), fterm_value, fexpr_value);
}

/**
 * is_applicable
 */

inline bool is_applicable(formalism::LiteralView<LiftedTag, formalism::StaticTag> element, const ApplicabilityContext& context)
{
    const auto atom_or_nullopt = formalism::try_ground(element.get_atom(), context.grounder());
    if (!atom_or_nullopt.has_value())
        return !element.get_polarity();

    return context.state().task().get_static_state().test(*atom_or_nullopt) == element.get_polarity();
}

inline bool is_applicable(formalism::LiteralView<LiftedTag, formalism::FluentTag> element, const ApplicabilityContext& context)
{
    const auto atom_or_nullopt = formalism::try_ground(element.get_atom(), context.grounder());
    if (!atom_or_nullopt.has_value())
        return !element.get_polarity();

    const auto fact_or_nullopt = context.fdr().get_fact(*atom_or_nullopt);

    if (!fact_or_nullopt.has_value())
        return !element.get_polarity();

    const auto& fact = *fact_or_nullopt;
    return (context.state().fact_storage().get(fact.get_variable().index()) == fact.get_value()) == element.get_polarity();
}

inline bool is_applicable(formalism::LiteralView<LiftedTag, formalism::DerivedTag> element, const ApplicabilityContext& context)
{
    const auto atom_or_nullopt = formalism::try_ground(element.get_atom(), context.grounder());
    if (!atom_or_nullopt.has_value())
        return !element.get_polarity();

    return context.state().atom_storage().test(atom_or_nullopt->index()) == element.get_polarity();
}

template<formalism::FactKind T>
bool is_applicable(formalism::LiteralListView<LiftedTag, T> elements, const ApplicabilityContext& context)
{
    return std::all_of(elements.begin(), elements.end(), [&](auto&& arg) { return is_applicable(arg, context); });
}

inline bool is_applicable(formalism::BooleanOperatorView<LiftedTag> element, const ApplicabilityContext& context) { return evaluate(element, context); }

inline bool is_applicable(formalism::BooleanOperatorListView<LiftedTag> elements, const ApplicabilityContext& context)
{
    return std::all_of(elements.begin(), elements.end(), [&](auto&& arg) { return is_applicable(arg, context); });
}

inline bool is_applicable(formalism::NumericEffectView<LiftedTag, formalism::FluentTag> element,
                          const ApplicabilityContext& context,
                          formalism::EffectFamilyList& ref_fluent_effect_families)
{
    auto fterm = formalism::try_ground(element.get_fterm(), context.grounder());
    if (!fterm.has_value())
    {
        if (element.get_operation() == formalism::NumericEffectOperatorKind::Assign)
            fterm = formalism::ground(element.get_fterm(), context.grounder()).first;
        else
            return false;
    }

    const auto fterm_index = fterm->index();
    if (ref_fluent_effect_families.size() <= fterm_index.value)
        ref_fluent_effect_families.resize(fterm_index.value + 1, formalism::EffectFamily::None);

    // Check non-conflicting effects
    const auto family = formalism::effect_family(element.get_operation());
    if (!formalism::is_compatible_effect_family(family, ref_fluent_effect_families[fterm_index.value]))
        return false;  /// incompatible effects

    ref_fluent_effect_families[fterm_index.value] = family;

    // Check fterm is well-defined in context
    if (element.get_operation() != formalism::NumericEffectOperatorKind::Assign)
    {
        if (std::isnan(context.state().numeric_storage().get(fterm_index)))
            return false;  /// target function is undefined and operator is not assign
    }

    // Check fexpr is well-defined in context
    return !std::isnan(evaluate(element.get_fexpr(), context));
}

inline bool is_applicable(formalism::NumericEffectOperatorListView<LiftedTag, formalism::FluentTag> elements,
                          const ApplicabilityContext& context,
                          formalism::EffectFamilyList& ref_fluent_effect_families)
{
    return std::all_of(elements.begin(), elements.end(), [&](auto&& arg) { return is_applicable(arg, context, ref_fluent_effect_families); });
}

inline bool is_applicable(formalism::NumericEffectView<LiftedTag, formalism::AuxiliaryTag> element, const ApplicabilityContext& context)
{
    // Check fexpr is well-defined in context
    return !std::isnan(evaluate(element.get_fexpr(), context));
}

// ConjunctiveCondition

inline bool is_applicable(formalism::ConjunctiveConditionView<LiftedTag> element, const ApplicabilityContext& context)
{
    return is_applicable(element.template get_literals<formalism::StaticTag>(), context)  //
           && is_applicable(element.template get_literals<formalism::FluentTag>(), context)
           && is_applicable(element.template get_literals<formalism::DerivedTag>(), context)  //
           && is_applicable(element.get_numeric_constraints(), context);
}

// ConjunctiveEffect

inline bool
is_applicable(formalism::ConjunctiveEffectView<LiftedTag> element, const ApplicabilityContext& context, formalism::EffectFamilyList& ref_fluent_effect_families)
{
    return is_applicable(element.get_numeric_effects(), context, ref_fluent_effect_families)
           && (!element.get_auxiliary_numeric_effect().has_value() || is_applicable(element.get_auxiliary_numeric_effect().value(), context));
}

// Axiom

inline bool is_applicable(formalism::AxiomView<LiftedTag> element, const ApplicabilityContext& context) { return is_applicable(element.get_body(), context); }

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::planning
{
// NumericEffectView

extern template ygg::float_t evaluate(formalism::NumericEffectView<LiftedTag, formalism::FluentTag> element, const ApplicabilityContext& context);
extern template ygg::float_t evaluate(formalism::NumericEffectView<LiftedTag, formalism::AuxiliaryTag> element, const ApplicabilityContext& context);

/**
 * is_applicable
 */

// LiteralListView

extern template bool is_applicable(formalism::LiteralListView<LiftedTag, formalism::StaticTag> elements, const ApplicabilityContext& context);
extern template bool is_applicable(formalism::LiteralListView<LiftedTag, formalism::FluentTag> elements, const ApplicabilityContext& context);
extern template bool is_applicable(formalism::LiteralListView<LiftedTag, formalism::DerivedTag> elements, const ApplicabilityContext& context);

}

#endif

#endif
