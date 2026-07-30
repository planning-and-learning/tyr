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

#include "tyr/planning/plan.hpp"

#include "tyr/planning/ground/node.hpp"
#include "tyr/planning/lifted/node.hpp"
#include "tyr/planning/node.hpp"

#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

template<TaskKind Kind>
Plan<Kind>::Plan(Node<Kind> start_node) : Plan(start_node, LabeledNodeList<Kind> {})
{
}

template<TaskKind Kind>
Plan<Kind>::Plan(Node<Kind> start_node, LabeledNodeList<Kind> labeled_succ_nodes) :
    m_start_node(std::move(start_node)),
    m_labeled_succ_nodes(std::move(labeled_succ_nodes))
{
}

template<TaskKind Kind>
const Node<Kind>& Plan<Kind>::get_start_node() const noexcept
{
    return m_start_node;
}

template<TaskKind Kind>
const LabeledNodeList<Kind>& Plan<Kind>::get_labeled_succ_nodes() const noexcept
{
    return m_labeled_succ_nodes;
}

template<TaskKind Kind>
ygg::float_t Plan<Kind>::get_cost() const noexcept
{
    return !empty() ? m_labeled_succ_nodes.back().node.get_metric() : 0.;
}

template<TaskKind Kind>
size_t Plan<Kind>::get_length() const noexcept
{
    return m_labeled_succ_nodes.size();
}

template<TaskKind Kind>
bool Plan<Kind>::empty() const noexcept
{
    return m_labeled_succ_nodes.empty();
}

template class Plan<LiftedTag>;
template class Plan<GroundTag>;

}
