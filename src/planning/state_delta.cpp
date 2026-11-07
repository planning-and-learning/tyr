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

#include "tyr/planning/state_delta.hpp"

#include "tyr/planning/ground/state_storage.hpp"
#include "tyr/planning/lifted/state_storage.hpp"
#include "tyr/planning/state_builder.hpp"

#include <algorithm>
#include <cstddef>
#include <type_traits>

namespace tyr::planning
{

namespace
{

template<TaskKind Kind>
size_t num_fluent_variables(const urd::data<State<Kind>>& state)
{
    const auto& storage = state.fact_storage;
    if constexpr (std::same_as<Kind, LiftedTag>)
        return storage.indices.size();
    else
        return storage.values.size();
}

}

template<TaskKind Kind>
void compute_state_delta(const urd::data<State<Kind>>& parent, const urd::data<State<Kind>>& successor, StateDelta& out)
{
    out.clear();

    const auto num_variables = std::max(num_fluent_variables(parent), num_fluent_variables(successor));
    for (size_t i = 0; i < num_variables; ++i)
    {
        const auto variable = urd::index<formalism::planning::FDRVariable<formalism::FluentTag>>(static_cast<ygg::uint_t>(i));
        const auto from = parent.get(variable);
        const auto to = successor.get(variable);
        if (from != to)
            out.fluents.push_back(StateDelta::FluentChange { variable, from, to });
    }

    const auto num_fterms = std::max(parent.numeric_storage.values.size(), successor.numeric_storage.values.size());
    for (size_t i = 0; i < num_fterms; ++i)
    {
        const auto fterm = urd::index<formalism::planning::FunctionTerm<GroundTag, formalism::FluentTag>>(static_cast<ygg::uint_t>(i));
        const auto from = parent.get(fterm);
        const auto to = successor.get(fterm);
        if (!urd::EqualTo<ygg::float_t> {}(from, to))
            out.numerics.push_back(StateDelta::NumericChange { fterm, from, to });
    }
}

template void compute_state_delta(const urd::data<State<GroundTag>>& parent, const urd::data<State<GroundTag>>& successor, StateDelta& out);
template void compute_state_delta(const urd::data<State<LiftedTag>>& parent, const urd::data<State<LiftedTag>>& successor, StateDelta& out);

}
