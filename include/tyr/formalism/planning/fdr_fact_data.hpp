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

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>
#include <yggdrasil/containers/variant.hpp>
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"
#include "tyr/formalism/planning/fdr_variable_index.hpp"

namespace ygg
{

template<::tyr::formalism::FactKind T>
struct Data<::tyr::formalism::planning::FDRFact<T>>
{
    ygg::Index<::tyr::formalism::planning::FDRVariable<T>> variable;
    ::tyr::formalism::planning::FDRValue value;

    Data() = default;
    Data(ygg::Index<::tyr::formalism::planning::FDRVariable<T>> variable_, ::tyr::formalism::planning::FDRValue value_) : variable(variable_), value(value_) {}
    // Python constructor
    template<typename C>
    Data(::ygg::View<ygg::Index<::tyr::formalism::planning::FDRVariable<T>>, C> variable_, ::tyr::formalism::planning::FDRValue value_) : variable(), value(value_)
    {
        set(variable_, variable);
    }
    Data(const Data& other) = default;
    Data& operator=(const Data& other) = default;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(variable);
        ygg::clear(value);
    }

    auto cista_members() const noexcept { return std::tie(variable, value); }
    auto identifying_members() const noexcept { return std::tie(variable, value); }
};

static_assert(ygg::uses_trivial_storage_v<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>);

}

#endif
