/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/syntax/formatter.hpp"

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
                out = fmt::format_to(out, "{}: {}({}) columns=[{}]", id, Tag::operation, fmt::join(children, ","), fmt::join(columns, ","));
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

std::string explain(const QueryPlan& plan)
{
    fmt::memory_buffer out;
    detail::format_query_plan(std::back_inserter(out), plan);
    return fmt::to_string(out);
}
}

#if YGG_ENABLE_FMT_FORMATTERS
fmt::format_context::iterator fmt::formatter<tyr::datalog::database::QueryPlan>::format(const tyr::datalog::database::QueryPlan& plan,
                                                                                         fmt::format_context& ctx) const
{ return tyr::datalog::database::detail::format_query_plan(ctx.out(), plan); }

template<typename Plan>
    requires boost::mp11::mp_contains<boost::mp11::mp_list<tyr::datalog::database::ProjectionPlan,
                                                           tyr::datalog::database::JoinPlan,
                                                           tyr::datalog::database::SetPlan,
                                                           tyr::datalog::database::SelectEqualPlan,
                                                           tyr::datalog::database::SelectValuePlan,
                                                           tyr::datalog::database::DistancePlan,
                                                           tyr::datalog::database::GenericJoinPlan>,
                                      Plan>::value
fmt::format_context::iterator fmt::formatter<Plan>::format(const Plan& plan, fmt::format_context& ctx) const
{ return fmt::format_to(ctx.out(), "[{}]", fmt::join(plan.output_columns(), ", ")); }

template struct fmt::formatter<tyr::datalog::database::ProjectionPlan>;
template struct fmt::formatter<tyr::datalog::database::JoinPlan>;
template struct fmt::formatter<tyr::datalog::database::SetPlan>;
template struct fmt::formatter<tyr::datalog::database::SelectEqualPlan>;
template struct fmt::formatter<tyr::datalog::database::SelectValuePlan>;
template struct fmt::formatter<tyr::datalog::database::DistancePlan>;
template struct fmt::formatter<tyr::datalog::database::GenericJoinPlan>;
#endif
