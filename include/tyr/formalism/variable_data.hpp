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

#ifndef TYR_FORMALISM_VARIABLE_DATA_HPP_
#define TYR_FORMALISM_VARIABLE_DATA_HPP_

#include "tyr/formalism/declarations.hpp"

#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <utility>

namespace urd
{

template<>
struct data<::tyr::formalism::Variable>
{
    urd::key<::tyr::formalism::Variable> index;
    urd::flat_string name;

    data() = default;

    data(urd::flat_string name_) : index(), name(std::move(name_)) {}

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) noexcept = default;
    data& operator=(data&& other) noexcept = default;
    ~data() = default;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"name", &data::name>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::name> {}; }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::name> {}; }

    [[nodiscard]] urd::key<::tyr::formalism::Variable> key() const noexcept { return urd::key<::tyr::formalism::Variable>(index); }
};

}

#endif
