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

#ifndef TYR_FORMALISM_PLANNING_INTERNING_HPP_
#define TYR_FORMALISM_PLANNING_INTERNING_HPP_

#include "tyr/formalism/planning/canonicalization.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/write_context.hpp"

#include <yggdrasil/formalism/interning.hpp>

/// Insertion into the planning repository: records are canonicalized before they are interned. Kept out of
/// repository.hpp, so code that only reads does not compile the canonicalization.
namespace tyr::formalism
{

using ygg::formalism::insert;

/// A record the repository holds as a symbol is made canonical before it is interned (a row is ygg's).
template<typename T>
    requires ygg::formalism::SupportsSymbol<Repository, T>
void prepare_for_insert(Repository&, urd::data<T>& data)
{ canonicalize(data); }

/// A lifted schema (a predicate, function, action or axiom) as the destination holds it: at the schema's own index. The
/// ground repository holds every schema it grounds there: in the domain layer it shares with the lifted task, or, for the
/// task's own derived predicates and axioms, as the copies instantiate_ground_task makes of them, in order.
template<typename T>
urd::index_view<T, Repository> schema_in_destination(urd::index_view<T, Repository> schema, WriteContext<Builder, Repository>& context)
{ return context.destination().view(schema.index()); }

}

#endif
