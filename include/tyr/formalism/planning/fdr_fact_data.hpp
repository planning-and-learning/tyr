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

#ifndef TYR_FORMALISM_FDR_FACT_DATA_HPP_
#define TYR_FORMALISM_FDR_FACT_DATA_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"

#include <tuple>
#include <urd/elements/flat.hpp>
#include <utility>

namespace urd
{

template<::tyr::formalism::FactKind T>
struct data<::tyr::formalism::planning::FDRFact<T>>
{
    urd::index<::tyr::formalism::planning::FDRVariable<T>> variable;
    ::tyr::formalism::planning::FDRValue value;

    data() = default;
    data(urd::index<::tyr::formalism::planning::FDRVariable<T>> variable_, ::tyr::formalism::planning::FDRValue value_) : variable(variable_), value(value_) {}

    static constexpr auto cista_members() noexcept { return urd::Members<&data::variable, &data::value> {}; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::variable, &data::value> {}; }
};

}

#endif
