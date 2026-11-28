/*
 * Copyright (C) 2026 Dominik Drexler
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

#ifndef TYR_FORMALISM_INTERNING_HPP_
#define TYR_FORMALISM_INTERNING_HPP_

#include "tyr/formalism/canonicalization.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/grounding_context.hpp"
#include "tyr/formalism/repository.hpp"

#include <cstddef>
#include <urd/elements/view.hpp>
#include <yggdrasil/formalism/interning.hpp>
#include <yggdrasil/formalism/values.hpp>

/// What the repository's insert preparation needs beside yggdrasil's (prepare_for_insert makes a record canonical: its
/// fields as they promise, and canonicalization.hpp's overloads). Included where the repository's members are
/// instantiated (src/formalism/repository.cpp) and where a grounder needs schema_in_destination; kept out of
/// repository.hpp, so code that only reads or inserts does not compile the canonicalization.
namespace tyr::formalism
{

/// A lifted schema (a predicate, function, action or axiom) as the destination holds it: at the schema's own index. The
/// ground repository holds every schema it grounds there: in the domain layer it shares with the lifted task, or, for the
/// task's own derived predicates and axioms, as the copies instantiate_ground_task makes of them, in order.
template<typename T>
urd::index_view<T, Repository> schema_in_destination(urd::index_view<T, Repository> schema, GroundingContext<Builder, Repository>& context)
{ return context.destination().view(schema.index()); }

}

#endif
