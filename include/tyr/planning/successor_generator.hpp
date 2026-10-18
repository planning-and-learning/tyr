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

#ifndef TYR_PLANNING_SUCCESSOR_GENERATOR_HPP_
#define TYR_PLANNING_SUCCESSOR_GENERATOR_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/successor_generator_concepts.hpp"

#include <concepts>
#include <deque>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>
#include <yggdrasil/containers/shared_object_pool.hpp>
#include <yggdrasil/containers/span.hpp>
#include <yggdrasil/core/concepts.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/binding_view.hpp>

namespace tyr::planning
{

/// The binding is present exactly when status is APPLICABLE and borrows the task repository.
struct ActionBindingResult
{
    ActionBindingStatus status;
    std::optional<formalism::planning::ActionBindingView> binding;
};

}

#endif
