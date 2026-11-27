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

#ifndef TYR_PLANNING_STATES_STORAGES_LIFTED_FACT_STORAGE_BUILDER_HPP_
#define TYR_PLANNING_STATES_STORAGES_LIFTED_FACT_STORAGE_BUILDER_HPP_

#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/states/storages/fact_storage_data.hpp"
#include "tyr/planning/states/storages/state_storage.hpp"
#include "tyr/planning/states/storages/policies/policy.hpp"

#include <cassert>
#include <climits>
#include <cstddef>
#include <ranges>
#include <tuple>
#include <utility>
#include <yggdrasil/core/config.hpp>

namespace urd
{

/// A lifted state's fluent facts being built: a bit per (binary) fluent variable (state_data.hpp).
template<typename C>
struct builder<::tyr::planning::FactStorage<::tyr::LiftedTag>, C>
{
    urd::key<::tyr::planning::FactStorage<::tyr::LiftedTag>> index;
    urd::built_t<::tyr::planning::StateBitWords, C> indices;

    void set(urd::data<::tyr::formalism::FDRFact<::tyr::formalism::FluentTag>> fact)
    {
        assert(fact.value.value < 2);  // Lifted fluent variables are binary.
        const auto variable = static_cast<ygg::uint_t>(fact.variable.value);
        if (fact.value != ::tyr::formalism::FDRValue::none())
            indices.insert(variable);
        else
            indices.erase(variable);
    }

    void clear() { indices.clear(); }

    /// Takes the bits of a view of a builder of its kind (another state's).
    void assign(const urd::borrowed_view<::tyr::planning::FactStorage<::tyr::LiftedTag>, C, builder>& storage) { indices = storage.get_indices(); }

    void swap(builder& other) noexcept
    {
        std::swap(index, other.index);
        indices.swap(other.indices);
    }

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"indices", &builder::indices>> {}; }

    static constexpr auto functions() noexcept
    {
        return urd::Functions<urd::function<"set", &builder::set, urd::arg<"fact">>,
                              urd::function<"clear", &builder::clear>,
                              urd::function<"assign", &builder::assign, urd::arg<"storage">>,
                              urd::function<"swap", &builder::swap, urd::arg<"other">>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};

}  // namespace urd

#endif
