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

#ifndef TYR_PLANNING_SUCCESSOR_GENERATOR_CONCEPTS_HPP_
#define TYR_PLANNING_SUCCESSOR_GENERATOR_CONCEPTS_HPP_

#include "tyr/declarations.hpp"
#include <urd/repr/sequences/segmented.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/node_concepts.hpp"
#include "tyr/planning/state_view_concepts.hpp"

#include <concepts>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <vector>
#include <urd/repr/slots/slots.hpp>
#include <urd/store.hpp>
#include <yggdrasil/core/config.hpp>
#include <urd/elements/flat.hpp>
#include <yggdrasil/execution/declarations.hpp>

namespace tyr::planning
{

/// Result of checking an offered tuple without publishing its action binding.
enum class ActionBindingStatus : std::uint8_t
{
    APPLICABLE,
    OUTSIDE_PARAMETER_DOMAIN,
    INAPPLICABLE
};

struct ActionBindingResult;

/// Ground enumeration borrows compiled bindings; lifted enumeration borrows reusable binding data.
template<TaskKind Kind>
using BorrowedActionRowView =
    std::conditional_t<std::same_as<Kind, GroundTag>, formalism::planning::ActionRowView, formalism::planning::ActionRowDataView>;

/// Borrowed lists append distinct builders without invalidating previous outputs or the source.
/// Callers keep those builders alive and unchanged for as long as their views are used.
template<TaskKind Kind>
using SuccessorBuilders = urd::Store<urd::sequence<urd::builder<State<Kind>>>, urd::stable_refs>;

}

#endif
