/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_SYNTAX_FORMATTER_HPP_
#define TYR_DATALOG_DATABASE_SYNTAX_FORMATTER_HPP_

#include "tyr/datalog/lifted/database/syntax/query.hpp"
#include "tyr/formalism/formatter.hpp"
#include "yggdrasil/formalism/formatter.hpp"
#include "yggdrasil/formatting/formatter.hpp"

#include <boost/mp11.hpp>
#include <fmt/ranges.h>
#include <iterator>
#include <ranges>
#include <string>
#include <urd/format.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>

namespace tyr::datalog::database
{
/// The plan as text: each node by its number, its operation, children and columns (an input its slot, a generic join its
/// variable order), then its roots.
std::string explain(const QueryPlan& plan);
}  // namespace tyr::datalog::database

#if YGG_ENABLE_FMT_FORMATTERS
namespace fmt
{
/// A query plan, as explain writes it.
template<>
struct formatter<tyr::datalog::database::QueryPlan, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::datalog::database::QueryPlan& plan, fmt::format_context& ctx) const;
};

/// An operation's plan, as the columns it produces: how a query's record writes it.
template<typename Plan>
    requires boost::mp11::mp_contains<boost::mp11::mp_list<tyr::datalog::database::ProjectionPlan,
                                                           tyr::datalog::database::JoinPlan,
                                                           tyr::datalog::database::SetPlan,
                                                           tyr::datalog::database::SelectEqualPlan,
                                                           tyr::datalog::database::SelectValuePlan,
                                                           tyr::datalog::database::DistancePlan,
                                                           tyr::datalog::database::GenericJoinPlan>,
                                      Plan>::value
struct formatter<Plan, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const Plan& plan, fmt::format_context& ctx) const;
};
}  // namespace fmt
#endif

#endif
