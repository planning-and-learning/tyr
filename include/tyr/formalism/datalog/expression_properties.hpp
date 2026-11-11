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

#include <urd/elements/equal_to.hpp>
#include <urd/elements/hash.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/std_set.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::formalism::datalog
{
/**
 * Forward declarations
 */

template<TaskKind T, FactKind F>
void collect_fterms(ygg::float_t element, urd::Store<urd::set<FunctionTermView<T, F>>>& result);

template<TaskKind T, FactKind F1, FactKind F2>
void collect_fterms(FunctionTermView<T, F1> element, urd::Store<urd::set<FunctionTermView<T, F2>>>& result);

template<TaskKind T, FactKind F, typename... Vs>
void collect_fterms(const std::variant<Vs...>& element, urd::Store<urd::set<FunctionTermView<T, F>>>& result);

template<TaskKind T, FactKind F>
void collect_fterms(UnaryOperatorView<T, ::tyr::formalism::ArithmeticOperatorKind> element, urd::Store<urd::set<FunctionTermView<T, F>>>& result);

template<TaskKind T, FactKind F, OperatorKind O>
void collect_fterms(BinaryOperatorView<T, O> element, urd::Store<urd::set<FunctionTermView<T, F>>>& result);

template<TaskKind T, FactKind F>
void collect_fterms(MultiOperatorView<T, ::tyr::formalism::ArithmeticOperatorKind> element, urd::Store<urd::set<FunctionTermView<T, F>>>& result);

template<TaskKind T, FactKind F1, FactKind F2>
void collect_fterms(NumericEffectView<T, F1> element, urd::Store<urd::set<FunctionTermView<T, F2>>>& result);

template<TaskKind T, RelationKind R>
auto collect_fluent_reads(RuleView<T, R> rule);

/**
 * Implementations
 */

template<TaskKind T, FactKind F>
inline void collect_fterms(ygg::float_t, urd::Store<urd::set<FunctionTermView<T, F>>>&)
{
}

template<TaskKind T, FactKind F1, FactKind F2>
inline void collect_fterms(FunctionTermView<T, F1> element, urd::Store<urd::set<FunctionTermView<T, F2>>>& result)
{
    if constexpr (std::same_as<F1, F2>)
        result.insert(element);
}

template<TaskKind T, FactKind F, typename... Vs>
inline void collect_fterms(const std::variant<Vs...>& element, urd::Store<urd::set<FunctionTermView<T, F>>>& result)
{
    visit([&](auto&& arg) { collect_fterms(arg, result); }, element);
}

template<TaskKind T, FactKind F>
inline void collect_fterms(UnaryOperatorView<T, ::tyr::formalism::ArithmeticOperatorKind> element, urd::Store<urd::set<FunctionTermView<T, F>>>& result)
{ collect_fterms(element.get_arg(), result); }

template<TaskKind T, FactKind F, OperatorKind O>
inline void collect_fterms(BinaryOperatorView<T, O> element, urd::Store<urd::set<FunctionTermView<T, F>>>& result)
{
    collect_fterms(element.get_lhs(), result);
    collect_fterms(element.get_rhs(), result);
}

template<TaskKind T, FactKind F>
inline void collect_fterms(MultiOperatorView<T, ::tyr::formalism::ArithmeticOperatorKind> element, urd::Store<urd::set<FunctionTermView<T, F>>>& result)
{
    for (const auto& arg : element.get_args())
        collect_fterms(arg, result);
}

template<TaskKind T, FactKind F1, FactKind F2>
inline void collect_fterms(NumericEffectView<T, F1> element, urd::Store<urd::set<FunctionTermView<T, F2>>>& result)
{
    collect_fterms(element.get_fterm(), result);
    collect_fterms(element.get_fexpr(), result);
}

namespace detail
{
template<typename Effect, typename Result>
void collect_effect_reads(Effect effect, bool reads_target, Result& result)
{
    collect_fterms(effect.get_fexpr(), result);
    if (reads_target)
        collect_fterms(effect.get_fterm(), result);
}

template<TaskKind T, typename Result>
void collect_semantic_head_reads(NumericEffectOperatorView<T, FluentTag> head, Result& result)
{ collect_effect_reads(head, head.get_operator() != NumericEffectOperatorKind::Assign, result); }

template<typename Rule, typename Result>
void collect_rule_fluent_reads(Rule rule, Result& result)
{
    for (const auto constraint : rule.get_body().get_numeric_constraints())
        collect_fterms(constraint, result);

    if constexpr (requires { rule.get_head(); })
        collect_semantic_head_reads(rule.get_head(), result);
}
}

template<TaskKind T, RelationKind R>
inline auto collect_fluent_reads(RuleView<T, R> rule)
{
    auto result = urd::Store<urd::set<FunctionTermView<T, FluentTag>>> {};
    detail::collect_rule_fluent_reads(rule, result);
    return result;
}

template<TaskKind T, FactKind F>
inline auto collect_fterms(BooleanOperatorView<T> element)
{
    auto result = urd::Store<urd::set<FunctionTermView<T, F>>> {};
    collect_fterms(element, result);
    auto result_vec = urd::Store<urd::sequence<FunctionTermView<T, F>>>(result.begin(), result.end());
    std::sort(result_vec.begin(), result_vec.end());
    return result_vec;
}

}

#endif
