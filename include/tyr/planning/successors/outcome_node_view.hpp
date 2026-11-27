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

#ifndef TYR_PLANNING_SUCCESSORS_OUTCOME_NODE_VIEW_HPP_
#define TYR_PLANNING_SUCCESSORS_OUTCOME_NODE_VIEW_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/nodes/node_view.hpp"
#include "tyr/planning/successors/outcome_node_builder.hpp"
#include "tyr/planning/successors/outcome_node_data.hpp"

#include <urd/elements/view.hpp>

/// An outcome of a transition: its node, its probability and whether it fails.
template<::tyr::TaskKind Kind, typename V>
struct urd::view_mixin<::tyr::planning::OutcomeNode<Kind>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] auto get_node() const noexcept { return self().template member<&urd::record_of<V>::node>(); }

    [[nodiscard]] auto get_probability() const noexcept { return self().template member<&urd::record_of<V>::probability>(); }

    [[nodiscard]] auto get_failed() const noexcept { return self().template member<&urd::record_of<V>::failed>(); }
};

#endif
