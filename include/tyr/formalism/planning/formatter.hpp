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

#ifndef TYR_FORMALISM_PLANNING_FORMATTER_HPP_
#define TYR_FORMALISM_PLANNING_FORMATTER_HPP_

#include "tyr/formalism/formatter.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"

#include <fmt/format.h>
#include <string>
#include <utility>

namespace tyr::formalism::planning
{
struct PlanFormatting
{
};

std::string to_string(const PlanningDomain& value);
std::string to_string(const PlanningTask<LiftedTag>& value);
std::string to_string(const PlanningTask<GroundTag>& value);

}  // namespace tyr::formalism::planning

namespace fmt
{

template<>
struct formatter<tyr::formalism::planning::PlanningDomain, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::formalism::planning::PlanningDomain& value, FormatContext& ctx) const
    { return fmt::format_to(ctx.out(), "{}", tyr::formalism::planning::to_string(value)); }
};

template<tyr::TaskKind T>
struct formatter<tyr::formalism::planning::PlanningTask<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::formalism::planning::PlanningTask<T>& value, FormatContext& ctx) const
    { return fmt::format_to(ctx.out(), "{}", tyr::formalism::planning::to_string(value)); }
};

}  // namespace fmt

namespace tyr::formalism::planning
{

std::string to_string(const urd::data<FDRVariable<FluentTag>>& value);

std::string to_string(const urd::data<Invariant>& value);
std::string to_string(const urd::data<FDRFact<FluentTag>>& value);

std::string to_string(const urd::data<ConjunctiveCondition<LiftedTag>>& value);
std::string to_string(const urd::data<ConjunctiveCondition<GroundTag>>& value);
std::string to_string(const urd::data<ConditionalEffect<LiftedTag>>& value);
std::string to_string(const urd::data<ConditionalEffect<GroundTag>>& value);
std::string to_string(const urd::data<ConjunctiveEffect<LiftedTag>>& value);
std::string to_string(const urd::data<ConjunctiveEffect<GroundTag>>& value);
std::string to_string(const urd::data<Action<LiftedTag>>& value);
std::string to_string(const urd::data<Action<GroundTag>>& value);
std::string to_string(const urd::data<Axiom<LiftedTag>>& value);
std::string to_string(const urd::data<Axiom<GroundTag>>& value);
std::string to_string(const urd::data<Metric>& value);
std::string to_string(const urd::data<Task<LiftedTag>>& value);
std::string to_string(const urd::data<Domain>& value);
std::string to_string(const urd::data<Task<GroundTag>>& value);
std::string to_string(const urd::data<ygg::Row<Action<LiftedTag>>>& value);
std::string to_string(const urd::data<ygg::Row<Axiom<LiftedTag>>>& value);

/// Each symbol type's view (SymbolRepositoryTypes) and each relation's binding view (RelationRepositoryTypes) as text;
/// instantiated for these lists in formatter.cpp.
template<typename T>
    requires boost::mp11::mp_contains<SymbolRepositoryTypes, T>::value
std::string to_string(urd::index_view<T, Repository> value);
template<typename R>
    requires boost::mp11::mp_contains<RelationRepositoryTypes, R>::value
std::string to_string(ygg::RowView<R, Repository> value);
std::string to_string(TermView value);
std::string to_string(ActionRowDataView value);

std::string to_string(FDRFactView<FluentTag> value);
std::string to_string(urd::owning_view<FDRFact<FluentTag>, Repository> value);

}  // namespace tyr::formalism::planning

namespace fmt
{

/// Every view through the repository prints as its to_string: urd's (of an index or a held record), and binding views.
template<template<typename, typename, typename...> typename V, typename H, typename... Rest>
    requires requires(const V<H, tyr::formalism::planning::Repository, Rest...>& value) { tyr::formalism::planning::to_string(value); }
struct formatter<V<H, tyr::formalism::planning::Repository, Rest...>, char>
{
    using View = V<H, tyr::formalism::planning::Repository, Rest...>;

    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const View& value, FormatContext& ctx) const
    { return fmt::format_to(ctx.out(), "{}", tyr::formalism::planning::to_string(value)); }
};

template<>
struct formatter<tyr::formalism::planning::FDRValue, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::formalism::planning::FDRValue& value, FormatContext& ctx) const
    { return fmt::format_to(ctx.out(), "{}", value.value); }
};

template<>
struct formatter<tyr::formalism::OptimizationDirection, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(tyr::formalism::OptimizationDirection value, FormatContext& ctx) const
    { return fmt::format_to(ctx.out(), "{}", tyr::formalism::to_string(value)); }
};

}  // namespace fmt

namespace tyr::formalism::planning
{

std::string to_string(const std::pair<ActionRowView, PlanFormatting>& value);
std::string to_string(const std::pair<ActionView<GroundTag>, PlanFormatting>& value);

}  // namespace tyr::formalism::planning

namespace fmt
{

template<typename View>
struct formatter<std::pair<View, tyr::formalism::planning::PlanFormatting>, char>
{
private:
    using Value = std::pair<View, tyr::formalism::planning::PlanFormatting>;

public:
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const Value& value, FormatContext& ctx) const
    { return fmt::format_to(ctx.out(), "{}", tyr::formalism::planning::to_string(value)); }
};

}  // namespace fmt

#endif
