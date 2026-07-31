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

#include "tyr/analysis/listeners.hpp"

#include "tyr/analysis/stratification.hpp"
#include "tyr/formalism/datalog/atom_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/literal_index.hpp"
#include "tyr/formalism/datalog/literal_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/predicate_view.hpp"

#include <cista/containers/vector.h>
#include <gtl/phmap.hpp>
#include <type_traits>
#include <utility>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/ids/index_mixins.hpp>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

namespace tyr::analysis
{
namespace
{
template<f::RelationKind R>
void add_function_listeners(fd::FunctionExpressionView expression, ygg::Index<fd::Rule<R>> rule, TypedListenerStratum<R>& listeners);

template<f::RelationKind R>
void add_function_listeners(ygg::float_t, ygg::Index<fd::Rule<R>>, TypedListenerStratum<R>&)
{
}

template<f::RelationKind R>
void add_function_listeners(fd::LiftedUnaryOperatorView expression, ygg::Index<fd::Rule<R>> rule, TypedListenerStratum<R>& listeners)
{
    add_function_listeners(expression.get_arg(), rule, listeners);
}

template<f::BinaryOperatorKind O, f::RelationKind R>
void add_function_listeners(fd::LiftedBinaryOperatorView<O> expression, ygg::Index<fd::Rule<R>> rule, TypedListenerStratum<R>& listeners)
{
    add_function_listeners(expression.get_lhs(), rule, listeners);
    add_function_listeners(expression.get_rhs(), rule, listeners);
}

template<f::RelationKind R>
void add_function_listeners(fd::LiftedMultiOperatorView expression, ygg::Index<fd::Rule<R>> rule, TypedListenerStratum<R>& listeners)
{
    for (const auto arg : expression.get_args())
        add_function_listeners(arg, rule, listeners);
}

template<f::FactKind T, f::RelationKind R>
void add_function_listeners(fd::FunctionTermView<T>, ygg::Index<fd::Rule<R>>, TypedListenerStratum<R>&)
{
}

template<f::RelationKind R>
void add_function_listeners(fd::FunctionTermView<f::FluentTag> term, ygg::Index<fd::Rule<R>> rule, TypedListenerStratum<R>& listeners)
{
    listeners.functions[term.get_function().get_index()].insert(rule);
}

template<f::RelationKind R>
void add_function_listeners(fd::LiftedArithmeticOperatorView expression, ygg::Index<fd::Rule<R>> rule, TypedListenerStratum<R>& listeners)
{
    visit([&](auto&& arg) { add_function_listeners(arg, rule, listeners); }, expression.get_variant());
}

template<f::RelationKind R>
void add_function_listeners(fd::FunctionExpressionView expression, ygg::Index<fd::Rule<R>> rule, TypedListenerStratum<R>& listeners)
{
    visit([&](auto&& arg) { add_function_listeners(arg, rule, listeners); }, expression.get_variant());
}

template<f::RelationKind R>
void add_function_listeners(fd::LiftedBooleanOperatorView expression, ygg::Index<fd::Rule<R>> rule, TypedListenerStratum<R>& listeners)
{
    visit(
        [&](auto&& arg)
        {
            add_function_listeners(arg.get_lhs(), rule, listeners);
            add_function_listeners(arg.get_rhs(), rule, listeners);
        },
        expression.get_variant());
}

template<f::RelationKind R>
void add_numeric_effect_head_listeners(fd::NumericEffectView<f::FluentTag> effect, ygg::Index<fd::Rule<R>> rule, TypedListenerStratum<R>& listeners)
{
    if (effect.get_operator() != f::NumericEffectOperatorKind::Assign)
        add_function_listeners(effect.get_fterm(), rule, listeners);

    add_function_listeners(effect.get_fexpr(), rule, listeners);
}

template<f::RelationKind R>
void add_head_listeners(fd::AtomView<f::FluentTag>, ygg::Index<fd::Rule<R>>, TypedListenerStratum<R>&)
{
}

template<f::RelationKind R>
void add_head_listeners(fd::NumericEffectOperatorView<f::FluentTag> head, ygg::Index<fd::Rule<R>> rule, TypedListenerStratum<R>& listeners)
{
    visit([&](auto&& effect) { add_numeric_effect_head_listeners(effect, rule, listeners); }, head.get_variant());
}

template<f::RelationKind R>
void add_stratum_listeners(const TypedRuleStratum<R>& stratum, const fd::Repository& context, TypedListenerStratum<R>& listeners)
{
    for (const auto rule : stratum)
    {
        const auto rule_view = ygg::make_view(rule, context);

        for (const auto literal : rule_view.get_body().template get_literals<f::FluentTag>())
            if (literal.get_polarity())
                listeners.predicates[literal.get_atom().get_predicate().get_index()].insert(rule);

        for (const auto constraint : rule_view.get_body().get_numeric_constraints())
            add_function_listeners(constraint, rule, listeners);

        add_head_listeners(rule_view.get_head(), rule, listeners);
    }
}
}

ListenerStrata compute_listeners(const RuleStrata& strata, const fd::Repository& context)
{
    auto listeners = ListenerStrata();

    for (const auto& stratum : strata.data)
    {
        auto listeners_in_stratum = ListenerStratum {};

        add_stratum_listeners(stratum.template get<f::PredicateTag>(), context, listeners_in_stratum.template get<f::PredicateTag>());
        add_stratum_listeners(stratum.template get<f::FunctionTag>(), context, listeners_in_stratum.template get<f::FunctionTag>());

        listeners.data.push_back(std::move(listeners_in_stratum));
    }

    // std::cout << listeners.data << std::endl;

    return listeners;
}
}
