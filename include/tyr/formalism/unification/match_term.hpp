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

#ifndef TYR_FORMALISM_UNIFICATION_MATCH_TERM_HPP_
#define TYR_FORMALISM_UNIFICATION_MATCH_TERM_HPP_

#include "tyr/formalism/unification/match_policy.hpp"
#include "tyr/formalism/unification/match_state.hpp"

#include <urd/elements/less.hpp>


namespace tyr::formalism::unification
{

template<typename Policy = DefaultMatchPolicy>
bool match_term(const Term& pattern, const Term& element, TermMatchState& state, const Policy& policy = {})
{
    return std::visit(
        [&]<typename Lhs>(const Lhs& lhs) -> bool
        {
            return std::visit(
                [&]<typename Rhs>(const Rhs& rhs) -> bool
                {
                    if constexpr (std::is_same_v<Lhs, ParameterIndex>)
                    {
                        if constexpr (std::is_same_v<Rhs, ParameterIndex>)
                        {
                            return policy.match_parameter_parameter(lhs, rhs, state);
                        }
                        else if constexpr (std::is_same_v<Rhs, urd::index<Object>>)
                        {
                            return policy.match_parameter_object(lhs, Term(rhs), state);
                        }
                        else
                        {
                            static_assert(urd::repr::dependent_false<Rhs>::value, "Missing case");
                        }
                    }
                    else if constexpr (std::is_same_v<Lhs, urd::index<Object>>)
                    {
                        if constexpr (std::is_same_v<Rhs, ParameterIndex>)
                        {
                            return policy.match_object_parameter(Term(lhs), rhs, state);
                        }
                        else if constexpr (std::is_same_v<Rhs, urd::index<Object>>)
                        {
                            return lhs == rhs;
                        }
                        else
                        {
                            static_assert(urd::repr::dependent_false<Rhs>::value, "Missing case");
                        }
                    }
                    else
                    {
                        static_assert(urd::repr::dependent_false<Lhs>::value, "Missing case");
                    }
                },
                element);
        },
        pattern);
}

}  // namespace tyr::formalism::unification

#endif