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

#ifndef TYR_DATALOG_FORMATTER_HPP_
#define TYR_DATALOG_FORMATTER_HPP_

#include "tyr/datalog/lifted/kckp/assignment.hpp"
#include "tyr/datalog/lifted/kckp/consistency_graph.hpp"
#include "tyr/datalog/statistics/program.hpp"
#include "tyr/datalog/statistics/rule.hpp"
#include "tyr/formalism/formatter.hpp"

#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <limits>
#include <ostream>
#include <ranges>
#include <string>
#include <urd/format.hpp>
#include <vector>
#include <yggdrasil/core/chrono.hpp>
#include <yggdrasil/formatting/cista_formatters.hpp>
#include <yggdrasil/io/iostream.hpp>

namespace fmt
{

template<>
struct formatter<tyr::datalog::StaticConsistencyGraph, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::datalog::StaticConsistencyGraph&, fmt::format_context& ctx) const;
};

template<>
struct formatter<tyr::datalog::ProgramStatistics, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::datalog::ProgramStatistics& value, fmt::format_context& ctx) const;
};

template<>
struct formatter<tyr::datalog::RuleStatistics, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::datalog::RuleStatistics& value, fmt::format_context& ctx) const;
};

template<>
struct formatter<tyr::datalog::AggregatedRuleStatistics, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::datalog::AggregatedRuleStatistics& value, fmt::format_context& ctx) const;
};

template<>
struct formatter<tyr::datalog::RuleWorkerStatistics, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::datalog::RuleWorkerStatistics& value, fmt::format_context& ctx) const;
};

template<>
struct formatter<tyr::datalog::AggregatedRuleWorkerStatistics, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::datalog::AggregatedRuleWorkerStatistics& value, fmt::format_context& ctx) const;
};

}  // namespace fmt

#endif
