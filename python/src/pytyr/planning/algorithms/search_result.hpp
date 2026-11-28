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

#ifndef TYR_PYTHON_PLANNING_ALGORITHMS_SEARCH_RESULT_HPP_
#define TYR_PYTHON_PLANNING_ALGORITHMS_SEARCH_RESULT_HPP_

#include "pytyr/planning/module.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/stl/chrono.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/vector.h>
#include <string>
#include <tyr/planning/algorithms/statistics.hpp>
#include <tyr/planning/algorithms/utils.hpp>
#include <tyr/planning/declarations.hpp>
#include <tyr/planning/nodes/node_view.hpp>
#include <tyr/planning/plans/plan_view.hpp>
#include <urd/python/bindings.hpp>

namespace tyr::planning
{

template<TaskKind Kind>
void bind_search_result(nb::module_& m, const std::string& name)
{
    using T = SearchResult<Kind>;

    nb::class_<T>(m, name.c_str())
        .def(nb::init<>())
        .def_rw("status", &T::status)
        .def_ro("plan", &T::plan)
        .def_rw("goal_node", &T::goal_node, nb::for_setter(nb::keep_alive<1, 2>()))
        .def_rw("cycle_range", &T::cycle_range)
        .def_rw("statistics", &T::statistics)
        .def_rw("worker_statistics", &T::worker_statistics)
        .def_prop_ro("worker_utilization", &T::get_worker_utilization);
}

}  // namespace tyr::planning

#endif
