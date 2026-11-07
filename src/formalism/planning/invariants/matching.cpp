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

#include "matching.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

#include "tyr/formalism/planning/mutable/atom.hpp"
#include "tyr/formalism/unification/match_state.hpp"
#include "tyr/formalism/unification/match_term.hpp"
#include "tyr/formalism/unification/structure_traits.hpp"
#include "tyr/formalism/unification/structure_traits_impl.hpp"
#include "tyr/formalism/unification/substitution.hpp"

#include <algorithm>
#include <yggdrasil/core/config.hpp>
#include <urd/config.hpp>

namespace tyr::formalism::planning::invariant
{
namespace
{

using TermMatchState = unification::MatchState<Term>;
using DefaultMatchPolicy = unification::DefaultMatchPolicy;

template<typename T, typename State, typename Matcher>
std::optional<State> match_structure(const T& pattern, const T& element, State state, Matcher matcher)
{
    const bool ok = unification::zip_terms(pattern, element, [&](const Term& lhs, const Term& rhs) { return matcher(lhs, rhs, state); });

    if (!ok)
        return std::nullopt;

    return state;
}

std::optional<InvariantSubstitution>
match_cover_against_atom(const MutableInvariant& inv, const MutableAtom<FluentTag>& pattern, const MutableAtom<FluentTag>& element)
{
    auto state = TermMatchState {
        .sigma = InvariantSubstitution::from_range(ParameterIndex { static_cast<ygg::uint_t>(inv.num_rigid_variables) }, inv.num_counted_variables),
        .counted = InvariantSubstitution {},
    };

    auto result = match_structure(pattern,
                                  element,
                                  std::move(state),
                                  [&](const Term& lhs, const Term& rhs, TermMatchState& st) -> bool
                                  { return unification::match_term(lhs, rhs, st, DefaultMatchPolicy {}); });

    if (!result.has_value())
        return std::nullopt;

    return std::move(result->sigma);
}

}  // namespace

bool covers(const MutableInvariant& inv, const MutableAtom<FluentTag>& element)
{
    for (const auto& atom : inv.atoms)
    {
        if (match_cover_against_atom(inv, atom, element).has_value())
            return true;
    }

    return false;
}

std::optional<InvariantSubstitution>
match_invariant_against_ground_atom(const MutableInvariant& inv, const MutableAtom<FluentTag>& pattern, const MutableAtom<FluentTag>& ground_atom)
{
    auto state = TermMatchState {
        .sigma = InvariantSubstitution::from_range(ParameterIndex { 0 }, inv.num_rigid_variables + inv.num_counted_variables),
        .counted = InvariantSubstitution {},
    };

    auto result = match_structure(pattern,
                                  ground_atom,
                                  std::move(state),
                                  [&](const Term& lhs, const Term& rhs, TermMatchState& st) -> bool
                                  { return unification::match_term(lhs, rhs, st, DefaultMatchPolicy {}); });

    if (!result.has_value())
        return std::nullopt;

    return std::move(result->sigma);
}

}  // namespace tyr::formalism::planning::invariant
