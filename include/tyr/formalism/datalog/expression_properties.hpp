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

#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::formalism::datalog
{
/**
 * Forward declarations
 */

template<TaskKind T, FactKind F>
void collect_fterms(ygg::float_t element, ygg::UnorderedSet<FunctionTermView<T, F>>& result);

template<TaskKind T, FactKind F1, FactKind F2>
void collect_fterms(FunctionTermView<T, F1> element, ygg::UnorderedSet<FunctionTermView<T, F2>>& result);

template<TaskKind T, FactKind F>
void collect_fterms(FunctionExpressionView<T> element, ygg::UnorderedSet<FunctionTermView<T, F>>& result);

template<TaskKind T, FactKind F>
void collect_fterms(UnaryOperatorView<T> element, ygg::UnorderedSet<FunctionTermView<T, F>>& result);

template<TaskKind T, FactKind F, BinaryOperatorKind O>
void collect_fterms(BinaryOperatorView<T, O> element, ygg::UnorderedSet<FunctionTermView<T, F>>& result);

template<TaskKind T, FactKind F>
void collect_fterms(MultiOperatorView<T> element, ygg::UnorderedSet<FunctionTermView<T, F>>& result);

template<TaskKind T, FactKind F>
void collect_fterms(ArithmeticOperatorView<T> element, ygg::UnorderedSet<FunctionTermView<T, F>>& result);

template<TaskKind T, FactKind F>
void collect_fterms(BooleanOperatorView<T> element, ygg::UnorderedSet<FunctionTermView<T, F>>& result);

template<TaskKind T, FactKind F1, FactKind F2>
void collect_fterms(NumericEffectView<T, F1> element, ygg::UnorderedSet<FunctionTermView<T, F2>>& result);

template<TaskKind T, FactKind F1, FactKind F2>
void collect_fterms(NumericEffectOperatorView<T, F1> element, ygg::UnorderedSet<FunctionTermView<T, F2>>& result);

template<TaskKind T, RelationKind R>
auto collect_fluent_reads(RuleView<T, R> rule);

/**
 * Implementations
 */

template<TaskKind T, FactKind F>
inline void collect_fterms(ygg::float_t, ygg::UnorderedSet<FunctionTermView<T, F>>&)
{
}

template<TaskKind T, FactKind F1, FactKind F2>
inline void collect_fterms(FunctionTermView<T, F1> element, ygg::UnorderedSet<FunctionTermView<T, F2>>& result)
{
    if constexpr (std::same_as<F1, F2>)
        result.insert(element);
}

template<TaskKind T, FactKind F>
inline void collect_fterms(FunctionExpressionView<T> element, ygg::UnorderedSet<FunctionTermView<T, F>>& result)
{
    visit([&](auto&& arg) { collect_fterms(arg, result); }, element.get_variant());
}

template<TaskKind T, FactKind F>
inline void collect_fterms(UnaryOperatorView<T> element, ygg::UnorderedSet<FunctionTermView<T, F>>& result)
{
    collect_fterms(element.get_arg(), result);
}

template<TaskKind T, FactKind F, BinaryOperatorKind O>
inline void collect_fterms(BinaryOperatorView<T, O> element, ygg::UnorderedSet<FunctionTermView<T, F>>& result)
{
    collect_fterms(element.get_lhs(), result);
    collect_fterms(element.get_rhs(), result);
}

template<TaskKind T, FactKind F>
inline void collect_fterms(MultiOperatorView<T> element, ygg::UnorderedSet<FunctionTermView<T, F>>& result)
{
    for (const auto& arg : element.get_args())
        collect_fterms(arg, result);
}

template<TaskKind T, FactKind F>
inline void collect_fterms(ArithmeticOperatorView<T> element, ygg::UnorderedSet<FunctionTermView<T, F>>& result)
{
    visit([&](auto&& arg) { collect_fterms(arg, result); }, element.get_variant());
}

template<TaskKind T, FactKind F>
inline void collect_fterms(BooleanOperatorView<T> element, ygg::UnorderedSet<FunctionTermView<T, F>>& result)
{
    visit([&](auto&& arg) { collect_fterms(arg, result); }, element.get_variant());
}

template<TaskKind T, FactKind F1, FactKind F2>
inline void collect_fterms(NumericEffectView<T, F1> element, ygg::UnorderedSet<FunctionTermView<T, F2>>& result)
{
    collect_fterms(element.get_fterm(), result);
    collect_fterms(element.get_fexpr(), result);
}

template<TaskKind T, FactKind F1, FactKind F2>
inline void collect_fterms(NumericEffectOperatorView<T, F1> element, ygg::UnorderedSet<FunctionTermView<T, F2>>& result)
{
    visit([&](auto&& arg) { collect_fterms(arg, result); }, element.get_variant());
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
void collect_semantic_head_reads(AtomView<T, FluentTag>, Result&)
{
}

template<TaskKind T, typename Result>
void collect_semantic_head_reads(NumericEffectOperatorView<T, FluentTag> head, Result& result)
{
    visit([&](auto effect) { collect_effect_reads(effect, effect.get_operator() != NumericEffectOperatorKind::Assign, result); }, head.get_variant());
}

template<typename Rule, typename Result>
void collect_rule_fluent_reads(Rule rule, Result& result)
{
    for (const auto constraint : rule.get_body().get_numeric_constraints())
        collect_fterms(constraint, result);

    collect_semantic_head_reads(rule.get_head(), result);

    for (const auto metric_effect : rule.get_metric_effects())
        visit(
            [&](auto effect)
            {
                const auto op = effect.get_operator();
                const auto reads_target =
                    op == NumericEffectOperatorKind::Assign || op == NumericEffectOperatorKind::ScaleUp || op == NumericEffectOperatorKind::ScaleDown;
                collect_effect_reads(effect, reads_target, result);
            },
            metric_effect.get_variant());
}
}

template<TaskKind T, RelationKind R>
inline auto collect_fluent_reads(RuleView<T, R> rule)
{
    auto result = ygg::UnorderedSet<FunctionTermView<T, FluentTag>> {};
    detail::collect_rule_fluent_reads(rule, result);
    return result;
}

template<TaskKind T, FactKind F>
inline auto collect_fterms(BooleanOperatorView<T> element)
{
    auto result = ygg::UnorderedSet<FunctionTermView<T, F>> {};
    visit([&](auto&& arg) { collect_fterms(arg, result); }, element.get_variant());
    auto result_vec = std::vector<FunctionTermView<T, F>>(result.begin(), result.end());
    std::sort(result_vec.begin(), result_vec.end());
    return result_vec;
}

}

#endif
