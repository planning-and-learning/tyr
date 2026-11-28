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

#ifndef TYR_PLANNING_STATES_STORAGES_GROUND_FACT_STORAGE_BUILDER_HPP_
#define TYR_PLANNING_STATES_STORAGES_GROUND_FACT_STORAGE_BUILDER_HPP_

#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/states/storages/fact_storage_data.hpp"
#include "tyr/planning/states/storages/ground/state_layout.hpp"
#include "tyr/planning/states/storages/policies/policy.hpp"
#include "tyr/planning/states/storages/state_storage.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <tuple>
#include <urd/elements/flat.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <yggdrasil/core/config.hpp>

namespace urd
{

/// A ground state's fluent facts being built: a value per FDR variable (state_data.hpp).
template<typename C>
struct builder<::tyr::planning::FactStorage<::tyr::GroundTag>, C>
{
    urd::key<::tyr::planning::FactStorage<::tyr::GroundTag>> index;
    urd::built_t<urd::flat_vector<ygg::uint_t>, C> values;

    void set(urd::data<::tyr::formalism::FDRFact<::tyr::formalism::FluentTag>> fact)
    {
        assert(static_cast<ygg::uint_t>(fact.variable.value) < values.size());
        values[static_cast<ygg::uint_t>(fact.variable.value)] = fact.value.value;
    }

    void resize(size_t size) { values.resize(size, 0); }

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"values", &builder::values>> {}; }

    static constexpr auto functions() noexcept
    { return urd::Functions<urd::function<"set", &builder::set, urd::arg<"fact">>, urd::function<"resize", &builder::resize, urd::arg<"size">>> {}; }
};

}  // namespace urd

#endif
