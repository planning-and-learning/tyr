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

#ifndef TYR_FORMALISM_PLANNING_INVARIANTS_MATCHING_HPP_
#define TYR_FORMALISM_PLANNING_INVARIANTS_MATCHING_HPP_

#include "tyr/formalism/atom_data.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/unification/structure_traits_impl.hpp"
#include "tyr/formalism/unification/substitution.hpp"

#include <cstddef>
#include <optional>

namespace tyr::formalism::invariant
{

using InvariantSubstitution = unification::SubstitutionFunction<Term>;

/// Whether `pattern`, an atom of an invariant with these numbers of rigid and counted variables, covers `element`: some
/// instance of its counted variables makes it `element`, its rigid variables as they are.
bool covers(std::size_t num_rigid_variables,
            std::size_t num_counted_variables,
            const urd::data<Atom<LiftedTag, FluentTag>>& pattern,
            const urd::data<Atom<LiftedTag, FluentTag>>& element);

/// Whether some atom of the candidate covers `element`.
bool covers(const urd::builder<Invariant, Repository>& inv, const urd::data<Atom<LiftedTag, FluentTag>>& element);

/// The substitution of every variable of the invariant that makes `pattern` the ground atom, if any.
std::optional<InvariantSubstitution> match_invariant_against_ground_atom(std::size_t num_rigid_variables,
                                                                         std::size_t num_counted_variables,
                                                                         const urd::data<Atom<LiftedTag, FluentTag>>& pattern,
                                                                         const urd::data<Atom<LiftedTag, FluentTag>>& ground_atom);

}  // namespace tyr::formalism::invariant

#endif
