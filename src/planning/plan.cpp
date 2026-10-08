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

#include "tyr/planning/state_view.hpp"
#include "tyr/planning/ground/successor_generator.hpp"
#include "tyr/planning/lifted/successor_generator.hpp"
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

template<TaskKind Kind>
PackedPlan<Kind> Plan<Kind>::pack() const
{
    auto nodes = PackedLabeledNodeList<Kind> {};
    nodes.reserve(m_labeled_succ_nodes.size());
    for (const auto& node : m_labeled_succ_nodes)
        nodes.push_back(node.pack());
    return PackedPlan<Kind>(m_start_node.pack(), std::move(nodes));
}

template<TaskKind Kind>
PackedPlan<Kind>::PackedPlan(PackedNode<Kind> start_node) : PackedPlan(std::move(start_node), PackedLabeledNodeList<Kind> {})
{
}

template<TaskKind Kind>
PackedPlan<Kind>::PackedPlan(PackedNode<Kind> start_node, PackedLabeledNodeList<Kind> labeled_succ_nodes) :
    m_start_node(std::move(start_node)),
    m_labeled_succ_nodes(std::move(labeled_succ_nodes))
{
}

template<TaskKind Kind>
const PackedNode<Kind>& PackedPlan<Kind>::get_start_node() const noexcept
{
    return m_start_node;
}

template<TaskKind Kind>
const PackedLabeledNodeList<Kind>& PackedPlan<Kind>::get_labeled_succ_nodes() const noexcept
{
    return m_labeled_succ_nodes;
}

template<TaskKind Kind>
ygg::float_t PackedPlan<Kind>::get_cost() const noexcept
{
    return !empty() ? m_labeled_succ_nodes.back().node.get_metric() : 0.;
}

template<TaskKind Kind>
size_t PackedPlan<Kind>::get_length() const noexcept
{
    return m_labeled_succ_nodes.size();
}

template<TaskKind Kind>
bool PackedPlan<Kind>::empty() const noexcept
{
    return m_labeled_succ_nodes.empty();
}

template<TaskKind Kind>
Plan<Kind> PackedPlan<Kind>::unpack() const
{
    auto nodes = LabeledNodeList<Kind> {};
    nodes.reserve(m_labeled_succ_nodes.size());
    for (const auto& node : m_labeled_succ_nodes)
        nodes.push_back(node.unpack());
    return Plan<Kind>(m_start_node.unpack(), std::move(nodes));
}

template<TaskKind Kind>
PackedPlan<Kind> replay_plan(const PackedNode<Kind>& initial,
                             std::span<const formalism::planning::ActionBindingView> actions,
                             SuccessorGenerator<Kind>& generator,
                             StateRepository<Kind>& states,
                             AxiomEvaluator<Kind>& axioms)
{
    auto steps = PackedLabeledNodeList<Kind> {};
    steps.reserve(actions.size());
    auto node = initial.unpack();
    for (const auto action : actions)
    {
        node = generator.get_successor_node(node, action, states, axioms);
        steps.push_back({ action, node.pack() });
    }
    return PackedPlan<Kind>(initial, std::move(steps));
}

template PackedPlan<GroundTag> replay_plan(const PackedNode<GroundTag>& initial,
                                           std::span<const formalism::planning::ActionBindingView> actions,
                                           SuccessorGenerator<GroundTag>& generator,
                                           StateRepository<GroundTag>& states,
                                           AxiomEvaluator<GroundTag>& axioms);
template PackedPlan<LiftedTag> replay_plan(const PackedNode<LiftedTag>& initial,
                                           std::span<const formalism::planning::ActionBindingView> actions,
                                           SuccessorGenerator<LiftedTag>& generator,
                                           StateRepository<LiftedTag>& states,
                                           AxiomEvaluator<LiftedTag>& axioms);

template class PackedPlan<LiftedTag>;
template class PackedPlan<GroundTag>;

template class Plan<LiftedTag>;
template class Plan<GroundTag>;

}
