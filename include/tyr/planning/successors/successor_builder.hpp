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
#include "tyr/planning/successors/successor_data.hpp"
#include "tyr/planning/successors/transition_builder.hpp"

#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <vector>

namespace urd
{
/// The builder of a successor: its label, a builder held in place (its binding interned, or a row built: a borrowed
/// binding copied), and its distributions, each held built (urd::built_child: a builder from the repository's pool),
/// their outcome states state builders of the node's repository, registered or not.
template<::tyr::TaskKind Kind, typename C>
struct builder<::tyr::planning::Successor<Kind>, C>
{
    urd::key<::tyr::planning::Successor<Kind>> index;
    urd::builder<::tyr::planning::Transition, C> label;
    std::vector<urd::built_child<::tyr::planning::Distribution<Kind>, C>> distributions;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"label", &builder::label>, urd::field<"distributions", &builder::distributions>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};
}

#endif
