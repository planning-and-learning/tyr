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
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/mutable/atom.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/unification/structure_traits_impl.hpp"

#include <algorithm>
#include <tuple>
#include <utility>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/semantics/comparison.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::formalism::planning
{

struct MutableInvariant : ygg::comparison::Mixin<MutableInvariant>
{
    size_t num_rigid_variables = 0;
    size_t num_counted_variables = 0;
    MutableAtomList<FluentTag> atoms;
    ygg::UnorderedSet<PredicateView<FluentTag>> predicates;

    MutableInvariant() = default;

    MutableInvariant(size_t num_rigid_variables_, size_t num_counted_variables_, MutableAtomList<FluentTag> atoms_) :
        num_rigid_variables(num_rigid_variables_),
        num_counted_variables(num_counted_variables_),
        atoms(std::move(atoms_)),
        predicates()
    {
        canonicalize();
    }
    MutableInvariant(size_t num_rigid_variables_,
                     size_t num_counted_variables_,
                     MutableAtomList<FluentTag> atoms_,
                     ygg::UnorderedSet<PredicateView<FluentTag>> predicates_) :
        num_rigid_variables(num_rigid_variables_),
        num_counted_variables(num_counted_variables_),
        atoms(std::move(atoms_)),
        predicates(std::move(predicates_))
    {
    }
    // A template, like MutableAtom's view constructors, so the view is instantiated where the repository is complete.
    template<typename C>
    MutableInvariant(ygg::View<ygg::Index<Invariant>, C> element) :
        num_rigid_variables(element.get_num_rigid_variables()),
        num_counted_variables(element.get_num_counted_variables()),
        atoms(),
        predicates()
    {
        for (const auto atom : element.get_atoms())
            atoms.emplace_back(atom);
        canonicalize();
    }

    void canonicalize()
    {
        std::sort(atoms.begin(), atoms.end());
        atoms.erase(std::unique(atoms.begin(), atoms.end()), atoms.end());

        predicates.clear();
        for (const auto& atom : atoms)
            predicates.insert(atom.predicate);
    }

    auto identifying_members() const noexcept { return std::tie(num_rigid_variables, num_counted_variables, atoms); }
};

using MutableInvariantList = std::vector<MutableInvariant>;

}

namespace tyr::formalism::unification
{

template<>
struct structure_traits<planning::MutableInvariant>
{
    using Type = planning::MutableInvariant;

    template<typename F>
    static bool zip_terms(const Type& lhs, const Type& rhs, F&& f)
    {
        if (lhs.num_rigid_variables != rhs.num_rigid_variables)
            return false;
        if (lhs.num_counted_variables != rhs.num_counted_variables)
            return false;

        return zip_terms(lhs.atoms, rhs.atoms, f);
    }

    template<typename F>
    static Type transform_terms(const Type& value, F&& f)
    {
        return Type(value.num_rigid_variables, value.num_counted_variables, transform_terms(value.atoms, f), value.predicates);
    }
};

}  // namespace tyr::formalism::unification

#endif
