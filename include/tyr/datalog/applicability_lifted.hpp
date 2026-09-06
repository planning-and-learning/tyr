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

template<formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> evaluate(formalism::datalog::FunctionTermView<LiftedTag, T> element, const ApplicabilityContext& context);

ygg::ClosedInterval<ygg::float_t> evaluate(formalism::datalog::FunctionExpressionView<LiftedTag> element, const ApplicabilityContext& context);

bool evaluate(formalism::datalog::BooleanOperatorView<LiftedTag> element, const ApplicabilityContext& context);

template<formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> evaluate(formalism::datalog::NumericEffectView<LiftedTag, T> element, const ApplicabilityContext& context);

template<formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> evaluate(formalism::datalog::NumericEffectOperatorView<LiftedTag, T> element, const ApplicabilityContext& context);

/**
 * is_applicable
 */

template<formalism::FactKind T>
bool is_applicable(formalism::datalog::LiteralView<LiftedTag, T> element, const ApplicabilityContext& context);

template<formalism::FactKind T>
bool is_applicable(formalism::datalog::LiteralListView<LiftedTag, T> elements, const ApplicabilityContext& context);

bool is_applicable(formalism::datalog::BooleanOperatorView<LiftedTag> element, const ApplicabilityContext& context);

bool is_applicable(formalism::datalog::BooleanOperatorListView<LiftedTag> elements, const ApplicabilityContext& context);

bool is_applicable(formalism::datalog::ConjunctiveConditionView<LiftedTag> element, const ApplicabilityContext& context);

template<formalism::RelationKind R>
bool is_applicable(formalism::datalog::RuleView<LiftedTag, R> element, const ApplicabilityContext& context);

/**
 * evaluate
 */

template<formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> evaluate(formalism::datalog::FunctionTermView<LiftedTag, T> element, const ApplicabilityContext& context)
{
    const auto binding = try_ground_binding(element, context.grounder);
    if (!binding)
        return {};

    return context.fact_sets.template get<T>().function[*binding];
}

inline ygg::ClosedInterval<ygg::float_t> evaluate(formalism::datalog::FunctionExpressionView<LiftedTag> element, const ApplicabilityContext& context)
{
    return evaluate_numeric_expression(element, [&](const auto term) { return evaluate(term, context); });
}

inline bool evaluate(formalism::datalog::BooleanOperatorView<LiftedTag> element, const ApplicabilityContext& context)
{
    return evaluate_numeric_expression(element, [&](const auto term) { return evaluate(term, context); });
}

template<formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> evaluate(formalism::datalog::NumericEffectView<LiftedTag, T> element, const ApplicabilityContext& context)
{
    return evaluate_numeric_effect(element, [&](const auto term) { return evaluate(term, context); });
}

template<formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> evaluate(formalism::datalog::NumericEffectOperatorView<LiftedTag, T> element, const ApplicabilityContext& context)
{
    return visit([&](auto&& arg) { return evaluate(arg, context); }, element.get_variant());
}

/**
 * is_applicable
 */

template<formalism::FactKind T>
bool is_applicable(formalism::datalog::LiteralView<LiftedTag, T> element, const ApplicabilityContext& context)
{
    const auto binding = try_ground_binding(element.get_atom(), context.grounder);
    if (!binding)
        return !element.get_polarity();

    return context.fact_sets.template get<T>().predicate.contains(*binding) == element.get_polarity();
}

template<formalism::FactKind T>
bool is_applicable(formalism::datalog::LiteralListView<LiftedTag, T> elements, const ApplicabilityContext& context)
{
    return std::all_of(elements.begin(), elements.end(), [&](auto&& arg) { return is_applicable(arg, context); });
}

inline bool is_applicable(formalism::datalog::BooleanOperatorView<LiftedTag> element, const ApplicabilityContext& context)
{
    return evaluate(element, context);
}

inline bool is_applicable(formalism::datalog::BooleanOperatorListView<LiftedTag> elements, const ApplicabilityContext& context)
{
    return std::all_of(elements.begin(), elements.end(), [&](auto&& arg) { return is_applicable(arg, context); });
}

inline bool is_applicable(formalism::datalog::ConjunctiveConditionView<LiftedTag> element, const ApplicabilityContext& context)
{
    return is_applicable(element.template get_literals<formalism::StaticTag>(), context)     //
           && is_applicable(element.template get_literals<formalism::FluentTag>(), context)  //
           && is_applicable(element.get_numeric_constraints(), context);
}

template<formalism::RelationKind R>
bool is_applicable(formalism::datalog::RuleView<LiftedTag, R> element, const ApplicabilityContext& context)
{
    return is_applicable(element.get_body(), context);
}

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::datalog
{
extern template bool is_applicable(formalism::datalog::LiteralView<LiftedTag, formalism::StaticTag> element, const ApplicabilityContext& context);
extern template bool is_applicable(formalism::datalog::LiteralView<LiftedTag, formalism::FluentTag> element, const ApplicabilityContext& context);

extern template bool is_applicable(formalism::datalog::LiteralListView<LiftedTag, formalism::StaticTag> elements, const ApplicabilityContext& context);
extern template bool is_applicable(formalism::datalog::LiteralListView<LiftedTag, formalism::FluentTag> elements, const ApplicabilityContext& context);

extern template ygg::ClosedInterval<ygg::float_t> evaluate(formalism::datalog::NumericEffectView<LiftedTag, formalism::FluentTag> element,
                                                           const ApplicabilityContext& context);
extern template ygg::ClosedInterval<ygg::float_t> evaluate(formalism::datalog::NumericEffectOperatorView<LiftedTag, formalism::FluentTag> element,
                                                           const ApplicabilityContext& context);
}

#endif

#endif
