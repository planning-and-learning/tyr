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

#ifndef TYR_PLANNING_STATE_DATA_HPP_
#define TYR_PLANNING_STATE_DATA_HPP_

#include "tyr/formalism/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/state_storage.hpp"
#include "tyr/planning/lifted/state_storage.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/config.hpp"

#include <concepts>
#include <tuple>
#include <urd/elements/flat.hpp>
#include <utility>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/semantics/canonicalization.hpp>

namespace tyr::planning
{
/// A registered state as its repository stores it, packed (StateRepository::packed): its index and its packed
/// storage. It compares by its fluent facts and numeric values, by which the repository deduplicates states; its
/// derived atoms follow from them.
template<TaskKind Kind>
struct PackedState
{
    using FactStorage = typename FactStorageBackend<Kind, StateStoragePolicyTag>::Packed;
    using AtomStorage = typename AtomStorageBackend<Kind, StateStoragePolicyTag>::Packed;
    using NumericStorage = typename NumericStorageBackend<Kind, StateStoragePolicyTag>::Packed;

    urd::index<State<Kind>> index;
    FactStorage fact_storage;
    AtomStorage atom_storage;
    NumericStorage numeric_storage;

    PackedState() = default;
    PackedState(FactStorage fact_storage_, AtomStorage atom_storage_, NumericStorage numeric_storage_) :
        index(),
        fact_storage(fact_storage_),
        atom_storage(atom_storage_),
        numeric_storage(numeric_storage_)
    {
    }

    friend bool operator==(const PackedState& lhs, const PackedState& rhs) { return urd::EqualTo<PackedState> {}(lhs, rhs); }

    static constexpr auto cista_members() noexcept
    { return urd::Members<&PackedState::index, &PackedState::fact_storage, &PackedState::atom_storage, &PackedState::numeric_storage> {}; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&PackedState::fact_storage, &PackedState::numeric_storage> {}; }
};

template<TaskKind Kind>
bool is_canonical(const PackedState<Kind>&) noexcept
{ return true; }

template<TaskKind Kind>
void canonicalize(PackedState<Kind>&) noexcept
{
}
}

namespace urd
{

/// A registered state, unpacked: its index and its storage, read only. A view of an index reads it, and so does the
/// builder that filled it, whose storage it is (builder<State<Kind>> derives from it). The repository's packed record of
/// a state is tyr::planning::PackedState.
template<::tyr::TaskKind Kind>
struct data<::tyr::planning::State<Kind>>
{
    urd::index<::tyr::planning::State<Kind>> index;
    ::tyr::planning::FactUnpackedStorage<Kind> fact_storage;
    ::tyr::planning::AtomUnpackedStorage<Kind> atom_storage;
    ::tyr::planning::NumericUnpackedStorage<Kind> numeric_storage;

    data() = default;
    data(::tyr::planning::FactUnpackedStorage<Kind> fact_storage_,
         ::tyr::planning::AtomUnpackedStorage<Kind> atom_storage_,
         ::tyr::planning::NumericUnpackedStorage<Kind> numeric_storage_) :
        index(),
        fact_storage(std::move(fact_storage_)),
        atom_storage(std::move(atom_storage_)),
        numeric_storage(std::move(numeric_storage_))
    {
    }
    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"fact_storage", &data::fact_storage>,
                           urd::field<"atom_storage", &data::atom_storage>,
                           urd::field<"numeric_storage", &data::numeric_storage>> {};
    }
    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::fact_storage, &data::atom_storage, &data::numeric_storage> {}; }
    /// A registered state is its index.
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::index> {}; }
};

}  // namespace urd

#endif
