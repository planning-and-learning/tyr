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

#ifndef TYR_PLANNING_GROUND_MATCH_TREE_NODES_GENERATOR_VIEW_HPP_
#define TYR_PLANNING_GROUND_MATCH_TREE_NODES_GENERATOR_VIEW_HPP_

#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/planning/ground/match_tree/declarations.hpp"
#include "tyr/planning/ground/match_tree/nodes/generator_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/generator_index.hpp"
#include "tyr/planning/ground/match_tree/nodes/node_view.hpp"

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{
using namespace ::tyr;
template<typename Tag, planning::match_tree::Context<Tag> C>
class View<ygg::Index<planning::match_tree::ElementGeneratorNode<Tag>>, C>
{
private:
    const C* m_context;
    ygg::Index<planning::match_tree::ElementGeneratorNode<Tag>> m_handle;

public:
    View(ygg::Index<planning::match_tree::ElementGeneratorNode<Tag>> handle, const C& context) noexcept : m_context(&context), m_handle(handle) {}

    const auto& get_data() const noexcept { return get_repository(*m_context)[m_handle]; }
    const auto& get_context() const noexcept { return *m_context; }
    const auto& get_handle() const noexcept { return m_handle; }

    auto get_elements() const noexcept { return ygg::make_view(get_data().elements, m_context->get_formalism_repository()); }

    auto identifying_members() const noexcept { return std::tie(m_handle, m_context->get_index()); }
};
}

#endif
