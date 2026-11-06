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
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/config.hpp"

#include <concepts>
#include <tuple>
#include <utility>
#include <yggdrasil/core/config.hpp>
#include <urd/elements/flat.hpp>
#include <urd/repr/core/dependent_false.hpp>
#include <yggdrasil/semantics/canonicalization.hpp>

namespace tyr::planning
{
/// A registered state as its repository stores it, packed (StateRepository::packed): its index and its packed
/// storage. It compares by its fluent facts and numeric values, by which the repository deduplicates states; its
/// derived atoms follow from them.
template<TaskKind Kind>
struct PackedState
{
    urd::index<State<Kind>> index;
    FactPackedStorage<Kind, StateStoragePolicyTag> fact_storage;
    AtomPackedStorage<Kind, StateStoragePolicyTag> atom_storage;
    NumericPackedStorage<Kind, StateStoragePolicyTag> numeric_storage;

    PackedState() = default;
    PackedState(FactPackedStorage<Kind, StateStoragePolicyTag> fact_storage_,
                AtomPackedStorage<Kind, StateStoragePolicyTag> atom_storage_,
                NumericPackedStorage<Kind, StateStoragePolicyTag> numeric_storage_) :
        index(),
        fact_storage(fact_storage_),
        atom_storage(atom_storage_),
        numeric_storage(numeric_storage_)
    {
    }

    urd::index<State<Kind>> get_index() const noexcept { return index; }

    template<formalism::FactKind T>
    auto get_atom_storage() const noexcept
    {
        if constexpr (std::same_as<T, formalism::FluentTag>)
            return fact_storage;
        else if constexpr (std::same_as<T, formalism::DerivedTag>)
            return atom_storage;
        else
            static_assert(urd::repr::dependent_false<T>::value, "Missing case");
    }

    auto get_numeric_variables() const noexcept { return numeric_storage; }

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

#endif
