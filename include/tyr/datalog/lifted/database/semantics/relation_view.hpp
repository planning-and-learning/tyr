/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_RELATION_VIEW_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_RELATION_VIEW_HPP_

#include "tyr/datalog/lifted/database/declarations.hpp"
#include "yggdrasil/formalism/relation_view.hpp"
#include "yggdrasil/formalism/repository.hpp"

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
/// Read access shared by the relation views (formalism/relation_view.hpp): interned (index) views and builder views.
/// A row is a Row<Schema> view: its values by position (value(i); get_values() the stored row); its columns give their
/// labels and types.
template<typename V>
concept RelationViewConcept = requires(const std::remove_reference_t<V>& view, size_t i, std::span<const ygg::Value> values) {
    { view.columns() } -> std::same_as<std::span<const ygg::ColumnLayout>>;
    { view.size() } -> std::same_as<size_t>;
    { view.empty() } -> std::same_as<bool>;
    { view.relation_identity() } -> std::same_as<ygg::RelationIdentity>;
    { view.row(i) } -> ygg::RowViewConcept<ygg::Schema>;
    { view.row(i).get_values() } -> std::same_as<ygg::RowPool::row_type>;
    { view.contains(values) } -> std::same_as<bool>;
    { view.contains(view.row(i)) } -> std::same_as<bool>;
};

/// Sized random-access inputs whose elements, possibly references, are relation views.
template<typename R>
concept RelationViewRange =
    std::ranges::random_access_range<const R> && std::ranges::sized_range<const R> && RelationViewConcept<std::ranges::range_reference_t<const R>>;

/// A row consumer: called with the canonical values of each row, one per column; returning false stops the
/// enumeration.
template<typename F>
concept RowPredicate = std::predicate<const F&, std::span<const ygg::Value>>;

/// The inputs of a query by slot: f(slot) returns the relation view bound to input slot `slot`.
template<typename F>
concept SlotRelations = std::invocable<const F&, size_t> && RelationViewConcept<std::invoke_result_t<const F&, size_t>>;

}  // namespace tyr::datalog::database

#endif
