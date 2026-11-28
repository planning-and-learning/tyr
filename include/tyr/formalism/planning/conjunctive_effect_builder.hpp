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

#ifndef TYR_FORMALISM_PLANNING_CONJUNCTIVE_EFFECT_BUILDER_HPP_
#define TYR_FORMALISM_PLANNING_CONJUNCTIVE_EFFECT_BUILDER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/literal_builder.hpp"
#include "tyr/formalism/numeric_effect_builder.hpp"
#include "tyr/formalism/planning/conjunctive_effect_data.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/fdr_fact_builder.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"

#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/concepts.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>

namespace urd
{

/// The builder of ConjunctiveEffect: its record's members, each in its built form (urd::built_t).
template<typename C>
struct builder<::tyr::formalism::ConjunctiveEffect<::tyr::LiftedTag>, C>
{
    urd::key<::tyr::formalism::ConjunctiveEffect<::tyr::LiftedTag>> index;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Literal<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>>, C> literals;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::NumericEffect<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>>, C> numeric_effects;
    urd::built_t<urd::flat_optional<urd::index<::tyr::formalism::NumericEffect<::tyr::LiftedTag, ::tyr::formalism::AuxiliaryTag>>>, C>
        auxiliary_numeric_effect;  // :action-cost

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"literals", &builder::literals>,
                           urd::field<"numeric_effects", &builder::numeric_effects>,
                           urd::field<"auxiliary_numeric_effect", &builder::auxiliary_numeric_effect>> {};
    }
};

/// The builder of ConjunctiveEffect: its record's members, each in its built form (urd::built_t).
template<typename C>
struct builder<::tyr::formalism::ConjunctiveEffect<::tyr::GroundTag>, C>
{
    urd::key<::tyr::formalism::ConjunctiveEffect<::tyr::GroundTag>> index;
    urd::built_t<urd::flat_vector<urd::data<::tyr::formalism::FDRFact<::tyr::formalism::FluentTag>>>, C> add_facts;
    urd::built_t<urd::flat_vector<urd::data<::tyr::formalism::FDRFact<::tyr::formalism::FluentTag>>>, C> del_facts;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::NumericEffect<::tyr::GroundTag, ::tyr::formalism::FluentTag>>>, C> numeric_effects;
    urd::built_t<urd::flat_optional<urd::index<::tyr::formalism::NumericEffect<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag>>>, C>
        auxiliary_numeric_effect;  // :action-cost

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"add_facts", &builder::add_facts>,
                           urd::field<"del_facts", &builder::del_facts>,
                           urd::field<"numeric_effects", &builder::numeric_effects>,
                           urd::field<"auxiliary_numeric_effect", &builder::auxiliary_numeric_effect>> {};
    }
};

}  // namespace urd

#endif
