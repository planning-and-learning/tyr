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

#ifndef TYR_PLANNING_JOINT_DISTRIBUTIONS_BUILDER_HPP_
#define TYR_PLANNING_JOINT_DISTRIBUTIONS_BUILDER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/joint_distributions_data.hpp"
#include "tyr/planning/joint_distribution_builder.hpp"

#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>

namespace urd
{

/// The builder of JointDistributions: its index and its record's fields, each in its built form (urd::built_t: a
/// child a key, or a pooled builder held by its handle).
template<::tyr::TaskKind Kind, typename C>
struct builder<::tyr::planning::JointDistributions<Kind>, C>
{
    urd::key<::tyr::planning::JointDistributions<Kind>> index;
    urd::built_t<urd::flat_vector<urd::index<::tyr::planning::JointDistribution<Kind>>>, C> distributions;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"distributions", &builder::distributions>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};

}

#endif
