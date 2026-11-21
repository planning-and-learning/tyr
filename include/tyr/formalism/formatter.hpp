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

#ifndef TYR_FORMALISM_FORMATTER_HPP_
#define TYR_FORMALISM_FORMATTER_HPP_

#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/unification/substitution.hpp"

#include <fmt/format.h>
#include <yggdrasil/formalism/formatter.hpp>
#include <yggdrasil/formatting/formatter.hpp>

#if !YGG_ENABLE_FMT_FORMATTERS
#error "tyr requires yggdrasil's fmt formatters (YGG_ENABLE_FMT_FORMATTERS=1)."
#endif

// The records of tyr's formalism are written by fmt::formatter specializations of their views, declared here (the
// shared records, PDDL), in formalism/planning/formatter.hpp (PDDL) and formalism/datalog/formatter.hpp (Datalog) and
// defined in the library: written once there, not in every file that prints a record. A record held as data (no
// repository to name its symbols) is written as urd writes a record.

namespace tyr::formalism::detail
{
/// The parse of tyr's formatters: a record takes no format spec.
struct without_format_spec
{
    constexpr fmt::format_parse_context::iterator parse(fmt::format_parse_context& context) const { return context.begin(); }
};
}  // namespace tyr::formalism::detail

namespace fmt
{

/// A parameter by its index, ?x<index>: a lifted atom or function term writes it as the variable its parameters name.
template<>
struct formatter<tyr::formalism::ParameterIndex, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::formalism::ParameterIndex& value, fmt::format_context& context) const;
};

template<>
struct formatter<tyr::formalism::BooleanOperatorKind, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(tyr::formalism::BooleanOperatorKind value, fmt::format_context& context) const;
};

template<>
struct formatter<tyr::formalism::ArithmeticOperatorKind, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(tyr::formalism::ArithmeticOperatorKind value, fmt::format_context& context) const;
};

template<>
struct formatter<tyr::formalism::NumericEffectOperatorKind, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(tyr::formalism::NumericEffectOperatorKind value, fmt::format_context& context) const;
};

}  // namespace fmt

// The shared records in PDDL: each record's view (through Repository) has an fmt::formatter whose format is defined in
// src/formalism/formatter.cpp, and explicitly instantiated there for each member of a family of records.

/// An object: its name.
template<>
struct fmt::formatter<urd::index_view<tyr::formalism::Object, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Object, tyr::formalism::Repository>& value, fmt::format_context& context) const;
};

/// A variable: its name (?x).
template<>
struct fmt::formatter<urd::index_view<tyr::formalism::Variable, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Variable, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A predicate as a domain declares it: (name ?x0 ?x1 ...).
template<tyr::formalism::FactKind F>
struct fmt::formatter<urd::index_view<tyr::formalism::Predicate<F>, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Predicate<F>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A function as a domain declares it: (name ?x0 ?x1 ...).
template<tyr::formalism::FactKind F>
struct fmt::formatter<urd::index_view<tyr::formalism::Function<F>, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Function<F>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

/// An atom: (predicate terms...).
template<tyr::TaskKind T, tyr::formalism::FactKind F>
struct fmt::formatter<urd::index_view<tyr::formalism::Atom<T, F>, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Atom<T, F>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A literal: its atom, or (not atom).
template<tyr::TaskKind T, tyr::formalism::FactKind F>
struct fmt::formatter<urd::index_view<tyr::formalism::Literal<T, F>, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Literal<T, F>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A function term: (function terms...).
template<tyr::TaskKind T, tyr::formalism::FactKind F>
struct fmt::formatter<urd::index_view<tyr::formalism::FunctionTerm<T, F>, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::FunctionTerm<T, F>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A function term's initial value: (= term value).
template<tyr::TaskKind T, tyr::formalism::FactKind F>
struct fmt::formatter<urd::index_view<tyr::formalism::FunctionTermValue<T, F>, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::FunctionTermValue<T, F>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A unary arithmetic expression: (- e).
template<tyr::TaskKind T, tyr::formalism::OperatorKind O>
struct fmt::formatter<urd::index_view<tyr::formalism::UnaryOperator<T, O>, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::UnaryOperator<T, O>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A binary arithmetic expression (+ a b), or a numeric comparison (>= a b); != is (not (= a b)).
template<tyr::TaskKind T, tyr::formalism::OperatorKind O>
struct fmt::formatter<urd::index_view<tyr::formalism::BinaryOperator<T, O>, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::BinaryOperator<T, O>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

/// An arithmetic expression over several arguments: (+ a b c).
template<tyr::TaskKind T, tyr::formalism::OperatorKind O>
struct fmt::formatter<urd::index_view<tyr::formalism::MultiOperator<T, O>, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::MultiOperator<T, O>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A numeric effect: (increase term expression).
template<tyr::TaskKind T, tyr::formalism::FactKind F>
struct fmt::formatter<urd::index_view<tyr::formalism::NumericEffect<T, F>, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::NumericEffect<T, F>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A condition: (and literals... constraints...), a ground one's facts as the literals they stand for.
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::ConjunctiveCondition<T>, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::ConjunctiveCondition<T>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A binding of a relation (a predicate's, a function's, an action's): (name values...); an axiom's, which has no name, its values.
template<typename R>
struct fmt::formatter<ygg::RowView<R, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const ygg::RowView<R, tyr::formalism::Repository>& value, fmt::format_context& context) const;
};

/// A substitution: each parameter of its domain with its value, `_` where it has none, in the order its parameters were
/// given.
template<typename T>
struct fmt::formatter<tyr::formalism::unification::SubstitutionFunction<T>, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::formalism::unification::SubstitutionFunction<T>& value, fmt::format_context& ctx) const;
};

/// A condition's variable dependency graph: one DOT graph per kind of binary dependency.
template<>
struct fmt::formatter<tyr::formalism::VariableDependencyGraph> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::formalism::VariableDependencyGraph& value, fmt::format_context& context) const;
};

#endif
