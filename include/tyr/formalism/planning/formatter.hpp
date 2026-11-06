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

#ifndef TYR_FORMALISM_PLANNING_FORMATTER_HPP_
#define TYR_FORMALISM_PLANNING_FORMATTER_HPP_

#include "tyr/formalism/formatter.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"

#include <fmt/format.h>
#include <string>
#include <utility>

namespace tyr::formalism::planning
{
struct PlanFormatting
{
};

std::string to_string(const PlanningDomain& value);
std::string to_string(const PlanningTask<LiftedTag>& value);
std::string to_string(const PlanningTask<GroundTag>& value);

}  // namespace tyr::formalism::planning

namespace fmt
{

template<>
struct formatter<tyr::formalism::planning::PlanningDomain, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::formalism::planning::PlanningDomain& value, FormatContext& ctx) const
    { return fmt::format_to(ctx.out(), "{}", tyr::formalism::planning::to_string(value)); }
};

template<tyr::TaskKind T>
struct formatter<tyr::formalism::planning::PlanningTask<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::formalism::planning::PlanningTask<T>& value, FormatContext& ctx) const
    { return fmt::format_to(ctx.out(), "{}", tyr::formalism::planning::to_string(value)); }
};

}  // namespace fmt

namespace tyr::formalism::planning
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
std::string to_string(const urd::data<Atom<LiftedTag, DerivedTag>>& value);
std::string to_string(const urd::data<Literal<LiftedTag, StaticTag>>& value);
std::string to_string(const urd::data<Literal<LiftedTag, FluentTag>>& value);
std::string to_string(const urd::data<Literal<LiftedTag, DerivedTag>>& value);
std::string to_string(const urd::data<Atom<GroundTag, StaticTag>>& value);
std::string to_string(const urd::data<Atom<GroundTag, FluentTag>>& value);
std::string to_string(const urd::data<Atom<GroundTag, DerivedTag>>& value);
std::string to_string(const urd::data<Literal<GroundTag, StaticTag>>& value);
std::string to_string(const urd::data<Literal<GroundTag, FluentTag>>& value);
std::string to_string(const urd::data<Literal<GroundTag, DerivedTag>>& value);
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
std::string to_string(const urd::data<NumericEffect<LiftedTag, AuxiliaryTag>>& value);
std::string to_string(const urd::data<NumericEffect<GroundTag, FluentTag>>& value);
std::string to_string(const urd::data<NumericEffect<GroundTag, AuxiliaryTag>>& value);
std::string to_string(const urd::data<FDRVariable<FluentTag>>& value);

std::string to_string(const urd::data<Invariant>& value);
std::string to_string(const urd::data<FDRFact<FluentTag>>& value);

std::string to_string(const urd::data<ConjunctiveCondition<LiftedTag>>& value);
std::string to_string(const urd::data<ConjunctiveCondition<GroundTag>>& value);
std::string to_string(const urd::data<ConditionalEffect<LiftedTag>>& value);
std::string to_string(const urd::data<ConditionalEffect<GroundTag>>& value);
std::string to_string(const urd::data<ConjunctiveEffect<LiftedTag>>& value);
std::string to_string(const urd::data<ConjunctiveEffect<GroundTag>>& value);
std::string to_string(const urd::data<Action<LiftedTag>>& value);
std::string to_string(const urd::data<Action<GroundTag>>& value);
std::string to_string(const urd::data<Axiom<LiftedTag>>& value);
std::string to_string(const urd::data<Axiom<GroundTag>>& value);
std::string to_string(const urd::data<Metric>& value);
std::string to_string(const urd::data<Task<LiftedTag>>& value);
std::string to_string(const urd::data<Domain>& value);
std::string to_string(const urd::data<Task<GroundTag>>& value);
std::string to_string(const urd::data<RelationBinding<Action<LiftedTag>>>& value);
std::string to_string(const urd::data<RelationBinding<Axiom<LiftedTag>>>& value);

std::string to_string(VariableView value);
std::string to_string(ObjectView value);
std::string to_string(TermView value);
std::string to_string(PredicateView<StaticTag> value);
std::string to_string(PredicateView<FluentTag> value);
std::string to_string(PredicateView<DerivedTag> value);
std::string to_string(FunctionView<StaticTag> value);
std::string to_string(FunctionView<FluentTag> value);
std::string to_string(FunctionView<AuxiliaryTag> value);
std::string to_string(PredicateBindingView<StaticTag> value);
std::string to_string(PredicateBindingView<FluentTag> value);
std::string to_string(PredicateBindingView<DerivedTag> value);
std::string to_string(FunctionBindingView<StaticTag> value);
std::string to_string(FunctionBindingView<FluentTag> value);
std::string to_string(FunctionBindingView<AuxiliaryTag> value);
std::string to_string(ActionBindingView value);
std::string to_string(ActionBindingDataView value);
std::string to_string(AxiomBindingView value);

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
std::string to_string(AtomView<LiftedTag, DerivedTag> value);
std::string to_string(LiteralView<LiftedTag, StaticTag> value);
std::string to_string(LiteralView<LiftedTag, FluentTag> value);
std::string to_string(LiteralView<LiftedTag, DerivedTag> value);
std::string to_string(AtomView<GroundTag, StaticTag> value);
std::string to_string(AtomView<GroundTag, FluentTag> value);
std::string to_string(AtomView<GroundTag, DerivedTag> value);
std::string to_string(LiteralView<GroundTag, StaticTag> value);
std::string to_string(LiteralView<GroundTag, FluentTag> value);
std::string to_string(LiteralView<GroundTag, DerivedTag> value);
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
std::string to_string(NumericEffectView<LiftedTag, AuxiliaryTag> value);
std::string to_string(NumericEffectView<GroundTag, FluentTag> value);
std::string to_string(NumericEffectView<GroundTag, AuxiliaryTag> value);
std::string to_string(FDRVariableView<FluentTag> value);

std::string to_string(InvariantView value);
std::string to_string(FDRFactView<FluentTag> value);
std::string to_string(urd::owned_view<FDRFact<FluentTag>, Repository> value);

std::string to_string(ConjunctiveConditionView<LiftedTag> value);
std::string to_string(ConjunctiveConditionView<GroundTag> value);
std::string to_string(ConditionalEffectView<LiftedTag> value);
std::string to_string(ConditionalEffectView<GroundTag> value);
std::string to_string(ConjunctiveEffectView<LiftedTag> value);
std::string to_string(ConjunctiveEffectView<GroundTag> value);
std::string to_string(ActionView<LiftedTag> value);
std::string to_string(ActionView<GroundTag> value);
std::string to_string(AxiomView<LiftedTag> value);
std::string to_string(AxiomView<GroundTag> value);
std::string to_string(MetricView value);
std::string to_string(TaskView<LiftedTag> value);
std::string to_string(DomainView value);
std::string to_string(TaskView<GroundTag> value);

}  // namespace tyr::formalism::planning

namespace fmt
{

/// Every view through the repository prints as its to_string: urd's (of an index or a held record), and binding views.
template<template<typename, typename, typename...> typename V, typename H, typename... Rest>
    requires requires(const V<H, tyr::formalism::planning::Repository, Rest...>& value) { tyr::formalism::planning::to_string(value); }
struct formatter<V<H, tyr::formalism::planning::Repository, Rest...>, char>
{
    using View = V<H, tyr::formalism::planning::Repository, Rest...>;

    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const View& value, FormatContext& ctx) const
    { return fmt::format_to(ctx.out(), "{}", tyr::formalism::planning::to_string(value)); }
};

template<>
struct formatter<tyr::formalism::planning::FDRValue, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::formalism::planning::FDRValue& value, FormatContext& ctx) const
    { return fmt::format_to(ctx.out(), "{}", value.value); }
};

template<>
struct formatter<tyr::formalism::OptimizationDirection, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(tyr::formalism::OptimizationDirection value, FormatContext& ctx) const
    { return fmt::format_to(ctx.out(), "{}", tyr::formalism::to_string(value)); }
};

}  // namespace fmt

namespace tyr::formalism::planning
{

std::string to_string(const std::pair<ActionBindingView, PlanFormatting>& value);
std::string to_string(const std::pair<ActionView<GroundTag>, PlanFormatting>& value);

}  // namespace tyr::formalism::planning

namespace fmt
{

template<typename View>
struct formatter<std::pair<View, tyr::formalism::planning::PlanFormatting>, char>
{
private:
    using Value = std::pair<View, tyr::formalism::planning::PlanFormatting>;

public:
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const Value& value, FormatContext& ctx) const
    { return fmt::format_to(ctx.out(), "{}", tyr::formalism::planning::to_string(value)); }
};

}  // namespace fmt

#endif
