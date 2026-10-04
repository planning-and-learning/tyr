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

#ifndef TYR_PLANNING_GROUND_MATCH_TREE_NODES_ATOM_VIEW_HPP_
#define TYR_PLANNING_GROUND_MATCH_TREE_NODES_ATOM_VIEW_HPP_

#include "tyr/formalism/planning/atom_view.hpp"
#include "tyr/planning/ground/match_tree/declarations.hpp"
#include "tyr/planning/ground/match_tree/nodes/atom_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/atom_index.hpp"
#include "tyr/planning/ground/match_tree/nodes/node_view.hpp"

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>
#include <yggdrasil/formalism/detail/view.hpp>

namespace ygg
{
using namespace ::tyr;
template<typename Tag, ygg::formalism::SymbolContextFor<planning::match_tree::AtomSelectorNode<Tag>> C>
class View<ygg::Index<planning::match_tree::AtomSelectorNode<Tag>>, C> :
    public ygg::formalism::detail::View<ygg::Index<planning::match_tree::AtomSelectorNode<Tag>>, C>
{
public:
    View(ygg::Index<planning::match_tree::AtomSelectorNode<Tag>> handle, const C& context) noexcept :
        ygg::formalism::detail::View<ygg::Index<planning::match_tree::AtomSelectorNode<Tag>>, C>(handle, context)
    {
    }

    auto get_atom() const noexcept { return ygg::make_view(this->get_data().atom, this->m_context->get_formalism_repository()); }
    auto get_true_child() const noexcept { return ygg::make_view(this->get_data().true_child, *this->m_context); }
    auto get_false_child() const noexcept { return ygg::make_view(this->get_data().false_child, *this->m_context); }
    auto get_dontcare_child() const noexcept { return ygg::make_view(this->get_data().dontcare_child, *this->m_context); }
};
}

#endif
