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

#ifndef TYR_FORMALISM_DATALOG_METRIC_DATA_HPP_
#define TYR_FORMALISM_DATALOG_METRIC_DATA_HPP_

#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/ground_function_expression_data.hpp"
#include "tyr/formalism/datalog/metric_index.hpp"

#include <tuple>
#include <utility>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{
using namespace ::tyr;

template<>
struct Data<::tyr::formalism::datalog::Metric>
{
    ygg::Index<::tyr::formalism::datalog::Metric> index;
    ygg::Data<::tyr::formalism::datalog::GroundFunctionExpression> fexpr;

    Data() = default;
    explicit Data(ygg::Data<::tyr::formalism::datalog::GroundFunctionExpression> fexpr_) : index(), fexpr(std::move(fexpr_)) {}
    template<typename C>
    explicit Data(::ygg::View<ygg::Data<::tyr::formalism::datalog::GroundFunctionExpression>, C> fexpr_) : index(), fexpr()
    {
        set(fexpr_, fexpr);
    }
    Data(const Data& other) = default;
    Data& operator=(const Data& other) = default;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(fexpr);
    }

    auto cista_members() const noexcept { return std::tie(index, fexpr); }
    auto identifying_members() const noexcept { return std::tie(fexpr); }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::datalog::Metric>);

}

#endif
