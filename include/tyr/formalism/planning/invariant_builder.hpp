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

#ifndef TYR_FORMALISM_PLANNING_INVARIANT_BUILDER_HPP_
#define TYR_FORMALISM_PLANNING_INVARIANT_BUILDER_HPP_

#include "tyr/formalism/atom_builder.hpp"
#include "tyr/formalism/planning/invariant_data.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"

#include <optional>
#include <urd/elements/builder.hpp>
#include <variant>
#include <vector>

namespace urd
{

/// The builder of Invariant: its record's members; each child built or interned (urd::child).
template<typename C>
struct builder<::tyr::formalism::planning::Invariant, C>
{
    urd::index<::tyr::formalism::planning::Invariant> index;
    ygg::uint_t num_rigid_variables = 0;
    ygg::uint_t num_counted_variables = 0;
    std::vector<urd::child<::tyr::formalism::Atom<::tyr::LiftedTag, ::tyr::formalism::FluentTag>, C>> atoms;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"num_rigid_variables", &builder::num_rigid_variables>,
                           urd::field<"num_counted_variables", &builder::num_counted_variables>,
                           urd::field<"atoms", &builder::atoms>> {};
    }

    static constexpr auto identifying_members() noexcept
    {
        return urd::Members<&builder::num_rigid_variables, &builder::num_counted_variables, &builder::atoms> {};
    }
};

}  // namespace urd

#endif
