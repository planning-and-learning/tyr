/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/datalog/formatter.hpp"

#include <array>
#include <string_view>

#if YGG_ENABLE_FMT_FORMATTERS
namespace dl = tyr::datalog::database::datalog;

fmt::format_context::iterator fmt::formatter<dl::AggregateFunction, char>::format(dl::AggregateFunction function, fmt::format_context& ctx) const
{
    static constexpr auto names = std::array<std::string_view, 4> { "COUNT", "SUM", "MIN", "MAX" };
    return fmt::format_to(ctx.out(), "{}", names[static_cast<size_t>(function)]);
}

fmt::format_context::iterator fmt::formatter<dl::Term, char>::format(const dl::Term& term, fmt::format_context& ctx) const
{ return term.is_variable() ? fmt::format_to(ctx.out(), "?{}", term.value) : fmt::format_to(ctx.out(), "{}", term.value); }

fmt::format_context::iterator fmt::formatter<dl::Comparison, char>::format(const dl::Comparison& comparison, fmt::format_context& ctx) const
{
    static constexpr auto names = std::array<std::string_view, 4> { "=", "!=", "<", "<=" };
    return fmt::format_to(ctx.out(), "{} {} {}", comparison.lhs, names[static_cast<size_t>(comparison.kind)], comparison.rhs);
}

fmt::format_context::iterator fmt::formatter<dl::Arithmetic, char>::format(const dl::Arithmetic& arithmetic, fmt::format_context& ctx) const
{
    static constexpr auto names = std::array<std::string_view, 6> { "+", "-", "*", "/", "min", "max" };
    return fmt::format_to(ctx.out(), "?{} = {} {} {}", arithmetic.result, arithmetic.lhs, names[static_cast<size_t>(arithmetic.operation)], arithmetic.rhs);
}
#endif
