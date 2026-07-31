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

#ifndef TYR_PLANNING_NODE_HPP_
#define TYR_PLANNING_NODE_HPP_

#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/planning/state_index.hpp"
#include "tyr/planning/state_view.hpp"
#include "tyr/planning/task.hpp"

#include <concepts>
#include <ranges>
#include <tuple>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/semantics/comparison.hpp>

namespace tyr::planning
{
template<TaskKind Kind>
class Node : public ygg::comparison::Mixin<Node<Kind>>
{
public:
    using TaskType = Task<Kind>;

    Node(StateView<Kind> state, ygg::float_t metric) noexcept : m_state(std::move(state)), m_metric(metric) {}

    const StateView<Kind>& get_state() const noexcept { return m_state; }
    ygg::float_t get_metric() const noexcept { return m_metric; }

    auto identifying_members() const noexcept { return std::tie(m_state, m_metric); }

private:
    StateView<Kind> m_state;
    ygg::float_t m_metric;
};

template<TaskKind Kind>
using NodeList = std::vector<Node<Kind>>;

template<TaskKind Kind>
struct LabeledNode
{
    ::tyr::formalism::planning::ActionBindingView label;
    Node<Kind> node;
};

template<TaskKind Kind>
using LabeledNodeList = std::vector<LabeledNode<Kind>>;

template<typename T, typename Kind>
concept NodeConcept = requires(const T& cn) {
    requires TaskKind<Kind>;
    { cn.get_state() } -> std::same_as<const StateView<Kind>&>;
    { cn.get_metric() } -> std::same_as<ygg::float_t>;
};

}

#endif
