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

#ifndef TYR_PLANNING_SUCCESSORS_TRANSITION_DATA_HPP_
#define TYR_PLANNING_SUCCESSORS_TRANSITION_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/planning/declarations.hpp"

#include <cstdint>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <yggdrasil/formalism/declarations.hpp>

namespace urd
{

/// A transition's label: its kind, its action binding (none for timed initial literals) and, for an end, the position of
/// the open action it ends among its state's (an action binding may run several times at once).
template<>
struct data<::tyr::planning::Transition>
{
    ::tyr::planning::TransitionKind kind;
    urd::flat_optional<urd::key<::tyr::formalism::Action<::tyr::LiftedTag>, ygg::Row<::tyr::formalism::Action<::tyr::LiftedTag>>>> binding;
    std::uint32_t open;

    data() = default;

    data(::tyr::planning::TransitionKind kind_, urd::flat_optional<urd::key<::tyr::formalism::Action<::tyr::LiftedTag>, ygg::Row<::tyr::formalism::Action<::tyr::LiftedTag>>>> binding_, std::uint32_t open_) :
        kind(kind_),
        binding(binding_),
        open(open_)
    {
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    { return urd::Fields<urd::field<"kind", &data::kind>, urd::field<"binding", &data::binding>, urd::field<"open", &data::open>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::kind, &data::binding, &data::open> {}; }
};

}

#endif
