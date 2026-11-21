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

#ifndef TYR_PYTHON_FORMALISM_DATALOG_BINDINGS_HPP_
#define TYR_PYTHON_FORMALISM_DATALOG_BINDINGS_HPP_

#include "module.hpp"

#include <tyr/formalism/datalog/declarations.hpp>

namespace tyr::formalism::datalog
{

using RepositoryBinding = nb::class_<Repository>;

/// Rules, rule bodies and programs; their insert on the repository bound by pytyr.formalism.planning, which is bound first.
void bind_formalism(nb::module_& m);

void bind_rule_body(nb::module_& m, RepositoryBinding& repository);
void bind_rule(nb::module_& m, RepositoryBinding& repository);
void bind_program(nb::module_& m, RepositoryBinding& repository);

}  // namespace tyr::formalism::datalog

#endif
