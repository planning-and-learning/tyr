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

#ifndef TYR_FORMALISM_DATALOG_CANONICALIZATION_HPP_
#define TYR_FORMALISM_DATALOG_CANONICALIZATION_HPP_

#include "tyr/formalism/atom_data.hpp"
#include "tyr/formalism/binary_operator_data.hpp"
#include "tyr/formalism/canonicalization.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_data.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_data.hpp"
#include "tyr/formalism/datalog/program_data.hpp"
#include "tyr/formalism/datalog/rule_data.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_data.hpp"
#include "tyr/formalism/function_term_data.hpp"
#include "tyr/formalism/function_term_value_data.hpp"
#include "tyr/formalism/literal_data.hpp"
#include "tyr/formalism/multi_operator_data.hpp"
#include "tyr/formalism/numeric_effect_data.hpp"
#include "tyr/formalism/object_data.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_data.hpp"
#include "tyr/formalism/unary_operator_data.hpp"
#include "tyr/formalism/variable_data.hpp"

#include <algorithm>
#include <yggdrasil/formalism/row_data.hpp>
#include <yggdrasil/semantics/canonicalization.hpp>

/// The canonical form of every datalog record (prepare_for_insert canonicalizes each record it inserts): records whose
/// members have an order to fix sort them; every other record has a no-op overload, being canonical as built.
namespace tyr::formalism::datalog
{
// yggdrasil's overloads for lists of indices: found by ordinary lookup, as the standard's two-step `using std::swap`
// (ADL does not find them for urd::index, whose namespace is urd's).
using ygg::canonicalize;
// tyr::formalism's overloads, e.g. for rows: ygg::Row<F> of a family of this namespace does not bring them by ADL.
using ::tyr::formalism::canonicalize;

template<TaskKind T>
void canonicalize(urd::data<ConjunctiveCondition<T>>& data)
{
    canonicalize(data.static_literals);
    canonicalize(data.fluent_literals);
    canonicalize(data.numeric_constraints);
}

template<TaskKind T>
void canonicalize(urd::data<ConjunctiveEffect<T>>& data)
{ canonicalize(data.numeric_effects); }

template<TaskKind T>
void canonicalize(urd::data<Rule<T>>& data)
{ canonicalize(data.heads); }

template<TaskKind T>
void canonicalize(urd::data<Program<T>>& data)
{
    canonicalize(data.static_predicates);
    canonicalize(data.fluent_predicates);
    canonicalize(data.static_functions);
    canonicalize(data.fluent_functions);
    canonicalize(data.objects);
    canonicalize(data.static_atoms);
    canonicalize(data.fluent_atoms);
    canonicalize(data.static_fterm_values);
    canonicalize(data.fluent_fterm_values);
    canonicalize(data.goal);
    canonicalize(data.rules);
}

}

#endif
