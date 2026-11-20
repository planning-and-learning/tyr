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

#include "tyr/formalism/datalog/formatter.hpp"

#include "tyr/formalism/atom_data.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_data.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_data.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/program_data.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_data.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/datalog/variable_dependency_graph.hpp"
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
#include "tyr/formalism/unary_operator_data.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_data.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <concepts>
#include <cstddef>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <string_view>
#include <urd/dot.hpp>
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
namespace td = tyr::formalism::datalog;
using tyr::GroundTag;
using tyr::LiftedTag;

fmt::format_context::iterator fmt::formatter<urd::index_view<tf::Object, td::Repository>>::format(const urd::index_view<tf::Object, td::Repository>& value,
                                                                                                  fmt::format_context& context) const
{ return fmt::format_to(context.out(), "{}", value.get_name()); }

fmt::format_context::iterator fmt::formatter<urd::index_view<tf::Variable, td::Repository>>::format(const urd::index_view<tf::Variable, td::Repository>& value,
                                                                                                    fmt::format_context& context) const
{ return fmt::format_to(context.out(), "{}", value.get_name()); }

template<tf::FactKind F>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::Predicate<F>, td::Repository>>::format(const urd::index_view<tf::Predicate<F>, td::Repository>& value,
                                                                          fmt::format_context& context) const
{ return fmt::format_to(context.out(), "{}/{}", value.get_name(), value.get_arity()); }

template<tf::FactKind F>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::Function<F>, td::Repository>>::format(const urd::index_view<tf::Function<F>, td::Repository>& value,
                                                                         fmt::format_context& context) const
{ return fmt::format_to(context.out(), "{}/{}", value.get_name(), value.get_arity()); }

template<tyr::TaskKind T, tf::FactKind F>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::Atom<T, F>, td::Repository>>::format(const urd::index_view<tf::Atom<T, F>, td::Repository>& value,
                                                                        fmt::format_context& context) const
{
    if constexpr (std::same_as<T, GroundTag>)
        return fmt::format_to(context.out(), "{}", value.get_row());
    else
    {
        if (value.get_terms().empty())
            return fmt::format_to(context.out(), "{}", value.get_predicate().get_name());
        // A parameter is written as the variable its parameters name at its index.
        const auto variables = value.get_parameters().get_variables();
        auto out = fmt::format_to(context.out(), "{}(", value.get_predicate().get_name());
        auto separator = std::string_view("");
        for (auto term : value.get_terms())
        {
            if (const auto* parameter = std::get_if<tf::ParameterIndex>(&term))
                out = fmt::format_to(out, "{}{}", separator, variables[parameter->value]);
            else
                out = fmt::format_to(out, "{}{}", separator, term);
            separator = ", ";
        }
        return fmt::format_to(out, ")");
    }
}

template<tyr::TaskKind T, tf::FactKind F>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::Literal<T, F>, td::Repository>>::format(const urd::index_view<tf::Literal<T, F>, td::Repository>& value,
                                                                           fmt::format_context& context) const
{
    if (value.get_polarity())
        return fmt::format_to(context.out(), "{}", value.get_atom());
    return fmt::format_to(context.out(), "not {}", value.get_atom());
}

template<tyr::TaskKind T, tf::FactKind F>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::FunctionTerm<T, F>, td::Repository>>::format(const urd::index_view<tf::FunctionTerm<T, F>, td::Repository>& value,
                                                                                fmt::format_context& context) const
{
    if constexpr (std::same_as<T, GroundTag>)
        return fmt::format_to(context.out(), "{}", value.get_row());
    else
    {
        if (value.get_terms().empty())
            return fmt::format_to(context.out(), "{}", value.get_function().get_name());
        // A parameter is written as the variable its parameters name at its index.
        const auto variables = value.get_parameters().get_variables();
        auto out = fmt::format_to(context.out(), "{}(", value.get_function().get_name());
        auto separator = std::string_view("");
        for (auto term : value.get_terms())
        {
            if (const auto* parameter = std::get_if<tf::ParameterIndex>(&term))
                out = fmt::format_to(out, "{}{}", separator, variables[parameter->value]);
            else
                out = fmt::format_to(out, "{}{}", separator, term);
            separator = ", ";
        }
        return fmt::format_to(out, ")");
    }
}

template<tyr::TaskKind T, tf::FactKind F>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::FunctionTermValue<T, F>, td::Repository>>::format(const urd::index_view<tf::FunctionTermValue<T, F>, td::Repository>& value,
                                                                                     fmt::format_context& context) const
{ return fmt::format_to(context.out(), "{} = {}", value.get_fterm(), value.get_value()); }

template<tyr::TaskKind T, tf::OperatorKind O>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::UnaryOperator<T, O>, td::Repository>>::format(const urd::index_view<tf::UnaryOperator<T, O>, td::Repository>& value,
                                                                                 fmt::format_context& context) const
{ return fmt::format_to(context.out(), "({}{})", value.get_operation(), value.get_arg()); }

template<tyr::TaskKind T, tf::OperatorKind O>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::BinaryOperator<T, O>, td::Repository>>::format(const urd::index_view<tf::BinaryOperator<T, O>, td::Repository>& value,
                                                                                  fmt::format_context& context) const
{
    // A comparison stands in a body as it is; an arithmetic expression is parenthesized, as it may be an argument.
    if constexpr (std::same_as<O, tf::BooleanOperatorKind>)
        return fmt::format_to(context.out(), "{} {} {}", value.get_lhs(), value.get_operation(), value.get_rhs());
    else
        return fmt::format_to(context.out(), "({} {} {})", value.get_lhs(), value.get_operation(), value.get_rhs());
}

template<tyr::TaskKind T, tf::OperatorKind O>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::MultiOperator<T, O>, td::Repository>>::format(const urd::index_view<tf::MultiOperator<T, O>, td::Repository>& value,
                                                                                 fmt::format_context& context) const
{ return fmt::format_to(context.out(), "({})", fmt::join(value.get_args(), fmt::format(" {} ", value.get_operation()))); }

template<tyr::TaskKind T, tf::FactKind F>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::NumericEffect<T, F>, td::Repository>>::format(const urd::index_view<tf::NumericEffect<T, F>, td::Repository>& value,
                                                                                 fmt::format_context& context) const
{
    // Each operator as the assignment it is: assign :=, increase +=, decrease -=, scale-up *=, scale-down /=.
    auto assignment = std::string_view(":=");
    if (value.get_operation() == tf::NumericEffectOperatorKind::Increase)
        assignment = "+=";
    else if (value.get_operation() == tf::NumericEffectOperatorKind::Decrease)
        assignment = "-=";
    else if (value.get_operation() == tf::NumericEffectOperatorKind::ScaleUp)
        assignment = "*=";
    else if (value.get_operation() == tf::NumericEffectOperatorKind::ScaleDown)
        assignment = "/=";
    return fmt::format_to(context.out(), "{} {} {}", value.get_fterm(), assignment, value.get_fexpr());
}

template<tyr::TaskKind T>
fmt::format_context::iterator
fmt::formatter<urd::index_view<td::ConjunctiveCondition<T>, td::Repository>>::format(const urd::index_view<td::ConjunctiveCondition<T>, td::Repository>& value,
                                                                                     fmt::format_context& context) const
{
    auto out = context.out();
    auto separator = std::string_view("");
    const auto write = [&](const auto& element)
    {
        out = fmt::format_to(out, "{}{}", separator, element);
        separator = ", ";
    };
    for (auto literal : value.get_static_literals())
        write(literal);
    for (auto literal : value.get_fluent_literals())
        write(literal);
    for (auto literal : value.get_derived_literals())
        write(literal);
    for (auto constraint : value.get_numeric_constraints())
        write(constraint);
    return out;
}

template<tyr::TaskKind T>
fmt::format_context::iterator fmt::formatter<urd::index_view<td::Rule<T>, td::Repository>>::format(const urd::index_view<td::Rule<T>, td::Repository>& value,
                                                                                                   fmt::format_context& context) const
{
    const auto body = value.get_body();
    const auto has_body = !body.get_static_literals().empty() || !body.get_fluent_literals().empty() || !body.get_derived_literals().empty()
                          || !body.get_numeric_constraints().empty();
    auto out = fmt::format_to(context.out(), "{}", fmt::join(value.get_fluent_heads(), ", "));
    if (!value.get_fluent_heads().empty() && !value.get_derived_heads().empty())
        out = fmt::format_to(out, ", ");
    out = fmt::format_to(out, "{}", fmt::join(value.get_derived_heads(), ", "));
    if (has_body)
        out = fmt::format_to(out, " :- {}", body);
    return fmt::format_to(out, ".  % cost {}", value.get_cost());
}

// A program, one statement per line: its predicates, functions and objects as comments, its facts and function values,
// its rules, and its goal as a query.
template<tyr::TaskKind T>
fmt::format_context::iterator
fmt::formatter<urd::index_view<td::Program<T>, td::Repository>>::format(const urd::index_view<td::Program<T>, td::Repository>& value,
                                                                        fmt::format_context& context) const
{
    auto out = context.out();
    auto line = std::string_view("");
    const auto new_line = urd::new_line();
    const auto comment = [&](std::string_view what, const auto& elements)
    {
        if (elements.empty())
            return;
        out = fmt::format_to(out, "{}% {}: {}", line, what, fmt::join(elements, ", "));
        line = new_line;
    };
    comment("static predicates", value.get_static_predicates());
    comment("fluent predicates", value.get_fluent_predicates());
    comment("derived predicates", value.get_derived_predicates());
    comment("static functions", value.get_static_functions());
    comment("fluent functions", value.get_fluent_functions());
    comment("objects", value.get_objects());
    const auto statements = [&](const auto& elements)
    {
        for (auto element : elements)
        {
            out = fmt::format_to(out, "{}{}.", line, element);
            line = new_line;
        }
    };
    statements(value.get_static_atoms());
    statements(value.get_fluent_atoms());
    statements(value.get_derived_atoms());
    statements(value.get_static_fterm_values());
    statements(value.get_fluent_fterm_values());
    for (auto rule : value.get_rules())
    {
        out = fmt::format_to(out, "{}{}", line, rule);
        line = new_line;
    }
    if (const auto goal = value.get_goal(); goal.has_value())
        out = fmt::format_to(out, "{}?- {}.", line, goal.value());
    return out;
}

template<typename R>
fmt::format_context::iterator fmt::formatter<ygg::RowView<R, td::Repository>>::format(const ygg::RowView<R, td::Repository>& value,
                                                                                      fmt::format_context& context) const
{
    // A rule has no name: its binding is its values.
    if constexpr (std::same_as<R, td::Rule<LiftedTag>>)
        return fmt::format_to(context.out(), "{}", ygg::values_text<R>(value));
    else
    {
        auto out = fmt::format_to(context.out(), "{}", value.get_relation().get_name());
        if (value.get_values().empty())
            return out;
        out = fmt::format_to(out, "(");
        for (std::size_t position = 0; position < value.get_values().size(); ++position)
        {
            if (position != 0)
                out = fmt::format_to(out, ", ");
            std::visit(
                [&]<typename V>(V column)
                {
                    if constexpr (std::same_as<V, urd::index<tf::Object>>)
                        out = fmt::format_to(out, "{}", urd::resolve(column, value.context()));
                    else
                        out = fmt::format_to(out, "{}", column);
                },
                value.get(position));
        }
        return fmt::format_to(out, ")");
    }
}

/// One DOT graph per kind of binary dependency, named after it: the condition's parameters, adjacent where they occur
/// together in a literal (or constraint) of that kind.
fmt::format_context::iterator fmt::formatter<td::VariableDependencyGraph>::format(const td::VariableDependencyGraph& value, fmt::format_context& context) const
{
    const auto& binary = value.binary();
    const auto parameter = [](const auto& /*graph*/, tf::ParameterIndex v) { return v; };
    return fmt::format_to(context.out(),
                          "{}{}{}{}{}{}{}",
                          urd::to_dot(binary.static_positive_literal, parameter, urd::unlabeled, "static_positive_literal"),
                          urd::to_dot(binary.static_negative_literal, parameter, urd::unlabeled, "static_negative_literal"),
                          urd::to_dot(binary.fluent_positive_literal, parameter, urd::unlabeled, "fluent_positive_literal"),
                          urd::to_dot(binary.fluent_negative_literal, parameter, urd::unlabeled, "fluent_negative_literal"),
                          urd::to_dot(binary.derived_positive_literal, parameter, urd::unlabeled, "derived_positive_literal"),
                          urd::to_dot(binary.derived_negative_literal, parameter, urd::unlabeled, "derived_negative_literal"),
                          urd::to_dot(binary.numeric_constraint, parameter, urd::unlabeled, "numeric_constraint"));
}

// Each family's formatter, once for each record of the family the repository stores.
template struct fmt::formatter<urd::index_view<tf::Predicate<tf::StaticTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Predicate<tf::FluentTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Predicate<tf::DerivedTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Function<tf::StaticTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Function<tf::FluentTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Function<tf::AuxiliaryTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Atom<LiftedTag, tf::StaticTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Atom<LiftedTag, tf::FluentTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Atom<LiftedTag, tf::DerivedTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Atom<GroundTag, tf::StaticTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Atom<GroundTag, tf::FluentTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Atom<GroundTag, tf::DerivedTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Literal<LiftedTag, tf::StaticTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Literal<LiftedTag, tf::FluentTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Literal<LiftedTag, tf::DerivedTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Literal<GroundTag, tf::StaticTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Literal<GroundTag, tf::FluentTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Literal<GroundTag, tf::DerivedTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTerm<LiftedTag, tf::StaticTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTerm<LiftedTag, tf::FluentTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTerm<LiftedTag, tf::AuxiliaryTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTerm<GroundTag, tf::StaticTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTerm<GroundTag, tf::FluentTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTerm<GroundTag, tf::AuxiliaryTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTermValue<GroundTag, tf::StaticTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTermValue<GroundTag, tf::FluentTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTermValue<GroundTag, tf::AuxiliaryTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::UnaryOperator<LiftedTag, tf::ArithmeticOperatorKind>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::BinaryOperator<LiftedTag, tf::ArithmeticOperatorKind>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::BinaryOperator<LiftedTag, tf::BooleanOperatorKind>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::MultiOperator<LiftedTag, tf::ArithmeticOperatorKind>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::NumericEffect<LiftedTag, tf::FluentTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::UnaryOperator<GroundTag, tf::ArithmeticOperatorKind>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::BinaryOperator<GroundTag, tf::ArithmeticOperatorKind>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::BinaryOperator<GroundTag, tf::BooleanOperatorKind>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::MultiOperator<GroundTag, tf::ArithmeticOperatorKind>, td::Repository>>;
template struct fmt::formatter<urd::index_view<tf::NumericEffect<GroundTag, tf::FluentTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<td::ConjunctiveCondition<LiftedTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<td::ConjunctiveCondition<GroundTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<td::Rule<LiftedTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<td::Rule<GroundTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<td::Program<LiftedTag>, td::Repository>>;
template struct fmt::formatter<urd::index_view<td::Program<GroundTag>, td::Repository>>;
template struct fmt::formatter<ygg::RowView<tf::Predicate<tf::StaticTag>, td::Repository>>;
template struct fmt::formatter<ygg::RowView<tf::Predicate<tf::FluentTag>, td::Repository>>;
template struct fmt::formatter<ygg::RowView<tf::Predicate<tf::DerivedTag>, td::Repository>>;
template struct fmt::formatter<ygg::RowView<tf::Function<tf::StaticTag>, td::Repository>>;
template struct fmt::formatter<ygg::RowView<tf::Function<tf::FluentTag>, td::Repository>>;
template struct fmt::formatter<ygg::RowView<tf::Function<tf::AuxiliaryTag>, td::Repository>>;
template struct fmt::formatter<ygg::RowView<td::Rule<LiftedTag>, td::Repository>>;
