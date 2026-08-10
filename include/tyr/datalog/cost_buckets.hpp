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

#include <limits>
#include <map>
#include <utility>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/semantics/comparison.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::datalog
{

class CostBuckets
{
public:
    using PredicateKey = ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag>;
    using FunctionKey = ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag>;
    using Interval = ygg::ClosedInterval<ygg::float_t>;
    using PredicateBucket = ygg::UnorderedSet<PredicateKey>;
    using FunctionBucket = ygg::UnorderedMap<FunctionKey, Interval>;

    struct Bucket
    {
        PredicateBucket predicate;
        FunctionBucket function;

        [[nodiscard]] bool empty() const noexcept { return predicate.empty() && function.empty(); }
    };

    void clear() noexcept
    {
        m_buckets.clear();
        m_current = Cost(0);
    }

    [[nodiscard]] Cost current_cost() const noexcept { return m_current; }
    [[nodiscard]] bool is_empty() const noexcept { return m_buckets.empty(); }

    [[nodiscard]] Cost min_cost() const noexcept { return m_buckets.empty() ? std::numeric_limits<Cost>::max() : m_buckets.begin()->first; }

    bool insert(Cost cost, PredicateKey key) { return m_buckets[cost].predicate.insert(key).second; }

    bool insert(Cost cost, FunctionKey key, Interval interval)
    {
        if (empty(interval))
            return false;

        auto& bucket = m_buckets[cost].function;
        const auto [it, inserted] = bucket.emplace(key, interval);
        if (inserted)
            return true;
        if (subset(interval, it->second))
            return false;

        it->second = hull(it->second, interval);
        return true;
    }

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

    void clear_current() { m_buckets.erase(m_current); }

    bool advance_to_next_nonempty()
    {
        if (is_empty())
            return false;
        m_current = min_cost();
        return true;
    }

    const PredicateBucket& get_current_bucket() const
    {
        static const PredicateBucket kEmpty {};
        const auto it = m_buckets.find(m_current);
        return it == m_buckets.end() ? kEmpty : it->second.predicate;
    }

    const FunctionBucket& get_current_function_bucket() const
    {
        static const FunctionBucket kEmpty {};
        const auto it = m_buckets.find(m_current);
        return it == m_buckets.end() ? kEmpty : it->second.function;
    }

    Bucket take(Cost cost)
    {
        const auto it = m_buckets.find(cost);
        if (it == m_buckets.end())
            return {};

        auto bucket = std::move(it->second);
        m_buckets.erase(it);
        return bucket;
    }

private:
    std::map<Cost, Bucket> m_buckets;
    Cost m_current = Cost(0);
};

}

#endif
