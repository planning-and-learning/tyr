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

#include "tyr/formalism/datas.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/views.hpp"

#include <fmt/core.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <ostream>
#include <yggdrasil/formatting/cista_formatters.hpp>
#include <yggdrasil/io/iostream.hpp>

namespace fmt
{

template<>
struct formatter<tyr::formalism::ParameterIndex, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::formalism::ParameterIndex& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "V{}", ygg::uint_t(value));
    }
};

template<>
struct formatter<tyr::formalism::Eq, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(tyr::formalism::Eq, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "=");
    }
};

template<>
struct formatter<tyr::formalism::Ne, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(tyr::formalism::Ne, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "!=");
    }
};

template<>
struct formatter<tyr::formalism::Le, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(tyr::formalism::Le, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "<=");
    }
};

template<>
struct formatter<tyr::formalism::Lt, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(tyr::formalism::Lt, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "<");
    }
};

template<>
struct formatter<tyr::formalism::Ge, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(tyr::formalism::Ge, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), ">=");
    }
};

template<>
struct formatter<tyr::formalism::Gt, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(tyr::formalism::Gt, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), ">");
    }
};

template<>
struct formatter<tyr::formalism::Add, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(tyr::formalism::Add, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "+");
    }
};

template<>
struct formatter<tyr::formalism::Sub, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(tyr::formalism::Sub, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "-");
    }
};

template<>
struct formatter<tyr::formalism::Mul, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(tyr::formalism::Mul, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "*");
    }
};

template<>
struct formatter<tyr::formalism::Div, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(tyr::formalism::Div, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "/");
    }
};

template<>
struct formatter<tyr::formalism::Assign, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(tyr::formalism::Assign, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "assign");
    }
};

template<>
struct formatter<tyr::formalism::Increase, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(tyr::formalism::Increase, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "increase");
    }
};

template<>
struct formatter<tyr::formalism::Decrease, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(tyr::formalism::Decrease, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "decrease");
    }
};

template<>
struct formatter<tyr::formalism::ScaleUp, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(tyr::formalism::ScaleUp, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "scale-up");
    }
};

template<>
struct formatter<tyr::formalism::ScaleDown, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(tyr::formalism::ScaleDown, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "scale-down");
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::Variable>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::Variable>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.name);
    }
};

template<typename C>
struct formatter<ygg::View<ygg::Index<tyr::formalism::Variable>, C>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const ygg::View<ygg::Index<tyr::formalism::Variable>, C>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_name());
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::Object>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::Object>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.name);
    }
};

template<typename C>
struct formatter<ygg::View<ygg::Index<tyr::formalism::Object>, C>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const ygg::View<ygg::Index<tyr::formalism::Object>, C>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_name());
    }
};

template<typename Tag>
struct formatter<ygg::Data<tyr::formalism::RelationBinding<Tag>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::RelationBinding<Tag>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{} {}", value.relation, fmt::join(ygg::to_strings(value.objects), " "));
    }
};

template<typename Tag>
struct formatter<ygg::Index<tyr::formalism::RelationBinding<Tag>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const ygg::Index<tyr::formalism::RelationBinding<Tag>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "<{},{}>", value.relation, value.row);
    }
};

template<typename Tag, typename C>
struct formatter<ygg::View<ygg::Index<tyr::formalism::RelationBinding<Tag>>, C>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const ygg::View<ygg::Index<tyr::formalism::RelationBinding<Tag>>, C>& value, FormatContext& ctx) const
    {
        if constexpr (requires { value.get_relation().get_name(); })
        {
            auto out = fmt::format_to(ctx.out(), "({}", value.get_relation().get_name());
            for (const auto object : value.get_objects())
                out = fmt::format_to(out, " {}", object);
            return fmt::format_to(out, ")");
        }
        return fmt::format_to(ctx.out(), "({})", fmt::join(ygg::to_strings(value.get_objects()), " "));
    }
};

template<>
struct formatter<ygg::Data<tyr::formalism::Term>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::Term>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.value);
    }
};

template<typename C>
struct formatter<ygg::View<ygg::Data<tyr::formalism::Term>, C>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const ygg::View<ygg::Data<tyr::formalism::Term>, C>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_variant());
    }
};

template<tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::Predicate<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::Predicate<T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}/{}", value.name, value.arity);
    }
};

template<tyr::formalism::FactKind T, typename C>
struct formatter<ygg::View<ygg::Index<tyr::formalism::Predicate<T>>, C>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const ygg::View<ygg::Index<tyr::formalism::Predicate<T>>, C>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}/{}", value.get_name(), value.get_arity());
    }
};

template<tyr::formalism::FactKind T>
struct formatter<ygg::Data<tyr::formalism::Function<T>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const ygg::Data<tyr::formalism::Function<T>>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}/{}", value.name, value.arity);
    }
};

template<tyr::formalism::FactKind T, typename C>
struct formatter<ygg::View<ygg::Index<tyr::formalism::Function<T>>, C>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const ygg::View<ygg::Index<tyr::formalism::Function<T>>, C>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}/{}", value.get_name(), value.get_arity());
    }
};

}

#endif
