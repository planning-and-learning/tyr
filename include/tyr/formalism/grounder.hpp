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

#ifndef TYR_FORMALISM_GROUNDER_HPP_
#define TYR_FORMALISM_GROUNDER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/canonicalization.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/enums.hpp"
#include "tyr/formalism/formatter.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/grounding_context.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <concepts>
#include <optional>
#include <type_traits>
#include <urd/elements/flat.hpp>
#include <utility>
#include <variant>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/declarations.hpp>
#include <yggdrasil/formalism/row_view.hpp>
#include <yggdrasil/formalism/values.hpp>

/// `ground` and `try_ground` of the shared records (terms, atoms, literals, function terms, numeric expressions and
/// effects); planning's records in planning/grounder.hpp, Datalog's in datalog/grounder.hpp.
namespace tyr::formalism
{

/**
 * ground
 */

/// Substitutes the context's binding into the terms: each term's object, as the value a row holds (without interning a
/// row).
void ground(TermListView terms, const GroundingContext<Builder, Repository>& context, std::vector<ygg::Value>& values);

/// The row of the atom's predicate the context's binding grounds it to.
template<FactKind F>
std::pair<PredicateRowView<F>, bool> ground_binding(AtomView<LiftedTag, F> element, GroundingContext<Builder, Repository>& context);

/// The key of the atom's row, interned as ground_binding does, without its view (which finds the layer that owns the row).
template<FactKind F>
std::pair<PredicateRowKey<F>, bool> ground_row(AtomView<LiftedTag, F> element, GroundingContext<Builder, Repository>& context);

/// The row of the function term's function the context's binding grounds it to, without interning the ground function
/// term.
template<FactKind F>
std::pair<FunctionRowView<F>, bool> ground_binding(FunctionTermView<LiftedTag, F> element, GroundingContext<Builder, Repository>& context);

template<FactKind T>
std::pair<FunctionTermView<GroundTag, T>, bool> ground(FunctionTermView<LiftedTag, T> element, GroundingContext<Builder, Repository>& context);

FunctionExpressionView<GroundTag> ground(FunctionExpressionView<LiftedTag> element, GroundingContext<Builder, Repository>& context);

std::pair<UnaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool> ground(UnaryOperatorView<LiftedTag, ArithmeticOperatorKind> element,
                                                                             GroundingContext<Builder, Repository>& context);

template<OperatorKind O>
std::pair<BinaryOperatorView<GroundTag, O>, bool> ground(BinaryOperatorView<LiftedTag, O> element, GroundingContext<Builder, Repository>& context);

std::pair<MultiOperatorView<GroundTag, ArithmeticOperatorKind>, bool> ground(MultiOperatorView<LiftedTag, ArithmeticOperatorKind> element,
                                                                             GroundingContext<Builder, Repository>& context);

template<FactKind T>
std::pair<AtomView<GroundTag, T>, bool> ground(AtomView<LiftedTag, T> element, GroundingContext<Builder, Repository>& context);

template<FactKind T>
std::pair<LiteralView<GroundTag, T>, bool> ground(LiteralView<LiftedTag, T> element, GroundingContext<Builder, Repository>& context);

template<FactKind T>
std::pair<NumericEffectView<GroundTag, T>, bool> ground(NumericEffectView<LiftedTag, T> element, GroundingContext<Builder, Repository>& context);

/**
 * try_ground
 */

template<FactKind F>
std::optional<FunctionRowView<F>> try_ground_binding(FunctionTermView<LiftedTag, F> element, GroundingContext<Builder, Repository>& context);

template<FactKind F>
std::optional<PredicateRowView<F>> try_ground_binding(AtomView<LiftedTag, F> element, GroundingContext<Builder, Repository>& context);

template<FactKind T>
std::optional<FunctionTermView<GroundTag, T>> try_ground(FunctionTermView<LiftedTag, T> element, GroundingContext<Builder, Repository>& context);

template<FactKind T>
std::optional<AtomView<GroundTag, T>> try_ground(AtomView<LiftedTag, T> element, GroundingContext<Builder, Repository>& context);

/**
 * ground
 */
inline void ground(TermListView terms, const GroundingContext<Builder, Repository>& context, std::vector<ygg::Value>& values)
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
std::pair<PredicateRowKey<F>, bool> ground_row(AtomView<LiftedTag, F> element, GroundingContext<Builder, Repository>& context)
{
    auto binding = checkout<ygg::Row<Predicate<F>>>(context.builder());

    urd::store(binding->relation, element.get_predicate());
    ground(element.get_terms(), context, binding->values);

    const auto [row, created] = context.destination().insert(*binding, context.builder());
    return { row.index(), created };
}

template<FactKind F>
std::pair<PredicateRowView<F>, bool> ground_binding(AtomView<LiftedTag, F> element, GroundingContext<Builder, Repository>& context)
{
    const auto [key, created] = ground_row(element, context);
    return { context.destination().view(key), created };
}

template<FactKind F>
std::pair<FunctionRowView<F>, bool> ground_binding(FunctionTermView<LiftedTag, F> element, GroundingContext<Builder, Repository>& context)
{
    auto binding = checkout<ygg::Row<Function<F>>>(context.builder());

    urd::store(binding->relation, element.get_function());
    ground(element.get_terms(), context, binding->values);

    return context.destination().insert(*binding, context.builder());
}

template<FactKind T>
std::pair<FunctionTermView<GroundTag, T>, bool> ground(FunctionTermView<LiftedTag, T> element, GroundingContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto fterm = checkout<FunctionTerm<GroundTag, T>>(context.builder());

    // Fill data
    urd::store(fterm->row, ground_binding(element, context).first);

    // Canonicalize and Serialize
    return context.destination().insert(*fterm, context.builder());
}

inline FunctionExpressionView<GroundTag> ground(FunctionExpressionView<LiftedTag> element, GroundingContext<Builder, Repository>& context)
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
                                                                                    GroundingContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto unary = checkout<UnaryOperator<GroundTag, ArithmeticOperatorKind>>(context.builder());

    // Fill data
    unary->operation = element.get_operation();
    urd::store(unary->arg, ground(element.get_arg(), context));

    // Canonicalize and Serialize
    return context.destination().insert(*unary, context.builder());
}

template<OperatorKind O>
std::pair<BinaryOperatorView<GroundTag, O>, bool> ground(BinaryOperatorView<LiftedTag, O> element, GroundingContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto binary = checkout<BinaryOperator<GroundTag, O>>(context.builder());

    // Fill data
    binary->operation = element.get_operation();
    urd::store(binary->lhs, ground(element.get_lhs(), context));
    urd::store(binary->rhs, ground(element.get_rhs(), context));

    // Canonicalize and Serialize
    return context.destination().insert(*binary, context.builder());
}

inline std::pair<MultiOperatorView<GroundTag, ArithmeticOperatorKind>, bool> ground(MultiOperatorView<LiftedTag, ArithmeticOperatorKind> element,
                                                                                    GroundingContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto multi = checkout<MultiOperator<GroundTag, ArithmeticOperatorKind>>(context.builder());

    // Fill data
    multi->operation = element.get_operation();
    for (const auto arg : element.get_args())
        urd::append(multi->args, ground(arg, context));

    // Canonicalize and Serialize
    return context.destination().insert(*multi, context.builder());
}

template<FactKind T>
std::pair<AtomView<GroundTag, T>, bool> ground(AtomView<LiftedTag, T> element, GroundingContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto atom = checkout<Atom<GroundTag, T>>(context.builder());

    // Fill data
    atom->row = ground_row(element, context).first;

    // Canonicalize and Serialize
    return context.destination().insert(*atom, context.builder());
}

template<FactKind T>
std::pair<LiteralView<GroundTag, T>, bool> ground(LiteralView<LiftedTag, T> element, GroundingContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto ground_literal = checkout<Literal<GroundTag, T>>(context.builder());

    // Fill data
    ground_literal->polarity = element.get_polarity();
    urd::store(ground_literal->atom, ground(element.get_atom(), context).first);

    // Canonicalize and Serialize
    return context.destination().insert(*ground_literal, context.builder());
}

template<FactKind T>
std::pair<NumericEffectView<GroundTag, T>, bool> ground(NumericEffectView<LiftedTag, T> element, GroundingContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto numeric_effect = checkout<NumericEffect<GroundTag, T>>(context.builder());

    // Fill data
    numeric_effect->operation = element.get_operation();
    urd::store(numeric_effect->fterm, ground(element.get_fterm(), context).first);
    urd::store(numeric_effect->fexpr, ground(element.get_fexpr(), context));

    // Canonicalize and Serialize
    return context.destination().insert(*numeric_effect, context.builder());
}

/**
 * try_ground
 */
template<FactKind F>
std::optional<FunctionRowView<F>> try_ground_binding(FunctionTermView<LiftedTag, F> element, GroundingContext<Builder, Repository>& context)
{
    auto binding = checkout<ygg::Row<Function<F>>>(context.builder());

    urd::store(binding->relation, element.get_function());
    ground(element.get_terms(), context, binding->values);

    return context.destination().find(*binding, context.builder());
}

template<FactKind F>
std::optional<PredicateRowView<F>> try_ground_binding(AtomView<LiftedTag, F> element, GroundingContext<Builder, Repository>& context)
{
    auto binding = checkout<ygg::Row<Predicate<F>>>(context.builder());

    urd::store(binding->relation, element.get_predicate());
    ground(element.get_terms(), context, binding->values);

    return context.destination().find(*binding, context.builder());
}

template<FactKind T>
std::optional<FunctionTermView<GroundTag, T>> try_ground(FunctionTermView<LiftedTag, T> element, GroundingContext<Builder, Repository>& context)
{
    const auto binding_or_nullopt = try_ground_binding(element, context);
    if (!binding_or_nullopt.has_value())
        return std::nullopt;

    auto fterm = checkout<FunctionTerm<GroundTag, T>>(context.builder());

    fterm->row = binding_or_nullopt->index();

    return context.destination().find(*fterm, context.builder());
}

template<FactKind T>
std::optional<AtomView<GroundTag, T>> try_ground(AtomView<LiftedTag, T> element, GroundingContext<Builder, Repository>& context)
{
    const auto binding_or_nullopt = try_ground_binding(element, context);
    if (!binding_or_nullopt.has_value())
        return std::nullopt;

    auto atom = checkout<Atom<GroundTag, T>>(context.builder());

    atom->row = binding_or_nullopt->index();

    return context.destination().find(*atom, context.builder());
}

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism
{
extern template std::pair<PredicateRowView<StaticTag>, bool> ground_binding(AtomView<LiftedTag, StaticTag> element,
                                                                            GroundingContext<Builder, Repository>& context);
extern template std::pair<PredicateRowView<FluentTag>, bool> ground_binding(AtomView<LiftedTag, FluentTag> element,
                                                                            GroundingContext<Builder, Repository>& context);
extern template std::pair<PredicateRowView<DerivedTag>, bool> ground_binding(AtomView<LiftedTag, DerivedTag> element,
                                                                             GroundingContext<Builder, Repository>& context);
extern template std::pair<FunctionRowView<StaticTag>, bool> ground_binding(FunctionTermView<LiftedTag, StaticTag> element,
                                                                           GroundingContext<Builder, Repository>& context);
extern template std::pair<FunctionRowView<FluentTag>, bool> ground_binding(FunctionTermView<LiftedTag, FluentTag> element,
                                                                           GroundingContext<Builder, Repository>& context);
extern template std::pair<FunctionRowView<AuxiliaryTag>, bool> ground_binding(FunctionTermView<LiftedTag, AuxiliaryTag> element,
                                                                              GroundingContext<Builder, Repository>& context);
extern template std::optional<FunctionRowView<StaticTag>> try_ground_binding(FunctionTermView<LiftedTag, StaticTag> element,
                                                                             GroundingContext<Builder, Repository>& context);
extern template std::optional<FunctionRowView<FluentTag>> try_ground_binding(FunctionTermView<LiftedTag, FluentTag> element,
                                                                             GroundingContext<Builder, Repository>& context);
extern template std::optional<FunctionRowView<AuxiliaryTag>> try_ground_binding(FunctionTermView<LiftedTag, AuxiliaryTag> element,
                                                                                GroundingContext<Builder, Repository>& context);
extern template std::optional<PredicateRowView<StaticTag>> try_ground_binding(AtomView<LiftedTag, StaticTag> element,
                                                                              GroundingContext<Builder, Repository>& context);
extern template std::optional<PredicateRowView<FluentTag>> try_ground_binding(AtomView<LiftedTag, FluentTag> element,
                                                                              GroundingContext<Builder, Repository>& context);
extern template std::optional<PredicateRowView<DerivedTag>> try_ground_binding(AtomView<LiftedTag, DerivedTag> element,
                                                                               GroundingContext<Builder, Repository>& context);

extern template std::pair<FunctionTermView<GroundTag, StaticTag>, bool> ground(FunctionTermView<LiftedTag, StaticTag> element,
                                                                               GroundingContext<Builder, Repository>& context);
extern template std::pair<FunctionTermView<GroundTag, FluentTag>, bool> ground(FunctionTermView<LiftedTag, FluentTag> element,
                                                                               GroundingContext<Builder, Repository>& context);
extern template std::pair<FunctionTermView<GroundTag, AuxiliaryTag>, bool> ground(FunctionTermView<LiftedTag, AuxiliaryTag> element,
                                                                                  GroundingContext<Builder, Repository>& context);

extern template std::pair<BinaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool> ground(BinaryOperatorView<LiftedTag, ArithmeticOperatorKind> element,
                                                                                              GroundingContext<Builder, Repository>& context);
extern template std::pair<BinaryOperatorView<GroundTag, BooleanOperatorKind>, bool> ground(BinaryOperatorView<LiftedTag, BooleanOperatorKind> element,
                                                                                           GroundingContext<Builder, Repository>& context);

extern template std::pair<AtomView<GroundTag, StaticTag>, bool> ground(AtomView<LiftedTag, StaticTag> element,
                                                                       GroundingContext<Builder, Repository>& grounder_context);
extern template std::pair<AtomView<GroundTag, FluentTag>, bool> ground(AtomView<LiftedTag, FluentTag> element,
                                                                       GroundingContext<Builder, Repository>& grounder_context);
extern template std::pair<AtomView<GroundTag, DerivedTag>, bool> ground(AtomView<LiftedTag, DerivedTag> element,
                                                                        GroundingContext<Builder, Repository>& grounder_context);

extern template std::pair<LiteralView<GroundTag, StaticTag>, bool> ground(LiteralView<LiftedTag, StaticTag> element,
                                                                          GroundingContext<Builder, Repository>& context);
extern template std::pair<LiteralView<GroundTag, FluentTag>, bool> ground(LiteralView<LiftedTag, FluentTag> element,
                                                                          GroundingContext<Builder, Repository>& context);
extern template std::pair<LiteralView<GroundTag, DerivedTag>, bool> ground(LiteralView<LiftedTag, DerivedTag> element,
                                                                           GroundingContext<Builder, Repository>& context);

extern template std::pair<NumericEffectView<GroundTag, FluentTag>, bool> ground(NumericEffectView<LiftedTag, FluentTag> element,
                                                                                GroundingContext<Builder, Repository>& context);
extern template std::pair<NumericEffectView<GroundTag, AuxiliaryTag>, bool> ground(NumericEffectView<LiftedTag, AuxiliaryTag> element,
                                                                                   GroundingContext<Builder, Repository>& context);

extern template std::optional<FunctionTermView<GroundTag, StaticTag>> try_ground(FunctionTermView<LiftedTag, StaticTag> element,
                                                                                 GroundingContext<Builder, Repository>& context);
extern template std::optional<FunctionTermView<GroundTag, FluentTag>> try_ground(FunctionTermView<LiftedTag, FluentTag> element,
                                                                                 GroundingContext<Builder, Repository>& context);
extern template std::optional<FunctionTermView<GroundTag, AuxiliaryTag>> try_ground(FunctionTermView<LiftedTag, AuxiliaryTag> element,
                                                                                    GroundingContext<Builder, Repository>& context);

extern template std::optional<AtomView<GroundTag, StaticTag>> try_ground(AtomView<LiftedTag, StaticTag> element,
                                                                         GroundingContext<Builder, Repository>& context);
extern template std::optional<AtomView<GroundTag, FluentTag>> try_ground(AtomView<LiftedTag, FluentTag> element,
                                                                         GroundingContext<Builder, Repository>& context);
extern template std::optional<AtomView<GroundTag, DerivedTag>> try_ground(AtomView<LiftedTag, DerivedTag> element,
                                                                          GroundingContext<Builder, Repository>& context);
}

#endif

#endif
