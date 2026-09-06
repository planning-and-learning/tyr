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

template<::tyr::TaskKind T, FactKind F>
bool is_canonical(const ygg::Data<Atom<T, F>>&)
{
    return true;
}

template<::tyr::TaskKind T, FactKind F>
bool is_canonical(const ygg::Data<Literal<T, F>>&)
{
    return true;
}

template<::tyr::TaskKind T, FactKind F>
bool is_canonical(const ygg::Data<FunctionTerm<T, F>>&)
{
    return true;
}

template<::tyr::TaskKind T>
bool is_canonical(const ygg::Data<FunctionExpression<T>>&)
{
    return true;
}

template<FactKind F>
bool is_canonical(const ygg::Data<FunctionTermValue<::tyr::GroundTag, F>>&)
{
    return true;
}

template<::tyr::TaskKind T, FactKind F>
bool is_canonical(const ygg::Data<NumericEffect<T, F>>&)
{
    return true;
}

template<::tyr::TaskKind T>
bool is_canonical(const ygg::Data<ConjunctiveCondition<T>>& data)
{
    return is_canonical(data.static_literals) && is_canonical(data.fluent_literals) && is_canonical(data.numeric_constraints);
}

template<::tyr::TaskKind T>
bool is_canonical(const ygg::Data<ConjunctiveEffect<T>>& data)
{
    return is_canonical(data.numeric_effects);
}

template<::tyr::TaskKind T>
bool is_canonical(const ygg::Data<ConditionalEffect<T>>&)
{
    return true;
}

template<::tyr::TaskKind T, RelationKind R>
bool is_canonical(const ygg::Data<Rule<T, R>>&)
{
    return true;
}

inline bool is_canonical(const ygg::Data<Metric>&) { return true; }

template<::tyr::TaskKind T>
bool is_canonical(const ygg::Data<Program<T>>& data)
{
    return is_canonical(data.static_predicates) && is_canonical(data.fluent_predicates) && is_canonical(data.static_functions)
           && is_canonical(data.fluent_functions) && is_canonical(data.objects) && is_canonical(data.static_atoms) && is_canonical(data.fluent_atoms)
           && is_canonical(data.static_fterm_values) && is_canonical(data.fluent_fterm_values) && is_canonical(data.goal) && is_canonical(data.metric)
           && is_canonical(data.predicate_rules) && is_canonical(data.function_rules);
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

template<::tyr::TaskKind T, FactKind F>
void canonicalize(ygg::Data<Atom<T, F>>&)
{
    // Trivially canonical
}

template<::tyr::TaskKind T, FactKind F>
void canonicalize(ygg::Data<Literal<T, F>>&)
{
    // Trivially canonical
}

template<::tyr::TaskKind T, FactKind F>
void canonicalize(ygg::Data<FunctionTerm<T, F>>&)
{
    // Trivially canonical
}

template<::tyr::TaskKind T>
void canonicalize(ygg::Data<FunctionExpression<T>>&)
{
    // Trivially canonical
}

template<FactKind F>
void canonicalize(ygg::Data<FunctionTermValue<::tyr::GroundTag, F>>&)
{
    // Trivially canonical
}

template<::tyr::TaskKind T, FactKind F>
void canonicalize(ygg::Data<NumericEffect<T, F>>&)
{
    // Trivially canonical
}

template<::tyr::TaskKind T>
void canonicalize(ygg::Data<ConjunctiveCondition<T>>& data)
{
    canonicalize(data.static_literals);
    canonicalize(data.fluent_literals);
    canonicalize(data.numeric_constraints);
}

template<::tyr::TaskKind T>
void canonicalize(ygg::Data<ConjunctiveEffect<T>>& data)
{
    canonicalize(data.numeric_effects);
}

template<::tyr::TaskKind T>
void canonicalize(ygg::Data<ConditionalEffect<T>>&)
{
}

template<::tyr::TaskKind T, RelationKind R>
void canonicalize(ygg::Data<Rule<T, R>>&)
{
    // Trivially canonical
}

inline void canonicalize(ygg::Data<Metric>&)
{
    // Trivially canonical
}

template<::tyr::TaskKind T>
void canonicalize(ygg::Data<Program<T>>& data)
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

}

#endif
