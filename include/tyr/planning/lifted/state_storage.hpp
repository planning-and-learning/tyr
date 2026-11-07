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

#ifndef TYR_PLANNING_LIFTED_STATE_STORAGE_HPP_
#define TYR_PLANNING_LIFTED_STATE_STORAGE_HPP_

#include "tyr/containers.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/iterators.hpp"

#include <boost/dynamic_bitset.hpp>
#include <cassert>
#include <cstddef>
#include <ranges>
#include <tuple>
#include <yggdrasil/core/config.hpp>
#include <urd/elements/hash_dynamic_bitset.hpp>

namespace tyr::planning
{

/// Shrinks `bitset` to end at its last set bit.
template<typename Block, typename Allocator>
void trim_trailing_zeros(boost::dynamic_bitset<Block, Allocator>& bitset)
{
    using Bitset = boost::dynamic_bitset<Block, Allocator>;

    // Scans from the back one block at a time: find_next only searches the last block, since the bitset ends there.
    while (!bitset.empty())
    {
        const auto begin = (bitset.size() - 1) / Bitset::bits_per_block * Bitset::bits_per_block;
        auto last = (begin == 0) ? bitset.find_first() : bitset.find_next(begin - 1);
        if (last == Bitset::npos)
        {
            bitset.resize(begin);
            continue;
        }
        for (auto pos = bitset.find_next(last); pos != Bitset::npos; pos = bitset.find_next(pos))
            last = pos;
        bitset.resize(last + 1);
        return;
    }
}

template<>
struct AtomUnpackedStorage<LiftedTag>
{
    boost::dynamic_bitset<> indices;

    [[nodiscard]] bool test(urd::index<formalism::planning::Atom<GroundTag, formalism::DerivedTag>> index) const
    { return tyr::test(static_cast<ygg::uint_t>(index.value), indices); }
    void set(urd::index<formalism::planning::Atom<GroundTag, formalism::DerivedTag>> index) { tyr::set(static_cast<ygg::uint_t>(index.value), true, indices); }
    void clear() { indices.clear(); }
    void swap(AtomUnpackedStorage& other) noexcept { indices.swap(other.indices); }

    static constexpr auto identifying_members() noexcept { return urd::Members<&AtomUnpackedStorage::indices> {}; }
};

template<>
struct FactUnpackedStorage<LiftedTag>
{
    boost::dynamic_bitset<> indices;

    [[nodiscard]] formalism::planning::FDRValue get(urd::index<formalism::planning::FDRVariable<formalism::FluentTag>> index) const
    { return formalism::planning::FDRValue(tyr::test(static_cast<ygg::uint_t>(index.value), indices)); }
    void set(urd::data<formalism::planning::FDRFact<formalism::FluentTag>> fact)
    {
        assert(fact.value.value < 2);  // Lifted fluent variables are binary.
        const auto index = static_cast<ygg::uint_t>(fact.variable.value);
        if (fact.value != formalism::planning::FDRValue::none())
            tyr::set(index, true, indices);
        else if (index < indices.size())
        {
            indices.reset(index);
            if (index + 1 == indices.size())
                trim_trailing_zeros(indices);
        }
    }
    /// The facts set (value 1), by ascending variable.
    [[nodiscard]] auto facts() const noexcept
    {
        return urd::transform(SetBits(indices),
                              [](std::size_t i)
                              {
                                  return urd::data<formalism::planning::FDRFact<formalism::FluentTag>> {
                                      urd::index<formalism::planning::FDRVariable<formalism::FluentTag>> { static_cast<ygg::uint_t>(i) },
                                      formalism::planning::FDRValue { 1 }
                                  };
                              });
    }
    void clear() { indices.clear(); }
    void swap(FactUnpackedStorage& other) noexcept { indices.swap(other.indices); }

    static constexpr auto identifying_members() noexcept { return urd::Members<&FactUnpackedStorage::indices> {}; }
};

}

#endif
