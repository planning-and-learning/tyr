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

namespace tyr::formalism::planning
{
// yggdrasil's overloads for lists of indices: found by ordinary lookup, as the standard's two-step `using std::swap`
// (ADL does not find them for urd::index, whose namespace is urd's).
using ygg::canonicalize;
using ygg::is_canonical;

/**
 * Datalog
 */

template<TaskKind T>
bool is_canonical(const urd::data<UnaryOperator<T, ArithmeticOperatorKind>>&)
{ return true; }

template<TaskKind T, OperatorKind Operator>
bool is_canonical(const urd::data<BinaryOperator<T, Operator>>& data)
{
    if constexpr (std::same_as<Operator, ArithmeticOperatorKind>)
        return (data.operator_kind != ArithmeticOperatorKind::Add && data.operator_kind != ArithmeticOperatorKind::Mul) || data.lhs <= data.rhs;
    return true;
}

template<TaskKind T>
bool is_canonical(const urd::data<MultiOperator<T, ArithmeticOperatorKind>>& data)
{ return std::is_sorted(data.args.begin(), data.args.end()); }

template<FactKind T>
bool is_canonical(const urd::data<Atom<LiftedTag, T>>&)
{ return true; }

template<FactKind T>
bool is_canonical(const urd::data<Literal<LiftedTag, T>>&)
{ return true; }

template<FactKind T>
bool is_canonical(const urd::data<Atom<GroundTag, T>>&)
{ return true; }

template<FactKind T>
bool is_canonical(const urd::data<Literal<GroundTag, T>>&)
{ return true; }

template<FactKind T>
bool is_canonical(const urd::data<FunctionTerm<LiftedTag, T>>&)
{ return true; }

template<FactKind T>
bool is_canonical(const urd::data<FunctionTerm<GroundTag, T>>&)
{ return true; }

template<FactKind T>
bool is_canonical(const urd::data<FunctionTermValue<GroundTag, T>>&)
{ return true; }

template<FactKind T>
bool is_canonical(const urd::data<NumericEffect<LiftedTag, T>>&)
{ return true; }

template<FactKind T>
bool is_canonical(const urd::data<NumericEffect<GroundTag, T>>&)
{ return true; }

inline bool is_canonical(const urd::data<ConditionalEffect<LiftedTag>>&) { return true; }

inline bool is_canonical(const urd::data<ConditionalEffect<GroundTag>>&) { return true; }

inline bool is_canonical(const urd::data<ConjunctiveEffect<LiftedTag>>& data) { return is_canonical(data.literals) && is_canonical(data.numeric_effects); }

inline bool is_canonical(const urd::data<ConjunctiveEffect<GroundTag>>& data)
{ return is_canonical(data.add_facts) && is_canonical(data.del_facts) && is_canonical(data.numeric_effects); }

inline bool is_canonical(const urd::data<Action<LiftedTag>>&) { return true; }

inline bool is_canonical(const urd::data<Action<GroundTag>>&) { return true; }

inline bool is_canonical(const urd::data<Axiom<LiftedTag>>&) { return true; }

inline bool is_canonical(const urd::data<Axiom<GroundTag>>&) { return true; }

inline bool is_canonical(const urd::data<Metric>&) { return true; }

inline bool is_canonical(const urd::data<Task<LiftedTag>>& data)
{
    return is_canonical(data.derived_predicates) && is_canonical(data.objects) && is_canonical(data.static_atoms) && is_canonical(data.fluent_atoms)
           && is_canonical(data.static_fterm_values) && is_canonical(data.fluent_fterm_values) && is_canonical(data.axioms);
}

inline bool is_canonical(const urd::data<Domain>& data)
{
    return is_canonical(data.static_predicates) && is_canonical(data.fluent_predicates) && is_canonical(data.derived_predicates)
           && is_canonical(data.static_functions) && is_canonical(data.fluent_functions) && is_canonical(data.constants) && is_canonical(data.actions)
           && is_canonical(data.axioms);
}

template<FactKind T>
bool is_canonical(const urd::data<FDRVariable<T>>&)
{ return true; }

inline bool is_canonical(const urd::data<Invariant>& data) { return is_canonical(data.atoms); }

template<FactKind T>
bool is_canonical(const urd::data<FDRFact<T>>&)
{ return true; }

inline bool is_canonical(const urd::data<ConjunctiveCondition<LiftedTag>>& data)
{
    return is_canonical(data.static_literals) && is_canonical(data.fluent_literals) && is_canonical(data.derived_literals)
           && is_canonical(data.numeric_constraints);
}

inline bool is_canonical(const urd::data<ConjunctiveCondition<GroundTag>>& data)
{
    return is_canonical(data.static_literals) && is_canonical(data.derived_literals) && is_canonical(data.positive_facts) && is_canonical(data.negative_facts)
           && is_canonical(data.numeric_constraints);
}

inline bool is_canonical(const urd::data<Task<GroundTag>>& data)
{
    return is_canonical(data.derived_predicates) && is_canonical(data.objects) && is_canonical(data.static_atoms) && is_canonical(data.fluent_atoms)
           && is_canonical(data.derived_atoms) && is_canonical(data.static_fterm_values) && is_canonical(data.fluent_fterm_values) && is_canonical(data.axioms)
           && is_canonical(data.fluent_variables) && is_canonical(data.fluent_facts) && is_canonical(data.ground_actions) && is_canonical(data.ground_axioms);
}

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
    if (!is_canonical(data))
        std::sort(data.args.begin(), data.args.end());
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
