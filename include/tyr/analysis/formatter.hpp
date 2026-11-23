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

#ifndef TYR_ANALYSIS_FORMATTER_HPP_
#define TYR_ANALYSIS_FORMATTER_HPP_

#include "tyr/analysis/declarations.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/planning/formatter.hpp"

#include <fmt/core.h>
#include <fmt/ranges.h>
#include <type_traits>
#include <yggdrasil/formatting/cista_formatters.hpp>

namespace fmt
{

template<typename C, typename Char>
struct range_format_kind<tyr::analysis::VariableDomainView<C>, Char, void> : std::false_type
{
};

}  // namespace fmt

#endif
