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

#ifndef TYR_FORMALISM_PLANNING_ACTION_BUILDER_HPP_
#define TYR_FORMALISM_PLANNING_ACTION_BUILDER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/conjunctive_condition_builder.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/action_data.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/choice_effect_builder.hpp"
#include "tyr/formalism/planning/conditional_effect_builder.hpp"
#include "tyr/formalism/planning/temporal_part_builder.hpp"
#include "tyr/formalism/variable_builder.hpp"

#include <urd/elements/builder.hpp>
#include <yggdrasil/formalism/row_builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/concepts.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/declarations.hpp>

namespace urd
{

/// The builder of Action: its record's members, each in its built form (urd::built_t).
template<typename C>
struct builder<::tyr::formalism::Action<::tyr::LiftedTag>, C>
{
    urd::key<::tyr::formalism::Action<::tyr::LiftedTag>> index;
    urd::built_t<urd::flat_string, C> name;
    urd::built_t<urd::flat_string, C> original_name;
    urd::built_t<ygg::uint_t, C> original_arity = 0;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Variable>>, C> variables;
    urd::built_t<urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::LiftedTag>>, C> condition;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::ConditionalEffect<::tyr::LiftedTag>>>, C> effects;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::ChoiceEffect<::tyr::LiftedTag>>>, C> choices;
    urd::built_t<urd::flat_optional<urd::index<::tyr::formalism::TemporalPart<::tyr::LiftedTag>>>, C> temporal_part;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"name", &builder::name>,
                           urd::field<"original_name", &builder::original_name>,
                           urd::field<"original_arity", &builder::original_arity>,
                           urd::field<"variables", &builder::variables>,
                           urd::field<"condition", &builder::condition>,
                           urd::field<"effects", &builder::effects>,
                           urd::field<"choices", &builder::choices>,
                           urd::field<"temporal_part", &builder::temporal_part>> {};
    }

    static constexpr auto identifying_members() noexcept
    {
        return urd::Members<&builder::original_name,
                            &builder::variables,
                            &builder::original_arity,
                            &builder::condition,
                            &builder::effects,
                            &builder::choices,
                            &builder::temporal_part> {};
    }
};

/// The builder of Action: its record's members, each in its built form (urd::built_t).
template<typename C>
struct builder<::tyr::formalism::Action<::tyr::GroundTag>, C>
{
    urd::key<::tyr::formalism::Action<::tyr::GroundTag>> index;
    urd::built_t<urd::key<::tyr::formalism::Action<::tyr::LiftedTag>, ygg::Row<::tyr::formalism::Action<::tyr::LiftedTag>>>, C> row;
    urd::built_t<urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::GroundTag>>, C> condition;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::ConditionalEffect<::tyr::GroundTag>>>, C> effects;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::ChoiceEffect<::tyr::GroundTag>>>, C> choices;
    urd::built_t<urd::flat_optional<urd::index<::tyr::formalism::TemporalPart<::tyr::GroundTag>>>, C> temporal_part;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"row", &builder::row>,
                           urd::field<"condition", &builder::condition>,
                           urd::field<"effects", &builder::effects>,
                           urd::field<"choices", &builder::choices>,
                           urd::field<"temporal_part", &builder::temporal_part>> {};
    }

    static constexpr auto identifying_members() noexcept
    { return urd::Members<&builder::row, &builder::condition, &builder::effects, &builder::choices, &builder::temporal_part> {}; }
};

}  // namespace urd

#endif
