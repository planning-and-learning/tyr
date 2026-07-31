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

#ifndef TYR_FORMALISM_PLANNING_GROUND_FUNCTION_TERM_VALUE_DATA_HPP_
#define TYR_FORMALISM_PLANNING_GROUND_FUNCTION_TERM_VALUE_DATA_HPP_

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/ground_function_term_index.hpp"
#include "tyr/formalism/planning/ground_function_term_value_index.hpp"
#include "tyr/formalism/term_data.hpp"

namespace ygg
{


template<::tyr::formalism::FactKind T>
struct Data<::tyr::formalism::planning::GroundFunctionTermValue<T>>
{
    ygg::Index<::tyr::formalism::planning::GroundFunctionTermValue<T>> index;
    ygg::Index<::tyr::formalism::planning::GroundFunctionTerm<T>> fterm;
    ygg::float_t value;

    Data() = default;
    Data(ygg::Index<::tyr::formalism::planning::GroundFunctionTerm<T>> fterm_, ygg::float_t value_) : index(), fterm(fterm_), value(value_) {}
    // Python constructor
    template<typename C>
    Data(::ygg::View<ygg::Index<::tyr::formalism::planning::GroundFunctionTerm<T>>, C> fterm_, ygg::float_t value_) : index(), fterm(), value(value_)
    {
        ygg::set(fterm_, fterm);
    }
    Data(const Data& other) = default;
    Data& operator=(const Data& other) = default;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(fterm);
        ygg::clear(value);
    }

    auto cista_members() const noexcept { return std::tie(index, fterm, value); }
    auto identifying_members() const noexcept { return std::tie(fterm, value); }
};

static_assert(ygg::uses_trivial_storage_v<::tyr::formalism::planning::GroundFunctionTermValue<::tyr::formalism::StaticTag>>);
}

#endif
