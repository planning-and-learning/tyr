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

#include <tuple>
#include <urd/elements/flat.hpp>
#include <utility>

namespace urd
{

template<>
struct data<::tyr::formalism::planning::Metric>
{
    urd::index<::tyr::formalism::planning::Metric> index;
    ::tyr::formalism::OptimizationDirection optimization_direction = ::tyr::formalism::OptimizationDirection::Minimize;
    ::tyr::formalism::FunctionExpression<::tyr::GroundTag> fexpr;

    data() = default;
    data(::tyr::formalism::OptimizationDirection optimization_direction_, ::tyr::formalism::FunctionExpression<::tyr::GroundTag> fexpr_) :
        index(),
        optimization_direction(optimization_direction_),
        fexpr(std::move(fexpr_))
    {
    }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::optimization_direction, &data::fexpr> {}; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::optimization_direction, &data::fexpr> {}; }
};

}

#endif
