/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_DATALOG_REPOSITORY_HPP_
#define TYR_DATALOG_DATABASE_DATALOG_REPOSITORY_HPP_

#include "tyr/datalog/lifted/database/datalog/view.hpp"
#include "yggdrasil/formalism/declarations.hpp"
#include "yggdrasil/formalism/symbol_repository.hpp"

#include <atomic>
#include <cstddef>
#include <optional>
#include <urd/config.hpp>
#include <utility>

namespace tyr::datalog::database::datalog
{
/// Interns Datalog records. Records are validated by prepare_for_insert (construction.hpp) before insertion; children
/// precede their parents. Each repository has its own number.
class DatalogRepository
{
public:
    using SymbolTypes = DatalogSymbolTypes;

private:
    // ponytail: one process-wide counter, numbers are never reused.
    inline static std::atomic<std::size_t> s_next_index { 0 };

    boost::mp11::mp_apply<::ygg::formalism::SymbolRepository, DatalogSymbolTypes> m_symbols;
    size_t m_index;

public:
    DatalogRepository();
    ~DatalogRepository();
    DatalogRepository(const DatalogRepository&) = delete;
    DatalogRepository& operator=(const DatalogRepository&) = delete;
    DatalogRepository(DatalogRepository&&) = delete;
    DatalogRepository& operator=(DatalogRepository&&) = delete;

    /// An index as its view here (urd's resolves contract).
    template<typename T>
    [[nodiscard]] urd::index_view<T, DatalogRepository> view(urd::index<T> index) const noexcept
    { return { index, this }; }
    /// The repository's number, as urd's views ask of a context.
    [[nodiscard]] std::size_t layer() const noexcept { return m_index; }
    friend const DatalogRepository& get_repository(const DatalogRepository& repository URD_LIFETIMEBOUND) noexcept { return repository; }

    template<typename T>
        requires ::ygg::formalism::SupportsSymbol<DatalogRepository, T>
    [[nodiscard]] size_t size() const noexcept
    { return m_symbols.template size<T>(); }

    template<typename T>
        requires ::ygg::formalism::SupportsSymbol<DatalogRepository, T>
    std::optional<urd::index_view<T, DatalogRepository>> find(const urd::data<T>& data) const noexcept
    {
        if (const auto index = m_symbols.template get<T>().find(data); !index.is_none())
            return urd::index_view<T, DatalogRepository>(index, this);
        return std::nullopt;
    }

    template<typename T>
        requires ::ygg::formalism::SupportsSymbol<DatalogRepository, T>
    std::pair<urd::index_view<T, DatalogRepository>, bool> insert(urd::data<T>& data)
    {
        const auto [index, created] = ::ygg::formalism::intern(m_symbols.template get<T>(), data);
        return { urd::index_view<T, DatalogRepository>(index, this), created };
    }

    template<typename T>
        requires ::ygg::formalism::SupportsSymbol<DatalogRepository, T>
    const urd::data<T>& operator[](urd::index<T> index) const noexcept
    { return m_symbols[index]; }
};
}  // namespace tyr::datalog::database::datalog

#endif
