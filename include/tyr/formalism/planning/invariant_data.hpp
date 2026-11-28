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

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/tags.hpp>
#include <utility>
#include <yggdrasil/core/config.hpp>

namespace urd
{

/// A synthesized invariant (Helmert 2009): at most one atom of the instance that binds the rigid variables
/// [0, num_rigid_variables) is true, where the counted variable num_rigid_variables ranges over all objects.
template<>
struct data<::tyr::formalism::Invariant>
{
    urd::key<::tyr::formalism::Invariant> index;
    ygg::uint_t num_rigid_variables = 0;
    ygg::uint_t num_counted_variables = 0;
    urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>> atoms;

    data() = default;

    data(ygg::uint_t num_rigid_variables_,
         ygg::uint_t num_counted_variables_,
         urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>> atoms_) :
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
    ~data() = default;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"num_rigid_variables", &data::num_rigid_variables>,
                           urd::field<"num_counted_variables", &data::num_counted_variables>,
                           urd::field<"atoms", &data::atoms, urd::sorted, urd::unique>> {};
    }

    static constexpr auto cista_members() noexcept
    { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::formalism::Invariant> key() const noexcept { return urd::key<::tyr::formalism::Invariant>(index); }
};

}

#endif
