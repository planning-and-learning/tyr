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

#include "tyr/datalog/ground/policies/numeric_support.hpp"

namespace fd = tyr::formalism::datalog;
namespace f = tyr::formalism;

namespace tyr::datalog
{

NumericSupportSelector<GroundTag>::NumericSupportSelector(const ConstFactsWorkspace<GroundTag>& static_facts,
                                                          const FactsWorkspace<GroundTag>& fluent_facts,
                                                          const NumericIntervalAnnotations<GroundTag>& annotations,
                                                          bool initial_intervals_cost_zero) :
    m_static_facts(static_facts),
    m_fluent_facts(fluent_facts),
    m_annotations(annotations),
    m_initial_intervals_cost_zero(initial_intervals_cost_zero)
{
}

ygg::ClosedInterval<ygg::float_t> NumericSupportSelector<GroundTag>::lookup_static(fd::GroundFunctionTermView<f::StaticTag> term) const
{
    return m_static_facts.fact_sets.function[term];
}

ygg::ClosedInterval<ygg::float_t> NumericSupportSelector<GroundTag>::current_interval(Key key) const { return m_fluent_facts.fact_sets.function[key]; }

const NumericIntervalAnnotations<GroundTag>::Entries* NumericSupportSelector<GroundTag>::find_entries(Key key) const
{
    return m_annotations.find_entries(key.get_function().get_index(), key.get_row().get_index().row);
}

}
