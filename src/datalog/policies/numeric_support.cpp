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

#include "tyr/datalog/policies/numeric_support.hpp"

#include "tyr/datalog/ground/policies/annotation_types.hpp"
#include "tyr/datalog/lifted/policies/annotation_types.hpp"

namespace fd = tyr::formalism::datalog;
namespace f = tyr::formalism;

namespace tyr::datalog
{

template<TaskKind Kind>
NumericSupportSelector<Kind>::NumericSupportSelector(const FactSets& fact_sets,
                                                     const NumericIntervalAnnotations<Kind>& annotations,
                                                     bool initial_intervals_cost_zero) :
    m_fact_sets(fact_sets),
    m_annotations(annotations),
    m_initial_intervals_cost_zero(initial_intervals_cost_zero)
{
}

template<TaskKind Kind>
typename NumericSupportSelector<Kind>::Key NumericSupportSelector<Kind>::fluent_key(fd::GroundFunctionTermView<f::FluentTag> term) const noexcept
{
    return term.get_row();
}

template<TaskKind Kind>
ygg::ClosedInterval<ygg::float_t> NumericSupportSelector<Kind>::lookup_static(fd::GroundFunctionTermView<f::StaticTag> term) const
{
    return m_fact_sets.static_sets.function[term];
}

template<TaskKind Kind>
ygg::ClosedInterval<ygg::float_t> NumericSupportSelector<Kind>::current_interval(Key key) const
{
    return m_fact_sets.fluent_sets.function[key];
}

template<TaskKind Kind>
const typename NumericIntervalAnnotations<Kind>::Entries* NumericSupportSelector<Kind>::find_entries(Key key) const
{
    return m_annotations.find_entries(key);
}

template class NumericSupportSelector<GroundTag>;
template class NumericSupportSelector<LiftedTag>;

}
