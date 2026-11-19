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

#ifndef TYR_FORMALISM_PLANNING_FDR_VARIABLE_BUILDER_HPP_
#define TYR_FORMALISM_PLANNING_FDR_VARIABLE_BUILDER_HPP_

#include "tyr/formalism/atom_builder.hpp"
#include "tyr/formalism/planning/fdr_variable_data.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"

#include <optional>
#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <variant>
#include <vector>

namespace urd
{

/// The builder of FDRVariable: its record's members, each in its built form (urd::built_t).
template<::tyr::formalism::FactKind T, typename C>
struct builder<::tyr::formalism::planning::FDRVariable<T>, C>
{
    urd::index<::tyr::formalism::planning::FDRVariable<T>> index;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, T>>>, C> atoms;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"atoms", &builder::atoms>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::Members<&builder::atoms> {}; }
};

}  // namespace urd

#endif
