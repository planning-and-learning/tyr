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
#include "tyr/formalism/declarations.hpp"

#include <tyr/formalism/function_data.hpp>
#include <tyr/formalism/function_view.hpp>

namespace tyr::formalism
{

void bind_function(nb::module_& m)
{
    urd::python::bind_data<Function<StaticTag>>(m);
    urd::python::bind_data<Function<FluentTag>>(m);
    urd::python::bind_data<Function<AuxiliaryTag>>(m);
}

}  // namespace tyr::formalism
