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
#include "tyr/formalism/datalog/grounder.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_view.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include <yggdrasil/formalism/row_view.hpp>
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/formalism/numeric_expression_utils.hpp"

#include <algorithm>
#include <utility>
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
    const auto binding = try_ground_binding(element, context.grounder());
    if (!binding)
        return {};

    return value(context.fact_sets().template get<T>(), (*binding).data());
}

inline ygg::ClosedInterval<ygg::float_t> evaluate(formalism::datalog::FunctionExpressionView<LiftedTag> element, const ApplicabilityContext& context)
{
    return formalism::evaluate_numeric_expression(element, [&](const auto term) { return evaluate(term, context); });
}

inline bool evaluate(formalism::datalog::BooleanOperatorView<LiftedTag> element, const ApplicabilityContext& context)
{
    return formalism::evaluate_numeric_expression(element, [&](const auto term) { return evaluate(term, context); });
}

template<formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> evaluate(formalism::datalog::NumericEffectView<LiftedTag, T> element, const ApplicabilityContext& context)
{
    return formalism::evaluate_numeric_effect(element, [&](const auto term) { return evaluate(term, context); });
}

/**
 * is_applicable
 */

template<formalism::FactKind T>
bool is_applicable(formalism::datalog::LiteralView<LiftedTag, T> element, const ApplicabilityContext& context)
{
    const auto binding = try_ground_binding(element.get_atom(), context.grounder());
    if (!binding)
        return !element.get_polarity();

    return contains(context.fact_sets().template get<T>(), (*binding).data()) == element.get_polarity();
}

template<formalism::FactKind T>
bool is_applicable(formalism::datalog::LiteralListView<LiftedTag, T> elements, const ApplicabilityContext& context)
{
    return std::all_of(elements.begin(), elements.end(), [&](auto&& arg) { return is_applicable(arg, context); });
}

inline bool is_applicable(formalism::datalog::BooleanOperatorView<LiftedTag> element, const ApplicabilityContext& context)
{ return evaluate(std::move(element), context); }

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
{ return is_applicable(element.get_body(), context); }

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
}

#endif

#endif
