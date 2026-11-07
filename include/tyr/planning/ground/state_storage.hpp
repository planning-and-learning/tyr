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
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_storage.hpp"

#include <boost/dynamic_bitset.hpp>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <tuple>
#include <urd/elements/flat.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

/// Where the fluent FDR values and derived atoms of a ground task's states lie, packed bitwise into ygg::uint_t blocks.
template<>
struct StateLayout<GroundTag>
{
    /// Bit range of a fluent FDR variable's value.
    struct VariableInfo
    {
        ygg::uint_t begin;
        uint8_t offset;
        uint8_t length;
    };

    explicit StateLayout(const Task<GroundTag>& task);

    urd::Store<urd::sequence<VariableInfo>> fluent_infos;
    ygg::uint_t fluent_num_blocks;
    ygg::uint_t derived_num_bits;
};

template<>
struct AtomUnpackedStorage<GroundTag>
{
    boost::dynamic_bitset<> indices;

    [[nodiscard]] bool test(urd::index<formalism::planning::Atom<GroundTag, formalism::DerivedTag>> index) const
    {
        assert(static_cast<ygg::uint_t>(index.value) < indices.size());
        return indices.test(static_cast<ygg::uint_t>(index.value));
    }
    void set(urd::index<formalism::planning::Atom<GroundTag, formalism::DerivedTag>> index)
    {
        assert(static_cast<ygg::uint_t>(index.value) < indices.size());
        indices.set(static_cast<ygg::uint_t>(index.value));
    }
    void resize(size_t size) { indices.resize(size, false); }
    void clear() { indices.clear(); }
    void swap(AtomUnpackedStorage& other) noexcept { indices.swap(other.indices); }

    static constexpr auto identifying_members() noexcept { return urd::Members<&AtomUnpackedStorage::indices> {}; }
};

template<>
struct FactUnpackedStorage<GroundTag>
{
    urd::Store<urd::sequence<ygg::uint_t>> values;

    [[nodiscard]] formalism::planning::FDRValue get(urd::index<formalism::planning::FDRVariable<formalism::FluentTag>> index) const
    {
        assert(static_cast<ygg::uint_t>(index.value) < values.size());
        return formalism::planning::FDRValue(values[static_cast<ygg::uint_t>(index.value)]);
    }
    void set(urd::data<formalism::planning::FDRFact<formalism::FluentTag>> fact)
    {
        assert(static_cast<ygg::uint_t>(fact.variable.value) < values.size());
        values[static_cast<ygg::uint_t>(fact.variable.value)] = fact.value.value;
    }
    /// The facts whose value is not none, by ascending variable.
    [[nodiscard]] auto facts() const noexcept
    {
        using formalism::planning::FDRValue;
        return std::views::iota(std::size_t { 0 }, values.size())
               | std::views::filter([this](std::size_t i) { return FDRValue(values[i]) != FDRValue::none(); })
               | std::views::transform(
                   [this](std::size_t i)
                   {
                       return urd::data<formalism::planning::FDRFact<formalism::FluentTag>> {
                           urd::index<formalism::planning::FDRVariable<formalism::FluentTag>> { static_cast<ygg::uint_t>(i) },
                           FDRValue(values[i])
                       };
                   });
    }
    void resize(size_t size) { values.resize(size, 0); }
    void clear() { values.clear(); }
    void swap(FactUnpackedStorage& other) noexcept { values.swap(other.values); }

    static constexpr auto identifying_members() noexcept { return urd::Members<&FactUnpackedStorage::values> {}; }
};

}

#endif
