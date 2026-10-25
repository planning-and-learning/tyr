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

#ifndef TYR_PLANNING_GROUND_STATE_STORAGE_HPP_
#define TYR_PLANNING_GROUND_STATE_STORAGE_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/tags.hpp"

#include <boost/dynamic_bitset.hpp>
#include <cassert>
#include <cstddef>
#include <tuple>
#include <yggdrasil/containers/dynamic_bitset.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

template<>
struct AtomUnpackedStorage<GroundTag>
{
    boost::dynamic_bitset<> indices;

    [[nodiscard]] bool test(ygg::Index<formalism::planning::Atom<GroundTag, formalism::DerivedTag>> index) const
    {
        assert(static_cast<ygg::uint_t>(index) < indices.size());
        return indices.test(static_cast<ygg::uint_t>(index));
    }
    void set(ygg::Index<formalism::planning::Atom<GroundTag, formalism::DerivedTag>> index)
    {
        assert(static_cast<ygg::uint_t>(index) < indices.size());
        indices.set(static_cast<ygg::uint_t>(index));
    }
    void resize(size_t size) { indices.resize(size, false); }
    void clear() { indices.clear(); }
    void swap(AtomUnpackedStorage& other) noexcept { indices.swap(other.indices); }

    static constexpr auto identifying_members() noexcept { return ygg::Members<&AtomUnpackedStorage::indices> {}; }
};

template<>
struct FactUnpackedStorage<GroundTag>
{
    std::vector<ygg::uint_t> values;

    [[nodiscard]] formalism::planning::FDRValue get(ygg::Index<formalism::planning::FDRVariable<formalism::FluentTag>> index) const
    {
        assert(static_cast<ygg::uint_t>(index) < values.size());
        return formalism::planning::FDRValue(values[static_cast<ygg::uint_t>(index)]);
    }
    void set(ygg::Data<formalism::planning::FDRFact<formalism::FluentTag>> fact)
    {
        assert(static_cast<ygg::uint_t>(fact.variable) < values.size());
        values[static_cast<ygg::uint_t>(fact.variable)] = static_cast<ygg::uint_t>(fact.value);
    }
    void resize(size_t size) { values.resize(size, 0); }
    void clear() { values.clear(); }
    void swap(FactUnpackedStorage& other) noexcept { values.swap(other.values); }

    static constexpr auto identifying_members() noexcept { return ygg::Members<&FactUnpackedStorage::values> {}; }
};

}

#endif
