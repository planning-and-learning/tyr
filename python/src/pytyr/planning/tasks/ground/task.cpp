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

#include "pytyr/planning/tasks/ground/task.hpp"

#include "pytyr/module.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/planning/tasks/ground/task.hpp"

#include <nanobind/stl/shared_ptr.h>
#include <boost/mp11.hpp>
#include <yggdrasil/python/bindings.hpp>
#include <yggdrasil/python/serialization.hpp>

namespace tyr::planning
{

void bind_ground_task(nb::module_& m)
{
    auto task = nb::class_<Task<GroundTag>>(m, "Task")  //
                    .def("get_formalism_task", &Task<GroundTag>::get_formalism_task, nb::rv_policy::reference_internal)
                    .def("get_repository", &Task<GroundTag>::get_repository)
                    .def("get_task", &Task<GroundTag>::get_task)
                    .def("get_fdr_context", &Task<GroundTag>::get_fdr_context, nb::rv_policy::reference_internal);
    ygg::add_print(task);
    ygg::python::bind_serialization(boost::mp11::mp_list<Task<GroundTag>> {});
}

}  // namespace tyr::planning
