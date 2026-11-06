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

#ifndef TYR_FORMALISM_DATALOG_NUMERIC_EFFECT_DATA_HPP_
#define TYR_FORMALISM_DATALOG_NUMERIC_EFFECT_DATA_HPP_

#include "tyr/formalism/datalog/declarations.hpp"

#include <tuple>
#include <utility>
#include <urd/elements/flat.hpp>

namespace urd
{

template<::tyr::TaskKind T, ::tyr::formalism::FactKind F>
struct data<::tyr::formalism::datalog::NumericEffect<T, F>>
{
    static_assert(std::same_as<F, ::tyr::formalism::FluentTag>, "Datalog numeric effects are currently only supported for fluent functions.");

    urd::index<::tyr::formalism::datalog::NumericEffect<T, F>> index;
    ::tyr::formalism::NumericEffectOperatorKind operator_kind = ::tyr::formalism::NumericEffectOperatorKind::Assign;
    urd::index<::tyr::formalism::datalog::FunctionTerm<T, F>> fterm;
    ::tyr::formalism::datalog::FunctionExpression<T> fexpr;

    data() = default;
    data(::tyr::formalism::NumericEffectOperatorKind operator_kind_,
         urd::index<::tyr::formalism::datalog::FunctionTerm<T, F>> fterm_,
         ::tyr::formalism::datalog::FunctionExpression<T> fexpr_) :
        index(),
        operator_kind(operator_kind_),
        fterm(fterm_),
        fexpr(fexpr_)
    {
    }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::operator_kind, &data::fterm, &data::fexpr> {}; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::operator_kind, &data::fterm, &data::fexpr> {}; }
};

}

#endif
