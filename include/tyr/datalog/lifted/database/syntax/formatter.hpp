/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_SYNTAX_FORMATTER_HPP_
#define TYR_DATALOG_DATABASE_SYNTAX_FORMATTER_HPP_

#include "tyr/datalog/lifted/database/syntax/query.hpp"
#include "yggdrasil/formatting/formatter.hpp"

#include <fmt/ranges.h>
#include <iterator>
#include <ranges>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>

namespace tyr::datalog::database
{
namespace detail
{
template<typename OutputIt>
OutputIt format_query_plan(OutputIt out, const QueryPlan& plan)
{
    // Plain values, so explanations do not depend on the optional public formatters.
    const auto value = [](auto index) { return index.value; };
    for (size_t id = 0; id < plan.node_count(); ++id)
        std::visit(
            [&]<typename Tag>(QueryView<Tag> operation)
            {
                urd::Store<urd::sequence<ygg::uint_t>> children;
                for_each_child(operation, [&](QueryView<> child) { children.push_back(child.index().value); });
                const auto columns =
                    operation.columns()
                    | std::views::transform([](const ygg::ColumnLayout& column) { return fmt::format("{}:{}", column.label.value, column.type); });
                out = fmt::format_to(out, "{}: {}({}) columns=[{}]", id, Tag::name, fmt::join(children, ","), fmt::join(columns, ","));
                if constexpr (std::same_as<Tag, QueryInputTag>)
                    out = fmt::format_to(out, " slot={}", operation.get_input_slot());
                if constexpr (std::same_as<Tag, QueryGenericJoinTag>)
                    out = fmt::format_to(out, " variables=[{}]", fmt::join(operation.get_variable_order() | std::views::transform(value), ","));
                out = fmt::format_to(out, "\n");
            },
            plan[urd::index<Query<>>(ygg::to_uint_t(id))].get_variant());
    return fmt::format_to(out, "roots=[{}]\n", fmt::join(plan.roots() | std::views::transform(value), ","));
}
}  // namespace detail

std::string explain(const QueryPlan& plan);
}  // namespace tyr::datalog::database

#if YGG_ENABLE_FMT_FORMATTERS
namespace fmt
{
template<>
struct formatter<tyr::datalog::database::QueryPlan, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::datalog::database::QueryPlan& plan, FormatContext& ctx) const
    { return tyr::datalog::database::detail::format_query_plan(ctx.out(), plan); }
};
}  // namespace fmt
#endif

#endif
