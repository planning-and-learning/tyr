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
#ifndef TYR_PLANNING_NODES_LABELED_NODE_BUILDER_HPP_
#define TYR_PLANNING_NODES_LABELED_NODE_BUILDER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/nodes/labeled_node_data.hpp"
#include "tyr/planning/nodes/node_builder.hpp"

#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/index.hpp>
#include <yggdrasil/formalism/declarations.hpp>
#include <yggdrasil/formalism/row_builder.hpp>

namespace urd
{
/// The builder of a labeled node: its label (an interned row of its action) and its node's builder.
template<::tyr::TaskKind Kind, typename C>
struct builder<::tyr::planning::LabeledNode<Kind>, C>
{
    urd::built_t<urd::key<::tyr::formalism::Action<::tyr::LiftedTag>, ygg::Row<::tyr::formalism::Action<::tyr::LiftedTag>>>, C> label;
    urd::built_t<urd::data<::tyr::planning::Node<Kind>>, C> node;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"label", &builder::label>, urd::field<"node", &builder::node>> {}; }
};
}

#endif
