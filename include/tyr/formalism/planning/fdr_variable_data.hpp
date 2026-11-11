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

#include "tyr/formalism/planning/declarations.hpp"

#include <tuple>
#include <urd/elements/flat.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace urd
{

template<::tyr::formalism::FactKind T>
struct data<::tyr::formalism::planning::FDRVariable<T>>
{
    urd::index<::tyr::formalism::planning::FDRVariable<T>> index;
    urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, T>>> atoms;

    data() = default;
    data(urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, T>>> atoms_) : index(), atoms(std::move(atoms_)) {}
    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::atoms> {}; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::atoms> {}; }
};

}

#endif
