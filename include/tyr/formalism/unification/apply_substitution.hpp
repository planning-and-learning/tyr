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

#ifndef TYR_FORMALISM_UNIFICATION_APPLY_SUBSTITUTION_HPP_
#define TYR_FORMALISM_UNIFICATION_APPLY_SUBSTITUTION_HPP_

#include "tyr/formalism/unification/structure_traits.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/formalism/unification/structure_traits_impl.hpp"
#include "tyr/formalism/unification/substitution.hpp"
#include "tyr/formalism/unification/term_operations.hpp"

#include <algorithm>
#include <cassert>
#include <vector>
#include <urd/config.hpp>

namespace tyr::formalism::unification
{

[[nodiscard]] inline Term apply_substitution_once(const Term& term, const SubstitutionFunction<Term>& rho)
{
    if (!is_parameter(term))
        return term;

    const auto p = get_parameter(term);
    if (!rho.is_bound(p))
        return term;

    const auto& value = rho[p];
    URD_ASSUME(value.has_value());
    return Term(*value);
}

template<TermUnifiableStructure T>
[[nodiscard]] T apply_substitution_once(const T& value, const SubstitutionFunction<Term>& rho)
{
    return transform_terms(value, [&](const Term& term) { return apply_substitution_once(term, rho); });
}

[[nodiscard]] inline Term apply_substitution(const Term& term, const SubstitutionFunction<Term>& rho)
{ return apply_substitution_once(term, rho); }

template<TermUnifiableStructure T>
[[nodiscard]] T apply_substitution(const T& value, const SubstitutionFunction<Term>& rho)
{ return apply_substitution_once(value, rho); }

[[nodiscard]] inline Term apply_substitution_fixpoint(const Term& term, const SubstitutionFunction<Term>& rho)
{
    auto current = term;
    auto seen = urd::Store<urd::sequence<ParameterIndex>> {};

    while (is_parameter(current))
    {
        const auto p = get_parameter(current);

        // Stop on cycles and return the repeated parameter unchanged.
        if (std::find(seen.begin(), seen.end(), p) != seen.end())
            return current;

        seen.push_back(p);

        if (!rho.is_bound(p))
            return current;

        const auto& value = rho[p];
        URD_ASSUME(value.has_value());
        current = Term(*value);
    }

    return current;
}

template<TermUnifiableStructure T>
[[nodiscard]] T apply_substitution_fixpoint(const T& value, const SubstitutionFunction<Term>& rho)
{
    return transform_terms(value, [&](const Term& term) { return apply_substitution_fixpoint(term, rho); });
}

[[nodiscard]] inline SubstitutionFunction<Term> compose_substitutions(const SubstitutionFunction<Term>& outer, const SubstitutionFunction<Term>& inner)
{
    auto parameters = urd::Store<urd::sequence<ParameterIndex>> {};
    parameters.reserve(inner.parameters().size() + outer.parameters().size());

    const auto append_unique = [&](const auto& source)
    {
        for (const auto parameter : source.parameters())
        {
            if (std::find(parameters.begin(), parameters.end(), parameter) == parameters.end())
                parameters.push_back(parameter);
        }
    };

    append_unique(inner);
    append_unique(outer);

    auto result = SubstitutionFunction<Term>(std::move(parameters));

    // The resulting substitution applies `inner` first and `outer` second.
    for (const auto parameter : result.parameters())
    {
        auto value = Term(parameter);
        value = apply_substitution_fixpoint(apply_substitution_fixpoint(value, inner), outer);

        if (value != Term(parameter))
        {
            [[maybe_unused]] const auto inserted = result.assign(parameter, value);
            assert(inserted);
        }
    }

    return result;
}

}  // namespace tyr::formalism::unification

#endif
