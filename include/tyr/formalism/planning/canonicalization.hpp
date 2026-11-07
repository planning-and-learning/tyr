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

#ifndef TYR_FORMALISM_PLANNING_CANONICALIZATION_HPP_
#define TYR_FORMALISM_PLANNING_CANONICALIZATION_HPP_

#include "tyr/formalism/canonicalization.hpp"
#include "tyr/formalism/declarations.hpp"
#include <yggdrasil/formalism/binding_data.hpp>
#include "tyr/formalism/function_data.hpp"
#include "tyr/formalism/object_data.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_data.hpp"
#include "tyr/formalism/variable_data.hpp"
#include "tyr/formalism/planning/action_data.hpp"
#include "tyr/formalism/planning/atom_data.hpp"
#include "tyr/formalism/planning/axiom_data.hpp"
#include "tyr/formalism/planning/binary_operator_data.hpp"
#include "tyr/formalism/planning/conditional_effect_data.hpp"
#include "tyr/formalism/planning/conjunctive_condition_data.hpp"
#include "tyr/formalism/planning/conjunctive_effect_data.hpp"
#include "tyr/formalism/planning/domain_data.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/fdr_variable_data.hpp"
#include "tyr/formalism/planning/function_term_data.hpp"
#include "tyr/formalism/planning/function_term_value_data.hpp"
#include "tyr/formalism/planning/invariant_data.hpp"
#include "tyr/formalism/planning/literal_data.hpp"
#include "tyr/formalism/planning/metric_data.hpp"
#include "tyr/formalism/planning/multi_operator_data.hpp"
#include "tyr/formalism/planning/numeric_effect_data.hpp"
#include "tyr/formalism/planning/task_data.hpp"
#include "tyr/formalism/planning/unary_operator_data.hpp"
#include "tyr/formalism/planning/declarations.hpp"

#include <algorithm>
#include <yggdrasil/semantics/canonicalization.hpp>

/// The canonical form of every planning record (prepare_for_insert canonicalizes each record it inserts): records whose
/// members have an order to fix sort them; every other record has a no-op overload, being canonical as built.
namespace tyr::formalism::planning
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

template<TaskKind T, OperatorKind Operator>
void canonicalize(urd::data<BinaryOperator<T, Operator>>& data)
{
    if constexpr (std::same_as<Operator, ArithmeticOperatorKind>)
    {
        if ((data.operator_kind == ArithmeticOperatorKind::Add || data.operator_kind == ArithmeticOperatorKind::Mul) && data.lhs > data.rhs)
            std::swap(data.lhs, data.rhs);
    }
}

template<TaskKind T>
void canonicalize(urd::data<MultiOperator<T, ArithmeticOperatorKind>>& data)
{
    ygg::canonicalize<false>(data.args);
}

template<FactKind T>
void canonicalize(urd::data<NumericEffect<LiftedTag, T>>&)
{
}

template<FactKind T>
void canonicalize(urd::data<NumericEffect<GroundTag, T>>&)
{
}

template<FactKind T>
void canonicalize(urd::data<Atom<LiftedTag, T>>&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(urd::data<Literal<LiftedTag, T>>&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(urd::data<Atom<GroundTag, T>>&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(urd::data<Literal<GroundTag, T>>&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(urd::data<FunctionTerm<LiftedTag, T>>&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(urd::data<FunctionTerm<GroundTag, T>>&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(urd::data<FunctionTermValue<GroundTag, T>>&)
{
    // Trivially canonical
}

/**
 * Planning
 */

inline void canonicalize(urd::data<ConditionalEffect<LiftedTag>>&) {}

inline void canonicalize(urd::data<ConditionalEffect<GroundTag>>&) {}

inline void canonicalize(urd::data<ConjunctiveEffect<LiftedTag>>& data)
{
    canonicalize(data.literals);
    canonicalize(data.numeric_effects);
}

inline void canonicalize(urd::data<ConjunctiveEffect<GroundTag>>& data)
{
    canonicalize(data.add_facts);
    canonicalize(data.del_facts);
    canonicalize(data.numeric_effects);
}

inline void canonicalize(urd::data<Action<LiftedTag>>& data) { canonicalize(data.effects); }

inline void canonicalize(urd::data<Action<GroundTag>>& data) { canonicalize(data.effects); }

inline void canonicalize(urd::data<Axiom<LiftedTag>>&) {}

inline void canonicalize(urd::data<Axiom<GroundTag>>&) {}

inline void canonicalize(urd::data<Metric>&) {}

inline void canonicalize(urd::data<Task<LiftedTag>>& data)
{
    canonicalize(data.derived_predicates);
    canonicalize(data.objects);
    canonicalize(data.static_atoms);
    canonicalize(data.fluent_atoms);
    canonicalize(data.static_fterm_values);
    canonicalize(data.fluent_fterm_values);
    canonicalize(data.axioms);
}

inline void canonicalize(urd::data<Domain>& data)
{
    canonicalize(data.static_predicates);
    canonicalize(data.fluent_predicates);
    canonicalize(data.derived_predicates);
    canonicalize(data.static_functions);
    canonicalize(data.fluent_functions);
    canonicalize(data.constants);
    canonicalize(data.actions);
    canonicalize(data.axioms);
}

template<FactKind T>
void canonicalize(urd::data<FDRVariable<T>>&)
{
    // Trivially canonical
}

inline void canonicalize(urd::data<Invariant>& data) { canonicalize(data.atoms); }

template<FactKind T>
void canonicalize(urd::data<FDRFact<T>>&)
{
    // Trivially canonical
}

inline void canonicalize(urd::data<ConjunctiveCondition<LiftedTag>>& data)
{
    canonicalize(data.static_literals);
    canonicalize(data.fluent_literals);
    canonicalize(data.derived_literals);
    canonicalize(data.numeric_constraints);
}

inline void canonicalize(urd::data<ConjunctiveCondition<GroundTag>>& data)
{
    canonicalize(data.static_literals);
    canonicalize(data.derived_literals);
    canonicalize(data.positive_facts);
    canonicalize(data.negative_facts);
    canonicalize(data.numeric_constraints);
}

inline void canonicalize(urd::data<Task<GroundTag>>& data)
{
    canonicalize(data.derived_predicates);
    canonicalize(data.objects);
    canonicalize(data.static_atoms);
    canonicalize(data.fluent_atoms);
    canonicalize(data.derived_atoms);
    canonicalize(data.static_fterm_values);
    canonicalize(data.fluent_fterm_values);
    canonicalize(data.axioms);
    canonicalize(data.fluent_variables);
    canonicalize(data.fluent_facts);
    canonicalize(data.ground_actions);
    canonicalize(data.ground_axioms);
}

}

#endif
