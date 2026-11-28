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
#ifndef TYR_PLANNING_PLANS_PLAN_BUILDER_HPP_
#define TYR_PLANNING_PLANS_PLAN_BUILDER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/nodes/labeled_node_builder.hpp"
#include "tyr/planning/nodes/node_builder.hpp"
#include "tyr/planning/plans/plan_data.hpp"

#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>

namespace urd
{
/// The builder of a plan: its start node's builder and its labeled nodes' builders (their states unpacked, registered or
/// not; their labels interned).
template<::tyr::TaskKind Kind, typename C>
struct builder<::tyr::planning::Plan<::tyr::planning::DeterministicTag, Kind>, C>
{
    urd::built_t<urd::data<::tyr::planning::Node<Kind>>, C> start_node;
    urd::built_t<urd::flat_vector<urd::data<::tyr::planning::LabeledNode<Kind>>>, C> labeled_succ_nodes;

    static constexpr auto fields() noexcept
    { return urd::Fields<urd::field<"start_node", &builder::start_node>, urd::field<"labeled_succ_nodes", &builder::labeled_succ_nodes>> {}; }

};
}

#endif
