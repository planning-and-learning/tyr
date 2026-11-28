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

#ifndef TYR_PLANNING_SUCCESSORS_DISTRIBUTION_DATA_HPP_
#define TYR_PLANNING_SUCCESSORS_DISTRIBUTION_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/successors/outcome_node_data.hpp"

#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <utility>

namespace urd
{
/// The outcomes chance draws from, their probabilities summing to 1: one distribution an adversary may pick, which only
/// the general environment has. Stored in the state repository, each once; its outcomes there too (their indices).
template<::tyr::TaskKind Kind>
struct data<::tyr::planning::Distribution<Kind>>
{
    urd::key<::tyr::planning::Distribution<Kind>> index;
    urd::flat_vector<urd::index<::tyr::planning::OutcomeNode<::tyr::planning::GeneralTag, Kind>>> outcomes;

    data() = default;

    explicit data(urd::flat_vector<urd::index<::tyr::planning::OutcomeNode<::tyr::planning::GeneralTag, Kind>>> outcomes_) : index(), outcomes(std::move(outcomes_)) {}

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"outcomes", &data::outcomes>> {}; }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::planning::Distribution<Kind>> key() const noexcept { return index; }
};
}

#endif
