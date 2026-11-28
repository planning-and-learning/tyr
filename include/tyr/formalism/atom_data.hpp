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

#ifndef TYR_FORMALISM_ATOM_DATA_HPP_
#define TYR_FORMALISM_ATOM_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <type_traits>
#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/index.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <utility>
#include <yggdrasil/formalism/declarations.hpp>

namespace urd
{

template<::tyr::formalism::FactKind F>
struct data<::tyr::formalism::Atom<::tyr::LiftedTag, F>>
{
    urd::key<::tyr::formalism::Atom<::tyr::LiftedTag, F>> index;
    urd::index<::tyr::formalism::Parameters> parameters;
    urd::index<::tyr::formalism::Predicate<F>> predicate;
    ::tyr::formalism::TermList terms;

    data() = default;

    data(urd::index<::tyr::formalism::Parameters> parameters_, urd::index<::tyr::formalism::Predicate<F>> predicate_, ::tyr::formalism::TermList terms_) :
        index(),
        parameters(parameters_),
        predicate(predicate_),
        terms(std::move(terms_))
    {
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    { return urd::Fields<urd::field<"parameters", &data::parameters>, urd::field<"predicate", &data::predicate>, urd::field<"terms", &data::terms>> {}; }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::formalism::Atom<::tyr::LiftedTag, F>> key() const noexcept
    { return urd::key<::tyr::formalism::Atom<::tyr::LiftedTag, F>>(index); }
};

template<::tyr::formalism::FactKind F>
struct data<::tyr::formalism::Atom<::tyr::GroundTag, F>>
{
    urd::key<::tyr::formalism::Atom<::tyr::GroundTag, F>> index;
    urd::key<::tyr::formalism::Predicate<F>, ygg::Row<::tyr::formalism::Predicate<F>>> row;

    data() = default;

    data(urd::key<::tyr::formalism::Predicate<F>, ygg::Row<::tyr::formalism::Predicate<F>>> row_) : index(), row(row_) {}

    static constexpr bool copyable() noexcept
    {
        static_assert(std::is_trivially_copyable_v<data>, "a copyable record is trivially copyable");
        return true;
    }

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"row", &data::row>> {}; }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::formalism::Atom<::tyr::GroundTag, F>> key() const noexcept
    { return urd::key<::tyr::formalism::Atom<::tyr::GroundTag, F>>(index); }
};

}

#endif
