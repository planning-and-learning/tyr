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

#ifndef TYR_FORMALISM_NUMERIC_EFFECT_DATA_HPP_
#define TYR_FORMALISM_NUMERIC_EFFECT_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/enums.hpp"

#include <concepts>
#include <stdexcept>
#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/index.hpp>

namespace tyr::formalism
{
/// The operator of a default-constructed NumericEffect<T, F>: auxiliary effects only increase.
template<FactKind F>
inline constexpr auto default_numeric_effect_operator = std::same_as<F, AuxiliaryTag> ? NumericEffectOperatorKind::Increase : NumericEffectOperatorKind::Assign;
}

namespace urd
{

template<::tyr::TaskKind T, ::tyr::formalism::NumericEffectFactKind F>
struct data<::tyr::formalism::NumericEffect<T, F>>
{
    urd::key<::tyr::formalism::NumericEffect<T, F>> index;
    ::tyr::formalism::NumericEffectOperatorKind operation = ::tyr::formalism::default_numeric_effect_operator<F>;
    urd::index<::tyr::formalism::FunctionTerm<T, F>> fterm;
    ::tyr::formalism::FunctionExpression<T> fexpr;

    data() = default;

    data(::tyr::formalism::NumericEffectOperatorKind operation_,
         urd::index<::tyr::formalism::FunctionTerm<T, F>> fterm_,
         ::tyr::formalism::FunctionExpression<T> fexpr_) :
        index(),
        operation(operation_),
        fterm(fterm_),
        fexpr(fexpr_)
    {
        if constexpr (std::same_as<F, ::tyr::formalism::AuxiliaryTag>)
            if (operation != ::tyr::formalism::default_numeric_effect_operator<F>)
                throw std::invalid_argument("auxiliary numeric effect must be Increase");
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    { return urd::Fields<urd::field<"operation", &data::operation>, urd::field<"fterm", &data::fterm>, urd::field<"fexpr", &data::fexpr>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::operation, &data::fterm, &data::fexpr> {}; }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::operation, &data::fterm, &data::fexpr> {}; }

    [[nodiscard]] urd::key<::tyr::formalism::NumericEffect<T, F>> key() const noexcept { return urd::key<::tyr::formalism::NumericEffect<T, F>>(index); }
};

}

#endif
