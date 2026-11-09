/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_INCREMENTAL_DELTA_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_INCREMENTAL_DELTA_HPP_

#include "tyr/datalog/lifted/database/semantics/operations.hpp"

#include <cassert>
#include <concepts>
#include <initializer_list>
#include <ranges>
#include <span>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace tyr::datalog::database::incremental
{
namespace detail
{
template<typename Tag>
struct NodeRules;
}

/// Actual set changes with one ordered schema. Added and removed rows are
/// disjoint. The caller guarantees additions were absent and removals present.
/// Exchanging the two inputs to update() reverses the change. Its builders are read through the layer, which must
/// outlive it.
struct Delta
{
    const ygg::FreeRelationRepository* layer;
    urd::builder<ygg::Relation<ygg::Schema>> added;
    urd::builder<ygg::Relation<ygg::Schema>> removed;

    Delta(const ygg::FreeRelationRepository& layer_, std::span<const ygg::ColumnLayout> columns) : layer(&layer_), added(columns), removed(columns) {}

    void clear() noexcept
    {
        added.clear();
        removed.clear();
    }
    /// The (added, removed) change, as views.
    [[nodiscard]] std::pair<ygg::FreeBuilderRelationView, ygg::FreeBuilderRelationView> change() const noexcept { return { layer->view(added), layer->view(removed) }; }
    [[nodiscard]] size_t memory_usage() const noexcept { return added.memory_usage() + removed.memory_usage(); }
};

/// An actual set change of one input: disjoint (added, removed) relations, read with std::get.
template<typename C>
concept RelationChange = requires(const C& change) {
    { std::get<0>(change) } -> RelationViewConcept;
    { std::get<1>(change) } -> RelationViewConcept;
};

/// The exact changes of a query's inputs by slot: f(slot).added and f(slot).removed are relation views, the rows added to
/// and removed from input slot `slot`.
template<typename F>
concept SlotChanges = std::invocable<const F&, size_t> && requires(const F& change, size_t slot) {
    { change(slot).added } -> RelationViewConcept;
    { change(slot).removed } -> RelationViewConcept;
};

/// Sized random-access changes, one per input.
template<typename R>
concept RelationDeltaRange =
    std::ranges::random_access_range<const R> && std::ranges::sized_range<const R> && RelationChange<std::ranges::range_reference_t<const R>>;

namespace detail
{
template<RelationViewConcept A, RelationViewConcept R>
void require_delta(const A& added, const R& removed, std::span<const ygg::ColumnLayout> columns)
{
    assert(std::ranges::equal(added.columns(), columns));
    assert(std::ranges::equal(removed.columns(), columns));
    for (size_t i = 0; i < added.size(); ++i)
        if (removed.contains(added.row(i)))
            throw std::invalid_argument("Incremental evaluation: added and removed rows overlap.");
}

/// Results and deltas exist only after a complete initialize or update.
void require_initialized(bool initialized);

/// Inputs are borrowed during a call and must not be the evaluator's own result or delta.
template<RelationViewConcept V>
void require_unaliased(const V& input, const urd::builder<ygg::Relation<ygg::Schema>>& result, const Delta& delta)
{
    const auto storage = input.relation_identity();
    if (storage == relation_identity(result) || storage == relation_identity(delta.added) || storage == relation_identity(delta.removed))
        throw std::invalid_argument("Incremental evaluation: input aliases the evaluator's result or delta.");
}

/// A change of an input with the given columns is disjoint and borrows no evaluator storage.
template<RelationChange C>
void require_change(const C& change, std::span<const ygg::ColumnLayout> columns, const urd::builder<ygg::Relation<ygg::Schema>>& result, const Delta& delta)
{
    require_delta(std::get<0>(change), std::get<1>(change), columns);
    require_unaliased(std::get<0>(change), result, delta);
    require_unaliased(std::get<1>(change), result, delta);
}

/// Applies an exact delta of the result: removed rows are present, added rows absent.
void apply_delta(const Delta& delta, urd::builder<ygg::Relation<ygg::Schema>>& result);
}  // namespace detail

}  // namespace tyr::datalog::database::incremental

#endif
