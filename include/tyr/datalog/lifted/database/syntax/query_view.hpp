/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_SYNTAX_QUERY_VIEW_HPP_
#define TYR_DATALOG_DATABASE_SYNTAX_QUERY_VIEW_HPP_

#include "tyr/datalog/lifted/database/syntax/query_data.hpp"

#include <boost/mp11.hpp>
#include <concepts>
#include <span>
#include <tuple>
#include <urd/elements/children.hpp>
#include <urd/elements/flat.hpp>

namespace urd
{
template<typename Tag, typename V>
struct view_mixin<tyr::datalog::database::Query<Tag>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] auto get_variant() const
        requires std::same_as<Tag, void>
    { return urd::resolve(self().data().variant, self().context()); }
    [[nodiscard]] std::span<const ygg::ColumnLayout> columns() const
    {
        if constexpr (std::same_as<Tag, void>)
            return std::visit([](auto concrete) { return concrete.columns(); }, get_variant());
        else if constexpr (std::same_as<Tag, tyr::datalog::database::QuerySemijoinTag> || std::same_as<Tag, tyr::datalog::database::QueryAntijoinTag>)
            return get_plan().lhs_columns();
        else if constexpr (boost::mp11::mp_contains<tyr::datalog::database::QueryPlannedTags, Tag>::value)
            return get_plan().output_columns();
        else
            return { self().data().columns.data(), self().data().columns.size() };
    }
    [[nodiscard]] auto get_arg() const
        requires boost::mp11::mp_contains<tyr::datalog::database::QueryUnaryTags, Tag>::value
    { return urd::resolve(self().data().arg, self().context()); }
    [[nodiscard]] auto get_lhs() const
        requires boost::mp11::mp_contains<tyr::datalog::database::QueryBinaryTags, Tag>::value
    { return urd::resolve(self().data().lhs, self().context()); }
    [[nodiscard]] auto get_rhs() const
        requires boost::mp11::mp_contains<tyr::datalog::database::QueryBinaryTags, Tag>::value
    { return urd::resolve(self().data().rhs, self().context()); }
    [[nodiscard]] auto get_input_slot() const
        requires std::same_as<Tag, tyr::datalog::database::QueryInputTag>
    { return self().data().input_slot; }
    [[nodiscard]] auto get_labels() const
        requires(std::same_as<Tag, tyr::datalog::database::QueryProjectTag> || std::same_as<Tag, tyr::datalog::database::QueryRenameTag>)
    { return std::span<const urd::index<ygg::Column>>(self().data().labels.data(), self().data().labels.size()); }
    [[nodiscard]] auto get_lhs_column() const
        requires std::same_as<Tag, tyr::datalog::database::QuerySelectEqualTag>
    { return self().data().lhs_column; }
    [[nodiscard]] auto get_rhs_column() const
        requires std::same_as<Tag, tyr::datalog::database::QuerySelectEqualTag>
    { return self().data().rhs_column; }
    [[nodiscard]] auto get_column() const
        requires std::same_as<Tag, tyr::datalog::database::QuerySelectValueTag>
    { return self().data().column; }
    [[nodiscard]] auto get_constant() const
        requires std::same_as<Tag, tyr::datalog::database::QuerySelectValueTag>
    { return self().data().constant; }
    [[nodiscard]] const auto& get_plan() const
        requires boost::mp11::mp_contains<tyr::datalog::database::QueryPlannedTags, Tag>::value
    { return self().data().plan; }
    [[nodiscard]] auto get_inputs() const
        requires std::same_as<Tag, tyr::datalog::database::QueryGenericJoinTag>
    { return std::span<const urd::index<tyr::datalog::database::Query<>>>(self().data().inputs.data(), self().data().inputs.size()); }
    [[nodiscard]] auto get_variable_order() const
        requires std::same_as<Tag, tyr::datalog::database::QueryGenericJoinTag>
    { return std::span<const urd::index<ygg::Column>>(self().data().variable_order.data(), self().data().variable_order.size()); }
    [[nodiscard]] auto get_output_order() const
        requires std::same_as<Tag, tyr::datalog::database::QueryGenericJoinTag>
    { return std::span<const urd::index<ygg::Column>>(self().data().output_order.data(), self().data().output_order.size()); }
};
}  // namespace urd

namespace tyr::datalog::database
{
/// Calls f(QueryView<>) for every operand of a query, in operand order.
template<typename Tag, std::invocable<QueryView<>> F>
void for_each_child(QueryView<Tag> view, F f)
{
    if constexpr (std::same_as<Tag, void>)
        std::visit([&](auto concrete) { for_each_child(concrete, f); }, view.get_variant());
    else
        urd::for_each_child<boost::mp11::mp_list<Query<>>, Query<Tag>>(view.data(), view.context(), f);
}
}  // namespace tyr::datalog::database

#endif
