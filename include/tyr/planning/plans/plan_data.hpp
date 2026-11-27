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
#ifndef TYR_PLANNING_PLANS_PLAN_DATA_HPP_
#define TYR_PLANNING_PLANS_PLAN_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/nodes/labeled_node_data.hpp"
#include "tyr/planning/nodes/node_data.hpp"

#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <utility>

namespace urd
{
/// A plan, packed: its start node and the labeled nodes that follow it, each its record (its state's key). A value.
template<::tyr::TaskKind Kind>
struct data<::tyr::planning::Plan<Kind>>
{
    urd::data<::tyr::planning::Node<Kind>> start_node;
    urd::flat_vector<urd::data<::tyr::planning::LabeledNode<Kind>>> labeled_succ_nodes;

    data() = default;
    data(urd::data<::tyr::planning::Node<Kind>> start_node_, urd::flat_vector<urd::data<::tyr::planning::LabeledNode<Kind>>> labeled_succ_nodes_) :
        start_node(start_node_),
        labeled_succ_nodes(std::move(labeled_succ_nodes_))
    {
    }
    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    { return urd::Fields<urd::field<"start_node", &data::start_node>, urd::field<"labeled_succ_nodes", &data::labeled_succ_nodes>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::start_node, &data::labeled_succ_nodes> {}; }
};
}

#endif
