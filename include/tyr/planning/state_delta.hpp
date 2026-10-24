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

#ifndef TYR_PLANNING_STATE_DELTA_HPP_
#define TYR_PLANNING_STATE_DELTA_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"
#include "tyr/planning/declarations.hpp"

#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/semantics/equal_to.hpp>

namespace tyr::planning
{

/// Exact changes of a successor relative to its parent: only variables and function terms whose value differs.
struct StateDelta
{
    struct FluentChange
    {
        ygg::Index<formalism::planning::FDRVariable<formalism::FluentTag>> variable;
        formalism::planning::FDRValue from;
        formalism::planning::FDRValue to;

        friend bool operator==(const FluentChange&, const FluentChange&) = default;
    };

    struct NumericChange
    {
        ygg::Index<formalism::planning::FunctionTerm<GroundTag, formalism::FluentTag>> fterm;
        ygg::float_t from;
        ygg::float_t to;

        friend bool operator==(const NumericChange& lhs, const NumericChange& rhs) noexcept
        { return lhs.fterm == rhs.fterm && ygg::EqualTo<ygg::float_t> {}(lhs.from, rhs.from) && ygg::EqualTo<ygg::float_t> {}(lhs.to, rhs.to); }
    };

    std::vector<FluentChange> fluents;    ///< Sorted by variable, unique.
    std::vector<NumericChange> numerics;  ///< Sorted by function term, unique.

    void clear() noexcept
    {
        fluents.clear();
        numerics.clear();
    }

    friend bool operator==(const StateDelta&, const StateDelta&) = default;
};

/// Computes the delta from `parent` to `successor` by comparing every fluent variable and function term.
/// Linear in the state size, so it is meant for tests and debug checks only.
template<TaskKind Kind>
void compute_state_delta(const ygg::Builder<State<Kind>>& parent, const ygg::Builder<State<Kind>>& successor, StateDelta& out);

}

#endif
