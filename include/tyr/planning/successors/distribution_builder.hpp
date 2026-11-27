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

#ifndef TYR_PLANNING_SUCCESSORS_DISTRIBUTION_BUILDER_HPP_
#define TYR_PLANNING_SUCCESSORS_DISTRIBUTION_BUILDER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/successors/distribution_data.hpp"
#include "tyr/planning/successors/outcome_node_builder.hpp"

#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>

namespace urd
{
/// The builder of a distribution: its outcomes, each a child (interned, or a builder from the repository's pool).
template<::tyr::TaskKind Kind, typename C>
struct builder<::tyr::planning::Distribution<Kind>, C>
{
    urd::key<::tyr::planning::Distribution<Kind>> index;
    urd::built_t<urd::flat_vector<urd::index<::tyr::planning::OutcomeNode<Kind>>>, C> outcomes;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"outcomes", &builder::outcomes>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};
}

#endif
