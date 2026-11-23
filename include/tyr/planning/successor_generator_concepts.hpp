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
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/declarations.hpp"

#include <concepts>
#include <cstdint>
#include <type_traits>
#include <urd/elements/builder.hpp>
#include <urd/kinds.hpp>
#include <urd/repr/sequences/segmented.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/slots/slots.hpp>
#include <urd/store.hpp>
#include <urd/tags.hpp>

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
using ActionBindingView = std::conditional_t<std::same_as<Kind, GroundTag>, formalism::ActionRowView, formalism::ActionRowBuilderView>;

/// The action schema a lifted binding names: an interned row's relation, or a borrowed binding's (a builder's), which is
/// the interned schema it was staged with (std::bad_variant_access where it is not interned).
/// Whether a binding reads its relation as an action view (an interned row, a record's borrowed view).
template<typename Binding>
concept ReadsActionView = requires(const Binding& binding) {
    { binding.get_relation() } -> std::same_as<formalism::ActionView<LiftedTag>>;
};

template<typename Binding>
formalism::ActionView<LiftedTag> action_of(const Binding& binding)
{
    if constexpr (ReadsActionView<Binding>)
        return binding.get_relation();
    else
        return std::get<formalism::ActionView<LiftedTag>>(binding.get_relation());
}

/// Borrowed lists append distinct builders without invalidating previous outputs or the source.
/// Callers keep those builders alive and unchanged for as long as their views are used.
template<TaskKind Kind>
using SuccessorBuilders = urd::Store<urd::sequence<urd::builder<State<Kind>, StateRepository<Kind>>>, urd::stable_refs>;

}

#endif
