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

#ifndef TYR_DATALOG_COST_BUCKETS_HPP_
#define TYR_DATALOG_COST_BUCKETS_HPP_

#include "tyr/datalog/policies/aggregation.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <algorithm>
#include <cassert>
#include <limits>
#include <urd/elements/hash.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/values/std_set.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/repr/wrappers/recycling.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{

class CostBuckets
{
public:
    using PredicateKey = formalism::datalog::PredicateRowIndex<formalism::FluentTag>;

private:
    using PredicateBucket = urd::Store<urd::set<PredicateKey>>;

public:
    struct Bucket
    {
        PredicateBucket predicate;

        [[nodiscard]] bool empty() const noexcept { return predicate.empty(); }
        /// Empties the bucket, keeping its storage for the next cost (CostBuckets' erased buckets are kept).
        void clear() { predicate.clear(); }
    };

    void clear() noexcept { m_buckets.clear(); }

    [[nodiscard]] bool is_empty() const noexcept { return m_buckets.empty(); }

    [[nodiscard]] Cost min_cost() const noexcept { return m_buckets.empty() ? std::numeric_limits<Cost>::max() : m_buckets.begin()->first; }

    bool insert(Cost cost, PredicateKey key) { return m_buckets[cost].predicate.insert(key).second; }

    bool erase(Cost cost, PredicateKey key)
    {
        const auto it = m_buckets.find(cost);
        if (it == m_buckets.end())
            return false;

        const auto erased = it->second.predicate.erase(key) > 0;
        if (it->second.empty())
            m_buckets.erase(it);
        return erased;
    }

    template<typename Update>
    void update(const Update& update, PredicateKey key)
    {
        if (update.old_cost.has_value())
            erase(*update.old_cost, key);
        insert(update.new_cost, key);
    }

    Bucket& at(Cost cost) { return m_buckets.at(cost); }

    bool erase(Cost cost) { return m_buckets.erase(cost) > 0; }

private:
    /// The buckets by cost, least first; an erased bucket is kept, emptied, for the next cost (retains), and a bucket
    /// stays where it is while others come and go (stable_refs).
    urd::Store<urd::map<Cost, Bucket>, urd::sorted, urd::retains, urd::stable_refs> m_buckets;
};

}

#endif
