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

#include "tyr/formalism/formatter.hpp"

#include "tyr/formalism/enums.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/variable_dependency_graph.hpp"

#include <concepts>
#include <cstddef>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <urd/dot.hpp>
#include <urd/format.hpp>
#include <variant>
#include <yggdrasil/formalism/formatter.hpp>
#include <yggdrasil/formatting/cista_formatters.hpp>

fmt::format_context::iterator fmt::formatter<tyr::formalism::ParameterIndex>::format(const tyr::formalism::ParameterIndex& value,
                                                                                     fmt::format_context& context) const
{ return fmt::format_to(context.out(), "?x{}", value.value); }

fmt::format_context::iterator fmt::formatter<tyr::formalism::BooleanOperatorKind>::format(tyr::formalism::BooleanOperatorKind value,
                                                                                          fmt::format_context& context) const
{ return fmt::format_to(context.out(), "{}", tyr::formalism::to_string(value)); }

fmt::format_context::iterator fmt::formatter<tyr::formalism::ArithmeticOperatorKind>::format(tyr::formalism::ArithmeticOperatorKind value,
                                                                                             fmt::format_context& context) const
{ return fmt::format_to(context.out(), "{}", tyr::formalism::to_string(value)); }

fmt::format_context::iterator fmt::formatter<tyr::formalism::NumericEffectOperatorKind>::format(tyr::formalism::NumericEffectOperatorKind value,
                                                                                                fmt::format_context& context) const
{ return fmt::format_to(context.out(), "{}", tyr::formalism::to_string(value)); }

// What each shared record is in PDDL. A list is written as its elements, each after a space, so that an empty one leaves
// its parentheses closed: (and), (p). A lifted atom or function term writes a parameter as the variable its parameters
// name at that index. A ground condition writes its facts as the literals they stand for (planning/formatter.hpp).

namespace tf = tyr::formalism;
using tyr::GroundTag;
using tyr::LiftedTag;

fmt::format_context::iterator fmt::formatter<urd::index_view<tf::Object, tf::Repository>>::format(const urd::index_view<tf::Object, tf::Repository>& value,
                                                                                                  fmt::format_context& context) const
{ return fmt::format_to(context.out(), "{}", value.get_name()); }

fmt::format_context::iterator fmt::formatter<urd::index_view<tf::Variable, tf::Repository>>::format(const urd::index_view<tf::Variable, tf::Repository>& value,
                                                                                                    fmt::format_context& context) const
{ return fmt::format_to(context.out(), "{}", value.get_name()); }

template<tf::FactKind F>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::Predicate<F>, tf::Repository>>::format(const urd::index_view<tf::Predicate<F>, tf::Repository>& value,
                                                                          fmt::format_context& context) const
{
    auto out = fmt::format_to(context.out(), "({}", value.get_name());
    for (std::size_t position = 0; position < value.get_arity(); ++position)
        out = fmt::format_to(out, " ?x{}", position);
    return fmt::format_to(out, ")");
}

template<tf::FactKind F>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::Function<F>, tf::Repository>>::format(const urd::index_view<tf::Function<F>, tf::Repository>& value,
                                                                         fmt::format_context& context) const
{
    auto out = fmt::format_to(context.out(), "({}", value.get_name());
    for (std::size_t position = 0; position < value.get_arity(); ++position)
        out = fmt::format_to(out, " ?x{}", position);
    return fmt::format_to(out, ")");
}

template<tyr::TaskKind T, tf::FactKind F>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::Atom<T, F>, tf::Repository>>::format(const urd::index_view<tf::Atom<T, F>, tf::Repository>& value,
                                                                        fmt::format_context& context) const
{
    if constexpr (std::same_as<T, GroundTag>)
        return fmt::format_to(context.out(), "{}", value.get_row());
    else
    {
        // A parameter is written as the variable its parameters name at its index.
        const auto variables = value.get_parameters().get_variables();
        auto out = fmt::format_to(context.out(), "({}", value.get_predicate().get_name());
        for (auto term : value.get_terms())
        {
            if (const auto* parameter = std::get_if<tf::ParameterIndex>(&term))
                out = fmt::format_to(out, " {}", variables[parameter->value]);
            else
                out = fmt::format_to(out, " {}", term);
        }
        return fmt::format_to(out, ")");
    }
}

template<tyr::TaskKind T, tf::FactKind F>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::Literal<T, F>, tf::Repository>>::format(const urd::index_view<tf::Literal<T, F>, tf::Repository>& value,
                                                                           fmt::format_context& context) const
{
    if (value.get_polarity())
        return fmt::format_to(context.out(), "{}", value.get_atom());
    return fmt::format_to(context.out(), "(not {})", value.get_atom());
}

template<tyr::TaskKind T, tf::FactKind F>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::FunctionTerm<T, F>, tf::Repository>>::format(const urd::index_view<tf::FunctionTerm<T, F>, tf::Repository>& value,
                                                                                fmt::format_context& context) const
{
    if constexpr (std::same_as<T, GroundTag>)
        return fmt::format_to(context.out(), "{}", value.get_row());
    else
    {
        // A parameter is written as the variable its parameters name at its index.
        const auto variables = value.get_parameters().get_variables();
        auto out = fmt::format_to(context.out(), "({}", value.get_function().get_name());
        for (auto term : value.get_terms())
        {
            if (const auto* parameter = std::get_if<tf::ParameterIndex>(&term))
                out = fmt::format_to(out, " {}", variables[parameter->value]);
            else
                out = fmt::format_to(out, " {}", term);
        }
        return fmt::format_to(out, ")");
    }
}

template<tyr::TaskKind T, tf::FactKind F>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::FunctionTermValue<T, F>, tf::Repository>>::format(const urd::index_view<tf::FunctionTermValue<T, F>, tf::Repository>& value,
                                                                                     fmt::format_context& context) const
{ return fmt::format_to(context.out(), "(= {} {})", value.get_fterm(), value.get_value()); }

template<tyr::TaskKind T, tf::OperatorKind O>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::UnaryOperator<T, O>, tf::Repository>>::format(const urd::index_view<tf::UnaryOperator<T, O>, tf::Repository>& value,
                                                                                 fmt::format_context& context) const
{ return fmt::format_to(context.out(), "({} {})", value.get_operation(), value.get_arg()); }

template<tyr::TaskKind T, tf::OperatorKind O>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::BinaryOperator<T, O>, tf::Repository>>::format(const urd::index_view<tf::BinaryOperator<T, O>, tf::Repository>& value,
                                                                                  fmt::format_context& context) const
{
    // PDDL compares with =, <, <=, > and >=: a != b is (not (= a b)).
    if constexpr (std::same_as<O, tf::BooleanOperatorKind>)
    {
        if (value.get_operation() == tf::BooleanOperatorKind::Ne)
            return fmt::format_to(context.out(), "(not (= {} {}))", value.get_lhs(), value.get_rhs());
    }
    return fmt::format_to(context.out(), "({} {} {})", value.get_operation(), value.get_lhs(), value.get_rhs());
}

template<tyr::TaskKind T, tf::OperatorKind O>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::MultiOperator<T, O>, tf::Repository>>::format(const urd::index_view<tf::MultiOperator<T, O>, tf::Repository>& value,
                                                                                 fmt::format_context& context) const
{
    auto out = fmt::format_to(context.out(), "({}", value.get_operation());
    for (auto argument : value.get_args())
        out = fmt::format_to(out, " {}", argument);
    return fmt::format_to(out, ")");
}

template<tyr::TaskKind T, tf::FactKind F>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::NumericEffect<T, F>, tf::Repository>>::format(const urd::index_view<tf::NumericEffect<T, F>, tf::Repository>& value,
                                                                                 fmt::format_context& context) const
{ return fmt::format_to(context.out(), "({} {} {})", value.get_operation(), value.get_fterm(), value.get_fexpr()); }

template<tyr::TaskKind T>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::ConjunctiveCondition<T>, tf::Repository>>::format(const urd::index_view<tf::ConjunctiveCondition<T>, tf::Repository>& value,
                                                                                     fmt::format_context& context) const
{
    auto out = fmt::format_to(context.out(), "(and");
    if constexpr (std::same_as<T, LiftedTag>)
    {
        for (auto literal : value.get_static_literals())
            out = fmt::format_to(out, " {}", literal);
        for (auto literal : value.get_fluent_literals())
            out = fmt::format_to(out, " {}", literal);
        for (auto literal : value.get_derived_literals())
            out = fmt::format_to(out, " {}", literal);
        for (auto constraint : value.get_numeric_constraints())
            out = fmt::format_to(out, " {}", constraint);
    }
    else
    {
        for (auto literal : value.get_static_literals())
            out = fmt::format_to(out, " {}", literal);
        for (auto literal : value.get_derived_literals())
            out = fmt::format_to(out, " {}", literal);
        for (auto fact : value.get_positive_facts())
            out = fmt::format_to(out, " {}", fact);
        // A fact that must not hold: its atom false, or, where its variable holds none of its atoms, one of them true.
        for (auto fact : value.get_negative_facts())
        {
            if (const auto atom = fact.get_atom(); atom.has_value())
                out = fmt::format_to(out, " (not {})", atom.value());
            else
                out = fmt::format_to(out, " (or {})", fmt::join(fact.get_variable().get_atoms(), " "));
        }
        for (auto constraint : value.get_numeric_constraints())
            out = fmt::format_to(out, " {}", constraint);
    }
    return fmt::format_to(out, ")");
}

template<typename R>
fmt::format_context::iterator fmt::formatter<ygg::RowView<R, tf::Repository>>::format(const ygg::RowView<R, tf::Repository>& value,
                                                                                      fmt::format_context& context) const
{
    // An axiom or a rule has no name: its binding is its values.
    if constexpr (std::same_as<R, tf::Axiom<LiftedTag>> || std::same_as<R, tyr::formalism::Rule<LiftedTag>>)
        return fmt::format_to(context.out(), "{}", ygg::values_text<R>(value));
    else
    {
        if (value.get_values().empty())
            return fmt::format_to(context.out(), "({})", value.get_relation().get_name());
        return fmt::format_to(context.out(), "({} {})", value.get_relation().get_name(), ygg::values_text<R>(value));
    }
}

/// One DOT graph per kind of binary dependency, named after it: the condition's parameters, adjacent where they occur
/// together in a literal (or constraint) of that kind.
fmt::format_context::iterator fmt::formatter<tf::VariableDependencyGraph>::format(const tf::VariableDependencyGraph& value, fmt::format_context& context) const
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
template struct fmt::formatter<urd::index_view<tf::Predicate<tf::StaticTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Predicate<tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Predicate<tf::DerivedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Function<tf::StaticTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Function<tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Function<tf::AuxiliaryTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Atom<LiftedTag, tf::StaticTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Atom<LiftedTag, tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Atom<LiftedTag, tf::DerivedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Atom<GroundTag, tf::StaticTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Atom<GroundTag, tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Atom<GroundTag, tf::DerivedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Literal<LiftedTag, tf::StaticTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Literal<LiftedTag, tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Literal<LiftedTag, tf::DerivedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Literal<GroundTag, tf::StaticTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Literal<GroundTag, tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Literal<GroundTag, tf::DerivedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTerm<LiftedTag, tf::StaticTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTerm<LiftedTag, tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTerm<LiftedTag, tf::AuxiliaryTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTerm<GroundTag, tf::StaticTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTerm<GroundTag, tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTerm<GroundTag, tf::AuxiliaryTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTermValue<GroundTag, tf::StaticTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTermValue<GroundTag, tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTermValue<GroundTag, tf::AuxiliaryTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::UnaryOperator<LiftedTag, tf::ArithmeticOperatorKind>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::UnaryOperator<GroundTag, tf::ArithmeticOperatorKind>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::BinaryOperator<LiftedTag, tf::ArithmeticOperatorKind>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::BinaryOperator<LiftedTag, tf::BooleanOperatorKind>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::BinaryOperator<GroundTag, tf::ArithmeticOperatorKind>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::BinaryOperator<GroundTag, tf::BooleanOperatorKind>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::MultiOperator<LiftedTag, tf::ArithmeticOperatorKind>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::MultiOperator<GroundTag, tf::ArithmeticOperatorKind>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::NumericEffect<LiftedTag, tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::NumericEffect<LiftedTag, tf::AuxiliaryTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::NumericEffect<GroundTag, tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::NumericEffect<GroundTag, tf::AuxiliaryTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::ConjunctiveCondition<LiftedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::ConjunctiveCondition<GroundTag>, tf::Repository>>;
template struct fmt::formatter<ygg::RowView<tf::Predicate<tf::StaticTag>, tf::Repository>>;
template struct fmt::formatter<ygg::RowView<tf::Predicate<tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<ygg::RowView<tf::Predicate<tf::DerivedTag>, tf::Repository>>;
template struct fmt::formatter<ygg::RowView<tf::Function<tf::StaticTag>, tf::Repository>>;
template struct fmt::formatter<ygg::RowView<tf::Function<tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<ygg::RowView<tf::Function<tf::AuxiliaryTag>, tf::Repository>>;
template struct fmt::formatter<ygg::RowView<tf::Action<LiftedTag>, tf::Repository>>;
template struct fmt::formatter<ygg::RowView<tf::Axiom<LiftedTag>, tf::Repository>>;
template struct fmt::formatter<ygg::RowView<tyr::formalism::Rule<LiftedTag>, tf::Repository>>;

template<typename T>
fmt::format_context::iterator
fmt::formatter<tyr::formalism::unification::SubstitutionFunction<T>>::format(const tyr::formalism::unification::SubstitutionFunction<T>& value,
                                                                             fmt::format_context& ctx) const
{
    auto out = fmt::format_to(ctx.out(), "SubstitutionFunction(");
    auto separator = "";
    for (const auto parameter : value.parameters())
    {
        if (const auto& bound = *value.try_get(parameter); bound.has_value())
            out = fmt::format_to(out, "{}{}={}", separator, parameter, *bound);
        else
            out = fmt::format_to(out, "{}{}=_", separator, parameter);
        separator = ", ";
    }
    return fmt::format_to(out, ")");
}

template struct fmt::formatter<tyr::formalism::unification::SubstitutionFunction<tyr::formalism::Term>>;
template struct fmt::formatter<tyr::formalism::unification::SubstitutionFunction<urd::index<tyr::formalism::Object>>>;
