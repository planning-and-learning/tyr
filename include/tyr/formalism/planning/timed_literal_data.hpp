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

#ifndef TYR_FORMALISM_PLANNING_TIMED_LITERAL_DATA_HPP_
#define TYR_FORMALISM_PLANNING_TIMED_LITERAL_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <type_traits>
#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/index.hpp>
#include <yggdrasil/core/config.hpp>

namespace urd
{

/// A timed initial literal (PDDL 2.2): a ground fluent literal that becomes true (or false) at a time, whatever the plan
/// does.
template<>
struct data<::tyr::formalism::TimedLiteral>
{
    urd::key<::tyr::formalism::TimedLiteral> index;
    ygg::float_t time = 0;
    urd::index<::tyr::formalism::Literal<::tyr::GroundTag, ::tyr::formalism::FluentTag>> literal;

    data() = default;

    data(ygg::float_t time_, urd::index<::tyr::formalism::Literal<::tyr::GroundTag, ::tyr::formalism::FluentTag>> literal_) :
        index(),
        time(time_),
        literal(literal_)
    {
    }

    static constexpr bool copyable() noexcept
    {
        static_assert(std::is_trivially_copyable_v<data>, "a copyable record is trivially copyable");
        return true;
    }

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"time", &data::time>, urd::field<"literal", &data::literal>> {}; }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::formalism::TimedLiteral> key() const noexcept { return urd::key<::tyr::formalism::TimedLiteral>(index); }
};

}  // namespace urd

#endif
