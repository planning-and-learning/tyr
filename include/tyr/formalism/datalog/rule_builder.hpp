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

#include "tyr/declarations.hpp"
#include "tyr/formalism/atom_builder.hpp"
#include "tyr/formalism/conjunctive_condition_builder.hpp"
#include "tyr/formalism/datalog/rule_body_builder.hpp"
#include "tyr/formalism/datalog/rule_data.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/variable_builder.hpp"

#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/concepts.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <yggdrasil/core/config.hpp>

namespace urd
{

/// The builder of Rule: its record's members, each in its built form (urd::built_t).
template<typename C>
struct builder<::tyr::formalism::Rule<::tyr::LiftedTag>, C>
{
    urd::key<::tyr::formalism::Rule<::tyr::LiftedTag>> index;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Variable>>, C> variables;
    urd::built_t<urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::LiftedTag>>, C> body;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>>, C> fluent_heads;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::LiftedTag, ::tyr::formalism::DerivedTag>>>, C> derived_heads;
    urd::built_t<ygg::float_t, C> cost { 0 };  ///< The edge cost of deriving a head.

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"variables", &builder::variables>,
                           urd::field<"body", &builder::body>,
                           urd::field<"fluent_heads", &builder::fluent_heads>,
                           urd::field<"derived_heads", &builder::derived_heads>,
                           urd::field<"cost", &builder::cost>> {};
    }
};

/// The builder of Rule: its record's members, each in its built form (urd::built_t).
template<typename C>
struct builder<::tyr::formalism::Rule<::tyr::GroundTag>, C>
{
    urd::key<::tyr::formalism::Rule<::tyr::GroundTag>> index;
    urd::built_t<urd::index<::tyr::formalism::RuleBody<::tyr::GroundTag>>, C> body;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::FluentTag>>>, C> fluent_heads;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>>>, C> derived_heads;
    urd::built_t<ygg::float_t, C> cost { 0 };  ///< The edge cost of deriving a head.

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"body", &builder::body>,
                           urd::field<"fluent_heads", &builder::fluent_heads>,
                           urd::field<"derived_heads", &builder::derived_heads>,
                           urd::field<"cost", &builder::cost>> {};
    }
};

}  // namespace urd

#endif
