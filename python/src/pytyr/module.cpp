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

#include "datalog/module.hpp"
#include "formalism/module.hpp"
#include "planning/module.hpp"
#include "serialization/module.hpp"

namespace tyr
{

void bind_module_definitions(nb::module_& m)
{
    auto formalism_module = m.def_submodule("formalism");
    formalism::bind_module_definitions(formalism_module);

    auto datalog_module = m.def_submodule("datalog");
    datalog::bind_module_definitions(datalog_module);

    auto planning_module = m.def_submodule("planning");
    planning::bind_module_definitions(planning_module);

    auto serialization_module = m.def_submodule("serialization");
    serialization::bind_module_definitions(serialization_module);
}

}  // namespace tyr
