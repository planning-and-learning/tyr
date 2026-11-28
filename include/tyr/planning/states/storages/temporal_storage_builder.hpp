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

#ifndef TYR_PLANNING_STATES_STORAGES_TEMPORAL_STORAGE_BUILDER_HPP_
#define TYR_PLANNING_STATES_STORAGES_TEMPORAL_STORAGE_BUILDER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/states/storages/state_storage.hpp"
#include "tyr/planning/states/storages/temporal_parts.hpp"
#include "tyr/planning/states/storages/temporal_storage_data.hpp"

#include <cstdint>
#include <span>
#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/concepts.hpp>
#include <urd/elements/index.hpp>
#include <urd/elements/view.hpp>
#include <utility>

namespace urd
{

/// A state's temporal part is built as its parts, the words a record holds read into them (in place: buffers kept).
template<typename C>
struct built_form<::tyr::planning::TemporalWords, C>
{
    using type = ::tyr::planning::TemporalParts;

    static void assign(const ::tyr::planning::TemporalWords& packed, type& into)
    { ::tyr::planning::unpack_temporal(std::span<const std::uint64_t>(packed.words.data(), packed.words.size()), into); }
};

/// A state's temporal part being built: its parts, unpacked (its record holds their words, temporal_storage_data.hpp).
template<::tyr::TaskKind Kind, typename C>
struct builder<::tyr::planning::TemporalStorage<Kind>, C>
{
    urd::key<::tyr::planning::TemporalStorage<Kind>> index;
    urd::built_t<::tyr::planning::TemporalWords, C> parts;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"parts", &builder::parts>> {}; }
};

}  // namespace urd

#endif
