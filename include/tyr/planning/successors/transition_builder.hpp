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

#ifndef TYR_PLANNING_SUCCESSORS_TRANSITION_BUILDER_HPP_
#define TYR_PLANNING_SUCCESSORS_TRANSITION_BUILDER_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/successors/transition_data.hpp"

#include <cstdint>
#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <yggdrasil/formalism/declarations.hpp>
#include <yggdrasil/formalism/row_builder.hpp>

namespace urd
{

/// The builder of a transition's label: its record's members, each in its built form (urd::built_t).
template<typename C>
struct builder<::tyr::planning::Transition, C>
{
    urd::key<::tyr::planning::Transition> index;
    urd::built_t<::tyr::planning::TransitionKind, C> kind;
    urd::built_t<urd::flat_optional<urd::key<::tyr::formalism::Action<::tyr::LiftedTag>, ygg::Row<::tyr::formalism::Action<::tyr::LiftedTag>>>>, C> binding;
    urd::built_t<std::uint32_t, C> open;

    static constexpr auto fields() noexcept
    { return urd::Fields<urd::field<"kind", &builder::kind>, urd::field<"binding", &builder::binding>, urd::field<"open", &builder::open>> {}; }
};

}

#endif
