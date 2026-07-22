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

#ifndef TYR_PLANNING_LIFTED_NODE_HPP_
#define TYR_PLANNING_LIFTED_NODE_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/lifted/state_view.hpp"
#include "tyr/planning/node.hpp"

#include <tuple>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/semantics/comparison.hpp>

/**
 * Definitions
 */

namespace tyr::planning
{
template<>
class Node<LiftedTag> : public ygg::comparison::Mixin<Node<LiftedTag>>
{
public:
    using TaskType = Task<LiftedTag>;

    Node(StateView<LiftedTag> state, ygg::float_t metric) noexcept : m_state(std::move(state)), m_metric(metric) {}

    const StateView<LiftedTag>& get_state() const noexcept { return m_state; }
    ygg::float_t get_metric() const noexcept { return m_metric; }

    auto identifying_members() const noexcept { return std::tie(m_state, m_metric); }

private:
    StateView<LiftedTag> m_state;
    ygg::float_t m_metric;
};

}

#endif
