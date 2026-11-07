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

#include <cassert>
#include <cstddef>
#include <ranges>
#include <tuple>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

template<>
struct FactUnpackedStorage<LiftedTag>
{
    StateBits indices;

    [[nodiscard]] formalism::planning::FDRValue get(urd::index<formalism::planning::FDRVariable<formalism::FluentTag>> index) const
    { return formalism::planning::FDRValue(indices.contains(static_cast<ygg::uint_t>(index.value))); }
    void set(urd::data<formalism::planning::FDRFact<formalism::FluentTag>> fact)
    {
        assert(fact.value.value < 2);  // Lifted fluent variables are binary.
        const auto index = static_cast<ygg::uint_t>(fact.variable.value);
        if (fact.value != formalism::planning::FDRValue::none())
            indices.insert(index);
        else
            indices.erase(index);
    }
    /// One past the last variable set: variables past it are none.
    [[nodiscard]] std::size_t num_variables() const noexcept { return indices.significant_words().size() * 64; }
    /// The facts set (value 1), by ascending variable.
    [[nodiscard]] auto facts() const noexcept
    {
        return urd::transform(indices,
                              [](ygg::uint_t i)
                              {
                                  return urd::data<formalism::planning::FDRFact<formalism::FluentTag>> {
                                      urd::index<formalism::planning::FDRVariable<formalism::FluentTag>> { i },
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
