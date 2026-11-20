/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_DECLARATIONS_HPP_
#define TYR_DATALOG_DATABASE_DECLARATIONS_HPP_

#include "yggdrasil/formalism/declarations.hpp"
#include "yggdrasil/formalism/free_relations.hpp"
#include "yggdrasil/formalism/values.hpp"

#include <boost/mp11.hpp>
#include <string_view>
#include <urd/elements/concepts.hpp>
#include <urd/elements/flat.hpp>

namespace tyr::datalog::database
{
/// The query operations. Each tag's name is its type's (urd::name: Query<QueryJoinTag> is QueryJoin); its operation,
/// the word a query plan is written with.
struct QueryInputTag
{
    static constexpr std::string_view name = "Input";
    static constexpr std::string_view operation = "input";
};

struct QueryEmptyTag
{
    static constexpr std::string_view name = "Empty";
    static constexpr std::string_view operation = "empty";
};

struct QueryJoinTag
{
    static constexpr std::string_view name = "Join";
    static constexpr std::string_view operation = "hash_join";
};

struct QueryProjectTag
{
    static constexpr std::string_view name = "Project";
    static constexpr std::string_view operation = "project";
};

struct QueryRenameTag
{
    static constexpr std::string_view name = "Rename";
    static constexpr std::string_view operation = "rename";
};

struct QuerySelectEqualTag
{
    static constexpr std::string_view name = "SelectEqual";
    static constexpr std::string_view operation = "select_equal";
};

struct QuerySelectValueTag
{
    static constexpr std::string_view name = "SelectValue";
    static constexpr std::string_view operation = "select_value";
};

struct QueryUnionTag
{
    static constexpr std::string_view name = "Union";
    static constexpr std::string_view operation = "union";
};

struct QueryDifferenceTag
{
    static constexpr std::string_view name = "Difference";
    static constexpr std::string_view operation = "difference";
};

struct QueryGenericJoinTag
{
    static constexpr std::string_view name = "GenericJoin";
    static constexpr std::string_view operation = "generic_join";
};

/// R ⋉ S: the rows of R with a match in S on their shared columns.
struct QuerySemijoinTag
{
    static constexpr std::string_view name = "Semijoin";
    static constexpr std::string_view operation = "semijoin";
};

/// R ▷ S: the rows of R without a match in S on their shared columns.
struct QueryAntijoinTag
{
    static constexpr std::string_view name = "Antijoin";
    static constexpr std::string_view operation = "antijoin";
};

using QueryConstructorTags = boost::mp11::mp_list<QueryInputTag,
                                                  QueryEmptyTag,
                                                  QueryJoinTag,
                                                  QueryProjectTag,
                                                  QueryRenameTag,
                                                  QuerySelectEqualTag,
                                                  QuerySelectValueTag,
                                                  QueryUnionTag,
                                                  QueryDifferenceTag,
                                                  QueryGenericJoinTag,
                                                  QuerySemijoinTag,
                                                  QueryAntijoinTag>;

/// Query tags grouped by the shape of their records.
using QueryJoinTags = boost::mp11::mp_list<QueryJoinTag, QuerySemijoinTag, QueryAntijoinTag>;  ///< left_operand, right_operand and a JoinPlan
using QuerySetTags = boost::mp11::mp_list<QueryUnionTag, QueryDifferenceTag>;                  ///< left_operand, right_operand and a SetPlan
using QueryBinaryTags = boost::mp11::mp_append<QueryJoinTags, QuerySetTags>;                   ///< left_operand and right_operand
using QueryUnaryTags = boost::mp11::mp_list<QueryProjectTag, QueryRenameTag, QuerySelectEqualTag, QuerySelectValueTag>;  ///< operand
/// The queries with a prepared plan.
using QueryPlannedTags =
    boost::mp11::mp_append<QueryJoinTags, QuerySetTags, boost::mp11::mp_list<QueryProjectTag, QuerySelectEqualTag, QuerySelectValueTag, QueryGenericJoinTag>>;

/// Tag void is the type-erased query holding one of the concrete query records.
template<typename Tag = void>
    requires(std::is_void_v<Tag> || boost::mp11::mp_contains<QueryConstructorTags, Tag>::value)
struct Query
{
    static constexpr std::string_view name = urd::joined_name<"Query", Tag::name>;
};

/// The query of any operation is named Query: its tag's argument, void, has no name.
template<>
struct Query<void>
{
    static constexpr std::string_view name = "Query";
};
class QueryPlan;
class QueryRepository;
template<typename Tag = void>
using QueryView = urd::index_view<Query<Tag>, QueryRepository>;

}  // namespace tyr::datalog::database

#endif
