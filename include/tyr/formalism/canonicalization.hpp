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

#include "tyr/formalism/datas.hpp"

#include <algorithm>
#include <urd/elements/less.hpp>
#include <yggdrasil/semantics/canonicalization.hpp>

namespace tyr::formalism
{

/**
 * Datalog
 */

inline bool is_canonical(const urd::data<Variable>&) { return true; }

inline bool is_canonical(const urd::data<Object>&) { return true; }

template<typename Tag>
bool is_canonical(const urd::data<RelationBinding<Tag>>&)
{ return true; }

inline bool is_canonical(const Term&) { return true; }

template<FactKind T>
bool is_canonical(const urd::data<Predicate<T>>&)
{ return true; }

template<FactKind T>
bool is_canonical(const urd::data<Function<T>>&)
{ return true; }

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
