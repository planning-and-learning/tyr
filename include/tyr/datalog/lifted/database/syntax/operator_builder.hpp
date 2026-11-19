/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SYNTAX_OPERATOR_BUILDER_HPP_
#define TYR_DATALOG_DATABASE_SYNTAX_OPERATOR_BUILDER_HPP_

#include "tyr/datalog/lifted/database/syntax/query.hpp"

#include <algorithm>
#include <concepts>
#include <span>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>

namespace tyr::datalog::database
{

/// Constructs queries in a repository: checkout, fill, insert_query.
class OperatorBuilder
{
    QueryRepository* m_repository;
    QueryBuilder* m_builder;

public:
    OperatorBuilder(QueryRepository& repository, QueryBuilder& builder) : m_repository(&repository), m_builder(&builder) {}

    QueryRepository& repository() noexcept { return *m_repository; }

    QueryBuilder& builder() noexcept { return *m_builder; }

    template<class Tag, std::invocable<urd::builder<Query<Tag>, QueryRepository>&> Fill>
    QueryView<> make(Fill fill)
    {
        auto data = checkout<Query<Tag>>(*m_builder);
        fill(*data);
        return insert_query(*m_repository, *m_builder, *data);
    }

    QueryView<> empty(std::span<const ygg::ColumnLayout> columns)
    {
        return make<QueryEmptyTag>([&](auto& data) { data.columns.assign(columns.begin(), columns.end()); });
    }

    template<class Tag>
    QueryView<> binary(QueryView<> left_operand, QueryView<> right_operand)
    {
        return make<Tag>(
            [&](auto& data)
            {
                data.left_operand = left_operand;
                data.right_operand = right_operand;
            });
    }

    template<class Tag>
    QueryView<> relabel(QueryView<> arg, std::span<const urd::index<ygg::Column>> labels)
    {
        return make<Tag>(
            [&](auto& data)
            {
                data.operand = arg;
                data.labels.assign(labels.begin(), labels.end());
            });
    }

    QueryView<> join(QueryView<> left_operand, QueryView<> right_operand) { return binary<QueryJoinTag>(left_operand, right_operand); }

    /// The source's operation over schema-identical children of this repository.
    QueryView<> clone(QueryView<> source, std::span<const QueryView<>> children) { return detail::clone_query(source, children, *m_repository, *m_builder); }

    /// Keeps the query's columns with the given labels, in that order.
    QueryView<> project(QueryView<> query, std::span<const urd::index<ygg::Column>> labels)
    {
        if (std::ranges::equal(columns_of(query), labels, {}, &ygg::ColumnLayout::label))
            return query;
        return relabel<QueryProjectTag>(query, labels);
    }

    /// Keeps the query's columns whose labels occur in the sorted set, in schema order.
    QueryView<> restrict(QueryView<> query, std::span<const urd::index<ygg::Column>> keep)
    {
        urd::Store<urd::sequence<urd::index<ygg::Column>>> labels;
        for (const auto& column : columns_of(query))
            if (std::ranges::binary_search(keep, column.label))
                labels.push_back(column.label);
        return project(query, labels);
    }

    /// Generic Join over the atoms; the variable order is also the output order.
    QueryView<> generic_join(std::span<const QueryView<>> atoms, std::span<const urd::index<ygg::Column>> order);
};

}  // namespace tyr::datalog::database

#endif
