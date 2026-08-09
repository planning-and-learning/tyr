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

std::string to_string(const ygg::Data<UnaryOperator<ygg::Data<FunctionExpression>>>& value);
std::string to_string(const ygg::Data<UnaryOperator<ygg::Data<GroundFunctionExpression>>>& value);
std::string to_string(const ygg::Data<BinaryOperator<ArithmeticOperatorKind, ygg::Data<FunctionExpression>>>& value);
std::string to_string(const ygg::Data<BinaryOperator<BooleanOperatorKind, ygg::Data<FunctionExpression>>>& value);
std::string to_string(const ygg::Data<BinaryOperator<ArithmeticOperatorKind, ygg::Data<GroundFunctionExpression>>>& value);
std::string to_string(const ygg::Data<BinaryOperator<BooleanOperatorKind, ygg::Data<GroundFunctionExpression>>>& value);
std::string to_string(const ygg::Data<MultiOperator<ygg::Data<FunctionExpression>>>& value);
std::string to_string(const ygg::Data<MultiOperator<ygg::Data<GroundFunctionExpression>>>& value);
std::string to_string(const ygg::Data<ArithmeticOperator<ygg::Data<FunctionExpression>>>& value);
std::string to_string(const ygg::Data<ArithmeticOperator<ygg::Data<GroundFunctionExpression>>>& value);
std::string to_string(const ygg::Data<BooleanOperator<ygg::Data<FunctionExpression>>>& value);
std::string to_string(const ygg::Data<BooleanOperator<ygg::Data<GroundFunctionExpression>>>& value);

std::string to_string(const ygg::Data<Atom<StaticTag>>& value);
std::string to_string(const ygg::Data<Atom<FluentTag>>& value);
std::string to_string(const ygg::Data<Literal<StaticTag>>& value);
std::string to_string(const ygg::Data<Literal<FluentTag>>& value);
std::string to_string(const ygg::Data<GroundAtom<StaticTag>>& value);
std::string to_string(const ygg::Data<GroundAtom<FluentTag>>& value);
std::string to_string(const ygg::Data<GroundLiteral<StaticTag>>& value);
std::string to_string(const ygg::Data<GroundLiteral<FluentTag>>& value);
std::string to_string(const ygg::Data<FunctionTerm<StaticTag>>& value);
std::string to_string(const ygg::Data<FunctionTerm<FluentTag>>& value);
std::string to_string(const ygg::Data<FunctionTerm<AuxiliaryTag>>& value);
std::string to_string(const ygg::Data<GroundFunctionTerm<StaticTag>>& value);
std::string to_string(const ygg::Data<GroundFunctionTerm<FluentTag>>& value);
std::string to_string(const ygg::Data<GroundFunctionTerm<AuxiliaryTag>>& value);
std::string to_string(const ygg::Data<GroundFunctionTermValue<StaticTag>>& value);
std::string to_string(const ygg::Data<GroundFunctionTermValue<FluentTag>>& value);
std::string to_string(const ygg::Data<GroundFunctionTermValue<AuxiliaryTag>>& value);
std::string to_string(const ygg::Data<NumericEffect<FluentTag>>& value);
std::string to_string(const ygg::Data<NumericEffectOperator<FluentTag>>& value);
std::string to_string(const ygg::Data<GroundNumericEffect<FluentTag>>& value);
std::string to_string(const ygg::Data<GroundNumericEffectOperator<FluentTag>>& value);

std::string to_string(const ygg::Data<FunctionExpression>& value);
std::string to_string(const ygg::Data<GroundFunctionExpression>& value);
std::string to_string(const ygg::Data<Metric>& value);
std::string to_string(const ygg::Data<ConjunctiveCondition>& value);
std::string to_string(const ygg::Data<ConjunctiveEffect>& value);
std::string to_string(const ygg::Data<ConditionalEffect>& value);
std::string to_string(const ygg::Data<Rule<PredicateTag>>& value);
std::string to_string(const ygg::Data<Rule<FunctionTag>>& value);
std::string to_string(const ygg::Data<GroundConjunctiveCondition>& value);
std::string to_string(const ygg::Data<GroundConjunctiveEffect>& value);
std::string to_string(const ygg::Data<GroundConditionalEffect>& value);
std::string to_string(const ygg::Data<GroundRule<PredicateTag>>& value);
std::string to_string(const ygg::Data<GroundRule<FunctionTag>>& value);
std::string to_string(const ygg::Data<Program>& value);
std::string to_string(const ygg::Data<GroundProgram>& value);
std::string to_string(const ygg::Data<RelationBinding<Rule<PredicateTag>>>& value);
std::string to_string(const ygg::Data<RelationBinding<Rule<FunctionTag>>>& value);

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

std::string to_string(AtomView<StaticTag> value);
std::string to_string(AtomView<FluentTag> value);
std::string to_string(LiteralView<StaticTag> value);
std::string to_string(LiteralView<FluentTag> value);
std::string to_string(GroundAtomView<StaticTag> value);
std::string to_string(GroundAtomView<FluentTag> value);
std::string to_string(GroundLiteralView<StaticTag> value);
std::string to_string(GroundLiteralView<FluentTag> value);
std::string to_string(FunctionTermView<StaticTag> value);
std::string to_string(FunctionTermView<FluentTag> value);
std::string to_string(FunctionTermView<AuxiliaryTag> value);
std::string to_string(GroundFunctionTermView<StaticTag> value);
std::string to_string(GroundFunctionTermView<FluentTag> value);
std::string to_string(GroundFunctionTermView<AuxiliaryTag> value);
std::string to_string(GroundFunctionTermValueView<StaticTag> value);
std::string to_string(GroundFunctionTermValueView<FluentTag> value);
std::string to_string(GroundFunctionTermValueView<AuxiliaryTag> value);
std::string to_string(NumericEffectView<FluentTag> value);
std::string to_string(NumericEffectOperatorView<FluentTag> value);
std::string to_string(GroundNumericEffectView<FluentTag> value);
std::string to_string(GroundNumericEffectOperatorView<FluentTag> value);

std::string to_string(FunctionExpressionView value);
std::string to_string(GroundFunctionExpressionView value);
std::string to_string(MetricView value);
std::string to_string(ConjunctiveConditionView value);
std::string to_string(ConjunctiveEffectView value);
std::string to_string(ConditionalEffectView value);
std::string to_string(RuleView<PredicateTag> value);
std::string to_string(RuleView<FunctionTag> value);
std::string to_string(GroundConjunctiveConditionView value);
std::string to_string(GroundConjunctiveEffectView value);
std::string to_string(GroundConditionalEffectView value);
std::string to_string(GroundRuleView<PredicateTag> value);
std::string to_string(GroundRuleView<FunctionTag> value);
std::string to_string(ProgramView<LiftedTag> value);
std::string to_string(ProgramView<GroundTag> value);

std::string to_string(const VariableDependencyGraph& value);

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

#endif
