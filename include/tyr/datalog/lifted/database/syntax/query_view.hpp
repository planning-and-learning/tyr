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
    { return self().template member<&urd::record_of<V>::variant>(); }

    [[nodiscard]] auto get_operand() const
        requires boost::mp11::mp_contains<tyr::datalog::database::QueryUnaryTags, Tag>::value
    { return self().template member<&urd::record_of<V>::operand>(); }

    [[nodiscard]] auto get_left_operand() const
        requires boost::mp11::mp_contains<tyr::datalog::database::QueryBinaryTags, Tag>::value
    { return self().template member<&urd::record_of<V>::left_operand>(); }

    [[nodiscard]] auto get_right_operand() const
        requires boost::mp11::mp_contains<tyr::datalog::database::QueryBinaryTags, Tag>::value
    { return self().template member<&urd::record_of<V>::right_operand>(); }

    [[nodiscard]] auto get_sources() const
        requires std::same_as<Tag, tyr::datalog::database::QueryDistanceTag>
    { return self().template member<&urd::record_of<V>::sources>(); }

    [[nodiscard]] auto get_edges() const
        requires std::same_as<Tag, tyr::datalog::database::QueryDistanceTag>
    { return self().template member<&urd::record_of<V>::edges>(); }

    [[nodiscard]] auto get_targets() const
        requires std::same_as<Tag, tyr::datalog::database::QueryDistanceTag>
    { return self().template member<&urd::record_of<V>::targets>(); }

    [[nodiscard]] auto get_input_slot() const
        requires std::same_as<Tag, tyr::datalog::database::QueryInputTag>
    { return self().template member<&urd::record_of<V>::input_slot>(); }

    [[nodiscard]] auto get_labels() const
        requires(std::same_as<Tag, tyr::datalog::database::QueryProjectTag> || std::same_as<Tag, tyr::datalog::database::QueryRenameTag>)
    {
        return std::span<const urd::index<ygg::Column>>(self().template member<&urd::record_of<V>::labels>().data(),
                                                        self().template member<&urd::record_of<V>::labels>().size());
    }

    [[nodiscard]] std::span<const ygg::ColumnLayout> get_columns() const
        requires(std::same_as<Tag, tyr::datalog::database::QueryInputTag> || std::same_as<Tag, tyr::datalog::database::QueryEmptyTag>
                 || std::same_as<Tag, tyr::datalog::database::QueryRenameTag>)
    { return { self().template member<&urd::record_of<V>::columns>().data(), self().template member<&urd::record_of<V>::columns>().size() }; }

    [[nodiscard]] auto get_left_column() const
        requires std::same_as<Tag, tyr::datalog::database::QuerySelectEqualTag>
    { return self().template member<&urd::record_of<V>::left_column>(); }

    [[nodiscard]] auto get_right_column() const
        requires std::same_as<Tag, tyr::datalog::database::QuerySelectEqualTag>
    { return self().template member<&urd::record_of<V>::right_column>(); }

    [[nodiscard]] auto get_column() const
        requires std::same_as<Tag, tyr::datalog::database::QuerySelectValueTag>
    { return self().template member<&urd::record_of<V>::column>(); }

    [[nodiscard]] auto get_constant() const
        requires std::same_as<Tag, tyr::datalog::database::QuerySelectValueTag>
    { return self().template member<&urd::record_of<V>::constant>(); }

    [[nodiscard]] auto get_constant_type() const
        requires std::same_as<Tag, tyr::datalog::database::QuerySelectValueTag>
    { return self().template member<&urd::record_of<V>::constant_type>(); }

    [[nodiscard]] auto get_distance_column() const
        requires std::same_as<Tag, tyr::datalog::database::QueryDistanceTag>
    { return self().template member<&urd::record_of<V>::distance_column>(); }

    [[nodiscard]] auto get_inputs() const
        requires std::same_as<Tag, tyr::datalog::database::QueryGenericJoinTag>
    { return self().template member<&urd::record_of<V>::inputs>(); }

    [[nodiscard]] auto get_variable_order() const
        requires std::same_as<Tag, tyr::datalog::database::QueryGenericJoinTag>
    {
        return std::span<const urd::index<ygg::Column>>(self().template member<&urd::record_of<V>::variable_order>().data(),
                                                        self().template member<&urd::record_of<V>::variable_order>().size());
    }

    [[nodiscard]] auto get_output_order() const
        requires std::same_as<Tag, tyr::datalog::database::QueryGenericJoinTag>
    {
        return std::span<const urd::index<ygg::Column>>(self().template member<&urd::record_of<V>::output_order>().data(),
                                                        self().template member<&urd::record_of<V>::output_order>().size());
    }

    [[nodiscard]] const auto& get_plan() const
        requires boost::mp11::mp_contains<tyr::datalog::database::QueryPlannedTags, Tag>::value
    { return self().template member<&urd::record_of<V>::plan>(); }

    [[nodiscard]] std::span<const ygg::ColumnLayout> columns() const
        requires(!std::same_as<Tag, void>)
    {
        if constexpr (std::same_as<Tag, tyr::datalog::database::QuerySemijoinTag> || std::same_as<Tag, tyr::datalog::database::QueryAntijoinTag>)
            return get_plan().left_columns();
        else if constexpr (boost::mp11::mp_contains<tyr::datalog::database::QueryPlannedTags, Tag>::value)
            return get_plan().output_columns();
        else
            return get_columns();
    }

    static constexpr auto functions() noexcept
    {
        if constexpr (std::same_as<Tag, void>)
            return urd::Functions<> {};
        else
            return urd::Functions<urd::function<"columns", &view_mixin::columns>> {};
    }
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
        urd::for_each_child<boost::mp11::mp_list<Query<>>, Query<Tag>>(view, f);
}

/// The columns of a query, read through its variant for a query of any operation (its operation's).
template<typename Tag>
[[nodiscard]] std::span<const ygg::ColumnLayout> columns_of(QueryView<Tag> view)
{
    if constexpr (std::same_as<Tag, void>)
        return std::visit([](auto concrete) { return concrete.columns(); }, view.get_variant());
    else
        return view.columns();
}
}  // namespace tyr::datalog::database

#endif
