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

#ifndef TYR_PLANNING_PICK_BUILDER_HPP_
#define TYR_PLANNING_PICK_BUILDER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/pick_data.hpp"
#include "tyr/formalism/planning/action_builder.hpp"
#include "tyr/formalism/planning/outcome_builder.hpp"
#include "tyr/formalism/object_builder.hpp"

#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>

namespace urd
{

/// The builder of Pick: its index and its record's fields, each in its built form (urd::built_t: a
/// child a key, or a pooled builder held by its handle).
template<::tyr::TaskKind Kind, typename C>
struct builder<::tyr::planning::Pick<Kind>, C>
{
    urd::key<::tyr::planning::Pick<Kind>> index;
    urd::built_t<urd::index<::tyr::formalism::Action<Kind>>, C> action;
    urd::built_t<urd::index<::tyr::formalism::Outcome<Kind>>, C> outcome;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Object>>, C> binding;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"action", &builder::action>, urd::field<"outcome", &builder::outcome>, urd::field<"binding", &builder::binding>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};

}

#endif
