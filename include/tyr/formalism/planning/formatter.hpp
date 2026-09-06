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
std::string to_string(const PlanningTask& value);
std::string to_string(const PlanningFDRTask& value);

}  // namespace tyr::formalism::planning

namespace fmt
{

template<>
struct formatter<tyr::formalism::planning::PlanningDomain, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::formalism::planning::PlanningDomain& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", tyr::formalism::planning::to_string(value));
    }
};

template<>
struct formatter<tyr::formalism::planning::PlanningTask, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::formalism::planning::PlanningTask& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", tyr::formalism::planning::to_string(value));
    }
};

template<>
struct formatter<tyr::formalism::planning::PlanningFDRTask, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::formalism::planning::PlanningFDRTask& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", tyr::formalism::planning::to_string(value));
    }
};

}  // namespace fmt

namespace tyr::formalism::planning
{

std::string to_string(const ygg::Data<UnaryOperator<LiftedTag>>& value);
std::string to_string(const ygg::Data<UnaryOperator<GroundTag>>& value);
std::string to_string(const ygg::Data<BinaryOperator<LiftedTag, ArithmeticOperatorKind>>& value);
std::string to_string(const ygg::Data<BinaryOperator<LiftedTag, BooleanOperatorKind>>& value);
std::string to_string(const ygg::Data<BinaryOperator<GroundTag, ArithmeticOperatorKind>>& value);
std::string to_string(const ygg::Data<BinaryOperator<GroundTag, BooleanOperatorKind>>& value);
std::string to_string(const ygg::Data<MultiOperator<LiftedTag>>& value);
std::string to_string(const ygg::Data<MultiOperator<GroundTag>>& value);
std::string to_string(const ygg::Data<ArithmeticOperator<LiftedTag>>& value);
std::string to_string(const ygg::Data<ArithmeticOperator<GroundTag>>& value);
std::string to_string(const ygg::Data<BooleanOperator<LiftedTag>>& value);
std::string to_string(const ygg::Data<BooleanOperator<GroundTag>>& value);

std::string to_string(const ygg::Data<Atom<LiftedTag, StaticTag>>& value);
std::string to_string(const ygg::Data<Atom<LiftedTag, FluentTag>>& value);
std::string to_string(const ygg::Data<Atom<LiftedTag, DerivedTag>>& value);
std::string to_string(const ygg::Data<Literal<LiftedTag, StaticTag>>& value);
std::string to_string(const ygg::Data<Literal<LiftedTag, FluentTag>>& value);
std::string to_string(const ygg::Data<Literal<LiftedTag, DerivedTag>>& value);
std::string to_string(const ygg::Data<Atom<GroundTag, StaticTag>>& value);
std::string to_string(const ygg::Data<Atom<GroundTag, FluentTag>>& value);
std::string to_string(const ygg::Data<Atom<GroundTag, DerivedTag>>& value);
std::string to_string(const ygg::Data<Literal<GroundTag, StaticTag>>& value);
std::string to_string(const ygg::Data<Literal<GroundTag, FluentTag>>& value);
std::string to_string(const ygg::Data<Literal<GroundTag, DerivedTag>>& value);
std::string to_string(const ygg::Data<FunctionTerm<LiftedTag, StaticTag>>& value);
std::string to_string(const ygg::Data<FunctionTerm<LiftedTag, FluentTag>>& value);
std::string to_string(const ygg::Data<FunctionTerm<LiftedTag, AuxiliaryTag>>& value);
std::string to_string(const ygg::Data<FunctionTerm<GroundTag, StaticTag>>& value);
std::string to_string(const ygg::Data<FunctionTerm<GroundTag, FluentTag>>& value);
std::string to_string(const ygg::Data<FunctionTerm<GroundTag, AuxiliaryTag>>& value);
std::string to_string(const ygg::Data<FunctionTermValue<GroundTag, StaticTag>>& value);
std::string to_string(const ygg::Data<FunctionTermValue<GroundTag, FluentTag>>& value);
std::string to_string(const ygg::Data<FunctionTermValue<GroundTag, AuxiliaryTag>>& value);
std::string to_string(const ygg::Data<NumericEffect<LiftedTag, FluentTag>>& value);
std::string to_string(const ygg::Data<NumericEffect<LiftedTag, AuxiliaryTag>>& value);
std::string to_string(const ygg::Data<NumericEffect<GroundTag, FluentTag>>& value);
std::string to_string(const ygg::Data<NumericEffect<GroundTag, AuxiliaryTag>>& value);
std::string to_string(const ygg::Data<NumericEffectOperator<LiftedTag, FluentTag>>& value);
std::string to_string(const ygg::Data<NumericEffectOperator<LiftedTag, AuxiliaryTag>>& value);
std::string to_string(const ygg::Data<NumericEffectOperator<GroundTag, FluentTag>>& value);
std::string to_string(const ygg::Data<NumericEffectOperator<GroundTag, AuxiliaryTag>>& value);
std::string to_string(const ygg::Data<FDRVariable<FluentTag>>& value);
std::string to_string(const ygg::Data<FDRFact<FluentTag>>& value);

std::string to_string(const ygg::Data<FunctionExpression<LiftedTag>>& value);
std::string to_string(const ygg::Data<FunctionExpression<GroundTag>>& value);
std::string to_string(const ygg::Data<ConjunctiveCondition<LiftedTag>>& value);
std::string to_string(const ygg::Data<ConjunctiveCondition<GroundTag>>& value);
std::string to_string(const ygg::Data<ConditionalEffect<LiftedTag>>& value);
std::string to_string(const ygg::Data<ConditionalEffect<GroundTag>>& value);
std::string to_string(const ygg::Data<ConjunctiveEffect<LiftedTag>>& value);
std::string to_string(const ygg::Data<ConjunctiveEffect<GroundTag>>& value);
std::string to_string(const ygg::Data<Action<LiftedTag>>& value);
std::string to_string(const ygg::Data<Action<GroundTag>>& value);
std::string to_string(const ygg::Data<Axiom<LiftedTag>>& value);
std::string to_string(const ygg::Data<Axiom<GroundTag>>& value);
std::string to_string(const ygg::Data<Metric>& value);
std::string to_string(const ygg::Data<Task>& value);
std::string to_string(const ygg::Data<Domain>& value);
std::string to_string(const ygg::Data<FDRTask>& value);
std::string to_string(const ygg::Data<RelationBinding<Action<LiftedTag>>>& value);
std::string to_string(const ygg::Data<RelationBinding<Axiom<LiftedTag>>>& value);

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
std::string to_string(AxiomBindingView value);

std::string to_string(UnaryOperatorView<LiftedTag> value);
std::string to_string(UnaryOperatorView<GroundTag> value);
std::string to_string(BinaryOperatorView<LiftedTag, ArithmeticOperatorKind> value);
std::string to_string(BinaryOperatorView<LiftedTag, BooleanOperatorKind> value);
std::string to_string(BinaryOperatorView<GroundTag, ArithmeticOperatorKind> value);
std::string to_string(BinaryOperatorView<GroundTag, BooleanOperatorKind> value);
std::string to_string(MultiOperatorView<LiftedTag> value);
std::string to_string(MultiOperatorView<GroundTag> value);
std::string to_string(ArithmeticOperatorView<LiftedTag> value);
std::string to_string(ArithmeticOperatorView<GroundTag> value);
std::string to_string(BooleanOperatorView<LiftedTag> value);
std::string to_string(BooleanOperatorView<GroundTag> value);

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
std::string to_string(NumericEffectOperatorView<LiftedTag, FluentTag> value);
std::string to_string(NumericEffectOperatorView<LiftedTag, AuxiliaryTag> value);
std::string to_string(NumericEffectOperatorView<GroundTag, FluentTag> value);
std::string to_string(NumericEffectOperatorView<GroundTag, AuxiliaryTag> value);
std::string to_string(FDRVariableView<FluentTag> value);
std::string to_string(FDRFactView<FluentTag> value);

std::string to_string(FunctionExpressionView<LiftedTag> value);
std::string to_string(FunctionExpressionView<GroundTag> value);
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
std::string to_string(TaskView value);
std::string to_string(DomainView value);
std::string to_string(FDRTaskView value);

}  // namespace tyr::formalism::planning

namespace fmt
{

template<typename T>
struct formatter<ygg::View<ygg::Index<T>, tyr::formalism::planning::Repository>, char>
{
    using View = ygg::View<ygg::Index<T>, tyr::formalism::planning::Repository>;

    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const View& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", tyr::formalism::planning::to_string(value));
    }
};

template<typename T>
struct formatter<ygg::View<ygg::Data<T>, tyr::formalism::planning::Repository>, char>
{
    using View = ygg::View<ygg::Data<T>, tyr::formalism::planning::Repository>;

    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const View& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", tyr::formalism::planning::to_string(value));
    }
};

template<>
struct formatter<tyr::formalism::planning::FDRValue, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::formalism::planning::FDRValue& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", ygg::uint_t(value));
    }
};

template<>
struct formatter<tyr::formalism::OptimizationDirection, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(tyr::formalism::OptimizationDirection value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", tyr::formalism::to_string(value));
    }
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
    using Value = std::pair<View, tyr::formalism::planning::PlanFormatting>;

    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const Value& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", tyr::formalism::planning::to_string(value));
    }
};

}  // namespace fmt

#endif
