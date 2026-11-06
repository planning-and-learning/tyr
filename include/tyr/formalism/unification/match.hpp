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

#ifndef TYR_FORMALISM_UNIFICATION_MATCH_HPP_
#define TYR_FORMALISM_UNIFICATION_MATCH_HPP_

#include "tyr/formalism/unification/match_result.hpp"
#include "tyr/formalism/unification/match_term.hpp"
#include "tyr/formalism/unification/structure_traits.hpp"
#include "tyr/formalism/unification/structure_traits_impl.hpp"

namespace tyr::formalism::unification
{

/// Attempts to match two structures while preserving failure information.
template<TermUnifiableStructure T, typename Policy = DefaultMatchPolicy>
[[nodiscard]] MatchResult<TermMatchState> match_ex(const T& pattern, const T& element, TermMatchState state, const Policy& policy = {})
{
    const bool ok = zip_terms(pattern, element, [&](const Term& lhs, const Term& rhs) { return match_term(lhs, rhs, state, policy); });

    if (!ok)
        return { std::nullopt, MatchFailure::structure_mismatch };

    return { std::move(state), MatchFailure::none };
}

/// Backward-compatible wrapper that discards failure details.
template<TermUnifiableStructure T, typename Policy = DefaultMatchPolicy>
[[nodiscard]] std::optional<TermMatchState> match(const T& pattern, const T& element, TermMatchState state, const Policy& policy = {})
{
    auto result = match_ex(pattern, element, std::move(state), policy);
    if (!result.has_value())
        return std::nullopt;

    return std::move(result.state);
}

/// Convenience overload that returns only the resulting sigma substitution.
template<TermUnifiableStructure T>
[[nodiscard]] std::optional<SubstitutionFunction<Term>> match(const T& pattern, const T& element, SubstitutionFunction<Term> sigma)
{
    TermMatchState state { std::move(sigma), SubstitutionFunction<Term>() };

    auto result = match(pattern, element, std::move(state), DefaultMatchPolicy {});
    if (!result.has_value())
        return std::nullopt;

    return std::move(result->sigma);
}

}

#endif