/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/syntax/query_construction.hpp"

#include "tyr/datalog/lifted/database/detail/repository_instantiations.hpp"

#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::datalog::database::detail
{
std::span<const ygg::ColumnLayout> child_columns(urd::index<Query<>> child, const QueryRepository& repository)
{
    if (child.value >= repository.size())
        throw std::invalid_argument("Query: children must precede their parent.");
    return repository.view(child).columns();
}
}

namespace tyr::datalog::database::detail
{
void prepare(urd::data<Query<QueryProjectTag>>& data, const QueryRepository& repository)
{ data.plan = ProjectionPlan(child_columns(data.arg, repository), { data.labels.data(), data.labels.size() }); }
}

namespace tyr::datalog::database::detail
{
void prepare(urd::data<Query<QueryRenameTag>>& data, const QueryRepository& repository)
{
    const auto schema = child_columns(data.arg, repository);
    if (data.labels.size() != schema.size())
        throw std::invalid_argument("Columns: renaming must preserve arity.");
    data.columns.set(schema.begin(), schema.end());
    for (size_t i = 0; i < schema.size(); ++i)
        data.columns[i].label = data.labels[i];
    ygg::validate_columns({ data.columns.data(), data.columns.size() });
}
}

namespace tyr::datalog::database::detail
{
void prepare(urd::data<Query<QuerySelectEqualTag>>& data, const QueryRepository& repository)
{ data.plan = SelectEqualPlan(child_columns(data.arg, repository), data.lhs_column, data.rhs_column); }
}

namespace tyr::datalog::database::detail
{
void prepare(urd::data<Query<QuerySelectValueTag>>& data, const QueryRepository& repository)
{ data.plan = SelectValuePlan(child_columns(data.arg, repository), data.column, data.constant_type, data.constant); }
}

namespace tyr::datalog::database::detail
{
void prepare(urd::data<Query<QueryGenericJoinTag>>& data, const QueryRepository& repository)
{
    urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::ColumnLayout>>>> schemas;
    for (const auto child : data.inputs)
    {
        const auto schema = child_columns(child, repository);
        schemas.emplace_back(schema.begin(), schema.end());
    }
    if (data.output_order.empty())
        for (const auto& schema : schemas)
            for (const auto& column : schema)
                if (std::ranges::find(data.output_order, column.label) == data.output_order.end())
                    data.output_order.push_back(column.label);
    data.plan = GenericJoinPlan(schemas, { data.variable_order.data(), data.variable_order.size() }, { data.output_order.data(), data.output_order.size() });
}
}

namespace tyr::datalog::database
{
void prepare_for_insert(QueryRepository& repository, urd::data<Query<>>& data)
{
    data.variant.apply(
        [&]<typename Tag>(urd::index<Query<Tag>> index)
        {
            if (index.value >= repository.template size<Query<Tag>>())
                throw std::invalid_argument("Query: root refers to a missing query.");
        });
}
}
