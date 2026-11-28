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

#ifndef TYR_PLANNING_STATES_STORAGES_TEMPORAL_STORAGE_DATA_HPP_
#define TYR_PLANNING_STATES_STORAGES_TEMPORAL_STORAGE_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/states/storages/temporal_parts.hpp"

#include <urd/elements/data.hpp>
#include <urd/elements/index.hpp>
#include <utility>

namespace urd
{

/// A state's temporal part (empty without durative actions or timed initial literals): its open durative actions, the
/// roles of its frontier timepoints, the timed initial literals that happened, and the temporal network over its frontier,
/// packed (temporal_parts.hpp). Its pool names them by its handle.
template<::tyr::TaskKind Kind>
struct data<::tyr::planning::TemporalStorage<Kind>>
{
    urd::key<::tyr::planning::TemporalStorage<Kind>> index;
    ::tyr::planning::TemporalWords parts;

    data() = default;

    explicit data(::tyr::planning::TemporalWords parts_) : index(), parts(std::move(parts_)) {}

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"parts", &data::parts>> {}; }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::planning::TemporalStorage<Kind>> key() const noexcept { return index; }
};

}  // namespace urd

#endif
