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

#ifndef TYR_FORMALISM_CANONICALIZATION_HPP_
#define TYR_FORMALISM_CANONICALIZATION_HPP_

#include "tyr/formalism/declarations.hpp"

#include <urd/elements/data.hpp>

namespace tyr::formalism
{

/**
 * Canonicalization of the records shared by planning and datalog (found by ADL from prepare_for_insert). Every record
 * has a canonicalize overload, a no-op where it is canonical as built, so a record without one is a compile error.
 */

inline void canonicalize(urd::data<Variable>&)
{  // Trivially canonical
}

inline void canonicalize(urd::data<Object>&)
{
    // Trivially canonical
}

template<typename Tag>
void canonicalize(urd::data<RelationBinding<Tag>>&)
{
    // Trivially canonical
}

inline void canonicalize(Term&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(urd::data<Predicate<T>>&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(urd::data<Function<T>>&)
{
    // Trivially canonical
}

}

#endif
