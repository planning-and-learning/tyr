/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_DATALOG_FORMATTER_HPP_
#define TYR_DATALOG_DATABASE_DATALOG_FORMATTER_HPP_

#include "tyr/datalog/lifted/database/datalog/declarations.hpp"
#include "tyr/formalism/formatter.hpp"

#include <fmt/format.h>

#if YGG_ENABLE_FMT_FORMATTERS
namespace fmt
{
/// COUNT, SUM, MIN or MAX.
template<>
struct formatter<tyr::datalog::database::datalog::AggregateFunction, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(tyr::datalog::database::datalog::AggregateFunction function, fmt::format_context& ctx) const;
};

/// A variable as ?n, a constant as its value.
template<>
struct formatter<tyr::datalog::database::datalog::Term, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::datalog::database::datalog::Term& term, fmt::format_context& ctx) const;
};

/// lhs op rhs.
template<>
struct formatter<tyr::datalog::database::datalog::Comparison, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::datalog::database::datalog::Comparison& comparison, fmt::format_context& ctx) const;
};

/// ?result = lhs op rhs.
template<>
struct formatter<tyr::datalog::database::datalog::Arithmetic, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::datalog::database::datalog::Arithmetic& arithmetic, fmt::format_context& ctx) const;
};
}  // namespace fmt
#endif

#endif
