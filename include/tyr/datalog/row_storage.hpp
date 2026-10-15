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

#ifndef TYR_DATALOG_ROW_STORAGE_HPP_
#define TYR_DATALOG_ROW_STORAGE_HPP_

#include "tyr/formalism/datalog/declarations.hpp"

#include <boost/dynamic_bitset.hpp>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <vector>
#include <yggdrasil/containers/dynamic_bitset.hpp>
#include <yggdrasil/containers/paged_vector.hpp>
#include <yggdrasil/containers/segmented_vector.hpp>
#include <yggdrasil/containers/sparse_bitset.hpp>

namespace tyr::datalog
{

/// Value-initialized elements indexed by dense rows, grown on access.
template<std::default_initializable T, bool ThreadSafe = false>
class DenseRowMap
{
public:
    const T* find(size_t row) const noexcept { return row < m_elements.size() ? &m_elements[row] : nullptr; }
    T* find(size_t row) noexcept { return row < m_elements.size() ? &m_elements[row] : nullptr; }

    T& get_or_create(size_t row)
    {
        if constexpr (ThreadSafe)
        {
            while (m_elements.size() <= row)
                m_elements.emplace_back();
        }
        else if (row >= m_elements.size())
            m_elements.resize(row + 1);
        return m_elements[row];
    }

private:
    std::conditional_t<ThreadSafe, ygg::SegmentedVector<T, 32, true>, std::vector<T>> m_elements;
};

/// Storage indexed by the rows of ground bindings: interned rows are dense, while packed rows
/// spread over a sparse key space and allocate small pages on first touch.
template<std::default_initializable T, bool ThreadSafe = false>
using RowMap = std::conditional_t<formalism::datalog::packed_ground_bindings, ygg::PagedVector<T, 6, ThreadSafe>, DenseRowMap<T, ThreadSafe>>;

/// Set of dense rows, iterated in ascending order.
class DenseRowSet
{
public:
    /// Returns whether row was absent.
    bool insert(size_t row)
    {
        if (ygg::test(row, m_bits))
            return false;
        ygg::set(row, true, m_bits);
        return true;
    }

    bool contains(size_t row) const noexcept { return ygg::test(row, m_bits); }

    void reset() noexcept { m_bits.reset(); }

    class const_iterator
    {
    public:
        using value_type = size_t;
        using reference = size_t;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;
        using iterator_concept = std::forward_iterator_tag;

        const_iterator() noexcept = default;
        const_iterator(const boost::dynamic_bitset<>& bits, size_t position) noexcept : m_bits(&bits), m_position(position) {}

        size_t operator*() const noexcept { return m_position; }

        const_iterator& operator++() noexcept
        {
            m_position = m_bits->find_next(m_position);
            return *this;
        }

        const_iterator operator++(int) noexcept
        {
            auto copy = *this;
            ++(*this);
            return copy;
        }

        friend bool operator==(const const_iterator& lhs, const const_iterator& rhs) noexcept { return lhs.m_position == rhs.m_position; }

    private:
        const boost::dynamic_bitset<>* m_bits = nullptr;
        size_t m_position = boost::dynamic_bitset<>::npos;
    };

    const_iterator begin() const noexcept { return const_iterator(m_bits, m_bits.find_first()); }
    const_iterator end() const noexcept { return const_iterator(m_bits, boost::dynamic_bitset<>::npos); }

private:
    boost::dynamic_bitset<> m_bits;
};

/// Set of rows of ground bindings: dense rows are iterated in ascending order, while packed rows
/// are sparse and cost O(touched words) to reset and iterate, in first-touch order of their words.
using RowSet = std::conditional_t<formalism::datalog::packed_ground_bindings, ygg::SparseBitset<>, DenseRowSet>;

}

#endif
