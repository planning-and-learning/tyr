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

#ifndef TYR_FORMALISM_FUNCTION_TERM_VALUE_DATA_HPP_
#define TYR_FORMALISM_FUNCTION_TERM_VALUE_DATA_HPP_

#include "tyr/formalism/declarations.hpp"

#include <tuple>
#include <urd/elements/flat.hpp>
#include <utility>
#include <yggdrasil/core/config.hpp>

namespace urd
{

template<::tyr::formalism::FactKind F>
struct data<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, F>>
{
    urd::index<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, F>> index;
    urd::index<::tyr::formalism::FunctionTerm<::tyr::GroundTag, F>> fterm;
    ygg::float_t value = 0.0;

    data() = default;
    data(urd::index<::tyr::formalism::FunctionTerm<::tyr::GroundTag, F>> fterm_, ygg::float_t value_) : index(), fterm(fterm_), value(value_) {}

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::fterm, &data::value> {}; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::fterm, &data::value> {}; }
};

}

#endif
