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

#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/state_repository.hpp"

template struct urd::builder<::tyr::planning::State<::tyr::GroundTag>, ::tyr::planning::StateRepository<::tyr::GroundTag>>;
template struct urd::builder<::tyr::planning::State<::tyr::LiftedTag>, ::tyr::planning::StateRepository<::tyr::LiftedTag>>;

namespace tyr::planning
{
// The range accessors StateViewConcept does not ask for (state_view_concepts.hpp), of both state views.
static_assert(::tyr::planning::IterableStateConcept<::tyr::planning::StateView<::tyr::GroundTag>>);
static_assert(::tyr::planning::IterableViewStateConcept<::tyr::planning::StateView<::tyr::GroundTag>>);
static_assert(::tyr::planning::IterableStateConcept<::tyr::planning::StateView<::tyr::LiftedTag>>);
static_assert(::tyr::planning::IterableViewStateConcept<::tyr::planning::StateView<::tyr::LiftedTag>>);
static_assert(::tyr::planning::IterableStateConcept<::tyr::planning::StateBuilderView<::tyr::GroundTag>>);
static_assert(::tyr::planning::IterableViewStateConcept<::tyr::planning::StateBuilderView<::tyr::GroundTag>>);
static_assert(::tyr::planning::IterableStateConcept<::tyr::planning::StateBuilderView<::tyr::LiftedTag>>);
static_assert(::tyr::planning::IterableViewStateConcept<::tyr::planning::StateBuilderView<::tyr::LiftedTag>>);
}
