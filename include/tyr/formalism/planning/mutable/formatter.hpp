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

#ifndef TYR_FORMALISM_PLANNING_MUTABLE_FORMATTER_HPP_
#define TYR_FORMALISM_PLANNING_MUTABLE_FORMATTER_HPP_

#include "tyr/formalism/formatter.hpp"
#include "tyr/formalism/planning/mutable/action.hpp"
#include "tyr/formalism/planning/mutable/atom.hpp"
#include "tyr/formalism/planning/mutable/conditional_effect.hpp"
#include "tyr/formalism/planning/mutable/conjunctive_condition.hpp"
#include "tyr/formalism/planning/mutable/conjunctive_effect.hpp"
#include "tyr/formalism/planning/mutable/literal.hpp"

#include <fmt/core.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <ostream>
#include <sstream>
#include <yggdrasil/formatting/cista_formatters.hpp>
#include <yggdrasil/io/iostream.hpp>

namespace fmt
{

template<tyr::formalism::FactKind T>
struct formatter<tyr::formalism::planning::MutableAtom<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::formalism::planning::MutableAtom<T>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({} {})", value.predicate.get_name(), fmt::join(ygg::to_strings(value.terms), " "));
    }
};

template<tyr::formalism::FactKind T>
struct formatter<tyr::formalism::planning::MutableLiteral<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::formalism::planning::MutableLiteral<T>& value, FormatContext& ctx) const
    {
        if (value.polarity)
        {
            return fmt::format_to(ctx.out(), "{}", value.atom);
        }
        return fmt::format_to(ctx.out(), "(not {})", value.atom);
    }
};

template<>
struct formatter<tyr::formalism::planning::MutableConjunctiveCondition, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::formalism::planning::MutableConjunctiveCondition& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "ConjunctiveCondition(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "num parent variables = ", value.num_parent_variables);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "num variables = ", value.num_variables);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static literals = ", value.static_literals);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent literals = ", value.fluent_literals);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::formalism::planning::MutableConjunctiveEffect, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::formalism::planning::MutableConjunctiveEffect& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "MutableConjunctiveEffect(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "num parent variables = ", value.num_parent_variables);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "num variables = ", value.num_variables);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "literals = ", value.literals);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::formalism::planning::MutableConditionalEffect, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::formalism::planning::MutableConditionalEffect& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "MutableConditionalEffect(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "num parent variables = ", value.num_parent_variables);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "num variables = ", value.num_variables);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "condition = ", value.condition);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "effect = ", value.effect);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::formalism::planning::MutableAction, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::formalism::planning::MutableAction& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "Action(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "num variables = ", value.num_variables);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "condition = ", value.condition);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "effects = ", value.effects);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

}

#endif
