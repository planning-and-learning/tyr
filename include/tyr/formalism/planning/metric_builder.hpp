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

#ifndef TYR_FORMALISM_PLANNING_METRIC_BUILDER_HPP_
#define TYR_FORMALISM_PLANNING_METRIC_BUILDER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/enums.hpp"
#include "tyr/formalism/planning/metric_data.hpp"
#include "tyr/formalism/planning/metric_view.hpp"

#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/concepts.hpp>
#include <urd/elements/index.hpp>

namespace urd
{

/// The builder of Metric: its record's members, each in its built form (urd::built_t).
template<typename C>
struct builder<::tyr::formalism::Metric, C>
{
    urd::key<::tyr::formalism::Metric> index;
    urd::built_t<::tyr::formalism::OptimizationDirection, C> optimization_direction = ::tyr::formalism::OptimizationDirection::Minimize;
    urd::built_t<::tyr::formalism::FunctionExpression<::tyr::GroundTag>, C> fexpr;

    static constexpr auto fields() noexcept
    { return urd::Fields<urd::field<"optimization_direction", &builder::optimization_direction>, urd::field<"fexpr", &builder::fexpr>> {}; }

};

}  // namespace urd

#endif
