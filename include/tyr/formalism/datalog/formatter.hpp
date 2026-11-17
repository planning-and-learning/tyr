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

// The datalog formalism in Datalog: each record's view (through datalog::Repository) has an fmt::formatter whose format
// is defined in src/formalism/datalog/formatter.cpp, and explicitly instantiated there for each member of a family of
// records. Variables keep their PDDL names (?x); a rule's cost is a comment after it.

/// An object: its name.
template<>
struct fmt::formatter<urd::index_view<tyr::formalism::Object, tyr::formalism::datalog::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Object, tyr::formalism::datalog::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A variable: its name (?x).
template<>
struct fmt::formatter<urd::index_view<tyr::formalism::Variable, tyr::formalism::datalog::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Variable, tyr::formalism::datalog::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A predicate: name/arity.
template<tyr::formalism::FactKind F>
struct fmt::formatter<urd::index_view<tyr::formalism::Predicate<F>, tyr::formalism::datalog::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Predicate<F>, tyr::formalism::datalog::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A function: name/arity.
template<tyr::formalism::FactKind F>
struct fmt::formatter<urd::index_view<tyr::formalism::Function<F>, tyr::formalism::datalog::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Function<F>, tyr::formalism::datalog::Repository>& value,
                                         fmt::format_context& context) const;
};

/// An atom: predicate(terms...), a nullary one predicate.
template<tyr::TaskKind T, tyr::formalism::FactKind F>
struct fmt::formatter<urd::index_view<tyr::formalism::Atom<T, F>, tyr::formalism::datalog::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Atom<T, F>, tyr::formalism::datalog::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A literal: its atom, or not atom.
template<tyr::TaskKind T, tyr::formalism::FactKind F>
struct fmt::formatter<urd::index_view<tyr::formalism::Literal<T, F>, tyr::formalism::datalog::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Literal<T, F>, tyr::formalism::datalog::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A function term: function(terms...).
template<tyr::TaskKind T, tyr::formalism::FactKind F>
struct fmt::formatter<urd::index_view<tyr::formalism::FunctionTerm<T, F>, tyr::formalism::datalog::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::FunctionTerm<T, F>, tyr::formalism::datalog::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A function term's value: term = value.
template<tyr::TaskKind T, tyr::formalism::FactKind F>
struct fmt::formatter<urd::index_view<tyr::formalism::FunctionTermValue<T, F>, tyr::formalism::datalog::Repository>> :
    tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::FunctionTermValue<T, F>, tyr::formalism::datalog::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A unary arithmetic expression: (-e).
template<tyr::TaskKind T, tyr::formalism::OperatorKind O>
struct fmt::formatter<urd::index_view<tyr::formalism::UnaryOperator<T, O>, tyr::formalism::datalog::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::UnaryOperator<T, O>, tyr::formalism::datalog::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A binary arithmetic expression (a + b), or a comparison a >= b.
template<tyr::TaskKind T, tyr::formalism::OperatorKind O>
struct fmt::formatter<urd::index_view<tyr::formalism::BinaryOperator<T, O>, tyr::formalism::datalog::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::BinaryOperator<T, O>, tyr::formalism::datalog::Repository>& value,
                                         fmt::format_context& context) const;
};

/// An arithmetic expression over several arguments: (a + b + c).
template<tyr::TaskKind T, tyr::formalism::OperatorKind O>
struct fmt::formatter<urd::index_view<tyr::formalism::MultiOperator<T, O>, tyr::formalism::datalog::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::MultiOperator<T, O>, tyr::formalism::datalog::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A numeric effect: term += expression (:=, +=, -=, *=, /=).
template<tyr::TaskKind T, tyr::formalism::FactKind F>
struct fmt::formatter<urd::index_view<tyr::formalism::NumericEffect<T, F>, tyr::formalism::datalog::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::NumericEffect<T, F>, tyr::formalism::datalog::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A rule's body: its literals and constraints, separated by commas.
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::datalog::ConjunctiveCondition<T>, tyr::formalism::datalog::Repository>> :
    tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::datalog::ConjunctiveCondition<T>, tyr::formalism::datalog::Repository>& value,
                                         fmt::format_context& context) const;
};

/// An effect: its numeric effects, separated by commas.
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::datalog::ConjunctiveEffect<T>, tyr::formalism::datalog::Repository>> :
    tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::datalog::ConjunctiveEffect<T>, tyr::formalism::datalog::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A rule: heads :- body.  % cost c (its heads separated by commas; without a body, heads.).
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::datalog::Rule<T>, tyr::formalism::datalog::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::datalog::Rule<T>, tyr::formalism::datalog::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A program: its predicates, functions and objects as comments, then its facts, function values and rules, one per line, and its goal as a query ?- body.
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::datalog::Program<T>, tyr::formalism::datalog::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::datalog::Program<T>, tyr::formalism::datalog::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A binding of a relation: a predicate's or a function's as its atom, name(values...); a rule's, which has no name, its values.
template<typename R>
struct fmt::formatter<ygg::RowView<R, tyr::formalism::datalog::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const ygg::RowView<R, tyr::formalism::datalog::Repository>& value, fmt::format_context& context) const;
};

/// A condition's variable dependency graph: one DOT graph per kind of binary dependency.
template<>
struct fmt::formatter<tyr::formalism::datalog::VariableDependencyGraph> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::formalism::datalog::VariableDependencyGraph& value, fmt::format_context& context) const;
};

#endif
