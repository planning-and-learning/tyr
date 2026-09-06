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

#ifndef TYR_FORMALISM_PLANNING_FDR_VARIABLE_DATA_HPP_
#define TYR_FORMALISM_PLANNING_FDR_VARIABLE_DATA_HPP_

#include "tyr/formalism/planning/atom_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_variable_index.hpp"

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{

template<::tyr::formalism::FactKind T>
struct Data<::tyr::formalism::planning::FDRVariable<T>>
{
    ygg::Index<::tyr::formalism::planning::FDRVariable<T>> index;
    ygg::IndexList<::tyr::formalism::planning::Atom<::tyr::GroundTag, T>> atoms;

    Data() = default;
    Data(ygg::IndexList<::tyr::formalism::planning::Atom<::tyr::GroundTag, T>> atoms_) : index(), atoms(std::move(atoms_)) {}
    // Python constructor
    template<typename C>
    Data(const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::Atom<::tyr::GroundTag, T>>, C>>& atoms_) : index(), atoms()
    {
        set(atoms_, atoms);
    }
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(atoms);
    }

    auto cista_members() const noexcept { return std::tie(index, atoms); }
    auto identifying_members() const noexcept { return std::tie(atoms); }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::FDRVariable<::tyr::formalism::StaticTag>>);
}

#endif
