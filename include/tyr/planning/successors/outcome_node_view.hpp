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

/// An outcome an adversary picks: its node and whether it fails.
template<::tyr::TaskKind Kind, typename V>
struct urd::view_mixin<::tyr::planning::OutcomeNode<::tyr::planning::NondeterministicTag, Kind>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] auto node() const noexcept { return self().template member<&urd::record_of<V>::node>(); }

    [[nodiscard]] auto failed() const noexcept { return self().template member<&urd::record_of<V>::failed>(); }
};

/// An outcome chance draws: its node, its probability and whether it fails.
template<::tyr::planning::ChanceEnvironment E, ::tyr::TaskKind Kind, typename V>
struct urd::view_mixin<::tyr::planning::OutcomeNode<E, Kind>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] auto node() const noexcept { return self().template member<&urd::record_of<V>::node>(); }

    [[nodiscard]] auto probability() const noexcept { return self().template member<&urd::record_of<V>::probability>(); }

    [[nodiscard]] auto failed() const noexcept { return self().template member<&urd::record_of<V>::failed>(); }
};

#endif
