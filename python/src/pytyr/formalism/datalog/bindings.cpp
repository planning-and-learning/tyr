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

#include <tyr/formalism/datalog/repository.hpp>

namespace tyr::formalism::datalog
{

void bind_formalism(nb::module_& m)
{
    auto repository = RepositoryBinding(m, "Repository");
    bind_binding(m, repository);
    bind_atom(m, repository);
    bind_ground_atom(m, repository);
    bind_literal(m, repository);
    bind_ground_literal(m, repository);
    bind_function_term(m, repository);
    bind_ground_function_term(m, repository);
    bind_ground_function_term_value(m, repository);
    bind_unary_operator(m, repository);
    bind_binary_operator(m, repository);
    bind_multi_operator(m, repository);
    bind_conjunctive_condition(m, repository);
    bind_numeric_effect(m, repository);
    bind_conjunctive_effect(m, repository);
    bind_conditional_effect(m, repository);
    bind_rule(m, repository);
    bind_ground_conjunctive_condition(m, repository);
    bind_ground_numeric_effect(m, repository);
    bind_ground_conjunctive_effect(m, repository);
    bind_ground_conditional_effect(m, repository);
    bind_ground_rule(m, repository);
    bind_metric(m, repository);
    bind_program(m, repository);
    bind_ground_program(m, repository);
    bind_arithmetic_operator(m, repository);
    bind_boolean_operator(m, repository);
    bind_function_expression(m, repository);
    bind_numeric_effect_operator(m, repository);
    bind_ground_function_expression(m, repository);
    bind_ground_numeric_effect_operator(m, repository);
    bind_object(m, repository);
    bind_variable(m, repository);
    bind_term(m, repository);
    bind_predicate(m, repository);
    bind_function(m, repository);
}

}  // namespace tyr::formalism::datalog
