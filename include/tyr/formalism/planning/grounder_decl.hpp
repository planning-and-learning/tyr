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

#include "tyr/formalism/planning/builder.hpp"
#include "tyr/formalism/planning/declarations.hpp"

#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::formalism::planning
{
struct GrounderContext
{
    Builder& builder;
    Repository& destination;
    ygg::IndexList<Object>& binding;
};

template<typename T>
struct GrounderCacheEntry;

template<>
struct GrounderCacheEntry<Action>
{
    using container_type = ygg::UnorderedMap<ygg::Index<RelationBinding<Action>>, ygg::Index<GroundAction>>;

    container_type container;
};

template<>
struct GrounderCacheEntry<Axiom>
{
    using container_type = ygg::UnorderedMap<ygg::Index<RelationBinding<Axiom>>, ygg::Index<GroundAxiom>>;

    container_type container;
};

}

#endif