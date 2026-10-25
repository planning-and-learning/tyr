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

#ifndef TYR_FORMALISM_UNIFICATION_MATCH_POLICY_HPP_
#define TYR_FORMALISM_UNIFICATION_MATCH_POLICY_HPP_

#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/unification/match_state.hpp"

#include <cstdint>
#include <optional>

namespace tyr::formalism::unification
{

enum class ParameterRole : std::uint8_t
{
    rigid,
    sigma,
    counted,
    ignored
};

struct DefaultMatchPolicy
{
    template<typename T>
    bool match_parameter_parameter(ParameterIndex lhs, ParameterIndex rhs, MatchState<T>& state) const
    {
        const auto lhs_role = resolve_role(lhs, state);
        const auto rhs_role = resolve_role(rhs, state);

        if (lhs_role == ParameterRole::ignored || rhs_role == ParameterRole::ignored)
            return true;

        if (lhs_role == ParameterRole::sigma)
            return state.sigma.assign_or_check(lhs, T(rhs));

        if (lhs_role == ParameterRole::counted)
            return state.counted.assign_or_check(lhs, T(rhs));

        if (rhs_role == ParameterRole::sigma)
            return state.sigma.assign_or_check(rhs, T(lhs));

        if (rhs_role == ParameterRole::counted)
            return state.counted.assign_or_check(rhs, T(lhs));

        return lhs == rhs;
    }

    template<typename T>
    bool match_parameter_object(ParameterIndex lhs, const T& rhs, MatchState<T>& state) const
    {
        const auto lhs_role = resolve_role(lhs, state);

        switch (lhs_role)
        {
            case ParameterRole::sigma:
                return state.sigma.assign_or_check(lhs, rhs);
            case ParameterRole::counted:
                return state.counted.assign_or_check(lhs, rhs);
            case ParameterRole::ignored:
                return true;
            case ParameterRole::rigid:
                return false;
        }

        return false;
    }

    template<typename T>
    bool match_object_parameter(const T& lhs, ParameterIndex rhs, MatchState<T>& state) const
    {
        const auto rhs_role = resolve_role(rhs, state);

        switch (rhs_role)
        {
            case ParameterRole::sigma:
                return state.sigma.assign_or_check(rhs, lhs);
            case ParameterRole::counted:
                return state.counted.assign_or_check(rhs, lhs);
            case ParameterRole::ignored:
                return true;
            case ParameterRole::rigid:
                return false;
        }

        return false;
    }

private:
    template<typename T>
    static ParameterRole resolve_role(ParameterIndex p, const MatchState<T>& state)
    {
        const bool in_sigma = state.sigma.contains_parameter(p);
        const bool in_counted = state.counted.contains_parameter(p);

        if (in_sigma)
            return ParameterRole::sigma;
        if (in_counted)
            return ParameterRole::counted;
        return ParameterRole::rigid;
    }
};

}

#endif