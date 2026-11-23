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

#ifndef TYR_FORMALISM_LITERAL_BUILDER_HPP_
#define TYR_FORMALISM_LITERAL_BUILDER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/atom_builder.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/literal_data.hpp"
#include "tyr/formalism/literal_view.hpp"

#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/concepts.hpp>
#include <urd/elements/index.hpp>

namespace urd
{

/// The builder of Literal: its record's members, each in its built form (urd::built_t).
template<::tyr::TaskKind T, ::tyr::formalism::FactKind F, typename C>
struct builder<::tyr::formalism::Literal<T, F>, C>
{
    urd::key<::tyr::formalism::Literal<T, F>> index;
    urd::built_t<urd::index<::tyr::formalism::Atom<T, F>>, C> atom;
    urd::built_t<bool, C> polarity = false;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"atom", &builder::atom>, urd::field<"polarity", &builder::polarity>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::Members<&builder::atom, &builder::polarity> {}; }
};

}  // namespace urd

#endif
