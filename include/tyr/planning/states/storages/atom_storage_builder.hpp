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

#ifndef TYR_PLANNING_STATES_STORAGES_ATOM_STORAGE_BUILDER_HPP_
#define TYR_PLANNING_STATES_STORAGES_ATOM_STORAGE_BUILDER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/states/storages/atom_storage_data.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/states/storages/state_storage.hpp"

#include <cmath>
#include <cstddef>
#include <limits>
#include <span>
#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/concepts.hpp>
#include <urd/elements/index.hpp>
#include <vector>
#include <yggdrasil/core/config.hpp>

namespace urd
{

/// A state's derived atoms being built: their bits (its record holds their words, state_data.hpp).
template<::tyr::TaskKind Kind, typename C>
struct builder<::tyr::planning::AtomStorage<Kind>, C>
{
    urd::key<::tyr::planning::AtomStorage<Kind>> index;
    urd::built_t<::tyr::planning::StateBitWords, C> indices;

    void set(urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> atom) { indices.insert(static_cast<ygg::uint_t>(atom.value)); }

    void clear() { indices.clear(); }

    /// Takes the bits of a view of a builder of its kind (another state's).
    void assign(const urd::borrowed_view<::tyr::planning::AtomStorage<Kind>, C, builder>& storage) { indices = storage.get_indices(); }

    void swap(builder& other) noexcept
    {
        std::swap(index, other.index);
        indices.swap(other.indices);
    }

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"indices", &builder::indices>> {}; }

    static constexpr auto functions() noexcept
    {
        return urd::Functions<urd::function<"set", &builder::set, urd::arg<"index">>,
                              urd::function<"clear", &builder::clear>,
                              urd::function<"assign", &builder::assign, urd::arg<"storage">>,
                              urd::function<"swap", &builder::swap, urd::arg<"other">>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};

}  // namespace urd

#endif
