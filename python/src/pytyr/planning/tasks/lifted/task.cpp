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

#include "pytyr/planning/tasks/lifted/task.hpp"

#include "pytyr/module.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"
#include "tyr/planning/tasks/lifted/task_grounder.hpp"

#include <nanobind/stl/optional.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/vector.h>
#include <boost/mp11.hpp>
#include <yggdrasil/execution/onetbb.hpp>
#include <yggdrasil/python/bindings.hpp>
#include <yggdrasil/python/serialization.hpp>

namespace tyr::planning
{

void bind_lifted_task(nb::module_& m)
{
    using namespace nb::literals;
    nb::enum_<GroundTaskInstantiationStatus>(m, "GroundTaskInstantiationStatus")
        .value("SUCCESS", GroundTaskInstantiationStatus::SUCCESS)
        .value("PROVEN_UNSOLVABLE", GroundTaskInstantiationStatus::PROVEN_UNSOLVABLE);

    nb::class_<GroundTaskInstantiationResult>(m, "GroundTaskInstantiationResult")
        .def(nb::init<>())
        .def(nb::init<TaskPtr<GroundTag>, GroundTaskInstantiationStatus>(), "task"_a, "status"_a)
        .def_rw("task", &GroundTaskInstantiationResult::task)
        .def_rw("status", &GroundTaskInstantiationResult::status);

    nb::class_<GroundTaskInstantiationOptions>(m, "GroundTaskInstantiationOptions")
        .def(nb::init<>())
        .def(nb::init<bool>(), "disable_invariant_synthesis"_a = true)
        .def_rw("disable_invariant_synthesis", &GroundTaskInstantiationOptions::disable_invariant_synthesis)
        .def_rw("invariant_order", &GroundTaskInstantiationOptions::invariant_order);

    auto task =
        nb::class_<Task<LiftedTag>>(m, "Task")  //
            .def(nb::new_([](const formalism::PlanningTask<LiftedTag>& source_task) { return std::make_shared<Task<LiftedTag>>(source_task); }),
                 "formalism_task"_a,
                 R"doc(
Create a planning task from a formalism task.

Parameters
----------
formalism_task : formalism.LiftedPlanningTask
    The formalism-level task used to construct the planning task.

Notes
-----
The formalism task stays usable and is shared, not copied: its repository and
FDR context are frozen. The planning task grounds and registers into layers of
its own over them (get_repository, get_fdr_context), so several planning tasks
of one formalism task do not see each other's writes. Do not insert into the
formalism task's repository afterwards.
        )doc")
            .def("get_formalism_task", &Task<LiftedTag>::get_formalism_task, nb::rv_policy::reference_internal)
            .def("get_repository", &Task<LiftedTag>::get_repository)
            .def("get_task", &Task<LiftedTag>::get_task)
            .def("get_fdr_context", &Task<LiftedTag>::get_fdr_context, nb::rv_policy::reference_internal)
            .def("instantiate_ground_task", &Task<LiftedTag>::instantiate_ground_task, "execution_context"_a, "options"_a = GroundTaskInstantiationOptions());
    ygg::add_print(task);
    ygg::python::bind_serialization(boost::mp11::mp_list<Task<LiftedTag>> {});
}

}  // namespace tyr::planning
