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
bool is_canonical(const ygg::Data<Atom<::tyr::LiftedTag, T>>&)
{
    return true;
}

template<FactKind T>
bool is_canonical(const ygg::Data<Literal<::tyr::LiftedTag, T>>&)
{
    return true;
}

template<FactKind T>
bool is_canonical(const ygg::Data<Atom<::tyr::GroundTag, T>>&)
{
    return true;
}

template<FactKind T>
bool is_canonical(const ygg::Data<Literal<::tyr::GroundTag, T>>&)
{
    return true;
}

template<FactKind T>
bool is_canonical(const ygg::Data<FunctionTerm<::tyr::LiftedTag, T>>&)
{
    return true;
}

inline bool is_canonical(const ygg::Data<FunctionExpression<::tyr::LiftedTag>>&) { return true; }

template<FactKind T>
bool is_canonical(const ygg::Data<FunctionTerm<::tyr::GroundTag, T>>&)
{
    return true;
}

inline bool is_canonical(const ygg::Data<FunctionExpression<::tyr::GroundTag>>&) { return true; }

template<FactKind T>
bool is_canonical(const ygg::Data<FunctionTermValue<::tyr::GroundTag, T>>&)
{
    return true;
}

template<FactKind T>
bool is_canonical(const ygg::Data<NumericEffect<::tyr::LiftedTag, T>>&)
{
    return true;
}

template<FactKind T>
bool is_canonical(const ygg::Data<NumericEffect<::tyr::GroundTag, T>>&)
{
    return true;
}

inline bool is_canonical(const ygg::Data<ConditionalEffect<::tyr::LiftedTag>>&) { return true; }

inline bool is_canonical(const ygg::Data<ConditionalEffect<::tyr::GroundTag>>&) { return true; }

inline bool is_canonical(const ygg::Data<ConjunctiveEffect<::tyr::LiftedTag>>& data)
{
    return is_canonical(data.literals) && is_canonical(data.numeric_effects);
}

inline bool is_canonical(const ygg::Data<ConjunctiveEffect<::tyr::GroundTag>>& data)
{
    return is_canonical(data.add_facts) && is_canonical(data.del_facts) && is_canonical(data.numeric_effects);
}

inline bool is_canonical(const ygg::Data<Action<::tyr::LiftedTag>>&) { return true; }

inline bool is_canonical(const ygg::Data<Action<::tyr::GroundTag>>&) { return true; }

inline bool is_canonical(const ygg::Data<Axiom<::tyr::LiftedTag>>&) { return true; }

inline bool is_canonical(const ygg::Data<Axiom<::tyr::GroundTag>>&) { return true; }

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

inline bool is_canonical(const ygg::Data<ConjunctiveCondition<::tyr::LiftedTag>>& data)
{
    return is_canonical(data.static_literals) && is_canonical(data.fluent_literals) && is_canonical(data.derived_literals)
           && is_canonical(data.numeric_constraints);
}

inline bool is_canonical(const ygg::Data<ConjunctiveCondition<::tyr::GroundTag>>& data)
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
    if (!is_canonical(data))
        std::sort(data.args.begin(), data.args.end());
}

template<FactKind T>
void canonicalize(ygg::Data<NumericEffect<::tyr::LiftedTag, T>>&)
{
}

template<FactKind T>
void canonicalize(ygg::Data<NumericEffect<::tyr::GroundTag, T>>&)
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
void canonicalize(ygg::Data<Atom<::tyr::LiftedTag, T>>&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(ygg::Data<Literal<::tyr::LiftedTag, T>>&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(ygg::Data<Atom<::tyr::GroundTag, T>>&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(ygg::Data<Literal<::tyr::GroundTag, T>>&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(ygg::Data<FunctionTerm<::tyr::LiftedTag, T>>&)
{
    // Trivially canonical
}

inline void canonicalize(ygg::Data<FunctionExpression<::tyr::LiftedTag>>&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(ygg::Data<FunctionTerm<::tyr::GroundTag, T>>&)
{
    // Trivially canonical
}

inline void canonicalize(ygg::Data<FunctionExpression<::tyr::GroundTag>>&)
{
    // Trivially canonical
}

template<FactKind T>
void canonicalize(ygg::Data<FunctionTermValue<::tyr::GroundTag, T>>&)
{
    // Trivially canonical
}

/**
 * Planning
 */

inline void canonicalize(ygg::Data<ConditionalEffect<::tyr::LiftedTag>>&) {}

inline void canonicalize(ygg::Data<ConditionalEffect<::tyr::GroundTag>>&) {}

inline void canonicalize(ygg::Data<ConjunctiveEffect<::tyr::LiftedTag>>& data)
{
    canonicalize(data.literals);
    canonicalize(data.numeric_effects);
}

inline void canonicalize(ygg::Data<ConjunctiveEffect<::tyr::GroundTag>>& data)
{
    canonicalize(data.add_facts);
    canonicalize(data.del_facts);
    canonicalize(data.numeric_effects);
}

inline void canonicalize(ygg::Data<Action<::tyr::LiftedTag>>& data) { canonicalize(data.effects); }

inline void canonicalize(ygg::Data<Action<::tyr::GroundTag>>& data) { canonicalize(data.effects); }

inline void canonicalize(ygg::Data<Axiom<::tyr::LiftedTag>>&) {}

inline void canonicalize(ygg::Data<Axiom<::tyr::GroundTag>>&) {}

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

inline void canonicalize(ygg::Data<ConjunctiveCondition<::tyr::LiftedTag>>& data)
{
    canonicalize(data.static_literals);
    canonicalize(data.fluent_literals);
    canonicalize(data.derived_literals);
    canonicalize(data.numeric_constraints);
}

inline void canonicalize(ygg::Data<ConjunctiveCondition<::tyr::GroundTag>>& data)
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
