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

#ifndef TYR_FORMALISM_DATALOG_EXPRESSION_PROPERTIES_HPP_
#define TYR_FORMALISM_DATALOG_EXPRESSION_PROPERTIES_HPP_

#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"

#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::formalism::datalog
{
/**
 * Forward declarations
 */

template<FactKind T>
void collect_fterms(ygg::float_t element, ygg::UnorderedSet<FunctionTermView<T>>& result);

template<FactKind T>
void collect_fterms(ygg::float_t element, ygg::UnorderedSet<GroundFunctionTermView<T>>& result);

template<FactKind T1, FactKind T2>
void collect_fterms(FunctionTermView<T1> element, ygg::UnorderedSet<FunctionTermView<T2>>& result);

template<FactKind T1, FactKind T2>
void collect_fterms(GroundFunctionTermView<T1> element, ygg::UnorderedSet<GroundFunctionTermView<T2>>& result);

template<FactKind T>
void collect_fterms(FunctionExpressionView element, ygg::UnorderedSet<FunctionTermView<T>>& result);

template<FactKind T>
void collect_fterms(GroundFunctionExpressionView element, ygg::UnorderedSet<GroundFunctionTermView<T>>& result);

template<FactKind T>
void collect_fterms(LiftedUnaryOperatorView element, ygg::UnorderedSet<FunctionTermView<T>>& result);

template<FactKind T>
void collect_fterms(GroundUnaryOperatorView element, ygg::UnorderedSet<GroundFunctionTermView<T>>& result);

template<FactKind T, BinaryOperatorKind O>
void collect_fterms(LiftedBinaryOperatorView<O> element, ygg::UnorderedSet<FunctionTermView<T>>& result);

template<FactKind T, BinaryOperatorKind O>
void collect_fterms(GroundBinaryOperatorView<O> element, ygg::UnorderedSet<GroundFunctionTermView<T>>& result);

template<FactKind T>
void collect_fterms(LiftedMultiOperatorView element, ygg::UnorderedSet<FunctionTermView<T>>& result);

template<FactKind T>
void collect_fterms(GroundMultiOperatorView element, ygg::UnorderedSet<GroundFunctionTermView<T>>& result);

template<FactKind T>
void collect_fterms(LiftedArithmeticOperatorView element, ygg::UnorderedSet<FunctionTermView<T>>& result);

template<FactKind T>
void collect_fterms(GroundArithmeticOperatorView element, ygg::UnorderedSet<GroundFunctionTermView<T>>& result);

template<FactKind T>
void collect_fterms(LiftedBooleanOperatorView element, ygg::UnorderedSet<FunctionTermView<T>>& result);

template<FactKind T>
void collect_fterms(GroundBooleanOperatorView element, ygg::UnorderedSet<GroundFunctionTermView<T>>& result);

template<FactKind T1, FactKind T2>
void collect_fterms(NumericEffectView<T1> element, ygg::UnorderedSet<FunctionTermView<T2>>& result);

template<FactKind T1, FactKind T2>
void collect_fterms(GroundNumericEffectView<T1> element, ygg::UnorderedSet<GroundFunctionTermView<T2>>& result);

template<FactKind T1, FactKind T2>
void collect_fterms(NumericEffectOperatorView<T1> element, ygg::UnorderedSet<FunctionTermView<T2>>& result);

template<FactKind T1, FactKind T2>
void collect_fterms(GroundNumericEffectOperatorView<T1> element, ygg::UnorderedSet<GroundFunctionTermView<T2>>& result);

/**
 * Implementations
 */

template<FactKind T>
inline void collect_fterms(ygg::float_t, ygg::UnorderedSet<FunctionTermView<T>>&)
{
}

template<FactKind T>
inline void collect_fterms(ygg::float_t, ygg::UnorderedSet<GroundFunctionTermView<T>>&)
{
}

template<FactKind T1, FactKind T2>
inline void collect_fterms(FunctionTermView<T1> element, ygg::UnorderedSet<FunctionTermView<T2>>& result)
{
    if constexpr (std::same_as<T1, T2>)
        result.insert(element);
}

template<FactKind T1, FactKind T2>
inline void collect_fterms(GroundFunctionTermView<T1> element, ygg::UnorderedSet<GroundFunctionTermView<T2>>& result)
{
    if constexpr (std::same_as<T1, T2>)
        result.insert(element);
}

template<FactKind T>
inline void collect_fterms(FunctionExpressionView element, ygg::UnorderedSet<FunctionTermView<T>>& result)
{
    visit([&](auto&& arg) { collect_fterms(arg, result); }, element.get_variant());
}

template<FactKind T>
inline void collect_fterms(GroundFunctionExpressionView element, ygg::UnorderedSet<GroundFunctionTermView<T>>& result)
{
    visit([&](auto&& arg) { collect_fterms(arg, result); }, element.get_variant());
}

template<FactKind T>
inline void collect_fterms(LiftedUnaryOperatorView element, ygg::UnorderedSet<FunctionTermView<T>>& result)
{
    collect_fterms(element.get_arg(), result);
}

template<FactKind T>
inline void collect_fterms(GroundUnaryOperatorView element, ygg::UnorderedSet<GroundFunctionTermView<T>>& result)
{
    collect_fterms(element.get_arg(), result);
}

template<FactKind T, BinaryOperatorKind O>
inline void collect_fterms(LiftedBinaryOperatorView<O> element, ygg::UnorderedSet<FunctionTermView<T>>& result)
{
    collect_fterms(element.get_lhs(), result);
    collect_fterms(element.get_rhs(), result);
}

template<FactKind T, BinaryOperatorKind O>
inline void collect_fterms(GroundBinaryOperatorView<O> element, ygg::UnorderedSet<GroundFunctionTermView<T>>& result)
{
    collect_fterms(element.get_lhs(), result);
    collect_fterms(element.get_rhs(), result);
}

template<FactKind T>
inline void collect_fterms(LiftedMultiOperatorView element, ygg::UnorderedSet<FunctionTermView<T>>& result)
{
    for (const auto& arg : element.get_args())
        collect_fterms(arg, result);
}

template<FactKind T>
inline void collect_fterms(GroundMultiOperatorView element, ygg::UnorderedSet<GroundFunctionTermView<T>>& result)
{
    for (const auto& arg : element.get_args())
        collect_fterms(arg, result);
}

template<FactKind T>
inline void collect_fterms(LiftedArithmeticOperatorView element, ygg::UnorderedSet<FunctionTermView<T>>& result)
{
    visit([&](auto&& arg) { collect_fterms(arg, result); }, element.get_variant());
}

template<FactKind T>
inline void collect_fterms(GroundArithmeticOperatorView element, ygg::UnorderedSet<GroundFunctionTermView<T>>& result)
{
    visit([&](auto&& arg) { collect_fterms(arg, result); }, element.get_variant());
}

template<FactKind T>
inline void collect_fterms(LiftedBooleanOperatorView element, ygg::UnorderedSet<FunctionTermView<T>>& result)
{
    visit([&](auto&& arg) { collect_fterms(arg, result); }, element.get_variant());
}

template<FactKind T>
inline void collect_fterms(GroundBooleanOperatorView element, ygg::UnorderedSet<GroundFunctionTermView<T>>& result)
{
    visit([&](auto&& arg) { collect_fterms(arg, result); }, element.get_variant());
}

template<FactKind T1, FactKind T2>
inline void collect_fterms(NumericEffectView<T1> element, ygg::UnorderedSet<FunctionTermView<T2>>& result)
{
    collect_fterms(element.get_fterm(), result);
    collect_fterms(element.get_fexpr(), result);
}

template<FactKind T1, FactKind T2>
inline void collect_fterms(GroundNumericEffectView<T1> element, ygg::UnorderedSet<GroundFunctionTermView<T2>>& result)
{
    collect_fterms(element.get_fterm(), result);
    collect_fterms(element.get_fexpr(), result);
}

template<FactKind T1, FactKind T2>
inline void collect_fterms(NumericEffectOperatorView<T1> element, ygg::UnorderedSet<FunctionTermView<T2>>& result)
{
    visit([&](auto&& arg) { collect_fterms(arg, result); }, element.get_variant());
}

template<FactKind T1, FactKind T2>
inline void collect_fterms(GroundNumericEffectOperatorView<T1> element, ygg::UnorderedSet<GroundFunctionTermView<T2>>& result)
{
    visit([&](auto&& arg) { collect_fterms(arg, result); }, element.get_variant());
}

template<FactKind T>
inline auto collect_fterms(LiftedBooleanOperatorView element)
{
    auto result = ygg::UnorderedSet<FunctionTermView<T>> {};
    visit([&](auto&& arg) { collect_fterms(arg, result); }, element.get_variant());
    auto result_vec = std::vector<FunctionTermView<T>>(result.begin(), result.end());
    std::sort(result_vec.begin(), result_vec.end());
    return result_vec;
}

template<FactKind T>
inline auto collect_fterms(GroundBooleanOperatorView element)
{
    auto result = ygg::UnorderedSet<GroundFunctionTermView<T>> {};
    visit([&](auto&& arg) { collect_fterms(arg, result); }, element.get_variant());
    auto result_vec = std::vector<GroundFunctionTermView<T>>(result.begin(), result.end());
    std::sort(result_vec.begin(), result_vec.end());
    return result_vec;
}
}

#endif
