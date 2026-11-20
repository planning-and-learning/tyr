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

#ifndef TYR_FORMALISM_DATALOG_GROUNDER_HPP_
#define TYR_FORMALISM_DATALOG_GROUNDER_HPP_

#include "tyr/analysis/declarations.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/interning.hpp"
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
#include "tyr/formalism/write_context.hpp"

#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::formalism::datalog
{

/**
 * ground
 */

template<FactKind F>
std::pair<FunctionRowView<F>, bool> ground(TermListView terms, FunctionView<F> function, WriteContext<Builder, Repository>& context);

template<FactKind F>
std::pair<FunctionTermView<GroundTag, F>, bool> ground(FunctionTermView<LiftedTag, F> element, WriteContext<Builder, Repository>& context);

FunctionExpressionView<GroundTag> ground(FunctionExpressionView<LiftedTag> element, WriteContext<Builder, Repository>& context);

std::pair<UnaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool> ground(UnaryOperatorView<LiftedTag, ArithmeticOperatorKind> element,
                                                                             WriteContext<Builder, Repository>& context);

template<OperatorKind O>
std::pair<BinaryOperatorView<GroundTag, O>, bool> ground(BinaryOperatorView<LiftedTag, O> element, WriteContext<Builder, Repository>& context);

std::pair<MultiOperatorView<GroundTag, ArithmeticOperatorKind>, bool> ground(MultiOperatorView<LiftedTag, ArithmeticOperatorKind> element,
                                                                             WriteContext<Builder, Repository>& context);

template<FactKind F>
std::pair<PredicateRowView<F>, bool> ground(TermListView terms, PredicateView<F> predicate, WriteContext<Builder, Repository>& context);

template<FactKind F>
std::pair<AtomView<GroundTag, F>, bool> ground(AtomView<LiftedTag, F> element, WriteContext<Builder, Repository>& context);

template<FactKind F>
std::pair<LiteralView<GroundTag, F>, bool> ground(LiteralView<LiftedTag, F> element, WriteContext<Builder, Repository>& context);

template<FactKind F>
std::pair<NumericEffectView<GroundTag, F>, bool> ground(NumericEffectView<LiftedTag, F> element, WriteContext<Builder, Repository>& context);

std::pair<ConjunctiveConditionView<GroundTag>, bool> ground(ConjunctiveConditionView<LiftedTag> element, WriteContext<Builder, Repository>& context);

/**
 * ground_binding
 */

template<FactKind F>
std::pair<PredicateRowView<F>, bool> ground_binding(AtomView<LiftedTag, F> element, WriteContext<Builder, Repository>& context);

/// The key of the atom's row, interned as ground_binding does, without its view (which finds the layer that owns the row).
template<FactKind F>
std::pair<PredicateRowIndex<F>, bool> ground_row(AtomView<LiftedTag, F> element, WriteContext<Builder, Repository>& context);

/// Grounds the binding without interning the enclosing GroundFunctionTerm symbol.
template<FactKind F>
std::pair<FunctionRowView<F>, bool> ground_binding(FunctionTermView<LiftedTag, F> element, WriteContext<Builder, Repository>& context);

std::pair<RuleRowView, bool> ground_binding(RuleView<LiftedTag> element, WriteContext<Builder, Repository>& context);

/**
 * try_ground
 */

template<FactKind F>
std::optional<FunctionRowView<F>> try_ground_binding(FunctionTermView<LiftedTag, F> element, WriteContext<Builder, Repository>& context);

template<FactKind F>
std::optional<PredicateRowView<F>> try_ground_binding(AtomView<LiftedTag, F> element, WriteContext<Builder, Repository>& context);

/**
 * ground
 */

/// Substitute terms into retained row values (objects as their canonical Values) without interning a binding.
inline void ground(TermListView terms, const WriteContext<Builder, Repository>& context, std::vector<ygg::Value>& values)
{
    values.clear();
    for (const auto term : terms)
        visit(
            [&]<typename Alternative>(Alternative arg)
            {
                if constexpr (std::same_as<Alternative, ParameterIndex>)
                    values.push_back(context.binding()[arg.value].value);
                else if constexpr (std::same_as<Alternative, ObjectView>)
                    values.push_back(arg.index().value);
                else
                    static_assert(false, "Missing case");
            },
            term);
}

template<FactKind F>
std::pair<FunctionRowView<F>, bool> ground(TermListView terms, FunctionView<F> function, WriteContext<Builder, Repository>& context)
{
    auto binding = datalog::checkout<ygg::Row<Function<F>>>(context.builder());

    urd::store(binding->relation, function);
    ground(terms, context, binding->values);

    // Canonicalize and Serialize
    return context.destination().insert(*binding, context.builder());
}

template<FactKind F>
std::pair<FunctionTermView<GroundTag, F>, bool> ground(FunctionTermView<LiftedTag, F> element, WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto fterm = datalog::checkout<FunctionTerm<GroundTag, F>>(context.builder());

    // Fill data
    urd::store(fterm->row, ground(element.get_terms(), element.get_function(), context).first);

    // Canonicalize and Serialize
    return context.destination().insert(*fterm, context.builder());
}

inline FunctionExpressionView<GroundTag> ground(FunctionExpressionView<LiftedTag> element, WriteContext<Builder, Repository>& context)
{
    return std::visit(
        [&]<typename Alternative>(const Alternative& arg) -> FunctionExpressionView<GroundTag>
        {
            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return arg;
            else
                return ground(arg, context).first;
        },
        element);
}

inline std::pair<UnaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool> ground(UnaryOperatorView<LiftedTag, ArithmeticOperatorKind> element,
                                                                                    WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto unary = datalog::checkout<UnaryOperator<GroundTag, ArithmeticOperatorKind>>(context.builder());

    // Fill data
    unary->operation = element.get_operation();
    urd::store(unary->arg, ground(element.get_arg(), context));

    // Canonicalize and Serialize
    return context.destination().insert(*unary, context.builder());
}

template<OperatorKind O>
std::pair<BinaryOperatorView<GroundTag, O>, bool> ground(BinaryOperatorView<LiftedTag, O> element, WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto binary = datalog::checkout<BinaryOperator<GroundTag, O>>(context.builder());

    // Fill data
    binary->operation = element.get_operation();
    urd::store(binary->lhs, ground(element.get_lhs(), context));
    urd::store(binary->rhs, ground(element.get_rhs(), context));

    // Canonicalize and Serialize
    return context.destination().insert(*binary, context.builder());
}

inline std::pair<MultiOperatorView<GroundTag, ArithmeticOperatorKind>, bool> ground(MultiOperatorView<LiftedTag, ArithmeticOperatorKind> element,
                                                                                    WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto multi = datalog::checkout<MultiOperator<GroundTag, ArithmeticOperatorKind>>(context.builder());

    // Fill data
    multi->operation = element.get_operation();
    for (const auto arg : element.get_args())
        urd::append(multi->args, ground(arg, context));

    // Canonicalize and Serialize
    return context.destination().insert(*multi, context.builder());
}

template<FactKind F>
std::pair<PredicateRowView<F>, bool> ground(TermListView terms, PredicateView<F> predicate, WriteContext<Builder, Repository>& context)
{
    auto binding = datalog::checkout<ygg::Row<Predicate<F>>>(context.builder());

    urd::store(binding->relation, predicate);
    ground(terms, context, binding->values);

    // Canonicalize and Serialize
    return context.destination().insert(*binding, context.builder());
}

template<FactKind F>
std::pair<AtomView<GroundTag, F>, bool> ground(AtomView<LiftedTag, F> element, WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto atom = datalog::checkout<Atom<GroundTag, F>>(context.builder());

    // Fill data
    urd::store(atom->row, ground(element.get_terms(), element.get_predicate(), context).first);

    // Canonicalize and Serialize
    return context.destination().insert(*atom, context.builder());
}

template<FactKind F>
std::pair<LiteralView<GroundTag, F>, bool> ground(LiteralView<LiftedTag, F> element, WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto ground_literal = datalog::checkout<Literal<GroundTag, F>>(context.builder());

    // Fill data
    ground_literal->polarity = element.get_polarity();
    urd::store(ground_literal->atom, ground(element.get_atom(), context).first);

    // Canonicalize and Serialize
    return context.destination().insert(*ground_literal, context.builder());
}

template<FactKind F>
std::pair<NumericEffectView<GroundTag, F>, bool> ground(NumericEffectView<LiftedTag, F> element, WriteContext<Builder, Repository>& context)
{
    auto numeric_effect = datalog::checkout<NumericEffect<GroundTag, F>>(context.builder());

    numeric_effect->operation = element.get_operation();
    urd::store(numeric_effect->fterm, ground(element.get_fterm(), context).first);
    urd::store(numeric_effect->fexpr, ground(element.get_fexpr(), context));

    return context.destination().insert(*numeric_effect, context.builder());
}

inline std::pair<ConjunctiveConditionView<GroundTag>, bool> ground(ConjunctiveConditionView<LiftedTag> element, WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto conj_cond = datalog::checkout<ConjunctiveCondition<GroundTag>>(context.builder());

    // Fill data
    for (const auto literal : element.template get_literals<StaticTag>())
        urd::append(conj_cond->static_literals, ground(literal, context).first);
    for (const auto literal : element.template get_literals<FluentTag>())
        urd::append(conj_cond->fluent_literals, ground(literal, context).first);
    for (const auto literal : element.template get_literals<DerivedTag>())
        urd::append(conj_cond->derived_literals, ground(literal, context).first);
    for (const auto numeric_constraint : element.get_numeric_constraints())
        urd::append(conj_cond->numeric_constraints, ground(numeric_constraint, context).first);

    // Canonicalize and Serialize
    return context.destination().insert(*conj_cond, context.builder());
}

/**
 * ground_binding
 */

template<FactKind F>
std::pair<PredicateRowIndex<F>, bool> ground_row(AtomView<LiftedTag, F> element, WriteContext<Builder, Repository>& context)
{
    auto binding = datalog::checkout<ygg::Row<Predicate<F>>>(context.builder());

    urd::store(binding->relation, element.get_predicate());
    ground(element.get_terms(), context, binding->values);

    const auto [row, created] = context.destination().insert(*binding, context.builder());
    return { row.index(), created };
}

template<FactKind F>
std::pair<PredicateRowView<F>, bool> ground_binding(AtomView<LiftedTag, F> element, WriteContext<Builder, Repository>& context)
{
    const auto [key, created] = ground_row(element, context);
    return { context.destination().view(key), created };
}

template<FactKind F>
std::pair<FunctionRowView<F>, bool> ground_binding(FunctionTermView<LiftedTag, F> element, WriteContext<Builder, Repository>& context)
{ return ground(element.get_terms(), element.get_function(), context); }

inline std::pair<RuleRowView, bool> ground_binding(RuleView<LiftedTag> element, WriteContext<Builder, Repository>& context)
{
    auto binding = datalog::checkout<ygg::Row<Rule<LiftedTag>>>(context.builder());

    urd::store(binding->relation, element);
    for (const auto object : context.binding())
        binding->values.push_back(object.value);

    return context.destination().insert(*binding, context.builder());
}

/**
 * try_ground
 */

template<FactKind F>
std::optional<FunctionRowView<F>> try_ground_binding(FunctionTermView<LiftedTag, F> element, WriteContext<Builder, Repository>& context)
{
    auto binding = datalog::checkout<ygg::Row<Function<F>>>(context.builder());

    urd::store(binding->relation, element.get_function());
    ground(element.get_terms(), context, binding->values);

    return context.destination().find(*binding, context.builder());
}

template<FactKind F>
std::optional<PredicateRowView<F>> try_ground_binding(AtomView<LiftedTag, F> element, WriteContext<Builder, Repository>& context)
{
    auto binding = datalog::checkout<ygg::Row<Predicate<F>>>(context.builder());

    urd::store(binding->relation, element.get_predicate());
    ground(element.get_terms(), context, binding->values);

    return context.destination().find(*binding, context.builder());
}
}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::datalog
{
extern template std::pair<FunctionRowView<StaticTag>, bool>
ground(TermListView terms, FunctionView<StaticTag> function, WriteContext<Builder, Repository>& context);
extern template std::pair<FunctionRowView<FluentTag>, bool>
ground(TermListView terms, FunctionView<FluentTag> function, WriteContext<Builder, Repository>& context);

extern template std::pair<FunctionTermView<GroundTag, StaticTag>, bool> ground(FunctionTermView<LiftedTag, StaticTag> element,
                                                                               WriteContext<Builder, Repository>& context);
extern template std::pair<FunctionTermView<GroundTag, FluentTag>, bool> ground(FunctionTermView<LiftedTag, FluentTag> element,
                                                                               WriteContext<Builder, Repository>& context);

extern template std::pair<BinaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool> ground(BinaryOperatorView<LiftedTag, ArithmeticOperatorKind> element,
                                                                                              WriteContext<Builder, Repository>& context);
extern template std::pair<BinaryOperatorView<GroundTag, BooleanOperatorKind>, bool> ground(BinaryOperatorView<LiftedTag, BooleanOperatorKind> element,
                                                                                           WriteContext<Builder, Repository>& context);

extern template std::pair<PredicateRowView<StaticTag>, bool>
ground(TermListView terms, PredicateView<StaticTag> predicate, WriteContext<Builder, Repository>& context);
extern template std::pair<PredicateRowView<FluentTag>, bool>
ground(TermListView terms, PredicateView<FluentTag> predicate, WriteContext<Builder, Repository>& context);
extern template std::pair<PredicateRowView<DerivedTag>, bool>
ground(TermListView terms, PredicateView<DerivedTag> predicate, WriteContext<Builder, Repository>& context);

extern template std::pair<AtomView<GroundTag, StaticTag>, bool> ground(AtomView<LiftedTag, StaticTag> element, WriteContext<Builder, Repository>& context);
extern template std::pair<AtomView<GroundTag, FluentTag>, bool> ground(AtomView<LiftedTag, FluentTag> element, WriteContext<Builder, Repository>& context);
extern template std::pair<AtomView<GroundTag, DerivedTag>, bool> ground(AtomView<LiftedTag, DerivedTag> element, WriteContext<Builder, Repository>& context);

extern template std::pair<LiteralView<GroundTag, StaticTag>, bool> ground(LiteralView<LiftedTag, StaticTag> element,
                                                                          WriteContext<Builder, Repository>& context);
extern template std::pair<LiteralView<GroundTag, FluentTag>, bool> ground(LiteralView<LiftedTag, FluentTag> element,
                                                                          WriteContext<Builder, Repository>& context);
extern template std::pair<LiteralView<GroundTag, DerivedTag>, bool> ground(LiteralView<LiftedTag, DerivedTag> element,
                                                                           WriteContext<Builder, Repository>& context);

extern template std::pair<NumericEffectView<GroundTag, FluentTag>, bool> ground(NumericEffectView<LiftedTag, FluentTag> element,
                                                                                WriteContext<Builder, Repository>& context);

extern template std::pair<PredicateRowView<StaticTag>, bool> ground_binding(AtomView<LiftedTag, StaticTag> element, WriteContext<Builder, Repository>& context);
extern template std::pair<PredicateRowView<FluentTag>, bool> ground_binding(AtomView<LiftedTag, FluentTag> element, WriteContext<Builder, Repository>& context);
extern template std::pair<PredicateRowView<DerivedTag>, bool> ground_binding(AtomView<LiftedTag, DerivedTag> element,
                                                                             WriteContext<Builder, Repository>& context);

extern template std::pair<FunctionRowView<StaticTag>, bool> ground_binding(FunctionTermView<LiftedTag, StaticTag> element,
                                                                           WriteContext<Builder, Repository>& context);
extern template std::pair<FunctionRowView<FluentTag>, bool> ground_binding(FunctionTermView<LiftedTag, FluentTag> element,
                                                                           WriteContext<Builder, Repository>& context);

extern template std::optional<FunctionRowView<StaticTag>> try_ground_binding(FunctionTermView<LiftedTag, StaticTag> element,
                                                                             WriteContext<Builder, Repository>& context);
extern template std::optional<FunctionRowView<FluentTag>> try_ground_binding(FunctionTermView<LiftedTag, FluentTag> element,
                                                                             WriteContext<Builder, Repository>& context);

extern template std::optional<PredicateRowView<StaticTag>> try_ground_binding(AtomView<LiftedTag, StaticTag> element,
                                                                              WriteContext<Builder, Repository>& context);
extern template std::optional<PredicateRowView<FluentTag>> try_ground_binding(AtomView<LiftedTag, FluentTag> element,
                                                                              WriteContext<Builder, Repository>& context);
extern template std::optional<PredicateRowView<DerivedTag>> try_ground_binding(AtomView<LiftedTag, DerivedTag> element,
                                                                               WriteContext<Builder, Repository>& context);

}

#endif

#endif
