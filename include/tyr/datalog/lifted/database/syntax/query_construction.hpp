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

template<typename Tag>
    requires(std::same_as<Tag, QueryInputTag> || std::same_as<Tag, QueryEmptyTag>)
void prepare(urd::data<Query<Tag>>& data, const QueryRepository&)
{ ygg::validate_columns({ data.columns.data(), data.columns.size() }); }

template<typename Tag>
    requires boost::mp11::mp_contains<QueryJoinTags, Tag>::value
void prepare(urd::data<Query<Tag>>& data, const QueryRepository& repository)
{ data.plan = JoinPlan(child_columns(data.lhs, repository), child_columns(data.rhs, repository)); }

template<typename Tag>
    requires boost::mp11::mp_contains<QuerySetTags, Tag>::value
void prepare(urd::data<Query<Tag>>& data, const QueryRepository& repository)
{ data.plan = SetPlan(child_columns(data.lhs, repository), child_columns(data.rhs, repository)); }

void prepare(urd::data<Query<QueryProjectTag>>& data, const QueryRepository& repository);

void prepare(urd::data<Query<QueryRenameTag>>& data, const QueryRepository& repository);

void prepare(urd::data<Query<QuerySelectEqualTag>>& data, const QueryRepository& repository);

void prepare(urd::data<Query<QuerySelectValueTag>>& data, const QueryRepository& repository);

void prepare(urd::data<Query<QueryDistanceTag>>& data, const QueryRepository& repository);

/// An omitted output order is canonicalized to first occurrence order.
void prepare(urd::data<Query<QueryGenericJoinTag>>& data, const QueryRepository& repository);
}  // namespace detail

/// Validates operands and derives the prepared metadata stored in the record.
template<typename Tag>
    requires(!std::same_as<Tag, void>)
void prepare_for_insert(QueryRepository& repository, urd::data<Query<Tag>>& data)
{ detail::prepare(data, repository); }

/// The concrete record must already be interned.
void prepare_for_insert(QueryRepository& repository, urd::data<Query<>>& data);

/// Interns a concrete query and then its heterogeneous root.
template<typename Tag>
QueryView<> insert_query(QueryRepository& repository, QueryBuilder& builder, urd::data<Query<Tag>>& data)
{
    auto root = checkout<Query<>>(builder);
    root->variant = insert(repository, data).first.index();
    return insert(repository, *root).first;
}
}  // namespace tyr::datalog::database

#endif
