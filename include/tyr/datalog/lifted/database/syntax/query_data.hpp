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
    using Variant = urd::index_variant<boost::mp11::mp_transform<tyr::datalog::database::Query, tyr::datalog::database::QueryConstructorTags>>;

    urd::index<tyr::datalog::database::Query<>> index;
    Variant variant;

    data() = default;
    explicit data(Variant variant_) : index(), variant(std::move(variant_)) {}

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::variant> {}; }
    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::variant> {}; }
};

template<>
struct data<tyr::datalog::database::Query<tyr::datalog::database::QueryInputTag>>
{
    urd::index<tyr::datalog::database::Query<tyr::datalog::database::QueryInputTag>> index;
    size_t input_slot = 0;
    urd::flat_vector<ygg::ColumnLayout> columns;

    data() = default;
    data(size_t input_slot_, std::span<const ygg::ColumnLayout> columns_) : input_slot(input_slot_) { columns.set(columns_.begin(), columns_.end()); }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::input_slot, &data::columns> {}; }
    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::input_slot, &data::columns> {}; }
};

template<>
struct data<tyr::datalog::database::Query<tyr::datalog::database::QueryEmptyTag>>
{
    urd::index<tyr::datalog::database::Query<tyr::datalog::database::QueryEmptyTag>> index;
    urd::flat_vector<ygg::ColumnLayout> columns;

    data() = default;
    explicit data(std::span<const ygg::ColumnLayout> columns_) { columns.set(columns_.begin(), columns_.end()); }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::columns> {}; }
    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::columns> {}; }
};

// Prepared schemas and positions are serialized but do not define operator identity.
template<typename Tag>
    requires boost::mp11::mp_contains<tyr::datalog::database::QueryJoinTags, Tag>::value
struct data<tyr::datalog::database::Query<Tag>>
{
    urd::index<tyr::datalog::database::Query<Tag>> index;
    urd::index<tyr::datalog::database::Query<>> lhs;
    urd::index<tyr::datalog::database::Query<>> rhs;
    tyr::datalog::database::JoinPlan plan;

    data() = default;
    data(urd::index<tyr::datalog::database::Query<>> lhs_, urd::index<tyr::datalog::database::Query<>> rhs_) : lhs(lhs_), rhs(rhs_) {}

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::lhs, &data::rhs> {}; }
    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::lhs, &data::rhs, &data::plan> {}; }
};

template<typename Tag>
    requires boost::mp11::mp_contains<tyr::datalog::database::QuerySetTags, Tag>::value
struct data<tyr::datalog::database::Query<Tag>>
{
    urd::index<tyr::datalog::database::Query<Tag>> index;
    urd::index<tyr::datalog::database::Query<>> lhs;
    urd::index<tyr::datalog::database::Query<>> rhs;
    tyr::datalog::database::SetPlan plan;

    data() = default;
    data(urd::index<tyr::datalog::database::Query<>> lhs_, urd::index<tyr::datalog::database::Query<>> rhs_) : lhs(lhs_), rhs(rhs_) {}

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::lhs, &data::rhs> {}; }
    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::lhs, &data::rhs, &data::plan> {}; }
};

template<>
struct data<tyr::datalog::database::Query<tyr::datalog::database::QueryProjectTag>>
{
    urd::index<tyr::datalog::database::Query<tyr::datalog::database::QueryProjectTag>> index;
    urd::index<tyr::datalog::database::Query<>> arg;
    urd::flat_vector<urd::index<ygg::Column>> labels;
    tyr::datalog::database::ProjectionPlan plan;

    data() = default;
    data(urd::index<tyr::datalog::database::Query<>> arg_, std::span<const urd::index<ygg::Column>> labels_) : arg(arg_)
    { labels.set(labels_.begin(), labels_.end()); }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::arg, &data::labels> {}; }
    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::arg, &data::labels, &data::plan> {}; }
};

template<>
struct data<tyr::datalog::database::Query<tyr::datalog::database::QueryRenameTag>>
{
    urd::index<tyr::datalog::database::Query<tyr::datalog::database::QueryRenameTag>> index;
    urd::index<tyr::datalog::database::Query<>> arg;
    urd::flat_vector<urd::index<ygg::Column>> labels;
    urd::flat_vector<ygg::ColumnLayout> columns;

    data() = default;
    data(urd::index<tyr::datalog::database::Query<>> arg_, std::span<const urd::index<ygg::Column>> labels_) : arg(arg_)
    { labels.set(labels_.begin(), labels_.end()); }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::arg, &data::labels> {}; }
    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::arg, &data::labels, &data::columns> {}; }
};

template<>
struct data<tyr::datalog::database::Query<tyr::datalog::database::QuerySelectEqualTag>>
{
    urd::index<tyr::datalog::database::Query<tyr::datalog::database::QuerySelectEqualTag>> index;
    urd::index<tyr::datalog::database::Query<>> arg;
    urd::index<ygg::Column> lhs_column;
    urd::index<ygg::Column> rhs_column;
    tyr::datalog::database::SelectEqualPlan plan;

    data() = default;
    data(urd::index<tyr::datalog::database::Query<>> arg_, urd::index<ygg::Column> lhs_column_, urd::index<ygg::Column> rhs_column_) :
        arg(arg_),
        lhs_column(lhs_column_),
        rhs_column(rhs_column_)
    {
    }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::arg, &data::lhs_column, &data::rhs_column> {}; }
    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::arg, &data::lhs_column, &data::rhs_column, &data::plan> {}; }
};

template<>
struct data<tyr::datalog::database::Query<tyr::datalog::database::QuerySelectValueTag>>
{
    urd::index<tyr::datalog::database::Query<tyr::datalog::database::QuerySelectValueTag>> index;
    urd::index<tyr::datalog::database::Query<>> arg;
    urd::index<ygg::Column> column;
    /// The constant's canonical value.
    ygg::Value constant = 0;
    /// The constant's type.
    ygg::TypeId constant_type {};
    tyr::datalog::database::SelectValuePlan plan;

    data() = default;
    /// The constant as its canonical value (encode_value).
    template<ygg::ColumnValueType T>
    data(urd::index<tyr::datalog::database::Query<>> arg_, urd::index<ygg::Column> column_, T value) :
        arg(arg_),
        column(column_),
        constant(ygg::encode_value(value)),
        constant_type(ygg::type_id<T>)
    {
    }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::arg, &data::column, &data::constant, &data::constant_type> {}; }
    static constexpr auto cista_members() noexcept
    { return urd::Members<&data::index, &data::arg, &data::column, &data::constant, &data::constant_type, &data::plan> {}; }
};

template<>
struct data<tyr::datalog::database::Query<tyr::datalog::database::QueryDistanceTag>>
{
    urd::index<tyr::datalog::database::Query<tyr::datalog::database::QueryDistanceTag>> index;
    urd::index<tyr::datalog::database::Query<>> sources;
    urd::index<tyr::datalog::database::Query<>> edges;
    urd::index<tyr::datalog::database::Query<>> targets;
    urd::index<ygg::Column> distance_column;
    tyr::datalog::database::DistancePlan plan;

    data() = default;
    data(urd::index<tyr::datalog::database::Query<>> sources_,
         urd::index<tyr::datalog::database::Query<>> edges_,
         urd::index<tyr::datalog::database::Query<>> targets_,
         urd::index<ygg::Column> distance_column_) :
        sources(sources_),
        edges(edges_),
        targets(targets_),
        distance_column(distance_column_)
    {
    }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::sources, &data::edges, &data::targets, &data::distance_column> {}; }
    static constexpr auto cista_members() noexcept
    { return urd::Members<&data::index, &data::sources, &data::edges, &data::targets, &data::distance_column, &data::plan> {}; }
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
         std::span<const urd::index<ygg::Column>> output_order_ = {}) :
        index()
    {
        inputs.set(inputs_.begin(), inputs_.end());
        variable_order.set(variable_order_.begin(), variable_order_.end());
        output_order.set(output_order_.begin(), output_order_.end());
    }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::inputs, &data::variable_order, &data::output_order> {}; }
    static constexpr auto cista_members() noexcept
    { return urd::Members<&data::index, &data::inputs, &data::variable_order, &data::output_order, &data::plan> {}; }
};
}  // namespace urd

#endif
