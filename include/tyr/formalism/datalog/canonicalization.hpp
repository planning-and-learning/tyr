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

#include "tyr/formalism/canonicalization.hpp"
#include "tyr/formalism/datalog/datas.hpp"

#include <algorithm>
#include <yggdrasil/semantics/canonicalization.hpp>
#include <yggdrasil/semantics/comparison.hpp>

namespace tyr::formalism::datalog
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
    return std::is_sorted(data.args.begin(), data.args.end());
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

inline bool is_canonical(const ygg::Data<ConjunctiveCondition>& data)
{
    return is_canonical(data.static_literals) && is_canonical(data.fluent_literals) && is_canonical(data.numeric_constraints);
}

inline bool is_canonical(const ygg::Data<GroundConjunctiveCondition>& data)
{
    return is_canonical(data.static_literals) && is_canonical(data.fluent_literals) && is_canonical(data.numeric_constraints);
}

inline bool is_canonical(const ygg::Data<ConjunctiveEffect>& data) { return is_canonical(data.numeric_effects); }

inline bool is_canonical(const ygg::Data<GroundConjunctiveEffect>& data) { return is_canonical(data.numeric_effects); }

inline bool is_canonical(const ygg::Data<ConditionalEffect>&) { return true; }

inline bool is_canonical(const ygg::Data<GroundConditionalEffect>&) { return true; }

template<RelationKind R>
bool is_canonical(const ygg::Data<Rule<R>>&)
{
    return true;
}

template<RelationKind R>
bool is_canonical(const ygg::Data<GroundRule<R>>&)
{
    return true;
}

inline bool is_canonical(const ygg::Data<Metric>&) { return true; }

inline bool is_canonical(const ygg::Data<Program>& data)
{
    return is_canonical(data.static_predicates) && is_canonical(data.fluent_predicates) && is_canonical(data.static_functions)
           && is_canonical(data.fluent_functions) && is_canonical(data.objects) && is_canonical(data.static_atoms) && is_canonical(data.fluent_atoms)
           && is_canonical(data.static_fterm_values) && is_canonical(data.fluent_fterm_values) && is_canonical(data.goal) && is_canonical(data.metric)
           && is_canonical(data.predicate_rules) && is_canonical(data.function_rules);
}

inline bool is_canonical(const ygg::Data<GroundProgram>& data)
{
    return is_canonical(data.static_predicates) && is_canonical(data.fluent_predicates) && is_canonical(data.static_functions)
           && is_canonical(data.fluent_functions) && is_canonical(data.objects) && is_canonical(data.static_atoms) && is_canonical(data.fluent_atoms)
           && is_canonical(data.static_fterm_values) && is_canonical(data.fluent_fterm_values) && is_canonical(data.goal) && is_canonical(data.metric)
           && is_canonical(data.predicate_ground_rules) && is_canonical(data.function_ground_rules);
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
    if (!is_canonical(data))
        std::sort(data.args.begin(), data.args.end());
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

template<FactKind T>
void canonicalize(ygg::Data<NumericEffect<T>>&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(ygg::Data<GroundNumericEffect<T>>&)
{
    // Trivially canonical
}

inline void canonicalize(ygg::Data<ConjunctiveCondition>& data)
{
    canonicalize(data.static_literals);
    canonicalize(data.fluent_literals);
    canonicalize(data.numeric_constraints);
}

inline void canonicalize(ygg::Data<GroundConjunctiveCondition>& data)
{
    canonicalize(data.static_literals);
    canonicalize(data.fluent_literals);
    canonicalize(data.numeric_constraints);
}

inline void canonicalize(ygg::Data<ConjunctiveEffect>& data) { canonicalize(data.numeric_effects); }

inline void canonicalize(ygg::Data<GroundConjunctiveEffect>& data) { canonicalize(data.numeric_effects); }

inline void canonicalize(ygg::Data<ConditionalEffect>&) {}

inline void canonicalize(ygg::Data<GroundConditionalEffect>&) {}

template<RelationKind R>
void canonicalize(ygg::Data<Rule<R>>&)
{
    // Trivially canonical
}

template<RelationKind R>
void canonicalize(ygg::Data<GroundRule<R>>&)
{
    // Trivially canonical
}

inline void canonicalize(ygg::Data<Metric>&)
{
    // Trivially canonical
}

inline void canonicalize(ygg::Data<Program>& data)
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
    canonicalize(data.metric);
    canonicalize(data.predicate_rules);
    canonicalize(data.function_rules);
}

inline void canonicalize(ygg::Data<GroundProgram>& data)
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
    canonicalize(data.metric);
    canonicalize(data.predicate_ground_rules);
    canonicalize(data.function_ground_rules);
}

}

#endif
