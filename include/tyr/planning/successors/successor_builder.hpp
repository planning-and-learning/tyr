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

#ifndef TYR_PLANNING_SUCCESSORS_SUCCESSOR_BUILDER_HPP_
#define TYR_PLANNING_SUCCESSORS_SUCCESSOR_BUILDER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/successors/distribution_builder.hpp"
#include "tyr/planning/successors/outcome_node_builder.hpp"
#include "tyr/planning/successors/successor_data.hpp"
#include "tyr/planning/successors/transition_builder.hpp"

#include <concepts>
#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <vector>

namespace urd
{
/// The builder of a successor whose outcomes nature picks directly, its children held only built (urd::built_only_t): its
/// label (its binding interned, or a row built: a borrowed binding copied) and its outcomes (builders from the
/// repository's pool), their states state builders of the node's repository, registered or not.
template<::tyr::planning::OutcomeEnvironment E, ::tyr::TaskKind Kind, typename C>
struct builder<::tyr::planning::Successor<E, Kind>, C>
{
    urd::key<::tyr::planning::Successor<E, Kind>> index;
    urd::built_only_t<urd::index<::tyr::planning::Transition>, C> label = urd::make_owned<urd::builder<::tyr::planning::Transition, C>>();
    urd::built_only_t<urd::flat_vector<urd::index<::tyr::planning::OutcomeNode<E, Kind>>>, C> outcomes;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"label", &builder::label>, urd::field<"outcomes", &builder::outcomes>> {}; }
};

/// The builder of a successor in the general environment, its children held only built (urd::built_only_t): its label
/// (its binding interned, or a row built: a borrowed binding copied) and its distributions (builders from the
/// repository's pool), their outcome states state builders of the node's repository, registered or not.
template<::tyr::TaskKind Kind, typename C>
struct builder<::tyr::planning::Successor<::tyr::planning::GeneralTag, Kind>, C>
{
    urd::key<::tyr::planning::Successor<::tyr::planning::GeneralTag, Kind>> index;
    urd::built_only_t<urd::index<::tyr::planning::Transition>, C> label = urd::make_owned<urd::builder<::tyr::planning::Transition, C>>();
    urd::built_only_t<urd::flat_vector<urd::index<::tyr::planning::Distribution<Kind>>>, C> distributions;

    static constexpr auto fields() noexcept
    { return urd::Fields<urd::field<"label", &builder::label>, urd::field<"distributions", &builder::distributions>> {}; }
};
}

namespace tyr::planning
{
/// Empties what nature picks from in a successor being built: its outcomes, or in the general environment its
/// distributions (they go back to their pool). A successor with none is not applicable.
template<NatureEnvironment E, TaskKind Kind>
void clear_outcomes(SuccessorBuilder<E, Kind>& out)
{
    if constexpr (std::same_as<E, GeneralTag>)
        out.distributions.clear();
    else
        out.outcomes.clear();
}

/// Whether a successor being built is applicable: nature has something to pick from.
template<NatureEnvironment E, TaskKind Kind>
bool has_outcomes(const SuccessorBuilder<E, Kind>& out)
{
    if constexpr (std::same_as<E, GeneralTag>)
        return !out.distributions.empty();
    else
        return !out.outcomes.empty();
}

}

#endif
