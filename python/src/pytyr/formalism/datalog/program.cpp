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

namespace tyr::formalism::datalog
{

namespace
{
template<TaskKind T>
void bind_program_kind(nb::module_& m, RepositoryBinding& repository)
{
    urd::python::bind_view<ProgramView<T>>(m);
    ygg::bind_insert<Program<T>>(repository);
}
}  // namespace

void bind_program(nb::module_& m, RepositoryBinding& repository)
{
    urd::python::bind_data<Program<LiftedTag>>(m);
    bind_program_kind<LiftedTag>(m, repository);
    urd::python::bind_data<Program<GroundTag>>(m);
    bind_program_kind<GroundTag>(m, repository);
}

}  // namespace tyr::formalism::datalog
