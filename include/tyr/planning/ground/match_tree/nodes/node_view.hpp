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

#ifndef TYR_PLANNING_GROUND_MATCH_TREE_NODES_NODE_VIEW_HPP_
#define TYR_PLANNING_GROUND_MATCH_TREE_NODES_NODE_VIEW_HPP_

#include "tyr/planning/ground/match_tree/declarations.hpp"
#include "tyr/planning/ground/match_tree/nodes/atom_view.hpp"
#include "tyr/planning/ground/match_tree/nodes/constraint_view.hpp"
#include "tyr/planning/ground/match_tree/nodes/generator_view.hpp"
#include "tyr/planning/ground/match_tree/nodes/node_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/variable_view.hpp"

#include <utility>
#include <yggdrasil/containers/variant.hpp>
#include <yggdrasil/core/types.hpp>

namespace ygg
{
using namespace ::tyr;
template<typename Tag, planning::match_tree::Context<Tag> C>
class View<ygg::Data<planning::match_tree::Node<Tag>>, C> : public ygg::DataViewBase<planning::match_tree::Node<Tag>, C>
{
public:
    View(const ygg::Data<planning::match_tree::Node<Tag>>& handle, const C& context) noexcept : ygg::DataViewBase<planning::match_tree::Node<Tag>, C>(handle, context) {}

    auto get_variant() const noexcept { return ygg::make_view(this->get_data().variant, this->get_context()); }
};
}

#endif
