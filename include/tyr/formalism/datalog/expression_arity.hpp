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

#ifndef TYR_FORMALISM_DATALOG_EXPRESSION_ARITY_HPP_
#define TYR_FORMALISM_DATALOG_EXPRESSION_ARITY_HPP_

#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_view.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <numeric>
#include <urd/elements/equal_to.hpp>
#include <urd/elements/hash.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/values/std_set.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::formalism::datalog
{

/**
 * collect_parameters
 */

void collect_parameters(ygg::float_t element, urd::Store<urd::set<ParameterIndex>>& result);

void collect_parameters(TermView element, urd::Store<urd::set<ParameterIndex>>& result);

template<FactKind F>
void collect_parameters(AtomView<LiftedTag, F> element, urd::Store<urd::set<ParameterIndex>>& result);

template<FactKind F>
void collect_parameters(LiteralView<LiftedTag, F> element, urd::Store<urd::set<ParameterIndex>>& result);

template<FactKind F>
void collect_parameters(FunctionTermView<LiftedTag, F> element, urd::Store<urd::set<ParameterIndex>>& result);

void collect_parameters(FunctionExpressionView<LiftedTag> element, urd::Store<urd::set<ParameterIndex>>& result);

void collect_parameters(UnaryOperatorView<LiftedTag, ::tyr::formalism::ArithmeticOperatorKind> element, urd::Store<urd::set<ParameterIndex>>& result);

template<OperatorKind O>
void collect_parameters(BinaryOperatorView<LiftedTag, O> element, urd::Store<urd::set<ParameterIndex>>& result);

void collect_parameters(MultiOperatorView<LiftedTag, ::tyr::formalism::ArithmeticOperatorKind> element, urd::Store<urd::set<ParameterIndex>>& result);

auto collect_parameters(BooleanOperatorView<LiftedTag> element);

/**
 * Implementations
 */

inline void collect_parameters(ygg::float_t, urd::Store<urd::set<ParameterIndex>>&) {}

inline void collect_parameters(TermView element, urd::Store<urd::set<ParameterIndex>>& result)
{
    visit(
        [&]<typename Alternative>(const Alternative& arg)
        {
            if constexpr (std::is_same_v<Alternative, ParameterIndex>)
                result.insert(arg);
            else if constexpr (std::is_same_v<Alternative, ObjectView>) {}
            else
                static_assert(false, "Missing case");
        },
        element);
}

template<FactKind F>
inline void collect_parameters(AtomView<LiftedTag, F> element, urd::Store<urd::set<ParameterIndex>>& result)
{
    for (const auto term : element.get_terms())
        collect_parameters(term, result);
}

template<FactKind F>
inline void collect_parameters(LiteralView<LiftedTag, F> element, urd::Store<urd::set<ParameterIndex>>& result)
{ collect_parameters(element.get_atom(), result); }

template<FactKind F>
inline void collect_parameters(FunctionTermView<LiftedTag, F> element, urd::Store<urd::set<ParameterIndex>>& result)
{
    for (const auto term : element.get_terms())
        collect_parameters(term, result);
}

inline void collect_parameters(FunctionExpressionView<LiftedTag> element, urd::Store<urd::set<ParameterIndex>>& result)
{
    visit([&](auto&& arg) { collect_parameters(arg, result); }, element);
}

inline void collect_parameters(UnaryOperatorView<LiftedTag, ::tyr::formalism::ArithmeticOperatorKind> element, urd::Store<urd::set<ParameterIndex>>& result)
{ collect_parameters(element.get_arg(), result); }

template<OperatorKind O>
inline void collect_parameters(BinaryOperatorView<LiftedTag, O> element, urd::Store<urd::set<ParameterIndex>>& result)
{
    collect_parameters(element.get_lhs(), result);
    collect_parameters(element.get_rhs(), result);
}

inline void collect_parameters(MultiOperatorView<LiftedTag, ::tyr::formalism::ArithmeticOperatorKind> element, urd::Store<urd::set<ParameterIndex>>& result)
{
    for (const auto arg : element.get_args())
        collect_parameters(arg, result);
}

template<FactKind F>
inline auto collect_parameters(AtomView<LiftedTag, F> element)
{
    auto result = urd::Store<urd::set<ParameterIndex>> {};
    collect_parameters(element, result);
    return result;
}

template<FactKind F>
inline auto collect_parameters(LiteralView<LiftedTag, F> element)
{
    auto result = urd::Store<urd::set<ParameterIndex>> {};
    collect_parameters(element, result);
    return result;
}

template<FactKind F>
inline auto collect_parameters(FunctionTermView<LiftedTag, F> element)
{
    auto result = urd::Store<urd::set<ParameterIndex>> {};
    collect_parameters(element, result);
    return result;
}

inline auto collect_parameters(BooleanOperatorView<LiftedTag> element)
{
    auto result = urd::Store<urd::set<ParameterIndex>> {};
    collect_parameters(element, result);
    return result;
}

/**
 * max_fterm_arity
 */

inline size_t max_fterm_arity(ygg::float_t element);

template<FactKind F>
size_t max_fterm_arity(FunctionTermView<LiftedTag, F> element);

size_t max_fterm_arity(FunctionExpressionView<LiftedTag> element);

size_t max_fterm_arity(UnaryOperatorView<LiftedTag, ::tyr::formalism::ArithmeticOperatorKind> element);

template<OperatorKind O>
size_t max_fterm_arity(BinaryOperatorView<LiftedTag, O> element);

size_t max_fterm_arity(MultiOperatorView<LiftedTag, ::tyr::formalism::ArithmeticOperatorKind> element);

/**
 * Implementations
 */

inline size_t max_fterm_arity(ygg::float_t) { return 0; }

template<FactKind F>
inline size_t max_fterm_arity(FunctionTermView<LiftedTag, F> element)
{ return element.get_function().get_arity(); }

inline size_t max_fterm_arity(FunctionExpressionView<LiftedTag> element)
{
    return visit([&](auto&& arg) { return max_fterm_arity(arg); }, element);
}

inline size_t max_fterm_arity(UnaryOperatorView<LiftedTag, ::tyr::formalism::ArithmeticOperatorKind> element) { return max_fterm_arity(element.get_arg()); }

template<OperatorKind O>
inline size_t max_fterm_arity(BinaryOperatorView<LiftedTag, O> element)
{ return std::max(max_fterm_arity(element.get_lhs()), max_fterm_arity(element.get_rhs())); }

inline size_t max_fterm_arity(MultiOperatorView<LiftedTag, ::tyr::formalism::ArithmeticOperatorKind> element)
{
    const auto child_fexprs = element.get_args();

    return std::accumulate(std::next(child_fexprs.begin()),  // Start from the second expression
                           child_fexprs.end(),
                           max_fterm_arity(child_fexprs.front()),
                           [&](const auto& value, const auto& child_expr) { return std::max(value, max_fterm_arity(child_expr)); });
}

/**
 * kckp_arity
 */

template<FactKind F>
inline size_t kckp_arity(LiteralView<LiftedTag, F> element)
{ return element.get_atom().get_predicate().get_arity(); }

template<FactKind F>
inline size_t kckp_arity(FunctionTermView<LiftedTag, F> element)
{ return element.get_function().get_arity(); }

inline size_t kckp_arity(BooleanOperatorView<LiftedTag> element) { return std::max(max_fterm_arity(element), collect_parameters(element).size()); }

/**
 * parameter_arity
 */

template<FactKind F>
inline size_t parameter_arity(LiteralView<LiftedTag, F> element)
{ return collect_parameters(element).size(); }

template<FactKind F>
inline size_t parameter_arity(FunctionTermView<LiftedTag, F> element)
{ return collect_parameters(element).size(); }

inline size_t parameter_arity(BooleanOperatorView<LiftedTag> element) { return collect_parameters(std::move(element)).size(); }

}

#endif
