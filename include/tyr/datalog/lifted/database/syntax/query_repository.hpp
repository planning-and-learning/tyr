/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_SYNTAX_QUERY_REPOSITORY_HPP_
#define TYR_DATALOG_DATABASE_SYNTAX_QUERY_REPOSITORY_HPP_

#include "tyr/datalog/lifted/database/syntax/query_view.hpp"
#include "yggdrasil/formalism/declarations.hpp"
#include "yggdrasil/formalism/symbol_context.hpp"

#include <cassert>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <urd/config.hpp>
#include <utility>

namespace tyr::datalog::database
{
/// Interns typed query records. Records are prepared by prepare_for_insert
/// The erased query record followed by one concrete record per constructor tag.
using QuerySymbolTypes = boost::mp11::mp_append<boost::mp11::mp_list<Query<>>, boost::mp11::mp_transform<Query, QueryConstructorTags>>;

/// (query_construction.hpp) before insertion; children precede their parents. Each repository has its own number.
class QueryRepository : public ::ygg::formalism::SymbolContext<QueryRepository, QuerySymbolTypes>
{
    using Base = ::ygg::formalism::SymbolContext<QueryRepository, QuerySymbolTypes>;

    size_t m_index;

public:
    QueryRepository();
    ~QueryRepository();

    /// An index as its view here (urd's resolves contract): what compositions of indices are read through (urd::resolve).
    template<typename T>
    [[nodiscard]] urd::index_view<T, QueryRepository> view(urd::index<T> index) const noexcept
    { return { index, this }; }
    QueryRepository(const QueryRepository&) = delete;
    QueryRepository& operator=(const QueryRepository&) = delete;
    QueryRepository(QueryRepository&&) = delete;
    QueryRepository& operator=(QueryRepository&&) = delete;

    /// The repository's number, as urd's views ask of a context.
    [[nodiscard]] std::size_t layer() const noexcept { return m_index; }
    friend const QueryRepository& get_repository(const QueryRepository& repository URD_LIFETIMEBOUND) noexcept { return repository; }
    /// Number of root queries. Root indices are topological.
    [[nodiscard]] size_t size() const { return Base::template size<Query<>>(); }
    using Base::size;
};

}  // namespace tyr::datalog::database

#endif
