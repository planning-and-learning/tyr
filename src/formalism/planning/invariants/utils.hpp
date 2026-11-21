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

#include "tyr/formalism/atom_data.hpp"
#include "tyr/formalism/planning/invariant_builder.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/unification/structure_traits_impl.hpp"

#include <algorithm>
#include <cstddef>
#include <urd/elements/clone.hpp>
#include <urd/elements/equal_to.hpp>
#include <urd/elements/less.hpp>
#include <urd/repr/slots/slots.hpp>
#include <urd/store.hpp>
#include <utility>
#include <variant>
#include <vector>

namespace tyr::formalism::invariant
{

/// What synthesis builds candidates with: the repository their predicates and objects are read through, and the pool
/// their atoms are held in (a candidate's atoms are records not interned yet). It outlives every candidate, and stays
/// where it is.
struct CandidateStorage
{
    const Repository* repository = nullptr;
    urd::Store<urd::slots<urd::data<Atom<LiftedTag, FluentTag>>>> atoms;
};

/// An atom of a candidate: the record its child holds (a candidate's atoms are always held).
inline const urd::data<Atom<LiftedTag, FluentTag>>& atom_of(const urd::child<Atom<LiftedTag, FluentTag>, Repository>& part)
{ return *std::get<urd::held_child<Atom<LiftedTag, FluentTag>, Repository>>(part); }

inline urd::data<Atom<LiftedTag, FluentTag>>& atom_of(urd::child<Atom<LiftedTag, FluentTag>, Repository>& part)
{ return *std::get<urd::held_child<Atom<LiftedTag, FluentTag>, Repository>>(part); }

/// Its atoms sorted and without duplicates: the canonical form candidates are compared in.
inline void canonicalize(urd::builder<Invariant, Repository>& inv)
{
    const auto less = urd::Less<urd::data<Atom<LiftedTag, FluentTag>>> {};
    const auto equal = urd::EqualTo<urd::data<Atom<LiftedTag, FluentTag>>> {};
    std::ranges::sort(inv.atoms, [&](const auto& lhs, const auto& rhs) { return less(atom_of(lhs), atom_of(rhs)); });
    const auto [first, last] = std::ranges::unique(inv.atoms, [&](const auto& lhs, const auto& rhs) { return equal(atom_of(lhs), atom_of(rhs)); });
    inv.atoms.erase(first, last);
}

/// A candidate of these atoms, each held in a slot of `storage`, canonical.
inline urd::builder<Invariant, Repository> make_candidate(std::size_t num_rigid_variables,
                                                          std::size_t num_counted_variables,
                                                          std::vector<urd::data<Atom<LiftedTag, FluentTag>>> atoms,
                                                          CandidateStorage& storage)
{
    auto result = urd::builder<Invariant, Repository>();
    result.num_rigid_variables = static_cast<ygg::uint_t>(num_rigid_variables);
    result.num_counted_variables = static_cast<ygg::uint_t>(num_counted_variables);
    for (auto& atom : atoms)
    {
        auto& part = result.atoms.emplace_back();
        part.emplace<urd::held_child<Atom<LiftedTag, FluentTag>, Repository>>(storage.atoms.make_handle());
        atom_of(part) = std::move(atom);
    }
    canonicalize(result);
    return result;
}

/// Whether some atom of the candidate has this predicate.
inline bool has_predicate(const urd::builder<Invariant, Repository>& inv, urd::index<Predicate<FluentTag>> predicate)
{
    return std::ranges::any_of(inv.atoms, [&](const auto& part) { return atom_of(part).predicate == predicate; });
}

/// The candidate's atom of this predicate, if any (an invariant's atoms have distinct predicates).
inline const urd::data<Atom<LiftedTag, FluentTag>>* find_part(const urd::builder<Invariant, Repository>& inv, urd::index<Predicate<FluentTag>> predicate)
{
    const auto it = std::ranges::find_if(inv.atoms, [&](const auto& part) { return atom_of(part).predicate == predicate; });
    return (it == inv.atoms.end()) ? nullptr : &atom_of(*it);
}

inline bool uses_counted(const Term& term, std::size_t num_rigid_variables)
{
    return std::visit(
        [&]<typename T>(const T& arg) -> bool
        {
            if constexpr (std::is_same_v<T, ParameterIndex>)
                return arg.value >= num_rigid_variables;
            else
                return false;
        },
        term);
}

inline std::size_t part_arity(const urd::data<Atom<LiftedTag, FluentTag>>& part, std::size_t num_rigid_variables)
{
    const bool has_counted = std::ranges::any_of(part.terms, [&](const Term& term) { return uses_counted(term, num_rigid_variables); });
    return has_counted ? part.terms.size() - 1 : part.terms.size();
}

inline bool atom_uses_counted(const urd::data<Atom<LiftedTag, FluentTag>>& atom, std::size_t num_rigid_variables)
{
    return std::ranges::any_of(atom.terms, [&](const Term& term) { return uses_counted(term, num_rigid_variables); });
}

inline urd::data<Atom<LiftedTag, FluentTag>> make_initial_atom(PredicateView<FluentTag> predicate, std::size_t counted_position)
{
    const auto arity = static_cast<std::size_t>(predicate.get_arity());

    auto result = urd::data<Atom<LiftedTag, FluentTag>>();
    result.predicate = predicate.index();

    if (counted_position == arity)
    {
        for (std::size_t i = 0; i < arity; ++i)
            result.terms.push_back(Term(ParameterIndex(i)));
    }
    else
    {
        const auto counted_index = ParameterIndex(arity - 1);
        for (std::size_t i = 0; i < arity; ++i)
        {
            if (i == counted_position)
                result.terms.push_back(Term(counted_index));
            else
                result.terms.push_back(Term((i < counted_position) ? ParameterIndex(i) : ParameterIndex(i - 1)));
        }
    }

    return result;
}

}

#endif
