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

#ifndef TYR_FORMALISM_PLANNING_AXIOM_DATA_HPP_
#define TYR_FORMALISM_PLANNING_AXIOM_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <type_traits>
#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <utility>
#include <yggdrasil/formalism/declarations.hpp>

namespace urd
{

template<>
struct data<::tyr::formalism::Axiom<::tyr::LiftedTag>>
{
    urd::key<::tyr::formalism::Axiom<::tyr::LiftedTag>> index;
    urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables;
    urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::LiftedTag>> body;
    urd::index<::tyr::formalism::Atom<::tyr::LiftedTag, ::tyr::formalism::DerivedTag>> head;

    data() = default;

    data(urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables_,
         urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::LiftedTag>> body_,
         urd::index<::tyr::formalism::Atom<::tyr::LiftedTag, ::tyr::formalism::DerivedTag>> head_) :
        index(),
        variables(std::move(variables_)),
        body(body_),
        head(head_)
    {
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    { return urd::Fields<urd::field<"variables", &data::variables>, urd::field<"body", &data::body>, urd::field<"head", &data::head>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::variables, &data::body, &data::head> {}; }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::variables, &data::body, &data::head> {}; }

    [[nodiscard]] urd::key<::tyr::formalism::Axiom<::tyr::LiftedTag>> key() const noexcept
    { return urd::key<::tyr::formalism::Axiom<::tyr::LiftedTag>>(index); }
};

template<>
struct data<::tyr::formalism::Axiom<::tyr::GroundTag>>
{
    urd::key<::tyr::formalism::Axiom<::tyr::GroundTag>> index;
    urd::key<::tyr::formalism::Axiom<::tyr::LiftedTag>, ygg::Row<::tyr::formalism::Axiom<::tyr::LiftedTag>>> row;
    urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::GroundTag>> body;
    urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> head;

    data() = default;

    data(urd::key<::tyr::formalism::Axiom<::tyr::LiftedTag>, ygg::Row<::tyr::formalism::Axiom<::tyr::LiftedTag>>> row_,
         urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::GroundTag>> body_,
         urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> head_) :
        index(),
        row(row_),
        body(body_),
        head(head_)
    {
    }

    static constexpr bool copyable() noexcept
    {
        static_assert(std::is_trivially_copyable_v<data>, "a copyable record is trivially copyable");
        return true;
    }

    static constexpr auto fields() noexcept
    { return urd::Fields<urd::field<"row", &data::row>, urd::field<"body", &data::body>, urd::field<"head", &data::head>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::row> {}; }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::row, &data::body, &data::head> {}; }

    [[nodiscard]] urd::key<::tyr::formalism::Axiom<::tyr::GroundTag>> key() const noexcept
    { return urd::key<::tyr::formalism::Axiom<::tyr::GroundTag>>(index); }
};

}

#endif
