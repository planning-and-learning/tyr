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

#ifndef TYR_PLANNING_STATES_STORAGES_FACT_STORAGE_DATA_HPP_
#define TYR_PLANNING_STATES_STORAGES_FACT_STORAGE_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/states/storages/policies/config.hpp"
#include "tyr/planning/states/storages/policies/policy.hpp"
#include "tyr/planning/states/storages/state_storage.hpp"

#include <type_traits>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <yggdrasil/core/config.hpp>

namespace urd
{

/// A ground state's fluent facts: a value per FDR variable. Its pool packs them bitwise (by its storage policy) and names
/// them by its handle, its key (urd::key_value_of).
template<>
struct data<::tyr::planning::FactStorage<::tyr::GroundTag>>
{
    urd::key<::tyr::planning::FactStorage<::tyr::GroundTag>> index;
    urd::flat_vector<ygg::uint_t> values;

    data() = default;

    explicit data(urd::flat_vector<ygg::uint_t> values_) : index(), values(std::move(values_)) {}

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"values", &data::values>> {}; }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::planning::FactStorage<::tyr::GroundTag>> key() const noexcept { return index; }
};

/// A lifted state's fluent facts: a bit per (binary) fluent variable, as words. Its pool packs them and names them by
/// its handle.
template<>
struct data<::tyr::planning::FactStorage<::tyr::LiftedTag>>
{
    urd::key<::tyr::planning::FactStorage<::tyr::LiftedTag>> index;
    ::tyr::planning::StateBitWords indices;

    data() = default;

    explicit data(::tyr::planning::StateBitWords indices_) : index(), indices(std::move(indices_)) {}

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"indices", &data::indices>> {}; }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::planning::FactStorage<::tyr::LiftedTag>> key() const noexcept { return index; }
};

}  // namespace urd

#endif
