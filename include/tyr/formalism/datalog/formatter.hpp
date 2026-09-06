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
    {
        return fmt::format_to(ctx.out(), "{}", tyr::formalism::datalog::to_string(value));
    }
};

}  // namespace fmt

namespace tyr::formalism::datalog
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
std::string to_string(const ygg::Data<Literal<LiftedTag, StaticTag>>& value);
std::string to_string(const ygg::Data<Literal<LiftedTag, FluentTag>>& value);
std::string to_string(const ygg::Data<Atom<GroundTag, StaticTag>>& value);
std::string to_string(const ygg::Data<Atom<GroundTag, FluentTag>>& value);
std::string to_string(const ygg::Data<Literal<GroundTag, StaticTag>>& value);
std::string to_string(const ygg::Data<Literal<GroundTag, FluentTag>>& value);
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
std::string to_string(const ygg::Data<NumericEffectOperator<LiftedTag, FluentTag>>& value);
std::string to_string(const ygg::Data<NumericEffect<GroundTag, FluentTag>>& value);
std::string to_string(const ygg::Data<NumericEffectOperator<GroundTag, FluentTag>>& value);

std::string to_string(const ygg::Data<FunctionExpression<LiftedTag>>& value);
std::string to_string(const ygg::Data<FunctionExpression<GroundTag>>& value);
std::string to_string(const ygg::Data<Metric>& value);
std::string to_string(const ygg::Data<ConjunctiveCondition<LiftedTag>>& value);
std::string to_string(const ygg::Data<ConjunctiveEffect<LiftedTag>>& value);
std::string to_string(const ygg::Data<ConditionalEffect<LiftedTag>>& value);
std::string to_string(const ygg::Data<Rule<LiftedTag, PredicateTag>>& value);
std::string to_string(const ygg::Data<Rule<LiftedTag, FunctionTag>>& value);
std::string to_string(const ygg::Data<ConjunctiveCondition<GroundTag>>& value);
std::string to_string(const ygg::Data<ConjunctiveEffect<GroundTag>>& value);
std::string to_string(const ygg::Data<ConditionalEffect<GroundTag>>& value);
std::string to_string(const ygg::Data<Rule<GroundTag, PredicateTag>>& value);
std::string to_string(const ygg::Data<Rule<GroundTag, FunctionTag>>& value);
std::string to_string(const ygg::Data<Program<LiftedTag>>& value);
std::string to_string(const ygg::Data<Program<GroundTag>>& value);
std::string to_string(const ygg::Data<RelationBinding<Rule<LiftedTag, PredicateTag>>>& value);
std::string to_string(const ygg::Data<RelationBinding<Rule<LiftedTag, FunctionTag>>>& value);

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
std::string to_string(NumericEffectOperatorView<LiftedTag, FluentTag> value);
std::string to_string(NumericEffectView<GroundTag, FluentTag> value);
std::string to_string(NumericEffectOperatorView<GroundTag, FluentTag> value);

std::string to_string(FunctionExpressionView<LiftedTag> value);
std::string to_string(FunctionExpressionView<GroundTag> value);
std::string to_string(MetricView value);
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

template<typename T>
struct formatter<ygg::View<ygg::Index<T>, tyr::formalism::datalog::Repository>, char>
{
    using View = ygg::View<ygg::Index<T>, tyr::formalism::datalog::Repository>;

    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const View& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", tyr::formalism::datalog::to_string(value));
    }
};

template<typename T>
struct formatter<ygg::View<ygg::Data<T>, tyr::formalism::datalog::Repository>, char>
{
    using View = ygg::View<ygg::Data<T>, tyr::formalism::datalog::Repository>;

    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const View& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", tyr::formalism::datalog::to_string(value));
    }
};

}  // namespace fmt

#endif
