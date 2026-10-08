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

#ifndef TYR_PLANNING_ALGORITHMS_OPENLISTS_ALTERNATING_HPP_
#define TYR_PLANNING_ALGORITHMS_OPENLISTS_ALTERNATING_HPP_

#include "tyr/planning/algorithms/openlists/interface.hpp"
#include "tyr/planning/algorithms/openlists/priority_queue.hpp"

#include <array>
#include <cassert>
#include <optional>
#include <yggdrasil/containers/tuple.hpp>

namespace tyr::planning
{

template<IsOpenList First, IsOpenList... Rest>
    requires (std::same_as<typename First::ItemType, typename Rest::ItemType> && ...) && std::move_constructible<typename First::ItemType>
class AlternatingOpenList
{
public:
    static constexpr std::size_t N = 1 + sizeof...(Rest);

    using ItemType = typename First::ItemType;

private:
    bool cur_empty()
    {
        assert(m_pos < N);

        auto result = bool();
        ygg::visit_at(m_queues, m_pos, [&result](auto&& queue) { result = queue.get().empty(); });

        return result;
    }

    size_t cur_size()
    {
        assert(m_pos < N);

        auto result = size();
        ygg::visit_at(m_queues, m_pos, [&result](auto&& queue) { result = queue.get().size(); });

        return result;
    }

    size_t cur_weight() const
    {
        assert(m_pos < N);

        return m_weights[m_pos];
    }

    void find_next_nonempty_queue()
    {
        do
        {
            if (++m_pos == N)
            {
                m_pos = 0;
            }
        } while (cur_empty());

        m_count = 0;
    }

public:
    AlternatingOpenList(First& first, Rest&... rest, std::array<size_t, N> weights) :
        m_queues(std::ref(first), std::ref(rest)...),
        m_weights(weights),
        m_pos(0),
        m_count(0)
    {
    }

    ItemType top()
    {
        assert(!empty());

        if (cur_empty() || m_count >= cur_weight())
        {
            find_next_nonempty_queue();
        }

        std::optional<ItemType> result;
        ygg::visit_at(m_queues, m_pos, [&result](auto&& queue) { result.emplace(queue.get().top()); });
        return std::move(*result);
    }

    void pop()
    {
        assert(!cur_empty());

        ygg::visit_at(m_queues, m_pos, [](auto&& queue) { queue.get().pop(); });

        ++m_count;
    }

    void clear()
    {
        std::apply([](auto&&... queues) { (queues.get().clear(), ...); }, m_queues);
    }

    bool empty() const
    {
        return std::apply([](auto&&... queues) { return (queues.get().empty() && ...); }, m_queues);
    }

    std::size_t size() const
    {
        return std::apply([](auto&&... queues) { return (queues.get().size() + ...); }, m_queues);
    }

    auto& get_weights() noexcept { return m_weights; }
    const auto& get_weights() const noexcept { return m_weights; }

private:
    std::tuple<std::reference_wrapper<First>, std::reference_wrapper<Rest>...> m_queues;

    std::array<size_t, N> m_weights;

    size_t m_pos;
    size_t m_count;
};

}

#endif
