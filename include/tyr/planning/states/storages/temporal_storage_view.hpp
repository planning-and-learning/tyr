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

#ifndef TYR_PLANNING_STATES_STORAGES_TEMPORAL_STORAGE_VIEW_HPP_
#define TYR_PLANNING_STATES_STORAGES_TEMPORAL_STORAGE_VIEW_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/states/storages/temporal_storage_builder.hpp"

#include <urd/elements/view.hpp>

/// A state's temporal part (a view of its builder): its open durative actions, frontier roles, timed initial literals
/// that happened and temporal network (temporal_parts.hpp).
template<::tyr::TaskKind Kind, typename V>
struct urd::view_mixin<::tyr::planning::TemporalStorage<Kind>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] decltype(auto) get_parts() const noexcept { return self().template member<&urd::record_of<V>::parts>(); }
};

#endif
