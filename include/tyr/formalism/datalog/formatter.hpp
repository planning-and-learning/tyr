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

#include "tyr/formalism/datalog/datas.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/variable_dependency_graph.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/formatter.hpp"

#include <fmt/core.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <ostream>
#include <sstream>
#include <yggdrasil/formatting/cista_formatters.hpp>
#include <yggdrasil/io/iostream.hpp>

namespace fmt
{

template<tyr::formalism::OpKind Op, typename T>
struct formatter<ygg::Data<tyr::formalism::datalog::UnaryOperator<Op, T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::UnaryOperator<Op, T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {})", Op {}, value.arg);
    }
};

template<tyr::formalism::OpKind Op, typename T>
struct formatter<tyr::formalism::datalog::UnaryOperatorView<Op, T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::UnaryOperatorView<Op, T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {})", Op {}, value.get_arg());
    }
};

template<tyr::formalism::OpKind Op, typename T>
struct formatter<ygg::Data<tyr::formalism::datalog::BinaryOperator<Op, T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::BinaryOperator<Op, T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {} {})", Op {}, value.lhs, value.rhs);
    }
};

template<tyr::formalism::OpKind Op, typename T>
struct formatter<tyr::formalism::datalog::BinaryOperatorView<Op, T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::BinaryOperatorView<Op, T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {} {})", Op {}, value.get_lhs(), value.get_rhs());
    }
};

template<tyr::formalism::OpKind Op, typename T>
struct formatter<ygg::Data<tyr::formalism::datalog::MultiOperator<Op, T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::MultiOperator<Op, T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {})", Op {}, fmt::join(ygg::to_strings(value.args), " "));
    }
};

template<tyr::formalism::OpKind Op, typename T>
struct formatter<tyr::formalism::datalog::MultiOperatorView<Op, T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::MultiOperatorView<Op, T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {})", Op {}, fmt::join(ygg::to_strings(value.get_args()), " "));
    }
};

template<typename T>
struct formatter<ygg::Data<tyr::formalism::datalog::ArithmeticOperator<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::ArithmeticOperator<T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.value);
    }
};

template<typename T>
struct formatter<tyr::formalism::datalog::ArithmeticOperatorView<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::ArithmeticOperatorView<T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_variant());
    }
};

template<typename T>
struct formatter<ygg::Data<tyr::formalism::datalog::BooleanOperator<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::BooleanOperator<T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.value);
    }
};

template<typename T>
struct formatter<tyr::formalism::datalog::BooleanOperatorView<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::BooleanOperatorView<T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_variant());
    }
};

template<tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::datalog::Atom<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::Atom<T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {})", value.predicate, fmt::join(ygg::to_strings(value.terms), " "));
    }
};

template<tyr::formalism::FactKind T>
struct formatter<tyr::formalism::datalog::AtomView<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::AtomView<T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {})", value.get_predicate().get_name(), fmt::join(ygg::to_strings(value.get_terms()), " "));
    }
};

template<tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::datalog::Literal<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::Literal<T>>& value, FormatContext& ctx) const
    {
        if (value.polarity)
            return fmt::format_to(ctx.out(), "{}", value.atom);
        return fmt::format_to(ctx.out(), "(not {})", value.atom);
    }
};

template<tyr::formalism::FactKind T>
struct formatter<tyr::formalism::datalog::LiteralView<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::LiteralView<T>& value, FormatContext& ctx) const
    {
        if (value.get_polarity())
            return fmt::format_to(ctx.out(), "{}", value.get_atom());
        return fmt::format_to(ctx.out(), "(not {})", value.get_atom());
    }
};

template<tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::datalog::GroundAtom<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::GroundAtom<T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({})", value.binding);
    }
};

template<tyr::formalism::FactKind T>
struct formatter<tyr::formalism::datalog::GroundAtomView<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::GroundAtomView<T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_row());
    }
};

template<tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::datalog::GroundLiteral<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::GroundLiteral<T>>& value, FormatContext& ctx) const
    {
        if (value.polarity)
            return fmt::format_to(ctx.out(), "{}", value.atom);
        return fmt::format_to(ctx.out(), "(not {})", value.atom);
    }
};

template<tyr::formalism::FactKind T>
struct formatter<tyr::formalism::datalog::GroundLiteralView<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::GroundLiteralView<T>& value, FormatContext& ctx) const
    {
        if (value.get_polarity())
            return fmt::format_to(ctx.out(), "{}", value.get_atom());
        return fmt::format_to(ctx.out(), "(not {})", value.get_atom());
    }
};

template<tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::datalog::FunctionTerm<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::FunctionTerm<T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {})", value.function, fmt::join(ygg::to_strings(value.terms), " "));
    }
};

template<tyr::formalism::FactKind T>
struct formatter<tyr::formalism::datalog::FunctionTermView<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::FunctionTermView<T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {})", value.get_function().get_name(), fmt::join(ygg::to_strings(value.get_terms()), " "));
    }
};

template<tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::datalog::GroundFunctionTerm<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::GroundFunctionTerm<T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({})", value.binding);
    }
};

template<tyr::formalism::FactKind T>
struct formatter<tyr::formalism::datalog::GroundFunctionTermView<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::GroundFunctionTermView<T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_row());
    }
};

template<tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::datalog::GroundFunctionTermValue<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::GroundFunctionTermValue<T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "(= {} {})", value.fterm, value.value);
    }
};

template<tyr::formalism::FactKind T>
struct formatter<tyr::formalism::datalog::GroundFunctionTermValueView<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::GroundFunctionTermValueView<T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "(= {} {})", value.get_fterm(), value.get_value());
    }
};

template<tyr::formalism::NumericEffectOpKind Op, tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::datalog::NumericEffect<Op, T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::NumericEffect<Op, T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {} {})", Op::kind, value.fterm, value.fexpr);
    }
};

template<tyr::formalism::NumericEffectOpKind Op, tyr::formalism::FactKind T>
struct formatter<tyr::formalism::datalog::NumericEffectView<Op, T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::NumericEffectView<Op, T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {} {})", Op::kind, value.get_fterm(), value.get_fexpr());
    }
};

template<tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::datalog::NumericEffectOperator<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::NumericEffectOperator<T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.value);
    }
};

template<tyr::formalism::FactKind T>
struct formatter<tyr::formalism::datalog::NumericEffectOperatorView<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::NumericEffectOperatorView<T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_variant());
    }
};

template<tyr::formalism::NumericEffectOpKind Op, tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::datalog::GroundNumericEffect<Op, T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::GroundNumericEffect<Op, T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {} {})", Op::kind, value.fterm, value.fexpr);
    }
};

template<tyr::formalism::NumericEffectOpKind Op, tyr::formalism::FactKind T>
struct formatter<tyr::formalism::datalog::GroundNumericEffectView<Op, T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::GroundNumericEffectView<Op, T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {} {})", Op::kind, value.get_fterm(), value.get_fexpr());
    }
};

template<tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::datalog::GroundNumericEffectOperator<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::GroundNumericEffectOperator<T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.value);
    }
};

template<tyr::formalism::FactKind T>
struct formatter<tyr::formalism::datalog::GroundNumericEffectOperatorView<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::GroundNumericEffectOperatorView<T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_variant());
    }
};

template<>
struct formatter<tyr::formalism::datalog::VariableDependencyGraph, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::VariableDependencyGraph& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "graph {\n";
        const auto k = value.k();
        for (ygg::uint_t i = 0; i < k; ++i)
            fmt::print(os, "n{} [label=\"V{}\"];\n", i, i);
        for (ygg::uint_t i = 0; i < k; ++i)
            for (ygg::uint_t j = i + 1; j < k; ++j)
                if (value.binary().has_dependency(i, j))
                    fmt::print(os, "n{} -- n{};\n", i, j);
        os << "}\n";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::datalog::FunctionExpression>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::FunctionExpression>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.value);
    }
};

template<>
struct formatter<tyr::formalism::datalog::FunctionExpressionView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::FunctionExpressionView& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_variant());
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::datalog::GroundFunctionExpression>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::GroundFunctionExpression>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.value);
    }
};

template<>
struct formatter<tyr::formalism::datalog::GroundFunctionExpressionView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::GroundFunctionExpressionView& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_variant());
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::datalog::Metric>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::Metric>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.fexpr);
    }
};

template<>
struct formatter<tyr::formalism::datalog::MetricView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::MetricView& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_fexpr());
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::datalog::ConjunctiveCondition>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::ConjunctiveCondition>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "ConjunctiveCondition(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.index);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "variables = ", value.variables);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static literals = ", value.static_literals);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent literals = ", value.fluent_literals);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "numeric constraints = ", value.numeric_constraints);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::formalism::datalog::ConjunctiveConditionView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::ConjunctiveConditionView& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "ConjunctiveCondition(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "variables = ", value.get_variables());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static literals = ", value.template get_literals<tyr::formalism::StaticTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent literals = ", value.template get_literals<tyr::formalism::FluentTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "numeric constraints = ", value.get_numeric_constraints());
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::datalog::ConjunctiveEffect>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::ConjunctiveEffect>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "ConjunctiveEffect(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.index);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "numeric effects = ", value.numeric_effects);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::formalism::datalog::ConjunctiveEffectView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::ConjunctiveEffectView& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "ConjunctiveEffect(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "numeric effects = ", value.get_numeric_effects());
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::datalog::ConditionalEffect>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::ConditionalEffect>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "ConditionalEffect(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.index);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "variables = ", value.variables);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "condition = ", value.condition);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "effect = ", value.effect);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::formalism::datalog::ConditionalEffectView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::ConditionalEffectView& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "ConditionalEffect(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "variables = ", value.get_variables());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "condition = ", value.get_condition());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "effect = ", value.get_effect());
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<tyr::formalism::RelationKind R>
struct formatter<ygg::Data<tyr::formalism::datalog::Rule<R>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::Rule<R>>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "Rule(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.index);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "variables = ", value.variables);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "head = ", value.head);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "body = ", value.body);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "metric effects = ", value.metric_effects);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<tyr::formalism::RelationKind R>
struct formatter<tyr::formalism::datalog::RuleView<R>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::RuleView<R>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "Rule(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "variables = ", value.get_variables());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "head = ", value.get_head());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "body = ", value.get_body());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "metric effects = ", value.get_metric_effects());
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::datalog::GroundConjunctiveCondition>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::GroundConjunctiveCondition>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "GroundConjunctiveCondition(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.index);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static literals = ", value.static_literals);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent literals = ", value.fluent_literals);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "numeric constraints = ", value.numeric_constraints);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::formalism::datalog::GroundConjunctiveConditionView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::GroundConjunctiveConditionView& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "GroundConjunctiveCondition(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static literals = ", value.template get_literals<tyr::formalism::StaticTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent literals = ", value.template get_literals<tyr::formalism::FluentTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "numeric constraints = ", value.get_numeric_constraints());
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::datalog::GroundConjunctiveEffect>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::GroundConjunctiveEffect>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "GroundConjunctiveEffect(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.index);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "numeric effects = ", value.numeric_effects);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::formalism::datalog::GroundConjunctiveEffectView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::GroundConjunctiveEffectView& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "GroundConjunctiveEffect(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "numeric effects = ", value.get_numeric_effects());
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::datalog::GroundConditionalEffect>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::GroundConditionalEffect>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "GroundConditionalEffect(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.index);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "condition = ", value.condition);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "effect = ", value.effect);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::formalism::datalog::GroundConditionalEffectView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::GroundConditionalEffectView& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "GroundConditionalEffect(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "condition = ", value.get_condition());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "effect = ", value.get_effect());
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<tyr::formalism::RelationKind R>
struct formatter<ygg::Data<tyr::formalism::datalog::GroundRule<R>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::GroundRule<R>>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "GroundRule(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.index);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "head = ", value.head);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "body = ", value.body);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "metric effects = ", value.metric_effects);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<tyr::formalism::RelationKind R>
struct formatter<tyr::formalism::datalog::GroundRuleView<R>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::GroundRuleView<R>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "GroundRule(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "head = ", value.get_head());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "body = ", value.get_body());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "metric effects = ", value.get_metric_effects());
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::datalog::Program>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::Program>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "Program(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.index);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static predicates =", value.static_predicates);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent predicates = ", value.fluent_predicates);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static functions = ", value.static_functions);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent functions = ", value.fluent_functions);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "objects = ", value.objects);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static atoms = ", value.static_atoms);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent atoms = ", value.fluent_atoms);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static fterms = ", value.static_fterm_values);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent fterms = ", value.fluent_fterm_values);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "goal = ", value.goal);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "rules = ", value.predicate_rules);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "numeric rules = ", value.function_rules);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::formalism::datalog::ProgramView<tyr::LiftedTag>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::ProgramView<tyr::LiftedTag>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "Program(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static predicates =", value.template get_predicates<tyr::formalism::StaticTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent predicates = ", value.template get_predicates<tyr::formalism::FluentTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static functions = ", value.template get_functions<tyr::formalism::StaticTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent functions = ", value.template get_functions<tyr::formalism::FluentTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "objects = ", value.get_objects());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static atoms = ", value.template get_atoms<tyr::formalism::StaticTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent atoms = ", value.template get_atoms<tyr::formalism::FluentTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static fterms = ", value.template get_fterm_values<tyr::formalism::StaticTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent fterms = ", value.template get_fterm_values<tyr::formalism::FluentTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "goal = ", value.get_goal());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "rules = ", value.template get_rules<tyr::formalism::PredicateTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "numeric rules = ", value.template get_rules<tyr::formalism::FunctionTag>());
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::datalog::GroundProgram>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::datalog::GroundProgram>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "GroundProgram(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.index);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static predicates =", value.static_predicates);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent predicates = ", value.fluent_predicates);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static functions = ", value.static_functions);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent functions = ", value.fluent_functions);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "objects = ", value.objects);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static atoms = ", value.static_atoms);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent atoms = ", value.fluent_atoms);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static fterms = ", value.static_fterm_values);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent fterms = ", value.fluent_fterm_values);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "goal = ", value.goal);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "ground rules = ", value.predicate_ground_rules);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "numeric ground rules = ", value.function_ground_rules);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::formalism::datalog::ProgramView<tyr::GroundTag>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::ProgramView<tyr::GroundTag>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "GroundProgram(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static predicates =", value.template get_predicates<tyr::formalism::StaticTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent predicates = ", value.template get_predicates<tyr::formalism::FluentTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static functions = ", value.template get_functions<tyr::formalism::StaticTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent functions = ", value.template get_functions<tyr::formalism::FluentTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "objects = ", value.get_objects());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static atoms = ", value.template get_atoms<tyr::formalism::StaticTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent atoms = ", value.template get_atoms<tyr::formalism::FluentTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static fterms = ", value.template get_fterm_values<tyr::formalism::StaticTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent fterms = ", value.template get_fterm_values<tyr::formalism::FluentTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "goal = ", value.get_goal());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "ground rules = ", value.template get_ground_rules<tyr::formalism::PredicateTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "numeric ground rules = ", value.template get_ground_rules<tyr::formalism::FunctionTag>());
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

}  // namespace fmt
#endif
