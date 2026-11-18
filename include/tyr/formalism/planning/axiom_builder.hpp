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

#ifndef TYR_FORMALISM_PLANNING_AXIOM_BUILDER_HPP_
#define TYR_FORMALISM_PLANNING_AXIOM_BUILDER_HPP_

#include "tyr/formalism/atom_builder.hpp"
#include "tyr/formalism/planning/axiom_data.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/conjunctive_condition_builder.hpp"
#include "tyr/formalism/variable_builder.hpp"

#include <optional>
#include <urd/elements/builder.hpp>
#include <variant>
#include <vector>

namespace urd
{

/// The builder of Axiom: its record's members; each child built or interned (urd::child).
template<typename C>
struct builder<::tyr::formalism::planning::Axiom<::tyr::LiftedTag>, C>
{
    urd::index<::tyr::formalism::planning::Axiom<::tyr::LiftedTag>> index;
    std::vector<urd::child<::tyr::formalism::Variable, C>> variables;
    urd::child<::tyr::formalism::planning::ConjunctiveCondition<::tyr::LiftedTag>, C> body;
    urd::child<::tyr::formalism::Atom<::tyr::LiftedTag, ::tyr::formalism::DerivedTag>, C> head;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"variables", &builder::variables>, urd::field<"body", &builder::body>, urd::field<"head", &builder::head>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::Members<&builder::variables, &builder::body, &builder::head> {}; }
};

/// The builder of Axiom: its record's members; each child built or interned (urd::child).
template<typename C>
struct builder<::tyr::formalism::planning::Axiom<::tyr::GroundTag>, C>
{
    urd::index<::tyr::formalism::planning::Axiom<::tyr::GroundTag>> index;
    ygg::RowIndex<::tyr::formalism::planning::Axiom<::tyr::LiftedTag>> row;
    urd::child<::tyr::formalism::planning::ConjunctiveCondition<::tyr::GroundTag>, C> body;
    urd::child<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>, C> head;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"row", &builder::row>, urd::field<"body", &builder::body>, urd::field<"head", &builder::head>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::Members<&builder::row> {}; }
};

}  // namespace urd

#endif
