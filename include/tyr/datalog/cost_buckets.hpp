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

#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/policies/aggregation.hpp"
#include "tyr/formalism/declarations.hpp"

#include <cassert>
#include <limits>
#include <urd/elements/hash.hpp>
#include <urd/kinds.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/values/std_set.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/repr/wrappers/recycling.hpp>
#include <urd/store.hpp>
#include <urd/tags.hpp>
#include <utility>

namespace tyr::datalog
{

/// The heads not committed yet, of each mutable fact kind, by cost.
class CostBuckets
{
public:
    /// The heads of kind F at one cost.
    template<typename F>
    using PredicateBucket = urd::Store<urd::set<formalism::PredicateRowKey<F>>>;

    struct Bucket
    {
        MutableFamily<PredicateBucket> predicate;

        [[nodiscard]] bool empty() const noexcept
        {
            auto result = true;
            predicate.for_each([&](const auto& heads) { result = result && heads.empty(); });
            return result;
        }

        /// Empties the bucket, keeping its storage for the next cost (CostBuckets' erased buckets are kept).
        void clear() { predicate.clear(); }
    };

    void clear() noexcept { m_buckets.clear(); }

    [[nodiscard]] bool is_empty() const noexcept { return m_buckets.empty(); }

    [[nodiscard]] Cost min_cost() const noexcept { return m_buckets.empty() ? std::numeric_limits<Cost>::max() : m_buckets.begin()->first; }

    template<formalism::FactKind F>
    bool insert(Cost cost, formalism::PredicateRowKey<F> key)
    { return m_buckets[cost].predicate.template get<F>().insert(key).second; }

    template<formalism::FactKind F>
    bool erase(Cost cost, formalism::PredicateRowKey<F> key)
    {
        const auto it = m_buckets.find(cost);
        if (it == m_buckets.end())
            return false;

        const auto erased = it->second.predicate.template get<F>().erase(key) > 0;
        if (it->second.empty())
            m_buckets.erase(it);
        return erased;
    }

    template<typename Update, formalism::FactKind F>
    void update(const Update& update, formalism::PredicateRowKey<F> key)
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
