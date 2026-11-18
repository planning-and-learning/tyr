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

#ifndef TYR_FORMALISM_ATOM_BUILDER_HPP_
#define TYR_FORMALISM_ATOM_BUILDER_HPP_

#include "tyr/formalism/atom_data.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/object_builder.hpp"
#include "tyr/formalism/parameters_builder.hpp"
#include "tyr/formalism/predicate_builder.hpp"

#include <optional>
#include <urd/elements/builder.hpp>
#include <variant>
#include <vector>

namespace urd
{

/// The builder of Atom: its record's members; each child built or interned (urd::child).
template<::tyr::formalism::FactKind F, typename C>
struct builder<::tyr::formalism::Atom<::tyr::LiftedTag, F>, C>
{
    urd::index<::tyr::formalism::Atom<::tyr::LiftedTag, F>> index;
    urd::child<::tyr::formalism::Parameters, C> parameters;
    urd::child<::tyr::formalism::Predicate<F>, C> predicate;
    std::vector<::tyr::formalism::BuiltTerm<C>> terms;

    static constexpr auto fields() noexcept
    {
        return urd::
            Fields<urd::field<"parameters", &builder::parameters>, urd::field<"predicate", &builder::predicate>, urd::field<"terms", &builder::terms>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};

/// The builder of Atom: its record's members; each child built or interned (urd::child).
template<::tyr::formalism::FactKind F, typename C>
struct builder<::tyr::formalism::Atom<::tyr::GroundTag, F>, C>
{
    urd::index<::tyr::formalism::Atom<::tyr::GroundTag, F>> index;
    ygg::RowIndex<::tyr::formalism::Predicate<F>> row;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"row", &builder::row>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::Members<&builder::row> {}; }
};

}  // namespace urd

#endif
