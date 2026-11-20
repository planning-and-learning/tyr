/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_DATALOG_REPOSITORY_HPP_
#define TYR_DATALOG_DATABASE_DATALOG_REPOSITORY_HPP_

#include "tyr/datalog/lifted/database/datalog/view.hpp"
#include "yggdrasil/formalism/declarations.hpp"
#include "yggdrasil/formalism/symbol_context.hpp"

#include <cstddef>
#include <urd/config.hpp>

namespace tyr::datalog::database::datalog
{
/// Interns Datalog records. Records are prepared by prepare_for_insert (construction.hpp) before insertion; children
/// precede their parents. Each repository has its own number.
class DatalogRepository : public ::ygg::formalism::SymbolContext<DatalogRepository, DatalogSymbolTypes>
{
    size_t m_index;

public:
    DatalogRepository();
    ~DatalogRepository();
    DatalogRepository(const DatalogRepository&) = delete;
    DatalogRepository& operator=(const DatalogRepository&) = delete;
    DatalogRepository(DatalogRepository&&) = delete;
    DatalogRepository& operator=(DatalogRepository&&) = delete;

    /// The repository's number, as urd's views ask of a context.
    [[nodiscard]] std::size_t layer() const noexcept { return m_index; }

    friend const DatalogRepository& get_repository(const DatalogRepository& repository URD_LIFETIMEBOUND) noexcept { return repository; }
};
}  // namespace tyr::datalog::database::datalog

#endif
