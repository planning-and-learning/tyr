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

#ifndef TYR_PLANNING_PLAN_HPP_
#define TYR_PLANNING_PLAN_HPP_

#include "tyr/planning/node.hpp"

#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

template<TaskKind Kind>
class Plan
{
private:
    Node<Kind> m_start_node;
    LabeledNodeList<Kind> m_labeled_succ_nodes;

public:
    Plan(Node<Kind> start_node);
    Plan(Node<Kind> start_node, LabeledNodeList<Kind> labeled_succ_nodes);

    const Node<Kind>& get_start_node() const noexcept;
    const LabeledNodeList<Kind>& get_labeled_succ_nodes() const noexcept;
    ygg::float_t get_cost() const noexcept;
    size_t get_length() const noexcept;
    bool empty() const noexcept;
};
}

#endif
