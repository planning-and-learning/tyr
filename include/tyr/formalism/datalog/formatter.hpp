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

#ifndef TYR_FORMALISM_DATALOG_FORMATTER_HPP_
#define TYR_FORMALISM_DATALOG_FORMATTER_HPP_

#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/formatter.hpp"

#include <fmt/format.h>
#include <string>

namespace tyr::formalism::datalog
{

std::string to_string(const VariableDependencyGraph& value);

}  // namespace tyr::formalism::datalog

namespace fmt
{

template<>
struct formatter<tyr::formalism::datalog::VariableDependencyGraph, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::VariableDependencyGraph& value, FormatContext& ctx) const
    { return fmt::format_to(ctx.out(), "{}", tyr::formalism::datalog::to_string(value)); }
};

}  // namespace fmt

namespace tyr::formalism::datalog
{

std::string to_string(const urd::data<UnaryOperator<LiftedTag, ArithmeticOperatorKind>>& value);
std::string to_string(const urd::data<UnaryOperator<GroundTag, ArithmeticOperatorKind>>& value);
std::string to_string(const urd::data<BinaryOperator<LiftedTag, ArithmeticOperatorKind>>& value);
std::string to_string(const urd::data<BinaryOperator<LiftedTag, BooleanOperatorKind>>& value);
std::string to_string(const urd::data<BinaryOperator<GroundTag, ArithmeticOperatorKind>>& value);
std::string to_string(const urd::data<BinaryOperator<GroundTag, BooleanOperatorKind>>& value);
std::string to_string(const urd::data<MultiOperator<LiftedTag, ArithmeticOperatorKind>>& value);
std::string to_string(const urd::data<MultiOperator<GroundTag, ArithmeticOperatorKind>>& value);

std::string to_string(const urd::data<Atom<LiftedTag, StaticTag>>& value);
std::string to_string(const urd::data<Atom<LiftedTag, FluentTag>>& value);
std::string to_string(const urd::data<Literal<LiftedTag, StaticTag>>& value);
std::string to_string(const urd::data<Literal<LiftedTag, FluentTag>>& value);
std::string to_string(const urd::data<Atom<GroundTag, StaticTag>>& value);
std::string to_string(const urd::data<Atom<GroundTag, FluentTag>>& value);
std::string to_string(const urd::data<Literal<GroundTag, StaticTag>>& value);
std::string to_string(const urd::data<Literal<GroundTag, FluentTag>>& value);
std::string to_string(const urd::data<FunctionTerm<LiftedTag, StaticTag>>& value);
std::string to_string(const urd::data<FunctionTerm<LiftedTag, FluentTag>>& value);
std::string to_string(const urd::data<FunctionTerm<LiftedTag, AuxiliaryTag>>& value);
std::string to_string(const urd::data<FunctionTerm<GroundTag, StaticTag>>& value);
std::string to_string(const urd::data<FunctionTerm<GroundTag, FluentTag>>& value);
std::string to_string(const urd::data<FunctionTerm<GroundTag, AuxiliaryTag>>& value);
std::string to_string(const urd::data<FunctionTermValue<GroundTag, StaticTag>>& value);
std::string to_string(const urd::data<FunctionTermValue<GroundTag, FluentTag>>& value);
std::string to_string(const urd::data<FunctionTermValue<GroundTag, AuxiliaryTag>>& value);
std::string to_string(const urd::data<NumericEffect<LiftedTag, FluentTag>>& value);
std::string to_string(const urd::data<NumericEffect<GroundTag, FluentTag>>& value);

std::string to_string(const urd::data<ConjunctiveCondition<LiftedTag>>& value);
std::string to_string(const urd::data<ConjunctiveEffect<LiftedTag>>& value);
std::string to_string(const urd::data<ConditionalEffect<LiftedTag>>& value);
std::string to_string(const urd::data<Rule<LiftedTag, PredicateTag>>& value);
std::string to_string(const urd::data<Rule<LiftedTag, FunctionTag>>& value);
std::string to_string(const urd::data<ConjunctiveCondition<GroundTag>>& value);
std::string to_string(const urd::data<ConjunctiveEffect<GroundTag>>& value);
std::string to_string(const urd::data<ConditionalEffect<GroundTag>>& value);
std::string to_string(const urd::data<Rule<GroundTag, PredicateTag>>& value);
std::string to_string(const urd::data<Rule<GroundTag, FunctionTag>>& value);
std::string to_string(const urd::data<Program<LiftedTag>>& value);
std::string to_string(const urd::data<Program<GroundTag>>& value);
std::string to_string(const urd::data<RelationBinding<Rule<LiftedTag, PredicateTag>>>& value);
std::string to_string(const urd::data<RelationBinding<Rule<LiftedTag, FunctionTag>>>& value);

std::string to_string(VariableView value);
std::string to_string(ObjectView value);
std::string to_string(TermView value);
std::string to_string(PredicateView<StaticTag> value);
std::string to_string(PredicateView<FluentTag> value);
std::string to_string(FunctionView<StaticTag> value);
std::string to_string(FunctionView<FluentTag> value);
std::string to_string(FunctionView<AuxiliaryTag> value);
std::string to_string(PredicateBindingView<StaticTag> value);
std::string to_string(PredicateBindingView<FluentTag> value);
std::string to_string(FunctionBindingView<StaticTag> value);
std::string to_string(FunctionBindingView<FluentTag> value);
std::string to_string(FunctionBindingView<AuxiliaryTag> value);
std::string to_string(RuleBindingView<PredicateTag> value);
std::string to_string(RuleBindingView<FunctionTag> value);

std::string to_string(UnaryOperatorView<LiftedTag, ArithmeticOperatorKind> value);
std::string to_string(UnaryOperatorView<GroundTag, ArithmeticOperatorKind> value);
std::string to_string(BinaryOperatorView<LiftedTag, ArithmeticOperatorKind> value);
std::string to_string(BinaryOperatorView<LiftedTag, BooleanOperatorKind> value);
std::string to_string(BinaryOperatorView<GroundTag, ArithmeticOperatorKind> value);
std::string to_string(BinaryOperatorView<GroundTag, BooleanOperatorKind> value);
std::string to_string(MultiOperatorView<LiftedTag, ArithmeticOperatorKind> value);
std::string to_string(MultiOperatorView<GroundTag, ArithmeticOperatorKind> value);

std::string to_string(AtomView<LiftedTag, StaticTag> value);
std::string to_string(AtomView<LiftedTag, FluentTag> value);
std::string to_string(LiteralView<LiftedTag, StaticTag> value);
std::string to_string(LiteralView<LiftedTag, FluentTag> value);
std::string to_string(AtomView<GroundTag, StaticTag> value);
std::string to_string(AtomView<GroundTag, FluentTag> value);
std::string to_string(LiteralView<GroundTag, StaticTag> value);
std::string to_string(LiteralView<GroundTag, FluentTag> value);
std::string to_string(FunctionTermView<LiftedTag, StaticTag> value);
std::string to_string(FunctionTermView<LiftedTag, FluentTag> value);
std::string to_string(FunctionTermView<LiftedTag, AuxiliaryTag> value);
std::string to_string(FunctionTermView<GroundTag, StaticTag> value);
std::string to_string(FunctionTermView<GroundTag, FluentTag> value);
std::string to_string(FunctionTermView<GroundTag, AuxiliaryTag> value);
std::string to_string(FunctionTermValueView<GroundTag, StaticTag> value);
std::string to_string(FunctionTermValueView<GroundTag, FluentTag> value);
std::string to_string(FunctionTermValueView<GroundTag, AuxiliaryTag> value);
std::string to_string(NumericEffectView<LiftedTag, FluentTag> value);
std::string to_string(NumericEffectView<GroundTag, FluentTag> value);

std::string to_string(ConjunctiveConditionView<LiftedTag> value);
std::string to_string(ConjunctiveEffectView<LiftedTag> value);
std::string to_string(ConditionalEffectView<LiftedTag> value);
std::string to_string(RuleView<LiftedTag, PredicateTag> value);
std::string to_string(RuleView<LiftedTag, FunctionTag> value);
std::string to_string(ConjunctiveConditionView<GroundTag> value);
std::string to_string(ConjunctiveEffectView<GroundTag> value);
std::string to_string(ConditionalEffectView<GroundTag> value);
std::string to_string(RuleView<GroundTag, PredicateTag> value);
std::string to_string(RuleView<GroundTag, FunctionTag> value);
std::string to_string(ProgramView<LiftedTag> value);
std::string to_string(ProgramView<GroundTag> value);

}  // namespace tyr::formalism::datalog

namespace fmt
{

/// Every view through the repository prints as its to_string: urd's (of an index or a held record), and binding views.
template<template<typename, typename, typename...> typename V, typename H, typename... Rest>
    requires requires(const V<H, tyr::formalism::datalog::Repository, Rest...>& value) { tyr::formalism::datalog::to_string(value); }
struct formatter<V<H, tyr::formalism::datalog::Repository, Rest...>, char>
{
    using View = V<H, tyr::formalism::datalog::Repository, Rest...>;

    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const View& value, FormatContext& ctx) const
    { return fmt::format_to(ctx.out(), "{}", tyr::formalism::datalog::to_string(value)); }
};

}  // namespace fmt

#endif
