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

#ifndef TYR_SRC_PLANNING_METRIC_HPP_
#define TYR_SRC_PLANNING_METRIC_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/planning/applicability.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/state_builder.hpp"
#include "tyr/planning/lifted/state_builder.hpp"
#include "tyr/planning/task.hpp"

#include <cmath>
#include <stdexcept>
#include <yggdrasil/containers/optional.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/core/types.hpp>

namespace tyr::planning
{

template<TaskKind Kind>
ygg::float_t evaluate_metric(ygg::View<::cista::optional<ygg::Index<::tyr::formalism::planning::Metric>>, ::tyr::formalism::planning::Repository> metric,
                             ygg::View<::cista::optional<ygg::Index<::tyr::formalism::planning::GroundFunctionTermValue<::tyr::formalism::AuxiliaryTag>>>,
                                       ::tyr::formalism::planning::Repository> auxiliary_fterm_value,
                             const StateContext<Kind>& state_context)
{
    if (auxiliary_fterm_value)
        return auxiliary_fterm_value.value().get_value();

    const auto value = metric ? evaluate(metric.value().get_fexpr(), state_context) : 0.;

    return ygg::FloatTolerance<ygg::float_t>::canonicalize(value);
}

template<TaskKind Kind>
ygg::float_t evaluate_successor_metric(const Task<Kind>& task, const ygg::Builder<State<Kind>>& state, ygg::float_t auxiliary_value)
{
    auto context = StateContext<Kind> { task, state, auxiliary_value };
    if (const auto metric = task.get_task().get_metric())
        context.auxiliary_value = evaluate(metric.value().get_fexpr(), context);
    else
        ++context.auxiliary_value;  // Assume unit cost if no metric is given.

    const auto result = ygg::FloatTolerance<ygg::float_t>::canonicalize(context.auxiliary_value);
    if (!std::isfinite(result))
        throw std::runtime_error("Successor metric value is not finite.");
    return result;
}
}

#endif
