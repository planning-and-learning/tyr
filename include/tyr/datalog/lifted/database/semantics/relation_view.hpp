/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_RELATION_VIEW_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_RELATION_VIEW_HPP_

#include "tyr/datalog/lifted/database/declarations.hpp"
#include "yggdrasil/formalism/relation_view.hpp"
#include "yggdrasil/formalism/relation_view_concepts.hpp"
#include "yggdrasil/formalism/repository.hpp"
#include "yggdrasil/formalism/scratch_relation.hpp"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <iterator>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <urd/elements/view.hpp>
#include <utility>

namespace tyr::datalog::database
{
/// Sized random-access inputs whose elements, possibly references, are relation views.
template<typename R>
concept RelationViewRange = std::ranges::random_access_range<const R> && std::ranges::sized_range<const R>
                            && ygg::RelationViewConcept<std::ranges::range_reference_t<const R>, ygg::Schema>;

/// A row consumer: called with the canonical values of each row, one per column; returning false stops the
/// enumeration.
template<typename F>
concept RowPredicate = std::predicate<const F&, std::span<const ygg::Value>>;

/// The inputs of a query by slot: f(slot) returns the relation view bound to input slot `slot`.
template<typename F>
concept SlotRelations = std::invocable<const F&, std::size_t> && ygg::RelationViewConcept<std::invoke_result_t<const F&, std::size_t>, ygg::Schema>;

/// Whether a relation's view reads the scratch relation `out`: its context is out's repository (a builder's view of out;
/// an interned view of a relation of another repository reads none). An operation's output is none of its inputs.
template<ygg::RelationViewConcept<ygg::Schema> V>
[[nodiscard]] bool reads(const V& view, const ygg::FreeScratchRelation& out) noexcept
{ return &view.context() == &out.rows; }
}  // namespace tyr::datalog::database

#endif
