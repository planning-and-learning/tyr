/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_SYNTAX_QUERY_DATA_HPP_
#define TYR_DATALOG_DATABASE_SYNTAX_QUERY_DATA_HPP_

#include "tyr/datalog/lifted/database/declarations.hpp"
#include "tyr/datalog/lifted/database/semantics/distance.hpp"
#include "tyr/datalog/lifted/database/semantics/generic_join.hpp"
#include "yggdrasil/formalism/values.hpp"

#include <array>
#include <boost/mp11.hpp>
#include <cista/containers/variant.h>
#include <cista/containers/vector.h>
#include <concepts>
#include <cstddef>
#include <ranges>
#include <span>
#include <tuple>
#include <type_traits>
#include <urd/elements/flat.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace urd
{
template<>
struct data<tyr::datalog::database::Query<>>
{
    urd::index<tyr::datalog::database::Query<>> index;
    urd::index_variant<boost::mp11::mp_transform<tyr::datalog::database::Query, tyr::datalog::database::QueryConstructorTags>> variant;

    data() = default;

    explicit data(urd::index_variant<boost::mp11::mp_transform<tyr::datalog::database::Query, tyr::datalog::database::QueryConstructorTags>> variant_) :
        index(),
        variant(std::move(variant_))
    {
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"variant", &data::variant>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::variant> {}; }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::variant> {}; }
};

template<>
struct data<tyr::datalog::database::Query<tyr::datalog::database::QueryInputTag>>
{
    urd::index<tyr::datalog::database::Query<tyr::datalog::database::QueryInputTag>> index;
    size_t input_slot = 0;
    urd::flat_vector<urd::data<ygg::ColumnLayout>> columns;

    data() = default;

    data(size_t input_slot_, std::span<const urd::data<ygg::ColumnLayout>> columns_) : input_slot(input_slot_)
    { columns.set(columns_.begin(), columns_.end()); }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"input_slot", &data::input_slot>, urd::field<"columns", &data::columns>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::input_slot, &data::columns> {}; }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::input_slot, &data::columns> {}; }
};

template<>
struct data<tyr::datalog::database::Query<tyr::datalog::database::QueryEmptyTag>>
{
    urd::index<tyr::datalog::database::Query<tyr::datalog::database::QueryEmptyTag>> index;
    urd::flat_vector<urd::data<ygg::ColumnLayout>> columns;

    data() = default;

    explicit data(std::span<const urd::data<ygg::ColumnLayout>> columns_) { columns.set(columns_.begin(), columns_.end()); }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"columns", &data::columns>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::columns> {}; }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::columns> {}; }
};

// Prepared schemas and positions are serialized but do not define operator identity.
template<typename Tag>
    requires boost::mp11::mp_contains<tyr::datalog::database::QueryJoinTags, Tag>::value
struct data<tyr::datalog::database::Query<Tag>>
{
    urd::index<tyr::datalog::database::Query<Tag>> index;
    urd::index<tyr::datalog::database::Query<>> left_operand;
    urd::index<tyr::datalog::database::Query<>> right_operand;
    tyr::datalog::database::JoinPlan plan;

    data() = default;

    data(urd::index<tyr::datalog::database::Query<>> left_operand_,
         urd::index<tyr::datalog::database::Query<>> right_operand_,
         tyr::datalog::database::JoinPlan plan_) :
        left_operand(left_operand_),
        right_operand(right_operand_),
        plan(std::move(plan_))
    {
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    {
        return urd::
            Fields<urd::field<"left_operand", &data::left_operand>, urd::field<"right_operand", &data::right_operand>, urd::field<"plan", &data::plan>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::left_operand, &data::right_operand> {}; }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::left_operand, &data::right_operand, &data::plan> {}; }
};

template<typename Tag>
    requires boost::mp11::mp_contains<tyr::datalog::database::QuerySetTags, Tag>::value
struct data<tyr::datalog::database::Query<Tag>>
{
    urd::index<tyr::datalog::database::Query<Tag>> index;
    urd::index<tyr::datalog::database::Query<>> left_operand;
    urd::index<tyr::datalog::database::Query<>> right_operand;
    tyr::datalog::database::SetPlan plan;

    data() = default;

    data(urd::index<tyr::datalog::database::Query<>> left_operand_,
         urd::index<tyr::datalog::database::Query<>> right_operand_,
         tyr::datalog::database::SetPlan plan_) :
        left_operand(left_operand_),
        right_operand(right_operand_),
        plan(std::move(plan_))
    {
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    {
        return urd::
            Fields<urd::field<"left_operand", &data::left_operand>, urd::field<"right_operand", &data::right_operand>, urd::field<"plan", &data::plan>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::left_operand, &data::right_operand> {}; }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::left_operand, &data::right_operand, &data::plan> {}; }
};

template<>
struct data<tyr::datalog::database::Query<tyr::datalog::database::QueryProjectTag>>
{
    urd::index<tyr::datalog::database::Query<tyr::datalog::database::QueryProjectTag>> index;
    urd::index<tyr::datalog::database::Query<>> operand;
    urd::flat_vector<urd::index<ygg::Column>> labels;
    tyr::datalog::database::ProjectionPlan plan;

    data() = default;

    data(urd::index<tyr::datalog::database::Query<>> operand_, std::span<const urd::index<ygg::Column>> labels_, tyr::datalog::database::ProjectionPlan plan_) :
        operand(operand_),
        plan(std::move(plan_))
    { labels.set(labels_.begin(), labels_.end()); }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    { return urd::Fields<urd::field<"operand", &data::operand>, urd::field<"labels", &data::labels>, urd::field<"plan", &data::plan>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::operand, &data::labels> {}; }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::operand, &data::labels, &data::plan> {}; }
};

template<>
struct data<tyr::datalog::database::Query<tyr::datalog::database::QueryRenameTag>>
{
    urd::index<tyr::datalog::database::Query<tyr::datalog::database::QueryRenameTag>> index;
    urd::index<tyr::datalog::database::Query<>> operand;
    urd::flat_vector<urd::index<ygg::Column>> labels;
    urd::flat_vector<urd::data<ygg::ColumnLayout>> columns;

    data() = default;

    data(urd::index<tyr::datalog::database::Query<>> operand_,
         std::span<const urd::index<ygg::Column>> labels_,
         std::span<const urd::data<ygg::ColumnLayout>> columns_) :
        operand(operand_)
    {
        labels.set(labels_.begin(), labels_.end());
        columns.set(columns_.begin(), columns_.end());
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    { return urd::Fields<urd::field<"operand", &data::operand>, urd::field<"labels", &data::labels>, urd::field<"columns", &data::columns>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::operand, &data::labels> {}; }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::operand, &data::labels, &data::columns> {}; }
};

template<>
struct data<tyr::datalog::database::Query<tyr::datalog::database::QuerySelectEqualTag>>
{
    urd::index<tyr::datalog::database::Query<tyr::datalog::database::QuerySelectEqualTag>> index;
    urd::index<tyr::datalog::database::Query<>> operand;
    urd::index<ygg::Column> left_column;
    urd::index<ygg::Column> right_column;
    tyr::datalog::database::SelectEqualPlan plan;

    data() = default;

    data(urd::index<tyr::datalog::database::Query<>> operand_,
         urd::index<ygg::Column> left_column_,
         urd::index<ygg::Column> right_column_,
         tyr::datalog::database::SelectEqualPlan plan_) :
        operand(operand_),
        left_column(left_column_),
        right_column(right_column_),
        plan(std::move(plan_))
    {
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"operand", &data::operand>,
                           urd::field<"left_column", &data::left_column>,
                           urd::field<"right_column", &data::right_column>,
                           urd::field<"plan", &data::plan>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::operand, &data::left_column, &data::right_column> {}; }

    static constexpr auto cista_members() noexcept
    { return urd::Members<&data::index, &data::operand, &data::left_column, &data::right_column, &data::plan> {}; }
};

template<>
struct data<tyr::datalog::database::Query<tyr::datalog::database::QuerySelectValueTag>>
{
    urd::index<tyr::datalog::database::Query<tyr::datalog::database::QuerySelectValueTag>> index;
    urd::index<tyr::datalog::database::Query<>> operand;
    urd::index<ygg::Column> column;
    /// The constant's canonical value.
    ygg::Value constant = 0;
    /// The constant's type.
    ygg::TypeId constant_type {};
    tyr::datalog::database::SelectValuePlan plan;

    data() = default;

    data(urd::index<tyr::datalog::database::Query<>> operand_,
         urd::index<ygg::Column> column_,
         ygg::Value constant_,
         ygg::TypeId constant_type_,
         tyr::datalog::database::SelectValuePlan plan_) :
        operand(operand_),
        column(column_),
        constant(constant_),
        constant_type(constant_type_),
        plan(std::move(plan_))
    {
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"operand", &data::operand>,
                           urd::field<"column", &data::column>,
                           urd::field<"constant", &data::constant>,
                           urd::field<"constant_type", &data::constant_type>,
                           urd::field<"plan", &data::plan>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::operand, &data::column, &data::constant, &data::constant_type> {}; }

    static constexpr auto cista_members() noexcept
    { return urd::Members<&data::index, &data::operand, &data::column, &data::constant, &data::constant_type, &data::plan> {}; }
};

template<>
struct data<tyr::datalog::database::Query<tyr::datalog::database::QueryGenericJoinTag>>
{
    urd::index<tyr::datalog::database::Query<tyr::datalog::database::QueryGenericJoinTag>> index;
    urd::flat_vector<urd::index<tyr::datalog::database::Query<>>> inputs;
    urd::flat_vector<urd::index<ygg::Column>> variable_order;
    urd::flat_vector<urd::index<ygg::Column>> output_order;
    tyr::datalog::database::GenericJoinPlan plan;

    data() = default;

    /// An empty output order is canonicalized on insertion.
    data(std::span<const urd::index<tyr::datalog::database::Query<>>> inputs_,
         std::span<const urd::index<ygg::Column>> variable_order_,
         std::span<const urd::index<ygg::Column>> output_order_,
         tyr::datalog::database::GenericJoinPlan plan_) :
        plan(std::move(plan_))
    {
        inputs.set(inputs_.begin(), inputs_.end());
        variable_order.set(variable_order_.begin(), variable_order_.end());
        output_order.set(output_order_.begin(), output_order_.end());
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"inputs", &data::inputs>,
                           urd::field<"variable_order", &data::variable_order>,
                           urd::field<"output_order", &data::output_order>,
                           urd::field<"plan", &data::plan>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::inputs, &data::variable_order, &data::output_order> {}; }

    static constexpr auto cista_members() noexcept
    { return urd::Members<&data::index, &data::inputs, &data::variable_order, &data::output_order, &data::plan> {}; }
};
}  // namespace urd

#endif
