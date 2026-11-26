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

#ifndef TYR_PLANNING_JOINT_DISTRIBUTION_DATA_HPP_
#define TYR_PLANNING_JOINT_DISTRIBUTION_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/joint_outcome_data.hpp"

#include <type_traits>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <utility>

namespace urd
{

/// The joint outcomes chance draws from: one distribution an adversary may pick.
template<::tyr::TaskKind Kind>
struct data<::tyr::planning::JointDistribution<Kind>>
{
    urd::key<::tyr::planning::JointDistribution<Kind>> index;
    urd::flat_vector<urd::index<::tyr::planning::JointOutcome<Kind>>> joints;

    data() = default;

    explicit data(urd::flat_vector<urd::index<::tyr::planning::JointOutcome<Kind>>> joints_) : index(), joints(std::move(joints_)) {}

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"joints", &data::joints>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::joints> {}; }

    [[nodiscard]] urd::key<::tyr::planning::JointDistribution<Kind>> key() const noexcept { return urd::key<::tyr::planning::JointDistribution<Kind>>(index); }
};

}

#endif
