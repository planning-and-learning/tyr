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

#ifndef TYR_DATALOG_APPLICABILITY_HPP_
#define TYR_DATALOG_APPLICABILITY_HPP_

#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/numeric_utils.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/declarations.hpp"

#include <algorithm>
#include <stdexcept>
#include <yggdrasil/core/closed_interval.hpp>

namespace tyr::datalog
{

/**
 * evaluate
 */

template<formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> evaluate(formalism::datalog::FunctionTermView<GroundTag, T> element, const FactSets& fact_sets);

ygg::ClosedInterval<ygg::float_t> evaluate(formalism::datalog::FunctionExpressionView<GroundTag> element, const FactSets& fact_sets);

bool evaluate(formalism::datalog::BooleanOperatorView<GroundTag> element, const FactSets& fact_sets);

/**
 * is_applicable
 */

template<formalism::FactKind T>
bool is_applicable(formalism::datalog::LiteralView<GroundTag, T> element, const FactSets& fact_sets);

template<formalism::FactKind T>
bool is_applicable(formalism::datalog::LiteralListView<GroundTag, T> elements, const FactSets& fact_sets);

bool is_applicable(formalism::datalog::BooleanOperatorListView<GroundTag> elements, const FactSets& fact_sets);

// GroundConjunctiveCondition

bool is_applicable(formalism::datalog::ConjunctiveConditionView<GroundTag> element, const FactSets& fact_sets);

// GroundRule

template<formalism::RelationKind R>
bool is_applicable(formalism::datalog::RuleView<GroundTag, R> element, const FactSets& fact_sets);

/**
 * is_statically_applicable
 */

// GroundConjunctiveCondition

bool is_statically_applicable(formalism::datalog::ConjunctiveConditionView<GroundTag> element, const FactSets& fact_sets);

// GroundRule

template<formalism::RelationKind R>
bool is_statically_applicable(formalism::datalog::RuleView<GroundTag, R> element, const FactSets& fact_sets);

/**
 * evaluate
 */

template<formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> evaluate(formalism::datalog::FunctionTermView<GroundTag, T> element, const FactSets& fact_sets)
{
    return fact_sets.template get<T>().function[element];
}

inline ygg::ClosedInterval<ygg::float_t> evaluate(formalism::datalog::FunctionTermView<GroundTag, formalism::AuxiliaryTag>, const FactSets&)
{
    throw std::logic_error("Auxiliary function terms are not stored in datalog fact sets.");
}

inline ygg::ClosedInterval<ygg::float_t> evaluate(formalism::datalog::FunctionExpressionView<GroundTag> element, const FactSets& fact_sets)
{
    return evaluate_numeric_expression(element, [&](const auto term) { return evaluate(term, fact_sets); });
}

inline bool evaluate(formalism::datalog::BooleanOperatorView<GroundTag> element, const FactSets& fact_sets)
{
    return evaluate_numeric_expression(element, [&](const auto term) { return evaluate(term, fact_sets); });
}

/**
 * is_applicable
 */

template<formalism::FactKind T>
bool is_applicable(formalism::datalog::LiteralView<GroundTag, T> element, const FactSets& fact_sets)
{
    return fact_sets.template get<T>().predicate.contains(element.get_atom().get_row()) == element.get_polarity();
}

template<formalism::FactKind T>
bool is_applicable(formalism::datalog::LiteralListView<GroundTag, T> elements, const FactSets& fact_sets)
{
    return std::all_of(elements.begin(), elements.end(), [&](auto&& arg) { return is_applicable(arg, fact_sets); });
}

inline bool is_applicable(formalism::datalog::BooleanOperatorListView<GroundTag> elements, const FactSets& fact_sets)
{
    return std::all_of(elements.begin(), elements.end(), [&](auto&& arg) { return evaluate(arg, fact_sets); });
}

// GroundConjunctiveCondition

inline bool is_applicable(formalism::datalog::ConjunctiveConditionView<GroundTag> element, const FactSets& fact_sets)
{
    return is_applicable(element.template get_literals<formalism::StaticTag>(), fact_sets)     //
           && is_applicable(element.template get_literals<formalism::FluentTag>(), fact_sets)  //
           && is_applicable(element.get_numeric_constraints(), fact_sets);
}

inline bool is_dynamically_applicable(formalism::datalog::ConjunctiveConditionView<GroundTag> element, const FactSets& fact_sets)
{
    return is_applicable(element.template get_literals<formalism::FluentTag>(), fact_sets)
           && is_applicable(element.get_numeric_constraints(), fact_sets);
}

// GroundRule

template<formalism::RelationKind R>
inline bool is_applicable(formalism::datalog::RuleView<GroundTag, R> element, const FactSets& fact_sets)
{
    return is_applicable(element.get_body(), fact_sets);
}

/**
 * is_statically_applicable
 */

// GroundConjunctiveCondition

inline bool is_statically_applicable(formalism::datalog::ConjunctiveConditionView<GroundTag> element, const FactSets& fact_sets)
{
    return is_applicable(element.template get_literals<formalism::StaticTag>(), fact_sets);
}

// GroundRule

template<formalism::RelationKind R>
inline bool is_statically_applicable(formalism::datalog::RuleView<GroundTag, R> element, const FactSets& fact_sets)
{
    return is_statically_applicable(element.get_body(), fact_sets);
}

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::datalog
{
extern template ygg::ClosedInterval<ygg::float_t> evaluate(formalism::datalog::FunctionTermView<GroundTag, formalism::StaticTag> element,
                                                           const FactSets& fact_sets);
extern template ygg::ClosedInterval<ygg::float_t> evaluate(formalism::datalog::FunctionTermView<GroundTag, formalism::FluentTag> element,
                                                           const FactSets& fact_sets);

/**
 * is_applicable
 */

extern template bool is_applicable(formalism::datalog::LiteralView<GroundTag, formalism::StaticTag> element, const FactSets& fact_sets);
extern template bool is_applicable(formalism::datalog::LiteralView<GroundTag, formalism::FluentTag> element, const FactSets& fact_sets);

extern template bool is_applicable(formalism::datalog::LiteralListView<GroundTag, formalism::StaticTag> elements, const FactSets& fact_sets);
extern template bool is_applicable(formalism::datalog::LiteralListView<GroundTag, formalism::FluentTag> elements, const FactSets& fact_sets);

// GroundConjunctiveCondition

// GroundRule

}

#endif

#endif
