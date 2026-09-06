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

#ifndef TYR_DATALOG_APPLICABILITY_LIFTED_HPP_
#define TYR_DATALOG_APPLICABILITY_LIFTED_HPP_

#include "tyr/datalog/applicability_lifted_decl.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/numeric_utils.hpp"
#include "tyr/formalism/datalog/grounder.hpp"
#include "tyr/formalism/datalog/views.hpp"

#include <algorithm>
#include <yggdrasil/core/closed_interval.hpp>

namespace tyr::datalog
{

/**
 * evaluate
 */

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::FunctionTermView<::tyr::LiftedTag, T> element, const ApplicabilityContext& context);

ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::FunctionExpressionView<::tyr::LiftedTag> element, const ApplicabilityContext& context);

bool evaluate(::tyr::formalism::datalog::LiftedBooleanOperatorView element, const ApplicabilityContext& context);

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::NumericEffectView<::tyr::LiftedTag, T> element, const ApplicabilityContext& context);

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::NumericEffectOperatorView<::tyr::LiftedTag, T> element, const ApplicabilityContext& context);

/**
 * is_applicable
 */

template<::tyr::formalism::FactKind T>
bool is_applicable(::tyr::formalism::datalog::LiteralView<::tyr::LiftedTag, T> element, const ApplicabilityContext& context);

template<::tyr::formalism::FactKind T>
bool is_applicable(::tyr::formalism::datalog::LiteralListView<::tyr::LiftedTag, T> elements, const ApplicabilityContext& context);

bool is_applicable(::tyr::formalism::datalog::LiftedBooleanOperatorView element, const ApplicabilityContext& context);

bool is_applicable(::tyr::formalism::datalog::LiftedBooleanOperatorListView elements, const ApplicabilityContext& context);

bool is_applicable(::tyr::formalism::datalog::ConjunctiveConditionView<::tyr::LiftedTag> element, const ApplicabilityContext& context);

template<::tyr::formalism::RelationKind R>
bool is_applicable(::tyr::formalism::datalog::RuleView<::tyr::LiftedTag, R> element, const ApplicabilityContext& context);

/**
 * evaluate
 */

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::FunctionTermView<::tyr::LiftedTag, T> element, const ApplicabilityContext& context)
{
    const auto binding = try_ground_binding(element, context.grounder);
    if (!binding)
        return {};

    return context.fact_sets.template get<T>().function[*binding];
}

inline ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::FunctionExpressionView<::tyr::LiftedTag> element, const ApplicabilityContext& context)
{
    return evaluate_numeric_expression(element, [&](const auto term) { return evaluate(term, context); });
}

inline bool evaluate(::tyr::formalism::datalog::LiftedBooleanOperatorView element, const ApplicabilityContext& context)
{
    return evaluate_numeric_expression(element, [&](const auto term) { return evaluate(term, context); });
}

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::NumericEffectView<::tyr::LiftedTag, T> element, const ApplicabilityContext& context)
{
    return evaluate_numeric_effect(element, [&](const auto term) { return evaluate(term, context); });
}

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::NumericEffectOperatorView<::tyr::LiftedTag, T> element, const ApplicabilityContext& context)
{
    return visit([&](auto&& arg) { return evaluate(arg, context); }, element.get_variant());
}

/**
 * is_applicable
 */

template<::tyr::formalism::FactKind T>
bool is_applicable(::tyr::formalism::datalog::LiteralView<::tyr::LiftedTag, T> element, const ApplicabilityContext& context)
{
    const auto binding = try_ground_binding(element.get_atom(), context.grounder);
    if (!binding)
        return !element.get_polarity();

    return context.fact_sets.template get<T>().predicate.contains(*binding) == element.get_polarity();
}

template<::tyr::formalism::FactKind T>
bool is_applicable(::tyr::formalism::datalog::LiteralListView<::tyr::LiftedTag, T> elements, const ApplicabilityContext& context)
{
    return std::all_of(elements.begin(), elements.end(), [&](auto&& arg) { return is_applicable(arg, context); });
}

inline bool is_applicable(::tyr::formalism::datalog::LiftedBooleanOperatorView element, const ApplicabilityContext& context)
{
    return evaluate(element, context);
}

inline bool is_applicable(::tyr::formalism::datalog::LiftedBooleanOperatorListView elements, const ApplicabilityContext& context)
{
    return std::all_of(elements.begin(), elements.end(), [&](auto&& arg) { return is_applicable(arg, context); });
}

inline bool is_applicable(::tyr::formalism::datalog::ConjunctiveConditionView<::tyr::LiftedTag> element, const ApplicabilityContext& context)
{
    return is_applicable(element.template get_literals<::tyr::formalism::StaticTag>(), context)     //
           && is_applicable(element.template get_literals<::tyr::formalism::FluentTag>(), context)  //
           && is_applicable(element.get_numeric_constraints(), context);
}

template<::tyr::formalism::RelationKind R>
bool is_applicable(::tyr::formalism::datalog::RuleView<::tyr::LiftedTag, R> element, const ApplicabilityContext& context)
{
    return is_applicable(element.get_body(), context);
}

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::datalog
{
extern template bool is_applicable(::tyr::formalism::datalog::LiteralView<::tyr::LiftedTag, ::tyr::formalism::StaticTag> element, const ApplicabilityContext& context);
extern template bool is_applicable(::tyr::formalism::datalog::LiteralView<::tyr::LiftedTag, ::tyr::formalism::FluentTag> element, const ApplicabilityContext& context);

extern template bool is_applicable(::tyr::formalism::datalog::LiteralListView<::tyr::LiftedTag, ::tyr::formalism::StaticTag> elements, const ApplicabilityContext& context);
extern template bool is_applicable(::tyr::formalism::datalog::LiteralListView<::tyr::LiftedTag, ::tyr::formalism::FluentTag> elements, const ApplicabilityContext& context);

extern template ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::NumericEffectView<::tyr::LiftedTag, ::tyr::formalism::FluentTag> element,
                                                           const ApplicabilityContext& context);
extern template ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::NumericEffectOperatorView<::tyr::LiftedTag, ::tyr::formalism::FluentTag> element,
                                                           const ApplicabilityContext& context);
}

#endif

#endif
