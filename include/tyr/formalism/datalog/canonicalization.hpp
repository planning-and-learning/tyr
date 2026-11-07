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

#include "tyr/formalism/datalog/atom_data.hpp"
#include "tyr/formalism/datalog/binary_operator_data.hpp"
#include "tyr/formalism/datalog/conditional_effect_data.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_data.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_data.hpp"
#include "tyr/formalism/datalog/function_term_data.hpp"
#include "tyr/formalism/datalog/function_term_value_data.hpp"
#include "tyr/formalism/datalog/literal_data.hpp"
#include "tyr/formalism/datalog/multi_operator_data.hpp"
#include "tyr/formalism/datalog/numeric_effect_data.hpp"
#include "tyr/formalism/datalog/program_data.hpp"
#include "tyr/formalism/datalog/rule_data.hpp"
#include "tyr/formalism/datalog/unary_operator_data.hpp"
#include "tyr/formalism/canonicalization.hpp"
#include "tyr/formalism/declarations.hpp"
#include <yggdrasil/formalism/binding_data.hpp>
#include "tyr/formalism/function_data.hpp"
#include "tyr/formalism/object_data.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_data.hpp"
#include "tyr/formalism/variable_data.hpp"

#include <algorithm>
#include <yggdrasil/semantics/canonicalization.hpp>

/// The canonical form of every datalog record (prepare_for_insert canonicalizes each record it inserts): records whose
/// members have an order to fix sort them; every other record has a no-op overload, being canonical as built.
namespace tyr::formalism::datalog
{
// yggdrasil's overloads for lists of indices: found by ordinary lookup, as the standard's two-step `using std::swap`
// (ADL does not find them for urd::index, whose namespace is urd's).
using ygg::canonicalize;

/**
 * Datalog
 */

template<TaskKind T>
void canonicalize(urd::data<UnaryOperator<T, ArithmeticOperatorKind>>&)
{
    // Trivially canonical
}

template<TaskKind T, OperatorKind O>
void canonicalize(urd::data<BinaryOperator<T, O>>& data)
{
    if constexpr (std::same_as<O, ArithmeticOperatorKind>)
    {
        if ((data.operator_kind == ArithmeticOperatorKind::Add || data.operator_kind == ArithmeticOperatorKind::Mul) && data.lhs > data.rhs)
            std::swap(data.lhs, data.rhs);
    }
}

template<TaskKind T>
void canonicalize(urd::data<MultiOperator<T, ArithmeticOperatorKind>>& data)
{ ygg::canonicalize<false>(data.args); }

template<TaskKind T, FactKind F>
void canonicalize(urd::data<Atom<T, F>>&)
{
    // Trivially canonical
}

template<TaskKind T, FactKind F>
void canonicalize(urd::data<Literal<T, F>>&)
{
    // Trivially canonical
}

template<TaskKind T, FactKind F>
void canonicalize(urd::data<FunctionTerm<T, F>>&)
{
    // Trivially canonical
}

template<FactKind F>
void canonicalize(urd::data<FunctionTermValue<GroundTag, F>>&)
{
    // Trivially canonical
}

template<TaskKind T, FactKind F>
void canonicalize(urd::data<NumericEffect<T, F>>&)
{
    // Trivially canonical
}

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
void canonicalize(urd::data<ConditionalEffect<T>>&)
{
}

template<TaskKind T, RelationKind R>
void canonicalize(urd::data<Rule<T, R>>& data)
{
    if constexpr (std::same_as<R, PredicateTag>)
        canonicalize(data.heads);
}

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
    canonicalize(data.predicate_rules);
    canonicalize(data.function_rules);
}

}

#endif
