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

#ifndef TYR_SRC_FORMALISM_PLANNING_INVARIANTS_UTILS_HPP_
#define TYR_SRC_FORMALISM_PLANNING_INVARIANTS_UTILS_HPP_

#include "tyr/formalism/planning/invariants/invariant.hpp"
#include "tyr/formalism/planning/mutable/atom.hpp"
#include "tyr/formalism/planning/repository.hpp"

namespace tyr::formalism::planning::invariant
{

inline const MutableAtom<FluentTag>* find_part(const Invariant& inv, PredicateView<FluentTag> predicate)
{
    const auto it = std::find_if(inv.atoms.begin(),
                                 inv.atoms.end(),
                                 [&](const auto& atom) { return atom.predicate == predicate; });

    return (it == inv.atoms.end()) ? nullptr : &*it;
}

inline size_t part_arity(const MutableAtom<FluentTag>& part, size_t num_rigid_variables)
{
    bool has_counted = false;

    for (const auto& term : part.terms)
    {
        std::visit(
            [&](auto&& arg)
            {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, ParameterIndex>)
                {
                    if (static_cast<ygg::uint_t>(arg) >= num_rigid_variables)
                        has_counted = true;
                }
            },
            term.value);
    }

    return has_counted ? part.terms.size() - 1 : part.terms.size();
}

inline bool atom_uses_counted(const MutableAtom<FluentTag>& atom, size_t num_rigid_variables)
{
    return std::ranges::any_of(atom.terms,
                               [&](const auto& term)
                               {
                                   return std::visit(
                                       [&](auto&& arg) -> bool
                                       {
                                           using T = std::decay_t<decltype(arg)>;
                                           if constexpr (std::is_same_v<T, ParameterIndex>)
                                               return static_cast<ygg::uint_t>(arg) >= num_rigid_variables;
                                           return false;
                                       },
                                       term.value);
                               });
}

inline MutableAtom<FluentTag> make_initial_atom(PredicateView<FluentTag> predicate, size_t counted_position)
{
    const auto arity = static_cast<size_t>(predicate.get_arity());

    std::vector<ygg::Data<Term>> terms;
    terms.reserve(arity);

    if (counted_position == arity)
    {
        for (size_t i = 0; i < arity; ++i)
            terms.emplace_back(ygg::Data<Term>(ParameterIndex(i)));
    }
    else
    {
        const auto counted_index = ParameterIndex(arity - 1);

        for (size_t i = 0; i < arity; ++i)
        {
            if (i == counted_position)
            {
                terms.emplace_back(ygg::Data<Term>(counted_index));
            }
            else
            {
                const auto rigid_index = (i < counted_position) ? ParameterIndex(i) : ParameterIndex(i - 1);
                terms.emplace_back(ygg::Data<Term>(rigid_index));
            }
        }
    }

    return MutableAtom<FluentTag>(predicate, std::move(terms));
}

}

#endif
