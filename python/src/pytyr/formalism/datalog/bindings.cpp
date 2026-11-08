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

#include <nanobind/stl/optional.h>
#include <new>
#include <optional>
#include <tyr/formalism/datalog/repository.hpp>

namespace tyr::formalism::datalog
{

void bind_formalism(nb::module_& m)
{
    auto repository = RepositoryBinding(m, "Repository");
    // A layer over `parent_repository` (kept alive by it), its bindings storing object indices below `num_objects`.
    repository.def(
        "__init__",
        [](Repository* self, const Repository* parent_repository, std::optional<std::size_t> num_objects)
        {
            if (num_objects)
                new (self) Repository(*num_objects, parent_repository);
            else
                new (self) Repository(parent_repository);
        },
        nb::arg("parent_repository") = nullptr,
        nb::arg("num_objects") = nb::none(),
        nb::keep_alive<1, 2>());
    bind_row(m, repository);
    bind_atom(m, repository);
    bind_literal(m, repository);
    bind_function_term(m, repository);
    bind_function_term_value(m, repository);
    bind_unary_operator(m, repository);
    bind_binary_operator(m, repository);
    bind_multi_operator(m, repository);
    bind_conjunctive_condition(m, repository);
    bind_numeric_effect(m, repository);
    bind_conjunctive_effect(m, repository);
    bind_rule(m, repository);
    bind_program(m, repository);
    bind_object(m, repository);
    bind_variable(m, repository);
    bind_predicate(m, repository);
    bind_function(m, repository);
}

}  // namespace tyr::formalism::datalog
