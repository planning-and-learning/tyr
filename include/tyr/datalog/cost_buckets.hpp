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
#include <map>
#include <utility>
#include <vector>
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
    using PredicateKey = formalism::datalog::PredicateBindingView<formalism::FluentTag>;
    using FunctionKey = formalism::datalog::FunctionBindingView<formalism::FluentTag>;
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
        while (!m_buckets.empty())
            recycle(m_buckets.extract(m_buckets.begin()));
    }

    [[nodiscard]] bool is_empty() const noexcept { return m_buckets.empty(); }

    [[nodiscard]] Cost min_cost() const noexcept { return m_buckets.empty() ? std::numeric_limits<Cost>::max() : m_buckets.begin()->first; }

    bool insert(Cost cost, PredicateKey key) { return get_bucket(cost).predicate.insert(key).second; }

    bool insert(Cost cost, FunctionKey key, Interval interval)
    {
        if (empty(interval))
            return false;

        auto& bucket = get_bucket(cost).function;
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
            recycle(m_buckets.extract(it));
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

    bool erase(Cost cost)
    {
        auto node = m_buckets.extract(cost);
        const auto erased = !node.empty();
        recycle(std::move(node));
        return erased;
    }

private:
    void recycle(std::map<Cost, Bucket>::node_type node) noexcept
    {
        if (node.empty())
            return;
        node.mapped().predicate.clear();
        node.mapped().function.clear();
        assert(m_free.size() < m_free.capacity());
        m_free.push_back(std::move(node));
    }

    Bucket& get_bucket(Cost cost)
    {
        if (const auto it = m_buckets.find(cost); it != m_buckets.end())
            return it->second;
        if (!m_free.empty())
        {
            auto node = std::move(m_free.back());
            m_free.pop_back();
            node.key() = cost;
            return m_buckets.insert(std::move(node)).position->second;
        }
        // Reserve before creating a node so reset and recycling never allocate.
        if (m_free.capacity() <= m_buckets.size())
            m_free.reserve(std::max(m_buckets.size() + 1, m_free.capacity() * 2));
        return m_buckets.try_emplace(cost).first->second;
    }

    std::map<Cost, Bucket> m_buckets;
    std::vector<std::map<Cost, Bucket>::node_type> m_free;
};

}

#endif
