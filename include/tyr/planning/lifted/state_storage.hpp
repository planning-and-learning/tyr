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

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/tags.hpp"

#include <boost/dynamic_bitset.hpp>
#include <cassert>
#include <cstddef>
#include <limits>
#include <tuple>
#include <utility>
#include <vector>
#include <yggdrasil/containers/dynamic_bitset.hpp>
#include <yggdrasil/containers/tree_bitset_set.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/semantics/containers/dynamic_bitset_hash.hpp>

namespace tyr::planning
{

template<>
struct AtomUnpackedStorage<LiftedTag>
{
    boost::dynamic_bitset<> indices;

    bool test(ygg::Index<formalism::planning::Atom<GroundTag, formalism::DerivedTag>> index) const
    {
        return ygg::test(ygg::uint_t(index), indices);
    }
    void set(ygg::Index<formalism::planning::Atom<GroundTag, formalism::DerivedTag>> index) { ygg::set(ygg::uint_t(index), true, indices); }
    void clear() { indices.clear(); }
    void swap(AtomUnpackedStorage& other) noexcept { indices.swap(other.indices); }

    auto identifying_members() const noexcept { return std::tie(indices); }
};

template<>
struct FactUnpackedStorage<LiftedTag>
{
    using Block = boost::dynamic_bitset<>::block_type;

    static constexpr ygg::uint_t NO_BASE_OWNER = std::numeric_limits<ygg::uint_t>::max();

    boost::dynamic_bitset<> indices;
    /// Packed `indices` of the state this storage was unpacked or registered as, owned by the storage
    /// context identified by `base_owner`, and the block flips applied since. Enables delta insertion.
    ygg::TreeBitsetIndex<Block> base;
    ygg::uint_t base_owner = NO_BASE_OWNER;
    std::vector<ygg::TreeBitsetFlip<Block>> flips;

    formalism::planning::FDRValue get(ygg::Index<formalism::planning::FDRVariable<formalism::FluentTag>> index) const
    {
        return formalism::planning::FDRValue(ygg::test(ygg::uint_t(index), indices));
    }
    void set(ygg::Data<formalism::planning::FDRFact<formalism::FluentTag>> fact)
    {
        assert(ygg::uint_t(fact.value) < 2);  // Lifted fluent variables are binary.
        const auto index = ygg::uint_t(fact.variable);
        const auto value = !fact.value.is_none();
        if (ygg::test(index, indices) == value)
            return;

        if (base_owner != NO_BASE_OWNER)
            flips.push_back(ygg::TreeBitsetFlip<Block> { ygg::uint_t(index / boost::dynamic_bitset<>::bits_per_block),
                                                         Block { 1 } << (index % boost::dynamic_bitset<>::bits_per_block) });

        if (value)
            ygg::set(index, true, indices);
        else
        {
            indices.reset(index);
            if (index + 1 == indices.size())
                ygg::trim_trailing_zeros(indices);
        }
    }
    void rebase(ygg::TreeBitsetIndex<Block> packed, ygg::uint_t owner)
    {
        base = packed;
        base_owner = owner;
        flips.clear();
    }
    void clear()
    {
        indices.clear();
        rebase({}, NO_BASE_OWNER);
    }
    void swap(FactUnpackedStorage& other) noexcept
    {
        using std::swap;
        indices.swap(other.indices);
        swap(base, other.base);
        swap(base_owner, other.base_owner);
        flips.swap(other.flips);
    }

    auto identifying_members() const noexcept { return std::tie(indices); }
};

}

#endif
