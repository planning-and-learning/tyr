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

#include "tyr/formalism/atom_data.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_data.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/conjunctive_condition_data.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/program_data.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/rule_body_data.hpp"
#include "tyr/formalism/datalog/rule_body_view.hpp"
#include "tyr/formalism/datalog/rule_data.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_data.hpp"
#include "tyr/formalism/function_term_data.hpp"
#include "tyr/formalism/function_term_value_data.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/literal_data.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_data.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_data.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_data.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_data.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/unary_operator_data.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_data.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <concepts>
#include <cstddef>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <string_view>
#include <urd/format.hpp>
#include <utility>
#include <variant>
#include <yggdrasil/formalism/formatter.hpp>
#include <yggdrasil/formalism/row_data.hpp>
#include <yggdrasil/formalism/row_view.hpp>
#include <yggdrasil/formatting/cista_formatters.hpp>

// What each record of the datalog formalism is in Datalog. Terms and arguments are separated by ", ", the literals of a
// body by ", " too; a nullary atom is its predicate alone. A lifted atom or function term writes a parameter as the
// variable its parameters name at that index.

namespace tf = tyr::formalism;
using tyr::GroundTag;
using tyr::LiftedTag;

namespace
{
/**
 * What each shared record is in Datalog, written by its view through repository C: the formatters of the datalog
 * repository's records write their children with these, not through fmt's formatters.
 */

template<typename Out, typename C>
Out write(Out out, const urd::index_view<tf::Object, C>& value);
template<typename Out, typename C>
Out write(Out out, const urd::index_view<tf::Variable, C>& value);
template<typename Out, tf::FactKind F, typename C>
Out write(Out out, const urd::index_view<tf::Predicate<F>, C>& value);
template<typename Out, tf::FactKind F, typename C>
Out write(Out out, const urd::index_view<tf::Function<F>, C>& value);
template<typename Out, tyr::TaskKind T, tf::FactKind F, typename C>
Out write(Out out, const urd::index_view<tf::Atom<T, F>, C>& value);
template<typename Out, tyr::TaskKind T, tf::FactKind F, typename C>
Out write(Out out, const urd::index_view<tf::Literal<T, F>, C>& value);
template<typename Out, tyr::TaskKind T, tf::FactKind F, typename C>
Out write(Out out, const urd::index_view<tf::FunctionTerm<T, F>, C>& value);
template<typename Out, tyr::TaskKind T, tf::FactKind F, typename C>
Out write(Out out, const urd::index_view<tf::FunctionTermValue<T, F>, C>& value);
template<typename Out, tyr::TaskKind T, tf::OperatorKind O, typename C>
Out write(Out out, const urd::index_view<tf::UnaryOperator<T, O>, C>& value);
template<typename Out, tyr::TaskKind T, tf::OperatorKind O, typename C>
Out write(Out out, const urd::index_view<tf::BinaryOperator<T, O>, C>& value);
template<typename Out, tyr::TaskKind T, tf::OperatorKind O, typename C>
Out write(Out out, const urd::index_view<tf::MultiOperator<T, O>, C>& value);
template<typename Out, tyr::TaskKind T, tf::FactKind F, typename C>
Out write(Out out, const urd::index_view<tf::NumericEffect<T, F>, C>& value);

/// A number of a function expression.
template<typename Out>
Out write(Out out, ygg::float_t value)
{ return fmt::format_to(out, "{}", value); }

/// A function expression or a term: the alternative it holds.
template<typename Out, typename... Alternatives>
Out write(Out out, const std::variant<Alternatives...>& value)
{
    return std::visit([&](const auto& alternative) { return write(out, alternative); }, value);
}

/// The arguments of a lifted atom or function term: a parameter as the variable its parameters name at its index.
template<typename Out, typename View>
Out write_lifted_arguments(Out out, const View& value, std::string_view name)
{
    if (value.terms().empty())
        return fmt::format_to(out, "{}", name);
    const auto variables = value.parameters().variables();
    out = fmt::format_to(out, "{}(", name);
    auto separator = std::string_view("");
    for (auto term : value.terms())
    {
        out = fmt::format_to(out, "{}", separator);
        if (const auto* parameter = std::get_if<tf::ParameterIndex>(&term))
            out = write(out, variables[parameter->value]);
        else
            out = write(out, std::get<0>(term));
        separator = ", ";
    }
    return fmt::format_to(out, ")");
}

template<typename Out, typename C>
Out write(Out out, const urd::index_view<tf::Object, C>& value)
{ return fmt::format_to(out, "{}", value.name()); }

template<typename Out, typename C>
Out write(Out out, const urd::index_view<tf::Variable, C>& value)
{ return fmt::format_to(out, "{}", value.name()); }

template<typename Out, tf::FactKind F, typename C>
Out write(Out out, const urd::index_view<tf::Predicate<F>, C>& value)
{ return fmt::format_to(out, "{}/{}", value.name(), value.arity()); }

template<typename Out, tf::FactKind F, typename C>
Out write(Out out, const urd::index_view<tf::Function<F>, C>& value)
{ return fmt::format_to(out, "{}/{}", value.name(), value.arity()); }

template<typename Out, tyr::TaskKind T, tf::FactKind F, typename C>
Out write(Out out, const urd::index_view<tf::Atom<T, F>, C>& value)
{
    if constexpr (std::same_as<T, GroundTag>)
        return fmt::format_to(out, "{}", value.row());
    else
        return write_lifted_arguments(out, value, value.predicate().name().str());
}

template<typename Out, tyr::TaskKind T, tf::FactKind F, typename C>
Out write(Out out, const urd::index_view<tf::Literal<T, F>, C>& value)
{
    if (!value.polarity())
        out = fmt::format_to(out, "not ");
    return write(out, value.atom());
}

template<typename Out, tyr::TaskKind T, tf::FactKind F, typename C>
Out write(Out out, const urd::index_view<tf::FunctionTerm<T, F>, C>& value)
{
    if constexpr (std::same_as<T, GroundTag>)
        return fmt::format_to(out, "{}", value.row());
    else
        return write_lifted_arguments(out, value, value.function().name().str());
}

template<typename Out, tyr::TaskKind T, tf::FactKind F, typename C>
Out write(Out out, const urd::index_view<tf::FunctionTermValue<T, F>, C>& value)
{
    out = write(out, value.fterm());
    return fmt::format_to(out, " = {}", value.value());
}

template<typename Out, tyr::TaskKind T, tf::OperatorKind O, typename C>
Out write(Out out, const urd::index_view<tf::UnaryOperator<T, O>, C>& value)
{
    out = fmt::format_to(out, "({}", value.operation());
    out = write(out, value.operand());
    return fmt::format_to(out, ")");
}

template<typename Out, tyr::TaskKind T, tf::OperatorKind O, typename C>
Out write(Out out, const urd::index_view<tf::BinaryOperator<T, O>, C>& value)
{
    // A comparison stands in a body as it is; an arithmetic expression is parenthesized, as it may be an argument.
    constexpr auto parenthesized = !std::same_as<O, tf::BooleanOperatorKind>;
    if constexpr (parenthesized)
        out = fmt::format_to(out, "(");
    out = write(out, value.left_operand());
    out = fmt::format_to(out, " {} ", value.operation());
    out = write(out, value.right_operand());
    if constexpr (parenthesized)
        out = fmt::format_to(out, ")");
    return out;
}

template<typename Out, tyr::TaskKind T, tf::OperatorKind O, typename C>
Out write(Out out, const urd::index_view<tf::MultiOperator<T, O>, C>& value)
{
    out = fmt::format_to(out, "(");
    auto first = true;
    for (const auto& arg : value.operands())
    {
        if (!first)
            out = fmt::format_to(out, " {} ", value.operation());
        out = write(out, arg);
        first = false;
    }
    return fmt::format_to(out, ")");
}

template<typename Out, tyr::TaskKind T, tf::FactKind F, typename C>
Out write(Out out, const urd::index_view<tf::NumericEffect<T, F>, C>& value)
{
    // Each operator as the assignment it is: assign :=, increase +=, decrease -=, scale-up *=, scale-down /=.
    auto assignment = std::string_view(":=");
    if (value.operation() == tf::NumericEffectOperatorKind::Increase)
        assignment = "+=";
    else if (value.operation() == tf::NumericEffectOperatorKind::Decrease)
        assignment = "-=";
    else if (value.operation() == tf::NumericEffectOperatorKind::ScaleUp)
        assignment = "*=";
    else if (value.operation() == tf::NumericEffectOperatorKind::ScaleDown)
        assignment = "/=";
    out = write(out, value.fterm());
    out = fmt::format_to(out, " {} ", assignment);
    return write(out, value.fexpr());
}

/// The elements of a range, separated by `separator`.
template<typename Out, typename Range>
Out write_joined(Out out, const Range& elements, std::string_view separator)
{
    auto first = true;
    for (const auto& element : elements)
    {
        if (!first)
            out = fmt::format_to(out, "{}", separator);
        out = write(out, element);
        first = false;
    }
    return out;
}

/// A rule's body, lifted or ground: its literals and constraints, separated by commas.
template<typename Out, typename Body>
Out write_rule_body(Out out, const Body& value)
{
    auto separator = std::string_view("");
    const auto write_element = [&](const auto& element)
    {
        out = fmt::format_to(out, "{}", separator);
        out = write(out, element);
        separator = ", ";
    };
    for (auto literal : value.static_literals())
        write_element(literal);
    for (auto literal : value.fluent_literals())
        write_element(literal);
    for (auto literal : value.derived_literals())
        write_element(literal);
    for (auto constraint : value.numeric_constraints())
        write_element(constraint);
    return out;
}
}

// Rules, their ground bodies and programs in Datalog syntax; the shared records they hold are written by the writers
// above (the shared records' own formatters print PDDL, src/formalism/formatter.cpp).

fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::RuleBody<GroundTag>, tf::Repository>>::format(const urd::index_view<tf::RuleBody<GroundTag>, tf::Repository>& value,
                                                                                 fmt::format_context& context) const
{ return write_rule_body(context.out(), value); }

template<tyr::TaskKind T>
fmt::format_context::iterator fmt::formatter<urd::index_view<tf::Rule<T>, tf::Repository>>::format(const urd::index_view<tf::Rule<T>, tf::Repository>& value,
                                                                                                   fmt::format_context& context) const
{
    const auto body = value.body();
    const auto has_body =
        !body.static_literals().empty() || !body.fluent_literals().empty() || !body.derived_literals().empty() || !body.numeric_constraints().empty();
    auto out = write_joined(context.out(), value.fluent_heads(), ", ");
    if (!value.fluent_heads().empty() && !value.derived_heads().empty())
        out = fmt::format_to(out, ", ");
    out = write_joined(out, value.derived_heads(), ", ");
    if (has_body)
    {
        out = fmt::format_to(out, " :- ");
        out = write_rule_body(out, body);
    }
    return fmt::format_to(out, ".  % cost {}", value.cost());
}

// A program, one statement per line: its predicates, functions and objects as comments, its facts and function values,
// its rules, and its goal as a query.
template<tyr::TaskKind T>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::Program<T>, tf::Repository>>::format(const urd::index_view<tf::Program<T>, tf::Repository>& value,
                                                                        fmt::format_context& context) const
{
    auto out = context.out();
    auto line = std::string_view("");
    const auto new_line = urd::new_line();
    const auto comment = [&](std::string_view what, const auto& elements)
    {
        if (elements.empty())
            return;
        out = fmt::format_to(out, "{}% {}: ", line, what);
        out = write_joined(out, elements, ", ");
        line = new_line;
    };
    comment("static predicates", value.static_predicates());
    comment("fluent predicates", value.fluent_predicates());
    comment("derived predicates", value.derived_predicates());
    comment("static functions", value.static_functions());
    comment("fluent functions", value.fluent_functions());
    comment("objects", value.objects());
    const auto statements = [&](const auto& elements)
    {
        for (auto element : elements)
        {
            out = fmt::format_to(out, "{}", line);
            out = write(out, element);
            out = fmt::format_to(out, ".");
            line = new_line;
        }
    };
    statements(value.static_atoms());
    statements(value.fluent_atoms());
    statements(value.derived_atoms());
    statements(value.static_fterm_values());
    statements(value.fluent_fterm_values());
    for (auto rule : value.rules())
    {
        out = fmt::format_to(out, "{}{}", line, rule);
        line = new_line;
    }
    if (const auto goal = value.goal(); goal.has_value())
    {
        out = fmt::format_to(out, "{}?- ", line);
        out = write_rule_body(out, goal.value());
        out = fmt::format_to(out, ".");
    }
    return out;
}

// Each family's formatter, once for each kind.
template struct fmt::formatter<urd::index_view<tf::Rule<LiftedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Rule<GroundTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Program<LiftedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Program<GroundTag>, tf::Repository>>;
