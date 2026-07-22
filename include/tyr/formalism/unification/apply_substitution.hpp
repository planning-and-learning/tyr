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

#include <yggdrasil/semantics/comparison.hpp>
#include "tyr/formalism/unification/structure_traits.hpp"
#include "tyr/formalism/unification/structure_traits_impl.hpp"
#include "tyr/formalism/unification/substitution.hpp"
#include "tyr/formalism/unification/term_operations.hpp"

#include <algorithm>
#include <cassert>
#include <vector>

namespace tyr::formalism::unification
{

template<TermSubstitution S>
[[nodiscard]] ygg::Data<Term> apply_substitution_once(const ygg::Data<Term>& term, const S& rho)
{
    if (!is_parameter(term))
        return term;

    const auto p = get_parameter(term);
    if (!rho.is_bound(p))
        return term;

    return ygg::Data<Term>(*rho[p]);
}

template<TermUnifiableStructure T, TermSubstitution S>
[[nodiscard]] T apply_substitution_once(const T& value, const S& rho)
{
    return transform_terms(value, [&](const ygg::Data<Term>& term) { return apply_substitution_once(term, rho); });
}

template<TermSubstitution S>
[[nodiscard]] ygg::Data<Term> apply_substitution(const ygg::Data<Term>& term, const S& rho)
{
    return apply_substitution_once(term, rho);
}

template<TermUnifiableStructure T, TermSubstitution S>
[[nodiscard]] T apply_substitution(const T& value, const S& rho)
{
    return apply_substitution_once(value, rho);
}

template<TermSubstitution S>
[[nodiscard]] ygg::Data<Term> apply_substitution_fixpoint(const ygg::Data<Term>& term, const S& rho)
{
    auto current = term;
    auto seen = std::vector<ParameterIndex> {};

    while (is_parameter(current))
    {
        const auto p = get_parameter(current);

        // Stop on cycles and return the repeated parameter unchanged.
        if (std::find(seen.begin(), seen.end(), p) != seen.end())
            return current;

        seen.push_back(p);

        if (!rho.is_bound(p))
            return current;

        current = ygg::Data<Term>(*rho[p]);
    }

    return current;
}

template<TermUnifiableStructure T, TermSubstitution S>
[[nodiscard]] T apply_substitution_fixpoint(const T& value, const S& rho)
{
    return transform_terms(value, [&](const ygg::Data<Term>& term) { return apply_substitution_fixpoint(term, rho); });
}

template<TermSubstitution S1, TermSubstitution S2>
[[nodiscard]] SubstitutionFunction<ygg::Data<Term>> compose_substitutions(const S1& outer, const S2& inner)
{
    auto parameters = std::vector<ParameterIndex> {};
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

    auto result = SubstitutionFunction<ygg::Data<Term>>(std::move(parameters));

    // The resulting substitution applies `inner` first and `outer` second.
    for (const auto parameter : result.parameters())
    {
        auto value = ygg::Data<Term>(parameter);
        value = apply_substitution_fixpoint(apply_substitution_fixpoint(value, inner), outer);

        if (value != ygg::Data<Term>(parameter))
        {
            [[maybe_unused]] const auto inserted = result.assign(parameter, value);
            assert(inserted);
        }
    }

    return result;
}

}  // namespace tyr::formalism::unification

#endif
