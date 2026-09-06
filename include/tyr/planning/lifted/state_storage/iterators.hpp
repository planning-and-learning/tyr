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

#ifndef TYR_PLANNING_LIFTED_STATE_STORAGE_ITERATORS_HPP_
#define TYR_PLANNING_LIFTED_STATE_STORAGE_ITERATORS_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/indices.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/lifted/state_storage.hpp"
#include "tyr/planning/state_storage/iterators.hpp"

#include <boost/dynamic_bitset.hpp>
#include <cassert>
#include <iterator>
#include <yggdrasil/core/types.hpp>

namespace tyr::planning
{
/**
 * FDRFact
 */

template<class Tag>
class FDRFactIterator<LiftedTag, Tag>
{
public:
    using value_type = ygg::Data<formalism::planning::FDRFact<Tag>>;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::input_iterator_tag;
    using pointer = void;

    FDRFactIterator() noexcept : m_storage(nullptr), m_i(0) {}
    FDRFactIterator(const FactUnpackedStorage<LiftedTag>& storage, bool begin) noexcept :
        m_storage(&storage),
        m_i(begin ? m_storage->indices.find_first() : boost::dynamic_bitset<>::npos)
    {
    }

    value_type operator*() const noexcept
    {
        assert(m_storage);
        assert(m_storage->indices.test(m_i));
        return ygg::Data<formalism::planning::FDRFact<Tag>> { ygg::Index<formalism::planning::FDRVariable<Tag>> { static_cast<ygg::uint_t>(m_i) },
                                                                     formalism::planning::FDRValue { 1 } };
    }

    FDRFactIterator& operator++() noexcept
    {
        m_i = m_storage->indices.find_next(m_i);
        return *this;
    }
    FDRFactIterator operator++(int) noexcept
    {
        FDRFactIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    friend bool operator==(const FDRFactIterator& lhs, const FDRFactIterator& rhs) noexcept { return lhs.m_storage == rhs.m_storage && lhs.m_i == rhs.m_i; }
    friend bool operator!=(const FDRFactIterator& lhs, const FDRFactIterator& rhs) noexcept { return !(lhs == rhs); }

private:
    const FactUnpackedStorage<LiftedTag>* m_storage { nullptr };
    boost::dynamic_bitset<>::size_type m_i = 0;
};

template<typename Tag>
class FDRFactRange<LiftedTag, Tag> : public std::ranges::view_interface<FDRFactRange<LiftedTag, Tag>>
{
public:
    FDRFactRange() = default;
    explicit FDRFactRange(const FactUnpackedStorage<LiftedTag>& storage) : m_storage(&storage) {}

    auto begin() const { return FDRFactIterator<LiftedTag, Tag>(*m_storage, true); }
    auto end() const { return FDRFactIterator<LiftedTag, Tag>(*m_storage, false); }

private:
    const FactUnpackedStorage<LiftedTag>* m_storage { nullptr };
};
}

#endif
