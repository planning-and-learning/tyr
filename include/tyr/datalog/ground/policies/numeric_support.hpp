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

#ifndef TYR_DATALOG_GROUND_POLICIES_NUMERIC_SUPPORT_HPP_
#define TYR_DATALOG_GROUND_POLICIES_NUMERIC_SUPPORT_HPP_

#include "tyr/datalog/ground/policies/annotation_types.hpp"
#include "tyr/datalog/ground/workspaces/facts.hpp"
#include "tyr/datalog/policies/numeric_support_core.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <limits>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{

template<>
class NumericSupportSelector<GroundTag> :
    public NumericSupportSelectorCore<NumericSupportSelector<GroundTag>,
                                      ::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::FluentTag>,
                                      NumericSupportSelectorWorkspace<GroundTag>::SelectionEntry>
{
public:
    using Key = ::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::FluentTag>;
    using SelectionEntry = NumericSupportSelectorWorkspace<GroundTag>::SelectionEntry;
    using Core = NumericSupportSelectorCore<NumericSupportSelector<GroundTag>, Key, SelectionEntry>;

    NumericSupportSelector(const ConstFactsWorkspace<GroundTag>& static_facts,
                           const FactsWorkspace<GroundTag>& fluent_facts,
                           const NumericIntervalAnnotations<GroundTag>& annotations,
                           bool initial_intervals_cost_zero = false);

    /**
     * Storage accessors for the shared core.
     */

    static Key key_of(const SelectionEntry& entry) noexcept { return entry.key; }
    Key fluent_key(Key term) const noexcept { return term; }
    ygg::ClosedInterval<ygg::float_t> lookup_static(::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::StaticTag> term) const;
    ygg::ClosedInterval<ygg::float_t> current_interval(Key key) const;
    const NumericIntervalAnnotations<GroundTag>::Entries* find_entries(Key key) const;
    bool keys_equal(Key lhs, Key rhs) const noexcept { return lhs.get_index() == rhs.get_index(); }
    /// Runs without annotations price initial intervals at zero instead of treating them as unreachable.
    Cost missing_entries_cost() const noexcept { return m_initial_intervals_cost_zero ? Cost(0) : std::numeric_limits<Cost>::max(); }

private:
    const ConstFactsWorkspace<GroundTag>& m_static_facts;
    const FactsWorkspace<GroundTag>& m_fluent_facts;
    const NumericIntervalAnnotations<GroundTag>& m_annotations;
    bool m_initial_intervals_cost_zero;
};

using GroundNumericSupportSelector = NumericSupportSelector<GroundTag>;

}

#endif
