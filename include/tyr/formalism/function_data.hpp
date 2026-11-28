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

#ifndef TYR_FORMALISM_FUNCTION_DATA_HPP_
#define TYR_FORMALISM_FUNCTION_DATA_HPP_

#include "tyr/formalism/declarations.hpp"

#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <utility>
#include <yggdrasil/core/config.hpp>

namespace urd
{

template<::tyr::formalism::FactKind T>
struct data<::tyr::formalism::Function<T>>
{
    urd::key<::tyr::formalism::Function<T>> index;
    urd::flat_string name;
    ygg::uint_t arity = 0;

    data() = default;

    data(urd::flat_string name_, ygg::uint_t arity_) : index(), name(std::move(name_)), arity(arity_) {}

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"name", &data::name>, urd::field<"arity", &data::arity>> {}; }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::formalism::Function<T>> key() const noexcept { return urd::key<::tyr::formalism::Function<T>>(index); }
};

}

#endif
