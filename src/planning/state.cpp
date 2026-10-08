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
#include "tyr/planning/state_view.hpp"

namespace ygg
{
namespace planning = ::tyr::planning;

template struct Builder<planning::State<::tyr::GroundTag>>;
template struct Builder<planning::State<::tyr::LiftedTag>>;

template class View<Index<planning::State<::tyr::GroundTag>>, std::shared_ptr<planning::StateRepository<::tyr::GroundTag>>>;
template class View<Builder<planning::State<::tyr::GroundTag>>, planning::Task<::tyr::GroundTag>>;
template class View<Index<planning::PackedState<::tyr::GroundTag>>, std::shared_ptr<planning::StateRepository<::tyr::GroundTag>>>;
template class View<Index<planning::State<::tyr::LiftedTag>>, std::shared_ptr<planning::StateRepository<::tyr::LiftedTag>>>;
template class View<Builder<planning::State<::tyr::LiftedTag>>, planning::Task<::tyr::LiftedTag>>;
template class View<Index<planning::PackedState<::tyr::LiftedTag>>, std::shared_ptr<planning::StateRepository<::tyr::LiftedTag>>>;

static_assert(planning::IterableStateConcept<planning::StateView<::tyr::GroundTag>>);
static_assert(planning::IterableViewStateConcept<planning::StateView<::tyr::GroundTag>>);
static_assert(planning::IterableStateConcept<planning::StateView<::tyr::LiftedTag>>);
static_assert(planning::IterableViewStateConcept<planning::StateView<::tyr::LiftedTag>>);

}
