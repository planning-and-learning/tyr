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

#ifndef TYR_PLANNING_SUCCESSOR_GENERATOR_HPP_
#define TYR_PLANNING_SUCCESSOR_GENERATOR_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/state_index.hpp"

#include <concepts>
#include <vector>
#include <yggdrasil/core/concepts.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::planning
{

template<TaskKind Kind>
class SuccessorGenerator;

template<typename T, typename Kind>
concept SuccessorGeneratorConcept = requires(T& r,
                                             ygg::Index<State<Kind>> state_index,
                                             const Node<Kind>& node,
                                             NodeList<Kind>& successor_nodes,
                                             LabeledNodeList<Kind>& labeled_successor_nodes,
                                             ::tyr::formalism::planning::ActionBindingView binding) {
    requires TaskKind<Kind>;
    { r.get_initial_node() } -> std::same_as<Node<Kind>>;
    { r.get_successor_nodes(node) } -> std::same_as<NodeList<Kind>>;
    { r.get_successor_nodes(node, successor_nodes) } -> std::same_as<void>;
    { r.get_labeled_successor_nodes(node) } -> std::same_as<LabeledNodeList<Kind>>;
    { r.get_labeled_successor_nodes(node, labeled_successor_nodes) } -> std::same_as<void>;
    { r.get_successor_node(node, binding) } -> std::same_as<Node<Kind>>;
    { r.get_node(state_index) } -> std::same_as<Node<Kind>>;
    { r.get_index() } -> std::same_as<ygg::uint_t>;
};

}

#endif
