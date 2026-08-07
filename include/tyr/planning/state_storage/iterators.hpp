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

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/indices.hpp"
#include "tyr/planning/state_storage.hpp"

#include <boost/dynamic_bitset.hpp>
#include <cassert>
#include <cmath>
#include <iterator>
#include <ranges>
#include <utility>
#include <vector>
#include <yggdrasil/core/types.hpp>

namespace tyr::planning
{

/**
 * Atom
 */

template<class Tag>
class AtomIterator
{
public:
    using value_type = ygg::Index<::tyr::formalism::planning::GroundAtom<Tag>>;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::input_iterator_tag;
    using pointer = void;

    AtomIterator() noexcept : m_data(nullptr), m_i(0) {}
    AtomIterator(const boost::dynamic_bitset<>& data, bool begin) noexcept : m_data(&data), m_i(begin ? m_data->find_first() : boost::dynamic_bitset<>::npos) {}

    value_type operator*() const noexcept
    {
        assert(m_data);
        return ygg::Index<::tyr::formalism::planning::GroundAtom<Tag>> { static_cast<ygg::uint_t>(m_i) };
    }

    AtomIterator& operator++() noexcept
    {
        assert(m_data);
        m_i = m_data->find_next(m_i);
        return *this;
    }
    AtomIterator operator++(int) noexcept
    {
        AtomIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    friend bool operator==(const AtomIterator& lhs, const AtomIterator& rhs) noexcept { return lhs.m_data == rhs.m_data && lhs.m_i == rhs.m_i; }
    friend bool operator!=(const AtomIterator& lhs, const AtomIterator& rhs) noexcept { return !(lhs == rhs); }

private:
    const boost::dynamic_bitset<>* m_data { nullptr };
    boost::dynamic_bitset<>::size_type m_i = 0;
};

template<class Tag>
class AtomRange : public std::ranges::view_interface<AtomRange<Tag>>
{
public:
    AtomRange() = default;
    explicit AtomRange(const boost::dynamic_bitset<>& values) : m_data(&values) {}
    template<TaskKind Kind>
    explicit AtomRange(const AtomUnpackedStorage<Kind>& storage) : AtomRange(storage.indices)
    {
    }

    auto begin() const { return AtomIterator<Tag>(*m_data, true); }
    auto end() const { return AtomIterator<Tag>(*m_data, false); }

private:
    const boost::dynamic_bitset<>* m_data { nullptr };
};

/**
 * FDRFact
 */

template<TaskKind Kind, class IteratorTag>
class FDRFactIterator;

template<TaskKind Kind, class RangeTag>
class FDRFactRange;

/**
 * FunctionTermValue
 */

template<class Tag>
class FunctionTermValueIterator
{
public:
    using value_type = std::pair<ygg::Index<::tyr::formalism::planning::GroundFunctionTerm<Tag>>, ygg::float_t>;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::input_iterator_tag;
    using pointer = void;

    void skip_nan() noexcept
    {
        assert(m_data);
        while (m_i < m_data->size() && std::isnan((*m_data)[m_i]))
            ++m_i;
    }

    FunctionTermValueIterator() noexcept : m_data(nullptr), m_i(0) {}
    FunctionTermValueIterator(const std::vector<ygg::float_t>& data, bool begin) noexcept : m_data(&data), m_i(begin ? 0 : m_data->size())
    {
        if (begin)
            skip_nan();
    }

    value_type operator*() const noexcept
    {
        assert(m_data);
        return { ygg::Index<::tyr::formalism::planning::GroundFunctionTerm<Tag>> { static_cast<ygg::uint_t>(m_i) }, (*m_data)[m_i] };
    }

    FunctionTermValueIterator& operator++() noexcept
    {
        ++m_i;
        skip_nan();
        return *this;
    }
    FunctionTermValueIterator operator++(int) noexcept
    {
        FunctionTermValueIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    friend bool operator==(const FunctionTermValueIterator& lhs, const FunctionTermValueIterator& rhs) noexcept
    {
        return lhs.m_data == rhs.m_data && lhs.m_i == rhs.m_i;
    }
    friend bool operator!=(const FunctionTermValueIterator& lhs, const FunctionTermValueIterator& rhs) noexcept { return !(lhs == rhs); }

private:
    const std::vector<ygg::float_t>* m_data { nullptr };
    std::size_t m_i = 0;
};

template<class Tag>
class FunctionTermValueRange : public std::ranges::view_interface<FunctionTermValueRange<Tag>>
{
public:
    FunctionTermValueRange() = default;
    explicit FunctionTermValueRange(const std::vector<ygg::float_t>& values) : m_data(&values) {}
    template<TaskKind Kind>
    explicit FunctionTermValueRange(const NumericUnpackedStorage<Kind>& storage) : FunctionTermValueRange(storage.values)
    {
    }

    auto begin() const { return FunctionTermValueIterator<Tag>(*m_data, true); }
    auto end() const { return FunctionTermValueIterator<Tag>(*m_data, false); }

private:
    const std::vector<ygg::float_t>* m_data { nullptr };
};
}

#endif
