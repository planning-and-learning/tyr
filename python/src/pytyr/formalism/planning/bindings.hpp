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

#ifndef TYR_PYTHON_FORMALISM_PLANNING_BINDINGS_HPP_
#define TYR_PYTHON_FORMALISM_PLANNING_BINDINGS_HPP_

#include "module.hpp"

#include <tyr/formalism/planning/declarations.hpp>

namespace tyr::formalism::planning
{

using RepositoryBinding = nb::class_<Repository>;

void bind_formalism(nb::module_& m);
void bind_parser(nb::module_& m);

void bind_binding(nb::module_& m, RepositoryBinding& repository);
void bind_predicate(nb::module_& m, RepositoryBinding& repository);
void bind_function(nb::module_& m, RepositoryBinding& repository);
void bind_object(nb::module_& m, RepositoryBinding& repository);
void bind_variable(nb::module_& m, RepositoryBinding& repository);
void bind_term(nb::module_& m, RepositoryBinding& repository);
void bind_atom(nb::module_& m, RepositoryBinding& repository);
void bind_ground_atom(nb::module_& m, RepositoryBinding& repository);
void bind_literal(nb::module_& m, RepositoryBinding& repository);
void bind_ground_literal(nb::module_& m, RepositoryBinding& repository);
void bind_function_term(nb::module_& m, RepositoryBinding& repository);
void bind_ground_function_term(nb::module_& m, RepositoryBinding& repository);
void bind_ground_function_term_value(nb::module_& m, RepositoryBinding& repository);
void bind_unary_operator(nb::module_& m, RepositoryBinding& repository);
void bind_binary_operator(nb::module_& m, RepositoryBinding& repository);
void bind_multi_operator(nb::module_& m, RepositoryBinding& repository);
void bind_arithmetic_operator(nb::module_& m, RepositoryBinding& repository);
void bind_boolean_operator(nb::module_& m, RepositoryBinding& repository);
void bind_numeric_effect(nb::module_& m, RepositoryBinding& repository);
void bind_numeric_effect_operator(nb::module_& m, RepositoryBinding& repository);
void bind_ground_numeric_effect(nb::module_& m, RepositoryBinding& repository);
void bind_ground_numeric_effect_operator(nb::module_& m, RepositoryBinding& repository);
void bind_action(nb::module_& m, RepositoryBinding& repository);
void bind_axiom(nb::module_& m, RepositoryBinding& repository);
void bind_conditional_effect(nb::module_& m, RepositoryBinding& repository);
void bind_conjunctive_condition(nb::module_& m, RepositoryBinding& repository);
void bind_conjunctive_effect(nb::module_& m, RepositoryBinding& repository);
void bind_function_expression(nb::module_& m, RepositoryBinding& repository);
void bind_ground_action(nb::module_& m, RepositoryBinding& repository);
void bind_ground_axiom(nb::module_& m, RepositoryBinding& repository);
void bind_ground_conditional_effect(nb::module_& m, RepositoryBinding& repository);
void bind_ground_conjunctive_condition(nb::module_& m, RepositoryBinding& repository);
void bind_ground_conjunctive_effect(nb::module_& m, RepositoryBinding& repository);
void bind_ground_function_expression(nb::module_& m, RepositoryBinding& repository);
void bind_metric(nb::module_& m, RepositoryBinding& repository);
void bind_domain(nb::module_& m, RepositoryBinding& repository);
void bind_task(nb::module_& m, RepositoryBinding& repository);
void bind_fdr_task(nb::module_& m, RepositoryBinding& repository);
void bind_fdr_variable(nb::module_& m, RepositoryBinding& repository);
void bind_fdr_fact(nb::module_& m, RepositoryBinding& repository);
void bind_variable_domains(nb::module_& m);
void bind_mutable(nb::module_& m);

namespace invariant
{
void bind_invariants(nb::module_& m);
}

}  // namespace tyr::formalism::planning

#endif
