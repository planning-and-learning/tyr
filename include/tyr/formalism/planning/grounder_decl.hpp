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

#ifndef TYR_FORMALISM_PLANNING_GROUNDER_DECL_HPP_
#define TYR_FORMALISM_PLANNING_GROUNDER_DECL_HPP_

#include "tyr/formalism/planning/declarations.hpp"

#include <urd/elements/equal_to.hpp>
#include <urd/elements/hash.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>

namespace tyr::formalism::planning
{
template<typename T>
struct GrounderCacheEntry;

template<>
struct GrounderCacheEntry<Axiom<LiftedTag>>
{
    urd::Store<urd::map<ygg::formalism::RelationBindingIndex<Axiom<LiftedTag>, ::tyr::formalism::ObjectTag>, urd::index<Axiom<GroundTag>>>> container;
};

}

#endif
