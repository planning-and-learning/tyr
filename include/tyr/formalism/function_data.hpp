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

#include <string>
#include <tuple>
#include <utility>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{

template<::tyr::formalism::FactKind T>
struct Data<::tyr::formalism::Function<T>>
{
    ygg::Index<::tyr::formalism::Function<T>> index;
    ::cista::offset::string name;
    ygg::uint_t arity;

    Data() = default;
    Data(::cista::offset::string name_, ygg::uint_t arity_) : index(), name(std::move(name_)), arity(arity_) {}
    // Python constructor
    Data(const std::string& name_, ygg::uint_t arity_) : index(), name(name_), arity(arity_) {}
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    static constexpr auto cista_members() noexcept { return ygg::Members<&Data::index, &Data::name, &Data::arity> {}; }
    static constexpr auto identifying_members() noexcept { return ygg::Members<&Data::name, &Data::arity> {}; }
    void clear() noexcept { ygg::clear_members(*this); }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::Function<::tyr::formalism::StaticTag>>);
}

#endif
