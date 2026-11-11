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
#include <urd/elements/flat.hpp>

namespace tyr::datalog::database
{
struct QueryInputTag
{
    static constexpr std::string_view name = "input";
};
struct QueryEmptyTag
{
    static constexpr std::string_view name = "empty";
};
struct QueryJoinTag
{
    static constexpr std::string_view name = "hash_join";
};
struct QueryProjectTag
{
    static constexpr std::string_view name = "project";
};
struct QueryRenameTag
{
    static constexpr std::string_view name = "rename";
};
struct QuerySelectEqualTag
{
    static constexpr std::string_view name = "select_equal";
};
struct QuerySelectValueTag
{
    static constexpr std::string_view name = "select_value";
};
struct QueryUnionTag
{
    static constexpr std::string_view name = "union";
};
struct QueryDifferenceTag
{
    static constexpr std::string_view name = "difference";
};
struct QueryDistanceTag
{
    static constexpr std::string_view name = "distance";
};
struct QueryGenericJoinTag
{
    static constexpr std::string_view name = "generic_join";
};
/// R ⋉ S: the rows of R with a match in S on their shared columns.
struct QuerySemijoinTag
{
    static constexpr std::string_view name = "semijoin";
};
/// R ▷ S: the rows of R without a match in S on their shared columns.
struct QueryAntijoinTag
{
    static constexpr std::string_view name = "antijoin";
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
                                                  QueryDistanceTag,
                                                  QueryGenericJoinTag,
                                                  QuerySemijoinTag,
                                                  QueryAntijoinTag>;

/// Query tags grouped by the shape of their records.
using QueryJoinTags = boost::mp11::mp_list<QueryJoinTag, QuerySemijoinTag, QueryAntijoinTag>;                            ///< lhs, rhs and a JoinPlan
using QuerySetTags = boost::mp11::mp_list<QueryUnionTag, QueryDifferenceTag>;                                            ///< lhs, rhs and a SetPlan
using QueryBinaryTags = boost::mp11::mp_append<QueryJoinTags, QuerySetTags>;                                             ///< lhs and rhs
using QueryUnaryTags = boost::mp11::mp_list<QueryProjectTag, QueryRenameTag, QuerySelectEqualTag, QuerySelectValueTag>;  ///< arg
/// The queries with a prepared plan.
using QueryPlannedTags =
    boost::mp11::mp_append<QueryJoinTags,
                           QuerySetTags,
                           boost::mp11::mp_list<QueryProjectTag, QuerySelectEqualTag, QuerySelectValueTag, QueryDistanceTag, QueryGenericJoinTag>>;

/// Tag void is the type-erased query holding one of the concrete query records.
template<typename Tag = void>
    requires(std::is_void_v<Tag> || boost::mp11::mp_contains<QueryConstructorTags, Tag>::value)
struct Query
{
};
class QueryPlan;
class QueryRepository;
template<typename Tag = void>
using QueryView = urd::index_view<Query<Tag>, QueryRepository>;

}  // namespace tyr::datalog::database

#endif
