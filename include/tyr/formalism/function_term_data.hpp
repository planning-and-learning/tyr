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

#ifndef TYR_FORMALISM_FUNCTION_TERM_DATA_HPP_
#define TYR_FORMALISM_FUNCTION_TERM_DATA_HPP_

#include "tyr/formalism/declarations.hpp"

#include <tuple>
#include <urd/elements/flat.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <variant>
#include <vector>

namespace urd
{

template<::tyr::formalism::FactKind F>
struct data<::tyr::formalism::FunctionTerm<::tyr::LiftedTag, F>>
{
    urd::index<::tyr::formalism::FunctionTerm<::tyr::LiftedTag, F>> index;
    urd::index<::tyr::formalism::Function<F>> function;
    ::tyr::formalism::TermList terms;

    data() = default;
    data(urd::index<::tyr::formalism::Function<F>> function_, ::tyr::formalism::TermList terms_) : index(), function(function_), terms(std::move(terms_)) {}
    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::function, &data::terms> {}; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::function, &data::terms> {}; }
};

template<::tyr::formalism::FactKind F>
struct data<::tyr::formalism::FunctionTerm<::tyr::GroundTag, F>>
{
    urd::index<::tyr::formalism::FunctionTerm<::tyr::GroundTag, F>> index;
    ygg::RowIndex<::tyr::formalism::Function<F>> row;

    data() = default;
    data(ygg::RowIndex<::tyr::formalism::Function<F>> row_) : index(), row(row_) {}

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::row> {}; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::row> {}; }
};

}

#endif
