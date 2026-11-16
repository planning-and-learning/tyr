/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_SYNTAX_QUERY_CONSTRUCTION_HPP_
#define TYR_DATALOG_DATABASE_SYNTAX_QUERY_CONSTRUCTION_HPP_

#include "tyr/datalog/lifted/database/syntax/query_repository.hpp"
#include "yggdrasil/formalism/builder.hpp"
#include "yggdrasil/formalism/interning.hpp"

#include <algorithm>
#include <concepts>
#include <span>
#include <stdexcept>
#include <vector>

namespace tyr::datalog::database
{
using QueryBuilder = boost::mp11::mp_apply<::ygg::formalism::BuilderStorage, QuerySymbolTypes>;

using ::ygg::formalism::checkout;
using ::ygg::formalism::insert;

namespace detail
{
std::span<const ygg::ColumnLayout> child_columns(urd::index<Query<>> child, const QueryRepository& repository);

/// The derived field of a query (its plan, or a rename's columns) from its other fields and its children's columns:
/// what its creator computes before it is interned. Inputs and empty relations derive nothing.
template<typename Tag>
    requires(std::same_as<Tag, QueryInputTag> || std::same_as<Tag, QueryEmptyTag>)
void derive(urd::data<Query<Tag>>& /*data*/, const QueryRepository& /*repository*/)
{
}

template<typename Tag>
    requires boost::mp11::mp_contains<QueryJoinTags, Tag>::value
void derive(urd::data<Query<Tag>>& data, const QueryRepository& repository)
{ data.plan = JoinPlan(child_columns(data.left_operand, repository), child_columns(data.right_operand, repository)); }

template<typename Tag>
    requires boost::mp11::mp_contains<QuerySetTags, Tag>::value
void derive(urd::data<Query<Tag>>& data, const QueryRepository& repository)
{ data.plan = SetPlan(child_columns(data.left_operand, repository), child_columns(data.right_operand, repository)); }

void derive(urd::data<Query<QueryProjectTag>>& data, const QueryRepository& repository);

void derive(urd::data<Query<QueryRenameTag>>& data, const QueryRepository& repository);

void derive(urd::data<Query<QuerySelectEqualTag>>& data, const QueryRepository& repository);

void derive(urd::data<Query<QuerySelectValueTag>>& data, const QueryRepository& repository);

void derive(urd::data<Query<QueryDistanceTag>>& data, const QueryRepository& repository);

/// An omitted output order is canonicalized to first occurrence order.
void derive(urd::data<Query<QueryGenericJoinTag>>& data, const QueryRepository& repository);
}  // namespace detail

/// A query's columns are valid (an input's, an empty relation's); a record is interned as it is, its derived field
/// included (detail::derive, which its creator runs).
template<typename Tag>
    requires(!std::same_as<Tag, void>)
void prepare_for_insert(QueryRepository& /*repository*/, urd::data<Query<Tag>>& data)
{
    if constexpr (requires { data.columns; } && !std::same_as<Tag, QueryRenameTag>)
        ygg::validate_columns({ data.columns.data(), data.columns.size() });
}

/// The concrete record must already be interned.
void prepare_for_insert(QueryRepository& repository, urd::data<Query<>>& data);

/// Interns a query its creator built from its children and other fields: its derived field (a plan, a rename's
/// columns) is computed from them first (detail::derive). The view, and whether it was new.
template<typename Tag>
    requires(!std::same_as<Tag, void>)
std::pair<QueryView<Tag>, bool> derive_and_insert(QueryRepository& repository, urd::data<Query<Tag>>& data)
{
    detail::derive(data, repository);
    return insert(repository, data);
}

/// Creates a query (derive_and_insert), then interns its heterogeneous root.
template<typename Tag>
QueryView<> insert_query(QueryRepository& repository, QueryBuilder& builder, urd::data<Query<Tag>>& data)
{
    auto root = checkout<Query<>>(builder);
    root->variant = derive_and_insert(repository, data).first.index();
    return insert(repository, *root).first;
}
}  // namespace tyr::datalog::database

#endif
