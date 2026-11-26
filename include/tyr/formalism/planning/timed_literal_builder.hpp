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

#ifndef TYR_FORMALISM_PLANNING_TIMED_LITERAL_BUILDER_HPP_
#define TYR_FORMALISM_PLANNING_TIMED_LITERAL_BUILDER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/timed_literal_data.hpp"
#include "tyr/formalism/planning/timed_literal_view.hpp"

#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/concepts.hpp>
#include <urd/elements/index.hpp>
#include <yggdrasil/core/config.hpp>

namespace urd
{

/// The builder of TimedLiteral: its record's members, each in its built form (urd::built_t).
template<typename C>
struct builder<::tyr::formalism::TimedLiteral, C>
{
    urd::key<::tyr::formalism::TimedLiteral> index;
    ygg::float_t time = 0;
    urd::built_t<urd::index<::tyr::formalism::Literal<::tyr::GroundTag, ::tyr::formalism::FluentTag>>, C> literal;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"time", &builder::time>, urd::field<"literal", &builder::literal>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};

}  // namespace urd

#endif
