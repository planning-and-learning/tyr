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

#include <fmt/base.h>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/declarations.hpp>
#include <yggdrasil/formatting/without_format_spec.hpp>

#if !YGG_ENABLE_FMT_FORMATTERS
#error "tyr requires yggdrasil's fmt formatters (YGG_ENABLE_FMT_FORMATTERS=1)."
#endif

// tyr's generic formatters. A type that tyr writes otherwise than urd and yggdrasil do declares its fmt::formatter
// beside it (a record's in its view header, PDDL or Datalog), defined in the library: written once there, not in every
// file that prints it. A record held as data (no repository to name its symbols) is written as urd writes a record.

/// A binding of a relation (a predicate's, a function's, an action's): (name values...); an axiom's, which has no name,
/// its values. Defined in src/formalism/formatter.cpp. Every view with a row child includes this header, so that fmt
/// never picks urd's generic record formatter for a row first.
template<typename R>
struct fmt::formatter<ygg::RowView<R, tyr::formalism::Repository>> : ygg::without_format_spec
{
    fmt::format_context::iterator format(const ygg::RowView<R, tyr::formalism::Repository>& value, fmt::format_context& context) const;
};

#endif
