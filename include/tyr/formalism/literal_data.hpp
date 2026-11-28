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

#ifndef TYR_FORMALISM_LITERAL_DATA_HPP_
#define TYR_FORMALISM_LITERAL_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <type_traits>
#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/index.hpp>

namespace urd
{

template<::tyr::TaskKind T, ::tyr::formalism::FactKind F>
struct data<::tyr::formalism::Literal<T, F>>
{
    urd::key<::tyr::formalism::Literal<T, F>> index;
    urd::index<::tyr::formalism::Atom<T, F>> atom;
    bool polarity = false;

    data() = default;

    data(urd::index<::tyr::formalism::Atom<T, F>> atom_, bool polarity_) : index(), atom(atom_), polarity(polarity_) {}

    static constexpr bool copyable() noexcept
    {
        static_assert(std::is_trivially_copyable_v<data>, "a copyable record is trivially copyable");
        return true;
    }

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"atom", &data::atom>, urd::field<"polarity", &data::polarity>> {}; }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::formalism::Literal<T, F>> key() const noexcept { return urd::key<::tyr::formalism::Literal<T, F>>(index); }
};

}

#endif
