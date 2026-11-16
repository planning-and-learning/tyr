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
#include <string>

namespace tyr::formalism::datalog
{

std::string to_string(const VariableDependencyGraph& value);

}  // namespace tyr::formalism::datalog

namespace fmt
{

template<>
struct formatter<tyr::formalism::datalog::VariableDependencyGraph, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::formalism::datalog::VariableDependencyGraph& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", tyr::formalism::datalog::to_string(value));
    }
};

}  // namespace fmt

namespace tyr::formalism::datalog
{

std::string to_string(const urd::data<ygg::Row<Rule<LiftedTag>>>& value);

/// Each symbol type's view (SymbolRepositoryTypes) and each relation's binding view (RelationRepositoryTypes) as text;
/// instantiated for these lists in formatter.cpp.
template<typename T>
    requires boost::mp11::mp_contains<SymbolRepositoryTypes, T>::value
std::string to_string(urd::index_view<T, Repository> value);
template<typename R>
    requires boost::mp11::mp_contains<RelationRepositoryTypes, R>::value
std::string to_string(ygg::RowView<R, Repository> value);
std::string to_string(TermView value);

}  // namespace tyr::formalism::datalog

namespace fmt
{

/// Every view through the repository prints as its to_string: urd's (of an index or a held record), and binding views.
template<typename T, typename K>
    requires requires(const urd::index_view<T, tyr::formalism::datalog::Repository, K>& value) { tyr::formalism::datalog::to_string(value); }
struct formatter<urd::index_view<T, tyr::formalism::datalog::Repository, K>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    auto format(const urd::index_view<T, tyr::formalism::datalog::Repository, K>& value, format_context& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", tyr::formalism::datalog::to_string(value));
    }
};
template<typename T, typename R>
    requires requires(const urd::borrowed_view<T, tyr::formalism::datalog::Repository, R>& value) { tyr::formalism::datalog::to_string(value); }
struct formatter<urd::borrowed_view<T, tyr::formalism::datalog::Repository, R>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    auto format(const urd::borrowed_view<T, tyr::formalism::datalog::Repository, R>& value, format_context& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", tyr::formalism::datalog::to_string(value));
    }
};
template<typename T, typename R>
    requires requires(const urd::owning_view<T, tyr::formalism::datalog::Repository, R>& value) { tyr::formalism::datalog::to_string(value); }
struct formatter<urd::owning_view<T, tyr::formalism::datalog::Repository, R>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    auto format(const urd::owning_view<T, tyr::formalism::datalog::Repository, R>& value, format_context& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", tyr::formalism::datalog::to_string(value));
    }
};
template<typename T, typename R>
    requires requires(const urd::shared_view<T, tyr::formalism::datalog::Repository, R>& value) { tyr::formalism::datalog::to_string(value); }
struct formatter<urd::shared_view<T, tyr::formalism::datalog::Repository, R>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    auto format(const urd::shared_view<T, tyr::formalism::datalog::Repository, R>& value, format_context& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", tyr::formalism::datalog::to_string(value));
    }
};

}  // namespace fmt

#endif
