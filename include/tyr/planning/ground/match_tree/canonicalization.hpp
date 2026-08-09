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



#ifndef TYR_PLANNING_GROUND_MATCH_TREE_CANONICALIZATION_HPP_
#define TYR_PLANNING_GROUND_MATCH_TREE_CANONICALIZATION_HPP_

#include "tyr/planning/ground/match_tree/nodes/atom_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/constraint_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/generator_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/node_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/variable_data.hpp"

#include <algorithm>
#include <yggdrasil/semantics/canonicalization.hpp>
#include <yggdrasil/semantics/comparators.hpp>

namespace tyr::planning::match_tree
{

template<typename Tag>
bool is_canonical(const ygg::Data<AtomSelectorNode<Tag>>&)
{
    return true;
};

template<typename Tag>
bool is_canonical(const ygg::Data<VariableSelectorNode<Tag>>&)
{
    return true;
};

template<typename Tag>
bool is_canonical(const ygg::Data<NegativeFactSelectorNode<Tag>>&)
{
    return true;
};

template<typename Tag>
bool is_canonical(const ygg::Data<NumericConstraintSelectorNode<Tag>>&)
{
    return true;
};

template<typename Tag>
bool is_canonical(const ygg::Data<ElementGeneratorNode<Tag>>&)
{
    return true;
};

template<typename Tag>
bool is_canonical(const ygg::Data<Node<Tag>>&)
{
    return true;
};

template<typename Tag>
void canonicalize(ygg::Data<AtomSelectorNode<Tag>>&) {};

template<typename Tag>
void canonicalize(ygg::Data<VariableSelectorNode<Tag>>&) {};

template<typename Tag>
void canonicalize(ygg::Data<NegativeFactSelectorNode<Tag>>&) {};

template<typename Tag>
void canonicalize(ygg::Data<NumericConstraintSelectorNode<Tag>>&) {};

template<typename Tag>
void canonicalize(ygg::Data<ElementGeneratorNode<Tag>>&) {};

template<typename Tag>
void canonicalize(ygg::Data<Node<Tag>>&) {};

}

#endif
