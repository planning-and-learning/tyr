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

#include "tyr/formalism/binding_index.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/parameter_index.hpp"

#include <fmt/format.h>
#include <string>
#include <yggdrasil/formatting/formatter.hpp>

#if !YGG_ENABLE_FMT_FORMATTERS
#error "tyr requires yggdrasil's fmt formatters (YGG_ENABLE_FMT_FORMATTERS=1)."
#endif

namespace tyr::formalism
{

std::string to_string(const urd::data<Variable>& value);
std::string to_string(const urd::data<Object>& value);
std::string to_string(const Term& value);

std::string to_string(const urd::data<Predicate<StaticTag>>& value);
std::string to_string(const urd::data<Predicate<FluentTag>>& value);
std::string to_string(const urd::data<Predicate<DerivedTag>>& value);

std::string to_string(const urd::data<Function<StaticTag>>& value);
std::string to_string(const urd::data<Function<FluentTag>>& value);
std::string to_string(const urd::data<Function<AuxiliaryTag>>& value);

std::string to_string(const urd::data<RelationBinding<Predicate<StaticTag>>>& value);
std::string to_string(const urd::data<RelationBinding<Predicate<FluentTag>>>& value);
std::string to_string(const urd::data<RelationBinding<Predicate<DerivedTag>>>& value);

std::string to_string(const urd::data<RelationBinding<Function<StaticTag>>>& value);
std::string to_string(const urd::data<RelationBinding<Function<FluentTag>>>& value);
std::string to_string(const urd::data<RelationBinding<Function<AuxiliaryTag>>>& value);

}  // namespace tyr::formalism

namespace fmt
{

template<>
struct formatter<tyr::formalism::ParameterIndex, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::formalism::ParameterIndex& value, FormatContext& ctx) const
    { return fmt::format_to(ctx.out(), "V{}", value.value); }
};

template<>
struct formatter<tyr::formalism::BooleanOperatorKind, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(tyr::formalism::BooleanOperatorKind value, FormatContext& ctx) const
    { return fmt::format_to(ctx.out(), "{}", tyr::formalism::to_string(value)); }
};

template<>
struct formatter<tyr::formalism::ArithmeticOperatorKind, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(tyr::formalism::ArithmeticOperatorKind value, FormatContext& ctx) const
    { return fmt::format_to(ctx.out(), "{}", tyr::formalism::to_string(value)); }
};

template<>
struct formatter<tyr::formalism::NumericEffectOperatorKind, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(tyr::formalism::NumericEffectOperatorKind value, FormatContext& ctx) const
    { return fmt::format_to(ctx.out(), "{}", tyr::formalism::to_string(value)); }
};

template<typename T>
struct formatter<urd::data<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const urd::data<T>& value, FormatContext& ctx) const
    { return fmt::format_to(ctx.out(), "{}", to_string(value)); }
};

template<typename Tag>
struct formatter<ygg::formalism::RelationBindingIndex<Tag, tyr::formalism::ObjectTag>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const ygg::formalism::RelationBindingIndex<Tag, tyr::formalism::ObjectTag>& value, FormatContext& ctx) const
    { return fmt::format_to(ctx.out(), "<{},{}>", value.relation, value.row); }
};

}  // namespace fmt

#endif
