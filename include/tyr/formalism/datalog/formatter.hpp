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

// Rules, ground rule bodies and programs in Datalog: each view has an fmt::formatter whose format is defined in
// src/formalism/datalog/formatter.cpp. The shared records they hold print PDDL on their own (formalism/formatter.hpp) and
// Datalog inside a rule (p(x, y)). Variables keep their PDDL names (?x); a rule's cost is a comment after it.

/// A ground rule's body: its literals and constraints, separated by commas.
template<>
struct fmt::formatter<urd::index_view<tyr::formalism::RuleBody<tyr::GroundTag>, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::RuleBody<tyr::GroundTag>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A rule: heads :- body.  % cost c (its heads separated by commas; without a body, heads.).
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::Rule<T>, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Rule<T>, tyr::formalism::Repository>& value, fmt::format_context& context) const;
};

/// A program: its predicates, functions and objects as comments, then its facts, function values and rules, one per line, and its goal as a query ?- body.
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::Program<T>, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Program<T>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

#endif
