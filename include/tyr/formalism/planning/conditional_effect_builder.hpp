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

#ifndef TYR_FORMALISM_PLANNING_CONDITIONAL_EFFECT_BUILDER_HPP_
#define TYR_FORMALISM_PLANNING_CONDITIONAL_EFFECT_BUILDER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/conjunctive_condition_builder.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/conditional_effect_data.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_builder.hpp"
#include "tyr/formalism/variable_builder.hpp"

#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/concepts.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>

namespace urd
{

/// The builder of ConditionalEffect: its record's members, each in its built form (urd::built_t).
template<typename C>
struct builder<::tyr::formalism::ConditionalEffect<::tyr::LiftedTag>, C>
{
    urd::key<::tyr::formalism::ConditionalEffect<::tyr::LiftedTag>> index;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Variable>>, C> variables;
    urd::built_t<urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::LiftedTag>>, C> condition;
    urd::built_t<urd::index<::tyr::formalism::ConjunctiveEffect<::tyr::LiftedTag>>, C> effect;

    static constexpr auto fields() noexcept
    {
        return urd::
            Fields<urd::field<"variables", &builder::variables>, urd::field<"condition", &builder::condition>, urd::field<"effect", &builder::effect>> {};
    }

};

/// The builder of ConditionalEffect: its record's members, each in its built form (urd::built_t).
template<typename C>
struct builder<::tyr::formalism::ConditionalEffect<::tyr::GroundTag>, C>
{
    urd::key<::tyr::formalism::ConditionalEffect<::tyr::GroundTag>> index;
    urd::built_t<urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::GroundTag>>, C> condition;
    urd::built_t<urd::index<::tyr::formalism::ConjunctiveEffect<::tyr::GroundTag>>, C> effect;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"condition", &builder::condition>, urd::field<"effect", &builder::effect>> {}; }

};

}  // namespace urd

#endif
