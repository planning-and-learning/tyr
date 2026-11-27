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

#ifndef TYR_PLANNING_SUCCESSORS_OUTCOME_NODE_BUILDER_HPP_
#define TYR_PLANNING_SUCCESSORS_OUTCOME_NODE_BUILDER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/nodes/node_builder.hpp"
#include "tyr/planning/successors/outcome_node_data.hpp"

#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <yggdrasil/core/config.hpp>

namespace urd
{
/// The builder of an outcome: its node's builder (its state a child: built, or registered), its probability and whether it
/// fails.
template<::tyr::TaskKind Kind, typename C>
struct builder<::tyr::planning::OutcomeNode<Kind>, C>
{
    urd::built_t<urd::data<::tyr::planning::Node<Kind>>, C> node;
    urd::built_t<ygg::float_t, C> probability = 1;
    urd::built_t<bool, C> failed = false;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"node", &builder::node>, urd::field<"probability", &builder::probability>, urd::field<"failed", &builder::failed>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};
}

#endif
