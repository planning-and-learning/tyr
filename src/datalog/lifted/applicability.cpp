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

#include "tyr/datalog/lifted/applicability.hpp"

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::datalog
{

template ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::StaticTag> element,
                                                    const FactSets& fact_sets);
template ygg::ClosedInterval<ygg::float_t> evaluate(::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::FluentTag> element,
                                                    const FactSets& fact_sets);

/**
 * is_applicable
 */

template bool is_applicable(::tyr::formalism::datalog::GroundLiteralView<::tyr::formalism::StaticTag> element, const FactSets& fact_sets);
template bool is_applicable(::tyr::formalism::datalog::GroundLiteralView<::tyr::formalism::FluentTag> element, const FactSets& fact_sets);

template bool is_applicable(::tyr::formalism::datalog::GroundLiteralListView<::tyr::formalism::StaticTag> elements, const FactSets& fact_sets);
template bool is_applicable(::tyr::formalism::datalog::GroundLiteralListView<::tyr::formalism::FluentTag> elements, const FactSets& fact_sets);

// GroundConjunctiveCondition

// GroundRule

/**
 * is_valid_binding
 */

template bool is_valid_binding(::tyr::formalism::datalog::LiteralView<::tyr::formalism::StaticTag> element,
                               const FactSets& fact_sets,
                               ::tyr::formalism::datalog::GrounderContext& context);
template bool is_valid_binding(::tyr::formalism::datalog::LiteralView<::tyr::formalism::FluentTag> element,
                               const FactSets& fact_sets,
                               ::tyr::formalism::datalog::GrounderContext& context);

template bool is_valid_binding(::tyr::formalism::datalog::LiteralListView<::tyr::formalism::StaticTag> elements,
                               const FactSets& fact_sets,
                               ::tyr::formalism::datalog::GrounderContext& context);
template bool is_valid_binding(::tyr::formalism::datalog::LiteralListView<::tyr::formalism::FluentTag> elements,
                               const FactSets& fact_sets,
                               ::tyr::formalism::datalog::GrounderContext& context);

template ygg::ClosedInterval<ygg::float_t> is_valid_binding(::tyr::formalism::datalog::NumericEffectView<::tyr::formalism::FluentTag> element,
                                                            const FactSets& fact_sets,
                                                            ::tyr::formalism::datalog::GrounderContext& context);
template ygg::ClosedInterval<ygg::float_t> is_valid_binding(::tyr::formalism::datalog::NumericEffectOperatorView<::tyr::formalism::FluentTag> element,
                                                            const FactSets& fact_sets,
                                                            ::tyr::formalism::datalog::GrounderContext& context);

}

#endif
