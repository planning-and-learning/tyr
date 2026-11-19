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

#include <fmt/format.h>
#include <yggdrasil/formalism/formatter.hpp>
#include <yggdrasil/formatting/formatter.hpp>

#if !YGG_ENABLE_FMT_FORMATTERS
#error "tyr requires yggdrasil's fmt formatters (YGG_ENABLE_FMT_FORMATTERS=1)."
#endif

// The records of tyr's formalism are written by fmt::formatter specializations of their views, declared in
// formalism/planning/formatter.hpp (PDDL) and formalism/datalog/formatter.hpp (Datalog) and defined in the library:
// written once there, not in every file that prints a record. A record held as data (no repository to name its
// symbols) is written as urd writes a record.

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

#endif
