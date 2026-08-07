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
#include "tyr/planning/state_index.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/config.hpp"

#include <concepts>
#include <tuple>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/semantics/canonicalization.hpp>

namespace ygg
{
namespace planning = ::tyr::planning;

template<::tyr::TaskKind Kind>
struct Data<planning::State<Kind>>
{
public:
    using TaskType = planning::Task<Kind>;

    Data() noexcept = default;
    Data(ygg::Index<planning::State<Kind>> index,
         planning::FactPackedStorage<Kind, planning::StateStoragePolicyTag> fact_storage,
         planning::AtomPackedStorage<Kind, planning::StateStoragePolicyTag> atom_storage,
         planning::NumericPackedStorage<Kind, planning::StateStoragePolicyTag> numeric_storage) noexcept :
        m_index(index),
        m_fact_storage(fact_storage),
        m_atom_storage(atom_storage),
        m_numeric_storage(numeric_storage)
    {
    }

    ygg::Index<planning::State<Kind>> get_index() const noexcept { return m_index; }

    template<::tyr::formalism::FactKind T>
    auto get_atoms() const noexcept
    {
        if constexpr (std::same_as<T, ::tyr::formalism::FluentTag>)
            return m_fact_storage;
        else if constexpr (std::same_as<T, ::tyr::formalism::DerivedTag>)
            return m_atom_storage;
        else
            static_assert(ygg::dependent_false<T>::value, "Missing case");
    }

    auto get_numeric_variables() const noexcept { return m_numeric_storage; }

    auto identifying_members() const noexcept { return std::tie(m_fact_storage, m_atom_storage, m_numeric_storage); }

private:
    ygg::Index<planning::State<Kind>> m_index;
    planning::FactPackedStorage<Kind, planning::StateStoragePolicyTag> m_fact_storage;
    planning::AtomPackedStorage<Kind, planning::StateStoragePolicyTag> m_atom_storage;
    planning::NumericPackedStorage<Kind, planning::StateStoragePolicyTag> m_numeric_storage;
};

template<::tyr::TaskKind Kind>
bool is_canonical(const ygg::Data<planning::State<Kind>>&) noexcept
{
    return true;
}

template<::tyr::TaskKind Kind>
void canonicalize(ygg::Data<planning::State<Kind>>&) noexcept
{
}
}

#endif
