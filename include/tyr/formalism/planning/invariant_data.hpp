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
#ifndef TYR_FORMALISM_PLANNING_INVARIANT_DATA_HPP_
#define TYR_FORMALISM_PLANNING_INVARIANT_DATA_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

#include <tuple>
#include <utility>
#include <vector>
#include <urd/elements/flat.hpp>
#include <yggdrasil/core/config.hpp>

namespace urd
{

/// A synthesized invariant (Helmert 2009): at most one atom of the instance that binds the rigid variables
/// [0, num_rigid_variables) is true, where the counted variable num_rigid_variables ranges over all objects.
template<>
struct data<::tyr::formalism::planning::Invariant>
{
    urd::index<::tyr::formalism::planning::Invariant> index;
    ygg::uint_t num_rigid_variables = 0;
    ygg::uint_t num_counted_variables = 0;
    urd::flat_vector<urd::index<::tyr::formalism::planning::Atom<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>> atoms;

    data() = default;
    data(ygg::uint_t num_rigid_variables_,
         ygg::uint_t num_counted_variables_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::Atom<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>> atoms_) :
        index(),
        num_rigid_variables(num_rigid_variables_),
        num_counted_variables(num_counted_variables_),
        atoms(std::move(atoms_))
    {
    }
    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;

    static constexpr auto cista_members() noexcept
    { return urd::Members<&data::index, &data::num_rigid_variables, &data::num_counted_variables, &data::atoms> {}; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::num_rigid_variables, &data::num_counted_variables, &data::atoms> {}; }
};

}

#endif
