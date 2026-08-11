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
#include "tyr/formalism/planning/datas.hpp"
#include "tyr/formalism/planning/declarations.hpp"

#include <algorithm>
#include <yggdrasil/semantics/canonicalization.hpp>
#include <yggdrasil/semantics/comparison.hpp>

namespace tyr::formalism::planning
{

/**
 * Datalog
 */

template<typename T>
bool is_canonical(const ygg::Data<UnaryOperator<T>>&)
{
    return true;
}

template<BinaryOperatorKind Operator, typename T>
bool is_canonical(const ygg::Data<BinaryOperator<Operator, T>>& data)
{
    if constexpr (std::same_as<Operator, ArithmeticOperatorKind>)
        return (data.operator_kind != ArithmeticOperatorKind::Add && data.operator_kind != ArithmeticOperatorKind::Mul) || data.lhs <= data.rhs;
    return true;
}

template<typename T>
bool is_canonical(const ygg::Data<MultiOperator<T>>& data)
{
    return ygg::is_canonical<false>(data.args);
}

template<typename T>
bool is_canonical(const ygg::Data<BooleanOperator<T>>&)
{
    return true;
}

template<typename T>
bool is_canonical(const ygg::Data<ArithmeticOperator<T>>&)
{
    return true;
}

template<FactKind T>
bool is_canonical(const ygg::Data<Atom<T>>&)
{
    return true;
}

template<FactKind T>
bool is_canonical(const ygg::Data<Literal<T>>&)
{
    return true;
}

template<FactKind T>
bool is_canonical(const ygg::Data<GroundAtom<T>>&)
{
    return true;
}

template<FactKind T>
bool is_canonical(const ygg::Data<GroundLiteral<T>>&)
{
    return true;
}

template<FactKind T>
bool is_canonical(const ygg::Data<FunctionTerm<T>>&)
{
    return true;
}

inline bool is_canonical(const ygg::Data<FunctionExpression>&) { return true; }

template<FactKind T>
bool is_canonical(const ygg::Data<GroundFunctionTerm<T>>&)
{
    return true;
}

inline bool is_canonical(const ygg::Data<GroundFunctionExpression>&) { return true; }

template<FactKind T>
bool is_canonical(const ygg::Data<GroundFunctionTermValue<T>>&)
{
    return true;
}

template<FactKind T>
bool is_canonical(const ygg::Data<NumericEffect<T>>&)
{
    return true;
}

template<FactKind T>
bool is_canonical(const ygg::Data<GroundNumericEffect<T>>&)
{
    return true;
}

inline bool is_canonical(const ygg::Data<ConditionalEffect>&) { return true; }

inline bool is_canonical(const ygg::Data<GroundConditionalEffect>&) { return true; }

inline bool is_canonical(const ygg::Data<ConjunctiveEffect>& data) { return is_canonical(data.literals) && is_canonical(data.numeric_effects); }

inline bool is_canonical(const ygg::Data<GroundConjunctiveEffect>& data)
{
    return is_canonical(data.add_facts) && is_canonical(data.del_facts) && is_canonical(data.numeric_effects);
}

inline bool is_canonical(const ygg::Data<Action>&) { return true; }

inline bool is_canonical(const ygg::Data<GroundAction>&) { return true; }

inline bool is_canonical(const ygg::Data<Axiom>&) { return true; }

inline bool is_canonical(const ygg::Data<GroundAxiom>&) { return true; }

inline bool is_canonical(const ygg::Data<Metric>&) { return true; }

inline bool is_canonical(const ygg::Data<Task>& data)
{
    return is_canonical(data.derived_predicates) && is_canonical(data.objects) && is_canonical(data.static_atoms) && is_canonical(data.fluent_atoms)
           && is_canonical(data.static_fterm_values) && is_canonical(data.fluent_fterm_values) && is_canonical(data.axioms);
}

inline bool is_canonical(const ygg::Data<Domain>& data)
{
    return is_canonical(data.static_predicates) && is_canonical(data.fluent_predicates) && is_canonical(data.derived_predicates)
           && is_canonical(data.static_functions) && is_canonical(data.fluent_functions) && is_canonical(data.constants) && is_canonical(data.actions)
           && is_canonical(data.axioms);
}

template<FactKind T>
bool is_canonical(const ygg::Data<FDRVariable<T>>&)
{
    return true;
}

template<FactKind T>
bool is_canonical(const ygg::Data<FDRFact<T>>&)
{
    return true;
}

inline bool is_canonical(const ygg::Data<ConjunctiveCondition>& data)
{
    return is_canonical(data.static_literals) && is_canonical(data.fluent_literals) && is_canonical(data.derived_literals)
           && is_canonical(data.numeric_constraints);
}

inline bool is_canonical(const ygg::Data<GroundConjunctiveCondition>& data)
{
    return is_canonical(data.static_literals) && is_canonical(data.derived_literals) && is_canonical(data.positive_facts) && is_canonical(data.negative_facts)
           && is_canonical(data.numeric_constraints);
}

inline bool is_canonical(const ygg::Data<FDRTask>& data)
{
    return is_canonical(data.derived_predicates) && is_canonical(data.objects) && is_canonical(data.static_atoms) && is_canonical(data.fluent_atoms)
           && is_canonical(data.derived_atoms) && is_canonical(data.static_fterm_values) && is_canonical(data.fluent_fterm_values) && is_canonical(data.axioms)
           && is_canonical(data.fluent_variables) && is_canonical(data.fluent_facts) && is_canonical(data.ground_actions) && is_canonical(data.ground_axioms);
}

/**
 * Datalog
 */

template<typename T>
void canonicalize(ygg::Data<UnaryOperator<T>>&)
{
    // Trivially canonical
}

template<BinaryOperatorKind Operator, typename T>
void canonicalize(ygg::Data<BinaryOperator<Operator, T>>& data)
{
    if constexpr (std::same_as<Operator, ArithmeticOperatorKind>)
    {
        if ((data.operator_kind == ArithmeticOperatorKind::Add || data.operator_kind == ArithmeticOperatorKind::Mul) && data.lhs > data.rhs)
            std::swap(data.lhs, data.rhs);
    }
}

template<typename T>
void canonicalize(ygg::Data<MultiOperator<T>>& data)
{
    ygg::canonicalize<false>(data.args);
}

template<FactKind T>
void canonicalize(ygg::Data<NumericEffect<T>>&)
{
}

template<FactKind T>
void canonicalize(ygg::Data<GroundNumericEffect<T>>&)
{
}

template<typename T>
void canonicalize(ygg::Data<BooleanOperator<T>>&)
{
    // Trivially canonical
}

template<typename T>
void canonicalize(ygg::Data<ArithmeticOperator<T>>&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(ygg::Data<Atom<T>>&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(ygg::Data<Literal<T>>&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(ygg::Data<GroundAtom<T>>&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(ygg::Data<GroundLiteral<T>>&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(ygg::Data<FunctionTerm<T>>&)
{
    // Trivially canonical
}

inline void canonicalize(ygg::Data<FunctionExpression>&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(ygg::Data<GroundFunctionTerm<T>>&)
{
    // Trivially canonical
}

inline void canonicalize(ygg::Data<GroundFunctionExpression>&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(ygg::Data<GroundFunctionTermValue<T>>&)
{
    // Trivially canonical
}

/**
 * Planning
 */

inline void canonicalize(ygg::Data<ConditionalEffect>&) {}

inline void canonicalize(ygg::Data<GroundConditionalEffect>&) {}

inline void canonicalize(ygg::Data<ConjunctiveEffect>& data)
{
    canonicalize(data.literals);
    canonicalize(data.numeric_effects);
}

inline void canonicalize(ygg::Data<GroundConjunctiveEffect>& data)
{
    canonicalize(data.add_facts);
    canonicalize(data.del_facts);
    canonicalize(data.numeric_effects);
}

inline void canonicalize(ygg::Data<Action>& data) { canonicalize(data.effects); }

inline void canonicalize(ygg::Data<GroundAction>& data) { canonicalize(data.effects); }

inline void canonicalize(ygg::Data<Axiom>&) {}

inline void canonicalize(ygg::Data<GroundAxiom>&) {}

inline void canonicalize(ygg::Data<Metric>&) {}

inline void canonicalize(ygg::Data<Task>& data)
{
    canonicalize(data.derived_predicates);
    canonicalize(data.objects);
    canonicalize(data.static_atoms);
    canonicalize(data.fluent_atoms);
    canonicalize(data.static_fterm_values);
    canonicalize(data.fluent_fterm_values);
    canonicalize(data.axioms);
}

inline void canonicalize(ygg::Data<Domain>& data)
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
void canonicalize(ygg::Data<FDRVariable<T>>&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(ygg::Data<FDRFact<T>>&)
{
    // Trivially canonical
}

inline void canonicalize(ygg::Data<ConjunctiveCondition>& data)
{
    canonicalize(data.static_literals);
    canonicalize(data.fluent_literals);
    canonicalize(data.derived_literals);
    canonicalize(data.numeric_constraints);
}

inline void canonicalize(ygg::Data<GroundConjunctiveCondition>& data)
{
    canonicalize(data.static_literals);
    canonicalize(data.derived_literals);
    canonicalize(data.positive_facts);
    canonicalize(data.negative_facts);
    canonicalize(data.numeric_constraints);
}

inline void canonicalize(ygg::Data<FDRTask>& data)
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
