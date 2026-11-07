/*
 * Copyright (C) 2025-2026 Dominik Drexler
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef TYR_PLANNING_ALGORITHMS_OPENLISTS_PRIORITY_QUEUE_HPP_
#define TYR_PLANNING_ALGORITHMS_OPENLISTS_PRIORITY_QUEUE_HPP_

#include "tyr/planning/algorithms/openlists/interface.hpp"

#include <concepts>
#include <urd/repr/queues/queue.hpp>
#include <urd/store.hpp>

namespace tyr::planning
{

template<typename T>
concept IsPriorityQueueEntry = requires(const T a) {
    typename T::ItemType;
    { a.get_key() } -> std::totally_ordered;
    { a.get_item() } -> std::convertible_to<typename T::ItemType>;
};

/// Orders entries by key; a priority queue gives the least first.
struct KeyLess
{
    template<IsPriorityQueueEntry E>
    bool operator()(const E& l, const E& r) const
    { return l.get_key() < r.get_key(); }
};

/// Entries by key, least first; top().get_item() is the next item.
template<IsPriorityQueueEntry E>
using PriorityQueue = urd::Store<urd::queue<E>, urd::by_priority, urd::order_by<KeyLess>>;

}

#endif
