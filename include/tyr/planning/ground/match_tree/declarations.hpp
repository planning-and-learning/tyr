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

#ifndef TYR_PLANNING_GROUND_MATCH_TREE_DECLARATIONS_HPP_
#define TYR_PLANNING_GROUND_MATCH_TREE_DECLARATIONS_HPP_

#include "tyr/formalism/planning/declarations.hpp"

#include <concepts>
#include <variant>
#include <vector>
#include <boost/mp11.hpp>

namespace tyr::planning::match_tree
{
/**
 * Nodes
 */

template<typename Tag>
struct AtomSelectorNode
{
};

template<typename Tag>
struct VariableSelectorNode
{
};

template<typename Tag>
struct NegativeFactSelectorNode
{
};

template<typename Tag>
struct NumericConstraintSelectorNode
{
};

template<typename Tag>
struct ElementGeneratorNode
{
};

/// The node kinds of a match tree, in the order of the Node variant.
template<typename Tag>
using NodeTypes =
    boost::mp11::mp_list<AtomSelectorNode<Tag>, NumericConstraintSelectorNode<Tag>, VariableSelectorNode<Tag>, NegativeFactSelectorNode<Tag>, ElementGeneratorNode<Tag>>;

/// A match-tree node: one of the node kinds (NodeTypes, in that order). Stored as it is, not interned; read through a
/// Repository<Tag> (urd::resolve) as the std::variant of the kinds' views.
template<typename Tag>
using Node = urd::index_variant<NodeTypes<Tag>>;

template<typename Tag>
using RepositoryTypes = NodeTypes<Tag>;

/**
 * MatchTree
 */

template<typename Tag>
class MatchTree;
template<typename Tag>
using MatchTreePtr = std::unique_ptr<MatchTree<Tag>>;

/**
 * Aliases
 */

template<typename Tag>
class Repository;
template<typename Tag>
using RepositoryPtr = std::unique_ptr<Repository<Tag>>;

}

#endif
