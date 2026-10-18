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

#include <tuple>
#include <utility>
#include <vector>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{

/// A synthesized invariant (Helmert 2009): at most one atom of the instance that binds the rigid variables
/// [0, num_rigid_variables) is true, where the counted variable num_rigid_variables ranges over all objects.
template<>
struct Data<::tyr::formalism::planning::Invariant>
{
    ygg::Index<::tyr::formalism::planning::Invariant> index;
    ygg::uint_t num_rigid_variables;
    ygg::uint_t num_counted_variables;
    ygg::IndexList<::tyr::formalism::planning::Atom<::tyr::LiftedTag, ::tyr::formalism::FluentTag>> atoms;

    Data() = default;
    Data(ygg::uint_t num_rigid_variables_,
         ygg::uint_t num_counted_variables_,
         ygg::IndexList<::tyr::formalism::planning::Atom<::tyr::LiftedTag, ::tyr::formalism::FluentTag>> atoms_) :
        index(),
        num_rigid_variables(num_rigid_variables_),
        num_counted_variables(num_counted_variables_),
        atoms(std::move(atoms_))
    {
    }
    // Python constructor
    template<typename C>
    Data(ygg::uint_t num_rigid_variables_,
         ygg::uint_t num_counted_variables_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::Atom<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>, C>>& atoms_) :
        index(),
        num_rigid_variables(num_rigid_variables_),
        num_counted_variables(num_counted_variables_),
        atoms()
    {
        set(atoms_, atoms);
    }
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    auto cista_members() noexcept { return std::tie(index, num_rigid_variables, num_counted_variables, atoms); }
    auto cista_members() const noexcept { return std::tie(index, num_rigid_variables, num_counted_variables, atoms); }
    auto identifying_members() const noexcept { return std::tie(num_rigid_variables, num_counted_variables, atoms); }
    void clear() noexcept
    {
        std::apply([](auto&... member) { (ygg::clear(member), ...); }, cista_members());
    }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::Invariant>);
}

#endif
