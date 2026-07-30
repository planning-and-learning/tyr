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
#include "tyr/formalism/planning/planning_domain.hpp"
#include "tyr/formalism/planning/planning_fdr_task.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <fmt/core.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <ostream>
#include <sstream>
#include <yggdrasil/formatting/cista_formatters.hpp>
#include <yggdrasil/io/iostream.hpp>

namespace tyr::formalism::planning
{
struct PlanFormatting
{
};
}  // namespace tyr::formalism::planning

namespace fmt
{

template<>
struct formatter<tyr::formalism::planning::ConjunctiveEffectView, char>;

template<>
struct formatter<tyr::formalism::planning::GroundConjunctiveEffectView, char>;

template<>
struct formatter<tyr::formalism::planning::AxiomBindingView, char>;

template<tyr::formalism::FactKind T>
struct formatter<tyr::formalism::planning::FDRFactView<T>, char>;

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
struct formatter<tyr::formalism::planning::Minimize, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::Minimize&, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "minimize");
    }
};

template<>
struct formatter<tyr::formalism::planning::Maximize, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::Maximize&, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "maximize");
    }
};

template<tyr::formalism::OpKind Op, typename T>
struct formatter<ygg::Data<tyr::formalism::planning::UnaryOperator<Op, T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::UnaryOperator<Op, T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {})", Op {}, value.arg);
    }
};

template<tyr::formalism::OpKind Op, typename T>
struct formatter<ygg::Data<tyr::formalism::planning::BinaryOperator<Op, T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::BinaryOperator<Op, T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {} {})", Op {}, value.lhs, value.rhs);
    }
};

template<tyr::formalism::OpKind Op, typename T>
struct formatter<ygg::Data<tyr::formalism::planning::MultiOperator<Op, T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::MultiOperator<Op, T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {})", Op {}, fmt::join(ygg::to_strings(value.args), " "));
    }
};

template<typename T>
struct formatter<ygg::Data<tyr::formalism::planning::ArithmeticOperator<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::ArithmeticOperator<T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.value);
    }
};

template<typename T>
struct formatter<ygg::Data<tyr::formalism::planning::BooleanOperator<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::BooleanOperator<T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.value);
    }
};

template<tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::planning::Atom<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::Atom<T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {})", value.predicate, fmt::join(ygg::to_strings(value.terms), " "));
    }
};

template<tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::planning::Literal<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::Literal<T>>& value, FormatContext& ctx) const
    {
        if (value.polarity)
        {
            return fmt::format_to(ctx.out(), "{}", value.atom);
        }
        return fmt::format_to(ctx.out(), "(not {})", value.atom);
    }
};

template<tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::planning::GroundAtom<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::GroundAtom<T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({})", value.binding);
    }
};

template<tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::planning::GroundLiteral<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::GroundLiteral<T>>& value, FormatContext& ctx) const
    {
        if (value.polarity)
        {
            return fmt::format_to(ctx.out(), "{}", value.atom);
        }
        return fmt::format_to(ctx.out(), "(not {})", value.atom);
    }
};

template<tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::planning::FunctionTerm<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::FunctionTerm<T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {})", value.function, fmt::join(ygg::to_strings(value.terms), " "));
    }
};

template<tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::planning::GroundFunctionTerm<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::GroundFunctionTerm<T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({})", value.binding);
    }
};

template<tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::planning::GroundFunctionTermValue<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::GroundFunctionTermValue<T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "(= {} {})", value.fterm, value.value);
    }
};

template<tyr::formalism::NumericEffectOpKind Op, tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::planning::NumericEffect<Op, T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::NumericEffect<Op, T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {} {})", Op {}, value.fterm, value.fexpr);
    }
};

template<tyr::formalism::NumericEffectOpKind Op, tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::planning::GroundNumericEffect<Op, T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::GroundNumericEffect<Op, T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {} {})", Op {}, value.fterm, value.fexpr);
    }
};

template<tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::planning::NumericEffectOperator<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::NumericEffectOperator<T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.value);
    }
};

template<tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::planning::GroundNumericEffectOperator<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::GroundNumericEffectOperator<T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.value);
    }
};

template<tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::planning::FDRVariable<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::FDRVariable<T>>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "FDRVariable(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.index);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "atoms = ", value.atoms);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::planning::FDRFact<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::FDRFact<T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "<{},{}>", value.variable, value.value);
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::planning::FunctionExpression>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::FunctionExpression>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.value);
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::planning::GroundFunctionExpression>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::GroundFunctionExpression>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.value);
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::planning::ConjunctiveCondition>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::ConjunctiveCondition>& value, FormatContext& ctx) const
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
            fmt::print(os, "{}{}\n", "derived literals = ", value.derived_literals);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "numeric constraints = ", value.numeric_constraints);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::planning::GroundConjunctiveCondition>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::GroundConjunctiveCondition>& value, FormatContext& ctx) const
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
            fmt::print(os, "{}{}\n", "derived literals = ", value.derived_literals);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "positive facts = ", value.positive_facts);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "negative facts = ", value.negative_facts);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "numeric constraints = ", value.numeric_constraints);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::planning::ConditionalEffect>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::ConditionalEffect>& value, FormatContext& ctx) const
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
struct formatter<ygg::Data<tyr::formalism::planning::GroundConditionalEffect>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::GroundConditionalEffect>& value, FormatContext& ctx) const
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
struct formatter<ygg::Data<tyr::formalism::planning::ConjunctiveEffect>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::ConjunctiveEffect>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "ConjunctiveEffect(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.index);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent literals = ", value.literals);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent numeric effects = ", value.numeric_effects);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "auxiliary numeric effect = ", value.auxiliary_numeric_effect);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::planning::GroundConjunctiveEffect>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::GroundConjunctiveEffect>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "GroundConjunctiveEffect(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.index);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "add facts = ", value.add_facts);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "del facts = ", value.del_facts);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent numeric effects = ", value.numeric_effects);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "auxiliary numeric effect = ", value.auxiliary_numeric_effect);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::planning::Action>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::Action>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "Action(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.index);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "name = ", value.name);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "variables = ", value.variables);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "condition = ", value.condition);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "effects = ", value.effects);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::planning::GroundAction>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::GroundAction>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "GroundAction(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.index);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "binding = ", value.binding);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "condition = ", value.condition);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "effects = ", value.effects);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::planning::Axiom>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::Axiom>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "Axiom(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.index);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "variables = ", value.variables);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "body = ", value.body);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "head = ", value.head);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::planning::GroundAxiom>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::GroundAxiom>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "GroundAxiom(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.index);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "binding = ", value.binding);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "body = ", value.body);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "head = ", value.head);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::planning::Metric>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::Metric>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {})", value.objective, value.fexpr);
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::planning::Task>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::Task>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "Task(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.index);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "name = ", value.name);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "derived predicates = ", value.derived_predicates);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "objects = ", value.objects);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static atoms = ", value.static_atoms);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent atoms = ", value.fluent_atoms);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static numeric variables = ", value.static_fterm_values);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent numeric variables = ", value.fluent_fterm_values);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "auxiliary numeric variable = ", value.auxiliary_fterm_value);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "goal = ", value.goal);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "metric = ", value.metric);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "axioms = ", value.axioms);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::planning::Domain>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::Domain>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "Domain(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.index);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "name = ", value.name);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static predicates = ", value.static_predicates);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent predicates = ", value.fluent_predicates);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "derived predicates = ", value.derived_predicates);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static functions = ", value.static_functions);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent functions = ", value.fluent_functions);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "auxiliary function = ", value.auxiliary_function);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "constants = ", value.constants);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "actions = ", value.actions);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "axioms = ", value.axioms);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::planning::FDRTask>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::planning::FDRTask>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "FDRTask(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.index);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "name = ", value.name);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "derived predicates = ", value.derived_predicates);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "objects = ", value.objects);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static atoms = ", value.static_atoms);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent atoms = ", value.fluent_atoms);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "derived atoms = ", value.derived_atoms);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static numeric variables = ", value.static_fterm_values);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent numeric variables = ", value.fluent_fterm_values);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "auxiliary numeric variable = ", value.auxiliary_fterm_value);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "goal = ", value.goal);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "metric = ", value.metric);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "axioms = ", value.axioms);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent variables = ", value.fluent_variables);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent facts = ", value.fluent_facts);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "goal = ", value.goal);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "ground actions = ", value.ground_actions);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "ground axioms = ", value.ground_axioms);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<tyr::formalism::OpKind Op, typename T>
struct formatter<tyr::formalism::planning::UnaryOperatorView<Op, T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::UnaryOperatorView<Op, T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {})", Op {}, value.get_arg());
    }
};

template<tyr::formalism::OpKind Op, typename T>
struct formatter<tyr::formalism::planning::BinaryOperatorView<Op, T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::BinaryOperatorView<Op, T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {} {})", Op {}, value.get_lhs(), value.get_rhs());
    }
};

template<tyr::formalism::OpKind Op, typename T>
struct formatter<tyr::formalism::planning::MultiOperatorView<Op, T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::MultiOperatorView<Op, T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {})", Op {}, fmt::join(ygg::to_strings(value.get_args()), " "));
    }
};

template<typename T>
struct formatter<tyr::formalism::planning::ArithmeticOperatorView<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::ArithmeticOperatorView<T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_variant());
    }
};

template<typename T>
struct formatter<tyr::formalism::planning::BooleanOperatorView<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::BooleanOperatorView<T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_variant());
    }
};

template<tyr::formalism::FactKind T>
struct formatter<tyr::formalism::planning::AtomView<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::AtomView<T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {})", value.get_predicate().get_name(), fmt::join(ygg::to_strings(value.get_terms()), " "));
    }
};

template<tyr::formalism::FactKind T>
struct formatter<tyr::formalism::planning::LiteralView<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::LiteralView<T>& value, FormatContext& ctx) const
    {
        if (value.get_polarity())
        {
            return fmt::format_to(ctx.out(), "{}", value.get_atom());
        }
        return fmt::format_to(ctx.out(), "(not {})", value.get_atom());
    }
};

template<tyr::formalism::FactKind T>
struct formatter<tyr::formalism::planning::GroundAtomView<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::GroundAtomView<T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_row());
    }
};

template<tyr::formalism::FactKind T>
struct formatter<tyr::formalism::planning::GroundLiteralView<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::GroundLiteralView<T>& value, FormatContext& ctx) const
    {
        if (value.get_polarity())
        {
            return fmt::format_to(ctx.out(), "{}", value.get_atom());
        }
        return fmt::format_to(ctx.out(), "(not {})", value.get_atom());
    }
};

template<tyr::formalism::FactKind T>
struct formatter<tyr::formalism::planning::FunctionTermView<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::FunctionTermView<T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {})", value.get_function().get_name(), fmt::join(ygg::to_strings(value.get_terms()), " "));
    }
};

template<tyr::formalism::FactKind T>
struct formatter<tyr::formalism::planning::GroundFunctionTermView<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::GroundFunctionTermView<T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_row());
    }
};

template<tyr::formalism::FactKind T>
struct formatter<tyr::formalism::planning::GroundFunctionTermValueView<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::GroundFunctionTermValueView<T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "(= {} {})", value.get_fterm(), value.get_value());
    }
};

template<>
struct formatter<tyr::formalism::planning::FunctionExpressionView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::FunctionExpressionView& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_variant());
    }
};

template<>
struct formatter<tyr::formalism::planning::GroundFunctionExpressionView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::GroundFunctionExpressionView& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_variant());
    }
};

template<tyr::formalism::NumericEffectOpKind Op, tyr::formalism::FactKind T>
struct formatter<tyr::formalism::planning::NumericEffectView<Op, T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::NumericEffectView<Op, T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {} {})", Op {}, value.get_fterm(), value.get_fexpr());
    }
};

template<tyr::formalism::NumericEffectOpKind Op, tyr::formalism::FactKind T>
struct formatter<tyr::formalism::planning::GroundNumericEffectView<Op, T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::GroundNumericEffectView<Op, T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {} {})", Op {}, value.get_fterm(), value.get_fexpr());
    }
};

template<tyr::formalism::FactKind T>
struct formatter<tyr::formalism::planning::NumericEffectOperatorView<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::NumericEffectOperatorView<T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_variant());
    }
};

template<tyr::formalism::FactKind T>
struct formatter<tyr::formalism::planning::GroundNumericEffectOperatorView<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::GroundNumericEffectOperatorView<T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_variant());
    }
};

template<>
struct formatter<tyr::formalism::planning::ConjunctiveConditionView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::ConjunctiveConditionView& value, FormatContext& ctx) const
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
            fmt::print(os, "{}{}\n", "derived literals = ", value.template get_literals<tyr::formalism::DerivedTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "numeric constraints = ", value.get_numeric_constraints());
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<tyr::formalism::FactKind T>
struct formatter<tyr::formalism::planning::FDRFactView<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::FDRFactView<T>& value, FormatContext& ctx) const
    {
        const auto atom = value.get_atom();
        if (!atom.has_value())
        {
            return fmt::format_to(ctx.out(),
                                  "<{},{}>: (none-of {})",
                                  value.get_variable().get_index(),
                                  value.get_value(),
                                  fmt::join(ygg::to_strings(value.get_variable().get_atoms()), " "));
        }
        return fmt::format_to(ctx.out(), "<{},{}>: {}", value.get_variable().get_index(), value.get_value(), atom.value());
    }
};

template<>
struct formatter<tyr::formalism::planning::GroundConjunctiveConditionView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::GroundConjunctiveConditionView& value, FormatContext& ctx) const
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
            fmt::print(os, "{}{}\n", "derived literals = ", value.template get_literals<tyr::formalism::DerivedTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "positive facts = ", value.template get_facts<tyr::formalism::PositiveTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "negative facts = ", value.template get_facts<tyr::formalism::NegativeTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "numeric constraints = ", value.get_numeric_constraints());
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::formalism::planning::ConjunctiveEffectView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::ConjunctiveEffectView& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "ConjunctiveEffect(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent literals = ", value.get_literals());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent numeric effects = ", value.get_numeric_effects());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "auxiliary numeric effect = ", value.get_auxiliary_numeric_effect());
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::formalism::planning::GroundConjunctiveEffectView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::GroundConjunctiveEffectView& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "GroundConjunctiveEffect(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "add facts = ", value.template get_facts<tyr::formalism::PositiveTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "del facts = ", value.template get_facts<tyr::formalism::NegativeTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent numeric effects = ", value.get_numeric_effects());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "auxiliary numeric effect = ", value.get_auxiliary_numeric_effect());
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::formalism::planning::ConditionalEffectView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::ConditionalEffectView& value, FormatContext& ctx) const
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

template<>
struct formatter<tyr::formalism::planning::GroundConditionalEffectView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::GroundConditionalEffectView& value, FormatContext& ctx) const
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

template<>
struct formatter<tyr::formalism::planning::ActionView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::ActionView& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "Action(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "name = ", value.get_name());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "original name = ", value.get_original_name());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "variables = ", value.get_variables());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "condition = ", value.get_condition());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "effects = ", value.get_effects());
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::formalism::planning::AxiomBindingView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::AxiomBindingView& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", fmt::join(ygg::to_strings(value.get_objects()), " "));
    }
};

template<>
struct formatter<tyr::formalism::planning::GroundActionView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::GroundActionView& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "GroundAction(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "binding = ", value.get_row());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "action index = ", value.get_action().get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "condition = ", value.get_condition());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "effects = ", value.get_effects());
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<std::pair<tyr::formalism::planning::ActionBindingView, tyr::formalism::planning::PlanFormatting>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const std::pair<tyr::formalism::planning::ActionBindingView, tyr::formalism::planning::PlanFormatting>& value, FormatContext& ctx) const
    {
        auto out = fmt::format_to(ctx.out(), "({}", value.first.get_relation().get_original_name());
        for (size_t i = 0; i < value.first.get_relation().get_original_arity(); ++i)
        {
            out = fmt::format_to(out, " {}", value.first.get_objects()[i]);
        }
        return fmt::format_to(out, ")");
    }
};

template<>
struct formatter<std::pair<tyr::formalism::planning::GroundActionView, tyr::formalism::planning::PlanFormatting>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const std::pair<tyr::formalism::planning::GroundActionView, tyr::formalism::planning::PlanFormatting>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", std::make_pair(value.first.get_row(), value.second));
    }
};

template<>
struct formatter<tyr::formalism::planning::AxiomView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::AxiomView& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "Axiom(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "variables = ", value.get_variables());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "body = ", value.get_body());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "head = ", value.get_head());
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::formalism::planning::GroundAxiomView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::GroundAxiomView& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "GroundAxiom(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "binding = ", value.get_row());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "axiom index = ", value.get_axiom().get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "body = ", value.get_body());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "head = ", value.get_head());
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::formalism::planning::MetricView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::MetricView& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {})", value.get_objective(), value.get_fexpr());
    }
};

template<>
struct formatter<tyr::formalism::planning::TaskView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::TaskView& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "Task(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "name = ", value.get_name());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "derived predicates = ", value.get_derived_predicates());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "objects = ", value.get_objects());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static atoms = ", value.template get_atoms<tyr::formalism::StaticTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent atoms = ", value.template get_atoms<tyr::formalism::FluentTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static numeric variables = ", value.template get_fterm_values<tyr::formalism::StaticTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent numeric variables = ", value.template get_fterm_values<tyr::formalism::FluentTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "auxiliary numeric variable = ", value.get_auxiliary_fterm_value());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "goal = ", value.get_goal());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "metric = ", value.get_metric());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "axioms = ", value.get_axioms());
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::formalism::planning::DomainView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::DomainView& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "Domain(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "name = ", value.get_name());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static predicates = ", value.template get_predicates<tyr::formalism::StaticTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent predicates = ", value.template get_predicates<tyr::formalism::FluentTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "derived predicates = ", value.template get_predicates<tyr::formalism::DerivedTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static functions = ", value.template get_functions<tyr::formalism::StaticTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent functions = ", value.template get_functions<tyr::formalism::FluentTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "auxiliary function = ", value.get_auxiliary_function());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "constants = ", value.get_constants());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "actions = ", value.get_actions());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "axioms = ", value.get_axioms());
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<tyr::formalism::FactKind T>
struct formatter<tyr::formalism::planning::FDRVariableView<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::FDRVariableView<T>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "FDRVariable(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "atoms = ", value.get_atoms());
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::formalism::planning::FDRTaskView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::formalism::planning::FDRTaskView& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "FDRTask(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "name = ", value.get_name());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "derived predicates = ", value.get_derived_predicates());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "objects = ", value.get_objects());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static atoms = ", value.template get_atoms<tyr::formalism::StaticTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent atoms = ", value.template get_atoms<tyr::formalism::FluentTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "derived atoms = ", value.template get_atoms<tyr::formalism::DerivedTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static numeric variables = ", value.template get_fterm_values<tyr::formalism::StaticTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent numeric variables = ", value.template get_fterm_values<tyr::formalism::FluentTag>());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "auxiliary numeric variable = ", value.get_auxiliary_fterm_value());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "goal = ", value.get_goal());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "metric = ", value.get_metric());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "axioms = ", value.get_axioms());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent variables = ", value.get_fluent_variables());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent facts = ", value.get_fluent_facts());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "goal = ", value.get_goal());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "ground actions = ", value.get_ground_actions());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "ground axioms = ", value.get_ground_axioms());
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::formalism::planning::PlanningDomain, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::formalism::planning::PlanningDomain& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_domain());
    }
};

template<>
struct formatter<tyr::formalism::planning::PlanningTask, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::formalism::planning::PlanningTask& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_task());
    }
};

template<>
struct formatter<tyr::formalism::planning::PlanningFDRTask, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::formalism::planning::PlanningFDRTask& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_task());
    }
};

}

#endif
