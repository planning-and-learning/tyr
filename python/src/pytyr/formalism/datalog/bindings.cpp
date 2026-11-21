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

#include "bindings.hpp"

#include "pytyr/module.hpp"

#include <tyr/formalism/planning/repository.hpp>

namespace tyr::formalism::datalog
{

void bind_formalism(nb::module_& m)
{
    auto repository = nb::borrow<RepositoryBinding>(nb::type<Repository>());
    bind_rule_body(m, repository);
    bind_rule(m, repository);
    bind_program(m, repository);
}

}  // namespace tyr::formalism::datalog
