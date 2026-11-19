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

#ifndef TYR_FORMALISM_DATALOG_RULE_BUILDER_HPP_
#define TYR_FORMALISM_DATALOG_RULE_BUILDER_HPP_

#include "tyr/formalism/atom_builder.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_builder.hpp"
#include "tyr/formalism/datalog/rule_data.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/variable_builder.hpp"

#include <optional>
#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <variant>
#include <vector>

namespace urd
{

/// The builder of Rule: its record's members, each in its built form (urd::built_t).
template<typename C>
struct builder<::tyr::formalism::datalog::Rule<::tyr::LiftedTag>, C>
{
    urd::index<::tyr::formalism::datalog::Rule<::tyr::LiftedTag>> index;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Variable>>, C> variables;
    urd::built_t<urd::index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::LiftedTag>>, C> body;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>>, C> heads;
    urd::built_t<ygg::float_t, C> cost { 0 };  ///< The edge cost of deriving a head.

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"variables", &builder::variables>,
                           urd::field<"body", &builder::body>,
                           urd::field<"heads", &builder::heads>,
                           urd::field<"cost", &builder::cost>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::Members<&builder::variables, &builder::body, &builder::heads, &builder::cost> {}; }
};

/// The builder of Rule: its record's members, each in its built form (urd::built_t).
template<typename C>
struct builder<::tyr::formalism::datalog::Rule<::tyr::GroundTag>, C>
{
    urd::index<::tyr::formalism::datalog::Rule<::tyr::GroundTag>> index;
    urd::built_t<urd::index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::GroundTag>>, C> body;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::FluentTag>>>, C> heads;
    urd::built_t<ygg::float_t, C> cost { 0 };  ///< The edge cost of deriving a head.

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"body", &builder::body>, urd::field<"heads", &builder::heads>, urd::field<"cost", &builder::cost>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};

}  // namespace urd

#endif
