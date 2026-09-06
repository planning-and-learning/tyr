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

std::string to_string(const ygg::Data<LiftedUnaryOperatorType>& value);
std::string to_string(const ygg::Data<GroundUnaryOperatorType>& value);
std::string to_string(const ygg::Data<LiftedBinaryOperatorType<ArithmeticOperatorKind>>& value);
std::string to_string(const ygg::Data<LiftedBinaryOperatorType<BooleanOperatorKind>>& value);
std::string to_string(const ygg::Data<GroundBinaryOperatorType<ArithmeticOperatorKind>>& value);
std::string to_string(const ygg::Data<GroundBinaryOperatorType<BooleanOperatorKind>>& value);
std::string to_string(const ygg::Data<LiftedMultiOperatorType>& value);
std::string to_string(const ygg::Data<GroundMultiOperatorType>& value);
std::string to_string(const ygg::Data<ArithmeticOperator<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>>& value);
std::string to_string(const ygg::Data<ArithmeticOperator<ygg::Data<FunctionExpression<::tyr::GroundTag>>>>& value);
std::string to_string(const ygg::Data<BooleanOperator<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>>& value);
std::string to_string(const ygg::Data<BooleanOperator<ygg::Data<FunctionExpression<::tyr::GroundTag>>>>& value);

std::string to_string(const ygg::Data<Atom<::tyr::LiftedTag, StaticTag>>& value);
std::string to_string(const ygg::Data<Atom<::tyr::LiftedTag, FluentTag>>& value);
std::string to_string(const ygg::Data<Atom<::tyr::LiftedTag, DerivedTag>>& value);
std::string to_string(const ygg::Data<Literal<::tyr::LiftedTag, StaticTag>>& value);
std::string to_string(const ygg::Data<Literal<::tyr::LiftedTag, FluentTag>>& value);
std::string to_string(const ygg::Data<Literal<::tyr::LiftedTag, DerivedTag>>& value);
std::string to_string(const ygg::Data<Atom<::tyr::GroundTag, StaticTag>>& value);
std::string to_string(const ygg::Data<Atom<::tyr::GroundTag, FluentTag>>& value);
std::string to_string(const ygg::Data<Atom<::tyr::GroundTag, DerivedTag>>& value);
std::string to_string(const ygg::Data<Literal<::tyr::GroundTag, StaticTag>>& value);
std::string to_string(const ygg::Data<Literal<::tyr::GroundTag, FluentTag>>& value);
std::string to_string(const ygg::Data<Literal<::tyr::GroundTag, DerivedTag>>& value);
std::string to_string(const ygg::Data<FunctionTerm<::tyr::LiftedTag, StaticTag>>& value);
std::string to_string(const ygg::Data<FunctionTerm<::tyr::LiftedTag, FluentTag>>& value);
std::string to_string(const ygg::Data<FunctionTerm<::tyr::LiftedTag, AuxiliaryTag>>& value);
std::string to_string(const ygg::Data<FunctionTerm<::tyr::GroundTag, StaticTag>>& value);
std::string to_string(const ygg::Data<FunctionTerm<::tyr::GroundTag, FluentTag>>& value);
std::string to_string(const ygg::Data<FunctionTerm<::tyr::GroundTag, AuxiliaryTag>>& value);
std::string to_string(const ygg::Data<FunctionTermValue<::tyr::GroundTag, StaticTag>>& value);
std::string to_string(const ygg::Data<FunctionTermValue<::tyr::GroundTag, FluentTag>>& value);
std::string to_string(const ygg::Data<FunctionTermValue<::tyr::GroundTag, AuxiliaryTag>>& value);
std::string to_string(const ygg::Data<NumericEffect<::tyr::LiftedTag, FluentTag>>& value);
std::string to_string(const ygg::Data<NumericEffect<::tyr::LiftedTag, AuxiliaryTag>>& value);
std::string to_string(const ygg::Data<NumericEffect<::tyr::GroundTag, FluentTag>>& value);
std::string to_string(const ygg::Data<NumericEffect<::tyr::GroundTag, AuxiliaryTag>>& value);
std::string to_string(const ygg::Data<NumericEffectOperator<::tyr::LiftedTag, FluentTag>>& value);
std::string to_string(const ygg::Data<NumericEffectOperator<::tyr::LiftedTag, AuxiliaryTag>>& value);
std::string to_string(const ygg::Data<NumericEffectOperator<::tyr::GroundTag, FluentTag>>& value);
std::string to_string(const ygg::Data<NumericEffectOperator<::tyr::GroundTag, AuxiliaryTag>>& value);
std::string to_string(const ygg::Data<FDRVariable<FluentTag>>& value);
std::string to_string(const ygg::Data<FDRFact<FluentTag>>& value);

std::string to_string(const ygg::Data<FunctionExpression<::tyr::LiftedTag>>& value);
std::string to_string(const ygg::Data<FunctionExpression<::tyr::GroundTag>>& value);
std::string to_string(const ygg::Data<ConjunctiveCondition<::tyr::LiftedTag>>& value);
std::string to_string(const ygg::Data<ConjunctiveCondition<::tyr::GroundTag>>& value);
std::string to_string(const ygg::Data<ConditionalEffect<::tyr::LiftedTag>>& value);
std::string to_string(const ygg::Data<ConditionalEffect<::tyr::GroundTag>>& value);
std::string to_string(const ygg::Data<ConjunctiveEffect<::tyr::LiftedTag>>& value);
std::string to_string(const ygg::Data<ConjunctiveEffect<::tyr::GroundTag>>& value);
std::string to_string(const ygg::Data<Action<::tyr::LiftedTag>>& value);
std::string to_string(const ygg::Data<Action<::tyr::GroundTag>>& value);
std::string to_string(const ygg::Data<Axiom<::tyr::LiftedTag>>& value);
std::string to_string(const ygg::Data<Axiom<::tyr::GroundTag>>& value);
std::string to_string(const ygg::Data<Metric>& value);
std::string to_string(const ygg::Data<Task>& value);
std::string to_string(const ygg::Data<Domain>& value);
std::string to_string(const ygg::Data<FDRTask>& value);
std::string to_string(const ygg::Data<RelationBinding<Action<::tyr::LiftedTag>>>& value);
std::string to_string(const ygg::Data<RelationBinding<Axiom<::tyr::LiftedTag>>>& value);

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

std::string to_string(LiftedUnaryOperatorView value);
std::string to_string(GroundUnaryOperatorView value);
std::string to_string(LiftedBinaryOperatorView<ArithmeticOperatorKind> value);
std::string to_string(LiftedBinaryOperatorView<BooleanOperatorKind> value);
std::string to_string(GroundBinaryOperatorView<ArithmeticOperatorKind> value);
std::string to_string(GroundBinaryOperatorView<BooleanOperatorKind> value);
std::string to_string(LiftedMultiOperatorView value);
std::string to_string(GroundMultiOperatorView value);
std::string to_string(LiftedArithmeticOperatorView value);
std::string to_string(GroundArithmeticOperatorView value);
std::string to_string(LiftedBooleanOperatorView value);
std::string to_string(GroundBooleanOperatorView value);

std::string to_string(AtomView<::tyr::LiftedTag, StaticTag> value);
std::string to_string(AtomView<::tyr::LiftedTag, FluentTag> value);
std::string to_string(AtomView<::tyr::LiftedTag, DerivedTag> value);
std::string to_string(LiteralView<::tyr::LiftedTag, StaticTag> value);
std::string to_string(LiteralView<::tyr::LiftedTag, FluentTag> value);
std::string to_string(LiteralView<::tyr::LiftedTag, DerivedTag> value);
std::string to_string(AtomView<::tyr::GroundTag, StaticTag> value);
std::string to_string(AtomView<::tyr::GroundTag, FluentTag> value);
std::string to_string(AtomView<::tyr::GroundTag, DerivedTag> value);
std::string to_string(LiteralView<::tyr::GroundTag, StaticTag> value);
std::string to_string(LiteralView<::tyr::GroundTag, FluentTag> value);
std::string to_string(LiteralView<::tyr::GroundTag, DerivedTag> value);
std::string to_string(FunctionTermView<::tyr::LiftedTag, StaticTag> value);
std::string to_string(FunctionTermView<::tyr::LiftedTag, FluentTag> value);
std::string to_string(FunctionTermView<::tyr::LiftedTag, AuxiliaryTag> value);
std::string to_string(FunctionTermView<::tyr::GroundTag, StaticTag> value);
std::string to_string(FunctionTermView<::tyr::GroundTag, FluentTag> value);
std::string to_string(FunctionTermView<::tyr::GroundTag, AuxiliaryTag> value);
std::string to_string(FunctionTermValueView<::tyr::GroundTag, StaticTag> value);
std::string to_string(FunctionTermValueView<::tyr::GroundTag, FluentTag> value);
std::string to_string(FunctionTermValueView<::tyr::GroundTag, AuxiliaryTag> value);
std::string to_string(NumericEffectView<::tyr::LiftedTag, FluentTag> value);
std::string to_string(NumericEffectView<::tyr::LiftedTag, AuxiliaryTag> value);
std::string to_string(NumericEffectView<::tyr::GroundTag, FluentTag> value);
std::string to_string(NumericEffectView<::tyr::GroundTag, AuxiliaryTag> value);
std::string to_string(NumericEffectOperatorView<::tyr::LiftedTag, FluentTag> value);
std::string to_string(NumericEffectOperatorView<::tyr::LiftedTag, AuxiliaryTag> value);
std::string to_string(NumericEffectOperatorView<::tyr::GroundTag, FluentTag> value);
std::string to_string(NumericEffectOperatorView<::tyr::GroundTag, AuxiliaryTag> value);
std::string to_string(FDRVariableView<FluentTag> value);
std::string to_string(FDRFactView<FluentTag> value);

std::string to_string(FunctionExpressionView<::tyr::LiftedTag> value);
std::string to_string(FunctionExpressionView<::tyr::GroundTag> value);
std::string to_string(ConjunctiveConditionView<::tyr::LiftedTag> value);
std::string to_string(ConjunctiveConditionView<::tyr::GroundTag> value);
std::string to_string(ConditionalEffectView<::tyr::LiftedTag> value);
std::string to_string(ConditionalEffectView<::tyr::GroundTag> value);
std::string to_string(ConjunctiveEffectView<::tyr::LiftedTag> value);
std::string to_string(ConjunctiveEffectView<::tyr::GroundTag> value);
std::string to_string(ActionView<::tyr::LiftedTag> value);
std::string to_string(ActionView<::tyr::GroundTag> value);
std::string to_string(AxiomView<::tyr::LiftedTag> value);
std::string to_string(AxiomView<::tyr::GroundTag> value);
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
std::string to_string(const std::pair<ActionView<::tyr::GroundTag>, PlanFormatting>& value);

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
