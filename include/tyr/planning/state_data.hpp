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
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>
#include <yggdrasil/semantics/canonicalization.hpp>

namespace ygg
{
template<::tyr::TaskKind Kind>
struct Data<::tyr::planning::State<Kind>>
{
    ygg::Index<::tyr::planning::State<Kind>> index;
    ::tyr::planning::FactPackedStorage<Kind, ::tyr::planning::StateStoragePolicyTag> fact_storage;
    ::tyr::planning::AtomPackedStorage<Kind, ::tyr::planning::StateStoragePolicyTag> atom_storage;
    ::tyr::planning::NumericPackedStorage<Kind, ::tyr::planning::StateStoragePolicyTag> numeric_storage;

    Data() = default;
    Data(::tyr::planning::FactPackedStorage<Kind, ::tyr::planning::StateStoragePolicyTag> fact_storage_,
         ::tyr::planning::AtomPackedStorage<Kind, ::tyr::planning::StateStoragePolicyTag> atom_storage_,
         ::tyr::planning::NumericPackedStorage<Kind, ::tyr::planning::StateStoragePolicyTag> numeric_storage_) :
        index(),
        fact_storage(fact_storage_),
        atom_storage(atom_storage_),
        numeric_storage(numeric_storage_)
    {
    }

    ygg::Index<::tyr::planning::State<Kind>> get_index() const noexcept { return index; }

    template<::tyr::formalism::FactKind T>
    auto get_atom_storage() const noexcept
    {
        if constexpr (std::same_as<T, ::tyr::formalism::FluentTag>)
            return fact_storage;
        else if constexpr (std::same_as<T, ::tyr::formalism::DerivedTag>)
            return atom_storage;
        else
            static_assert(ygg::dependent_false<T>::value, "Missing case");
    }

    auto get_numeric_variables() const noexcept { return numeric_storage; }

    static constexpr auto cista_members() noexcept { return ygg::Members<&Data::index, &Data::fact_storage, &Data::atom_storage, &Data::numeric_storage> {}; }
    static constexpr auto identifying_members() noexcept { return ygg::Members<&Data::fact_storage, &Data::numeric_storage> {}; }
    void clear() noexcept { ygg::clear_members(*this); }
};

template<::tyr::TaskKind Kind>
bool is_canonical(const ygg::Data<::tyr::planning::State<Kind>>&) noexcept
{ return true; }

template<::tyr::TaskKind Kind>
void canonicalize(ygg::Data<::tyr::planning::State<Kind>>&) noexcept
{
}
}

#endif
