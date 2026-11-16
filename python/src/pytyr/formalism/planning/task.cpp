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

#include "binding_utils.hpp"
#include "bindings.hpp"
#include "pytyr/module.hpp"

#include <concepts>

namespace tyr::formalism::planning
{

namespace
{
template<TaskKind T>
void bind_task_kind(nb::module_& m, RepositoryBinding& repository)
{
    urd::python::bind_view<TaskView<T>>(m).def("get_num_objects", &TaskView<T>::get_num_objects);
    ygg::bind_insert<Task<T>>(repository);
}
}  // namespace

void bind_task(nb::module_& m, RepositoryBinding& repository)
{
    urd::python::bind_data<Task<LiftedTag>>(m);
    bind_task_kind<LiftedTag>(m, repository);
    urd::python::bind_data<Task<GroundTag>>(m);
    bind_task_kind<GroundTag>(m, repository);
}

}  // namespace tyr::formalism::planning
