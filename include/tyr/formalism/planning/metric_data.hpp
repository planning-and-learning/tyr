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

#ifndef TYR_FORMALISM_PLANNING_METRIC_DATA_HPP_
#define TYR_FORMALISM_PLANNING_METRIC_DATA_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/function_expression_data.hpp"
#include "tyr/formalism/planning/metric_index.hpp"

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{

template<>
struct Data<::tyr::formalism::planning::Metric>
{
    ygg::Index<::tyr::formalism::planning::Metric> index;
    ::tyr::formalism::OptimizationDirection optimization_direction = ::tyr::formalism::OptimizationDirection::Minimize;
    ygg::Data<::tyr::formalism::planning::FunctionExpression<::tyr::GroundTag>> fexpr;

    Data() = default;
    Data(::tyr::formalism::OptimizationDirection optimization_direction_, ygg::Data<::tyr::formalism::planning::FunctionExpression<::tyr::GroundTag>> fexpr_) :
        index(),
        optimization_direction(optimization_direction_),
        fexpr(fexpr_)
    {
    }
    // Python constructor
    template<typename C>
    Data(::tyr::formalism::OptimizationDirection optimization_direction_,
         ::ygg::View<ygg::Data<::tyr::formalism::planning::FunctionExpression<::tyr::GroundTag>>, C> fexpr_) :
        index(),
        optimization_direction(optimization_direction_),
        fexpr()
    {
        ygg::set(fexpr_, fexpr);
    }
    Data(const Data& other) = default;
    Data& operator=(const Data& other) = default;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        optimization_direction = ::tyr::formalism::OptimizationDirection::Minimize;
        ygg::clear(fexpr);
    }

    auto cista_members() const noexcept { return std::tie(index, optimization_direction, fexpr); }
    auto identifying_members() const noexcept { return std::tie(optimization_direction, fexpr); }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::Metric>);
}

#endif
