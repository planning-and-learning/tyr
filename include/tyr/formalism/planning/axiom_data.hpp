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

#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"

#include <tuple>
#include <urd/elements/flat.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace urd
{

template<>
struct data<::tyr::formalism::planning::Axiom<::tyr::LiftedTag>>
{
    urd::index<::tyr::formalism::planning::Axiom<::tyr::LiftedTag>> index;
    urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables;
    urd::index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::LiftedTag>> body;
    urd::index<::tyr::formalism::Atom<::tyr::LiftedTag, ::tyr::formalism::DerivedTag>> head;

    data() = default;
    data(urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables_,
         urd::index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::LiftedTag>> body_,
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

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::variables, &data::body, &data::head> {}; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::variables, &data::body, &data::head> {}; }
};

template<>
struct data<::tyr::formalism::planning::Axiom<::tyr::GroundTag>>
{
    urd::index<::tyr::formalism::planning::Axiom<::tyr::GroundTag>> index;
    ygg::RowIndex<::tyr::formalism::planning::Axiom<::tyr::LiftedTag>> row;
    urd::index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::GroundTag>> body;
    urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> head;

    data() = default;
    data(ygg::RowIndex<::tyr::formalism::planning::Axiom<::tyr::LiftedTag>> row_,
         urd::index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::GroundTag>> body_,
         urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> head_) :
        index(),
        row(row_),
        body(body_),
        head(head_)
    {
    }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::row, &data::body, &data::head> {}; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::row> {}; }
};

}

#endif
