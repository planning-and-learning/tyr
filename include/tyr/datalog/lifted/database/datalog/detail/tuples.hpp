/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_DATALOG_DETAIL_TUPLES_HPP_
#define TYR_DATALOG_DATABASE_DATALOG_DETAIL_TUPLES_HPP_

#include "yggdrasil/core/config.hpp"

#include <urd/repr/pools/pools.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/wrappers/multi_index_pool.hpp>
#include <urd/store.hpp>

/// The evaluator's sets of keys (evaluator.hpp): fixed-arity tuples of object ids.
namespace tyr::datalog::database::datalog::detail
{
/// Tuples of object ids, interned in a urd pool; a tuple is its handle, and iterating gives the handles.
using Rows = urd::Store<urd::pool<urd::Store<urd::sequence<ygg::uint_t>, urd::frozen>>, urd::multi_indexed>;
using RowHandle = Rows::handle_type;
/// What is kept per tuple of a pool, addressed by its handle.
template<typename T>
using PerRow = urd::Store<urd::sequence<T, RowHandle>>;
}  // namespace tyr::datalog::database::datalog::detail

#endif
