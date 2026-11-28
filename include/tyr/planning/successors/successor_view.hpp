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

#ifndef TYR_PLANNING_SUCCESSORS_SUCCESSOR_VIEW_HPP_
#define TYR_PLANNING_SUCCESSORS_SUCCESSOR_VIEW_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/successors/distribution_view.hpp"
#include "tyr/planning/successors/outcome_node_view.hpp"
#include "tyr/planning/successors/successor_builder.hpp"
#include "tyr/planning/successors/successor_data.hpp"
#include "tyr/planning/successors/transition_view.hpp"

#include <urd/elements/view.hpp>

/// A successor whose outcomes nature picks directly: its label and its outcomes.
template<::tyr::planning::OutcomeEnvironment E, ::tyr::TaskKind Kind, typename V>
struct urd::view_mixin<::tyr::planning::Successor<E, Kind>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] auto label() const noexcept { return self().template member<&urd::record_of<V>::label>(); }

    [[nodiscard]] auto outcomes() const noexcept { return self().template member<&urd::record_of<V>::outcomes>(); }
};

/// A successor in the general environment: its label and its distributions.
template<::tyr::TaskKind Kind, typename V>
struct urd::view_mixin<::tyr::planning::Successor<::tyr::planning::GeneralTag, Kind>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] auto label() const noexcept { return self().template member<&urd::record_of<V>::label>(); }

    [[nodiscard]] auto distributions() const noexcept { return self().template member<&urd::record_of<V>::distributions>(); }
};

#endif
