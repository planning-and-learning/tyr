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

#include <yggdrasil/semantics/canonicalization.hpp>
#include <yggdrasil/semantics/comparators.hpp>
#include "tyr/formalism/datas.hpp"

#include <algorithm>

namespace tyr::formalism
{

/**
 * Datalog
 */

inline bool is_canonical(const ygg::Data<Variable>&) { return true; }

inline bool is_canonical(const ygg::Data<Object>&) { return true; }

template<typename Tag>
bool is_canonical(const ygg::Data<RelationBinding<Tag>>&)
{
    return true;
}

inline bool is_canonical(const ygg::Data<Term>&) { return true; }

template<FactKind T>
bool is_canonical(const ygg::Data<Predicate<T>>&)
{
    return true;
}

template<FactKind T>
bool is_canonical(const ygg::Data<Function<T>>&)
{
    return true;
}

inline void canonicalize(ygg::Data<Variable>&)
{  // Trivially canonical
}

inline void canonicalize(ygg::Data<Object>&)
{
    // Trivially canonical
}

template<typename Tag>
void canonicalize(ygg::Data<RelationBinding<Tag>>&)
{
    // Trivially canonical
}

inline void canonicalize(ygg::Data<Term>&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(ygg::Data<Predicate<T>>&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(ygg::Data<Function<T>>&)
{
    // Trivially canonical
}

}

#endif
