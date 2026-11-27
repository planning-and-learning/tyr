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

#ifndef TYR_PLANNING_STATES_STATE_DATA_HPP_
#define TYR_PLANNING_STATES_STATE_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/planning/states/storages/atom_storage_data.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/states/storages/fact_storage_data.hpp"
#include "tyr/planning/states/storages/numeric_storage_data.hpp"
#include "tyr/planning/states/storages/state_storage.hpp"
#include "tyr/planning/states/storages/policies/config.hpp"
#include "tyr/planning/states/storages/policies/policy.hpp"

#include <concepts>
#include <tuple>
#include <type_traits>
#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <utility>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/semantics/canonicalization.hpp>

namespace urd
{

/// A state as its repository stores it: its index and the keys of its storages (each a handle in its pool, by the storage
/// policy). It is identified by its fluent facts, numeric values and temporal part, by which the repository deduplicates
/// states; its derived atoms follow from them. Its builder holds the storages being built (state_builder.hpp).
template<::tyr::TaskKind Kind>
struct data<::tyr::planning::State<Kind>>
{
    urd::key<::tyr::planning::State<Kind>> index;
    urd::key<::tyr::planning::FactStorage<Kind>> fact_storage;
    urd::key<::tyr::planning::AtomStorage<Kind>> atom_storage;
    urd::key<::tyr::planning::NumericStorage<Kind>> numeric_storage;
    urd::key<::tyr::planning::TemporalStorage<Kind>> temporal_storage;

    data() = default;

    data(urd::key<::tyr::planning::FactStorage<Kind>> fact_storage_,
         urd::key<::tyr::planning::AtomStorage<Kind>> atom_storage_,
         urd::key<::tyr::planning::NumericStorage<Kind>> numeric_storage_,
         urd::key<::tyr::planning::TemporalStorage<Kind>> temporal_storage_) :
        index(),
        fact_storage(fact_storage_),
        atom_storage(atom_storage_),
        numeric_storage(numeric_storage_),
        temporal_storage(temporal_storage_)
    {
    }

    static constexpr bool copyable() noexcept
    {
        static_assert(std::is_trivially_copyable_v<data>, "a copyable record is trivially copyable");
        return true;
    }

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"fact_storage", &data::fact_storage>,
                           urd::field<"atom_storage", &data::atom_storage>,
                           urd::field<"numeric_storage", &data::numeric_storage>,
                           urd::field<"temporal_storage", &data::temporal_storage>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::fact_storage, &data::numeric_storage, &data::temporal_storage> {}; }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::fact_storage, &data::atom_storage, &data::numeric_storage, &data::temporal_storage> {}; }

    [[nodiscard]] urd::key<::tyr::planning::State<Kind>> key() const noexcept { return urd::key<::tyr::planning::State<Kind>>(index); }
};

}  // namespace urd

namespace tyr::planning
{
}

#endif
