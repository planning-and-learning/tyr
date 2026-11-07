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

#ifndef TYR_PLANNING_STATE_STORAGE_ITERATORS_HPP_
#define TYR_PLANNING_STATE_STORAGE_ITERATORS_HPP_

#include <boost/dynamic_bitset.hpp>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <ranges>

namespace tyr::planning
{

/// The positions of the bits set in a dynamic_bitset, ascending; find_first/find_next skip clear blocks. The iterators
/// read the bitset, which must outlive them and stay unchanged.
/// ponytail: the one hand-written state iterator left; it goes once the state storages are dense-set Stores (their own ranges).
class SetBits : public std::ranges::view_interface<SetBits>
{
public:
    class iterator
    {
    public:
        using value_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;
        using iterator_concept = std::forward_iterator_tag;

        iterator() noexcept = default;
        iterator(const boost::dynamic_bitset<>& bits, std::size_t position) noexcept : m_bits(&bits), m_i(position) {}

        std::size_t operator*() const noexcept
        {
            assert(m_bits && m_i != boost::dynamic_bitset<>::npos);
            return m_i;
        }
        iterator& operator++() noexcept
        {
            m_i = m_bits->find_next(m_i);
            return *this;
        }
        iterator operator++(int) noexcept
        {
            auto result = *this;
            ++*this;
            return result;
        }
        friend bool operator==(const iterator& lhs, const iterator& rhs) noexcept { return lhs.m_bits == rhs.m_bits && lhs.m_i == rhs.m_i; }

    private:
        const boost::dynamic_bitset<>* m_bits { nullptr };
        std::size_t m_i = boost::dynamic_bitset<>::npos;
    };

    SetBits() = default;
    explicit SetBits(const boost::dynamic_bitset<>& bits) noexcept : m_bits(&bits) {}

    [[nodiscard]] iterator begin() const noexcept { return iterator(*m_bits, m_bits->find_first()); }
    [[nodiscard]] iterator end() const noexcept { return iterator(*m_bits, boost::dynamic_bitset<>::npos); }

private:
    const boost::dynamic_bitset<>* m_bits { nullptr };
};

}

/// Its iterators read the bitset, not the range object.
template<>
inline constexpr bool std::ranges::enable_borrowed_range<tyr::planning::SetBits> = true;

#endif
