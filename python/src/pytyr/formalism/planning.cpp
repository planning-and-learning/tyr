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

#include "planning/bindings.hpp"
#include "planning/module.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"
#include "tyr/formalism/planning/planning_fdr_task.hpp"
#include "tyr/formalism/planning/planning_task.hpp"

#include <cstddef>
#include <nanobind/stl/filesystem.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/vector.h>
#include <optional>
#include <yggdrasil/python/bindings.hpp>
#include <yggdrasil/python/type_casters.hpp>

namespace tyr::formalism::planning
{
using ygg::bind_fixed_uint;

/**
 * bind_module_definitions
 */

void bind_module_definitions(nb::module_& m)
{
    bind_parser(m);

    nb::enum_<OptimizationDirection>(m, "OptimizationDirection")
        .value("Minimize", OptimizationDirection::Minimize)
        .value("Maximize", OptimizationDirection::Maximize);

    /**
     * Common
     */

    bind_fixed_uint<FDRValue>(m, "FDRValue");

    /**
     * Invariants
     */

    invariant::bind_invariants(m);

    /**
     * Mutable
     */

    bind_mutable(m);

    /**
     * Domains
     */

    bind_variable_domains(m);

    bind_formalism(m);

    nb::class_<RepositoryFactory>(m, "RepositoryFactory")  //
        .def(nb::new_([]() { return std::make_shared<RepositoryFactory>(); }))
        .def(
            "create_repository",
            [](RepositoryFactory& factory, const Repository* parent_repository, std::optional<std::size_t> num_objects)
            { return num_objects ? factory.create_shared(*num_objects, parent_repository) : factory.create_shared(parent_repository); },
            "parent_repository"_a = nullptr,
            "num_objects"_a = nb::none(),
            nb::keep_alive<0, 2>());

    /**
     * FDRContext
     */

    nb::class_<FDRContext>(m, "FDRContext")  //
        .def(nb::new_([](RepositoryPtr repository) { return std::make_shared<FDRContext>(std::move(repository)); }), "repository"_a)
        .def(nb::new_([](const std::vector<std::vector<AtomView<GroundTag, FluentTag>>>& ground_mutex_groups, RepositoryPtr repository)
                      { return std::make_shared<FDRContext>(ground_mutex_groups, std::move(repository)); }),
             "ground_mutex_groups"_a,
             "repository"_a)
        .def("get_fact", nb::overload_cast<AtomView<GroundTag, FluentTag>>(&FDRContext::get_fact), "atom"_a, nb::keep_alive<0, 1>())
        .def("get_variables", &FDRContext::get_variables);

    /**
     * PlanningDomain
     */

    {
        nb::class_<PlanningDomain>(m, "PlanningDomain")  //
            .def(nb::init<DomainView, RepositoryPtr, RepositoryFactoryPtr, std::optional<std::filesystem::path>>(),
                 "domain"_a,
                 "repository"_a,
                 "repository_factory"_a,
                 "path"_a = nb::none())
            .def("get_domain", &PlanningDomain::get_domain, nb::keep_alive<0, 1>())
            .def("get_repository", &PlanningDomain::get_repository)
            .def("get_repository_factory", &PlanningDomain::get_repository_factory)
            .def("get_path", &PlanningDomain::get_path);
    }

    {
        nb::class_<PlanningTask>(m, "PlanningTask")  //
            .def(nb::new_([](TaskView task,
                             FDRContextPtr fdr_context,
                             RepositoryPtr repository,
                             PlanningDomain planning_domain,
                             std::optional<std::filesystem::path> path)
                          { return PlanningTask(task, std::move(fdr_context), std::move(repository), std::move(planning_domain), std::move(path)); }),
                 "task"_a,
                 "fdr_context"_a,
                 "repository"_a,
                 "planning_domain"_a,
                 "path"_a = nb::none())
            .def("get_task", &PlanningTask::get_task, nb::keep_alive<0, 1>())
            .def("get_repository", &PlanningTask::get_repository)
            .def("get_fdr_context", &PlanningTask::get_fdr_context, nb::rv_policy::reference_internal)
            .def("get_domain", &PlanningTask::get_domain, nb::rv_policy::reference_internal)
            .def("get_path", &PlanningTask::get_path)
            .def("get_variable_domains", &PlanningTask::get_variable_domains_view, nb::rv_policy::reference_internal);
    }

    {
        nb::class_<PlanningFDRTask>(m, "PlanningFDRTask")  //
            .def("get_task", &PlanningFDRTask::get_task, nb::keep_alive<0, 1>())
            .def("get_repository", &PlanningFDRTask::get_repository)
            .def("get_fdr_context", &PlanningFDRTask::get_fdr_context, nb::rv_policy::reference_internal)
            .def("get_domain", &PlanningFDRTask::get_domain, nb::rv_policy::reference_internal)
            .def("get_path", &PlanningFDRTask::get_path);
    }
}

}
