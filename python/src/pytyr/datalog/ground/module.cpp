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

#include "module.hpp"

#include "../datalog.hpp"

namespace tyr::datalog
{

void bind_ground_module_definitions(nb::module_& m)
{
    using ProgramT = Program<GroundTag>;
    using ConstWorkspace = ConstProgramWorkspace<GroundTag>;
    using Queue = QueueWorkspace<GroundTag>;

    bind_annotations<GroundTag>(m);
    bind_policies<GroundTag>(m);

    nb::class_<ConstWorkspace>(m, "ConstProgramWorkspace").def("get_program", [](const ConstWorkspace& self) { return self.program; }, nb::keep_alive<0, 1>());

    nb::class_<ProgramT>(m, "Program")
        .def(nb::init<::tyr::formalism::datalog::ProgramView<GroundTag>,
                      ::tyr::formalism::datalog::RepositoryPtr,
                      ::tyr::formalism::datalog::RepositoryFactoryPtr>(),
             "program"_a,
             "repository"_a,
             "repository_factory"_a)
        .def("get_program", &ProgramT::get_program, nb::keep_alive<0, 1>())
        .def("get_program_repository", nb::overload_cast<>(&ProgramT::get_program_repository), nb::rv_policy::reference_internal)
        .def("get_const_program_workspace", &ProgramT::get_const_program_workspace, nb::rv_policy::reference_internal);

    nb::class_<Queue>(m, "QueueWorkspace")
        .def(nb::new_([](ProgramT& program) { return Queue(program.get_program()); }), "program"_a, nb::keep_alive<1, 2>())
        .def("clear", &Queue::clear)
        .def("get_statistics", [](Queue& self) -> auto& { return self.statistics; }, nb::rv_policy::reference_internal);

    bind_common_configurations<GroundTag>(m);
}

}
