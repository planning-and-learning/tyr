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

#include "datalog/bindings.hpp"
#include "datalog/module.hpp"

#include <cstddef>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/shared_ptr.h>
#include <optional>
#include <tyr/formalism/datalog/repository.hpp>

namespace tyr::formalism::datalog
{

void bind_module_definitions(nb::module_& m)
{
    bind_formalism(m);

    nb::class_<RepositoryFactory>(m, "RepositoryFactory")  //
        .def(nb::new_([]() { return std::make_shared<RepositoryFactory>(); }))
        .def("create_repository",
             [](RepositoryFactory& factory, const Repository* parent_repository, std::optional<std::size_t> num_objects)
             {
                 return num_objects ? factory.create_shared(*num_objects, parent_repository) : factory.create_shared(parent_repository);
             },
             "parent_repository"_a = nullptr,
             "num_objects"_a = nb::none(),
             nb::keep_alive<0, 2>());
}

}  // namespace tyr::formalism::datalog
