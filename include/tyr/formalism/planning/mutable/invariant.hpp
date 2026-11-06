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

#ifndef TYR_FORMALISM_PLANNING_MUTABLE_INVARIANT_HPP_
#define TYR_FORMALISM_PLANNING_MUTABLE_INVARIANT_HPP_

#include "tyr/formalism/planning/grounder.hpp"
#include "tyr/formalism/planning/mutable/atom.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/unification/structure_traits_impl.hpp"

#include <algorithm>
#include <tuple>
#include <urd/elements/hash.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/values/std_set.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace tyr::formalism::planning
{

struct MutableInvariant : urd::compared_by_identity<MutableInvariant>
{
    size_t num_rigid_variables = 0;
    size_t num_counted_variables = 0;
    MutableAtomList<FluentTag> atoms;
    urd::Store<urd::set<PredicateView<FluentTag>>> predicates;

    MutableInvariant() = default;

    MutableInvariant(size_t num_rigid_variables_, size_t num_counted_variables_, MutableAtomList<FluentTag> atoms_) :
        num_rigid_variables(num_rigid_variables_),
        num_counted_variables(num_counted_variables_),
        atoms(std::move(atoms_)),
        predicates()
    { canonicalize(); }
    MutableInvariant(size_t num_rigid_variables_,
                     size_t num_counted_variables_,
                     MutableAtomList<FluentTag> atoms_,
                     urd::Store<urd::set<PredicateView<FluentTag>>> predicates_) :
        num_rigid_variables(num_rigid_variables_),
        num_counted_variables(num_counted_variables_),
        atoms(std::move(atoms_)),
        predicates(std::move(predicates_))
    {
    }

    void canonicalize()
    {
        std::sort(atoms.begin(), atoms.end());
        atoms.erase(std::unique(atoms.begin(), atoms.end()), atoms.end());

        predicates.clear();
        for (const auto& atom : atoms)
            predicates.insert(atom.predicate);
    }

    static constexpr auto identifying_members() noexcept
    { return urd::Members<&MutableInvariant::num_rigid_variables, &MutableInvariant::num_counted_variables, &MutableInvariant::atoms> {}; }
};

using MutableInvariantList = urd::Store<urd::sequence<MutableInvariant>>;

}

namespace tyr::formalism::unification
{

template<>
struct structure_traits<planning::MutableInvariant>
{
    using Type = planning::MutableInvariant;

    template<typename F>
    static bool zip_terms(const Type& lhs, const Type& rhs, F f)
    {
        if (lhs.num_rigid_variables != rhs.num_rigid_variables)
            return false;
        if (lhs.num_counted_variables != rhs.num_counted_variables)
            return false;

        return zip_terms(lhs.atoms, rhs.atoms, f);
    }

    template<typename F>
    static Type transform_terms(const Type& value, F f)
    { return Type(value.num_rigid_variables, value.num_counted_variables, transform_terms(value.atoms, f), value.predicates); }
};

}  // namespace tyr::formalism::unification

#endif
