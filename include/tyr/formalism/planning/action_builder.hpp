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

#include "tyr/formalism/planning/action_data.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/conditional_effect_builder.hpp"
#include "tyr/formalism/planning/conjunctive_condition_builder.hpp"
#include "tyr/formalism/variable_builder.hpp"

#include <optional>
#include <urd/elements/builder.hpp>
#include <variant>
#include <vector>

namespace urd
{

/// The builder of Action: its record's members; each child built or interned (urd::child).
template<typename C>
struct builder<::tyr::formalism::planning::Action<::tyr::LiftedTag>, C>
{
    urd::index<::tyr::formalism::planning::Action<::tyr::LiftedTag>> index;
    urd::flat_string name;
    urd::flat_string original_name;
    ygg::uint_t original_arity = 0;
    std::vector<urd::child<::tyr::formalism::Variable, C>> variables;
    urd::child<::tyr::formalism::planning::ConjunctiveCondition<::tyr::LiftedTag>, C> condition;
    std::vector<urd::child<::tyr::formalism::planning::ConditionalEffect<::tyr::LiftedTag>, C>> effects;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"name", &builder::name>,
                           urd::field<"original_name", &builder::original_name>,
                           urd::field<"original_arity", &builder::original_arity>,
                           urd::field<"variables", &builder::variables>,
                           urd::field<"condition", &builder::condition>,
                           urd::field<"effects", &builder::effects>> {};
    }

    static constexpr auto identifying_members() noexcept
    {
        return urd::Members<&builder::original_name, &builder::variables, &builder::original_arity, &builder::condition, &builder::effects> {};
    }
};

/// The builder of Action: its record's members; each child built or interned (urd::child).
template<typename C>
struct builder<::tyr::formalism::planning::Action<::tyr::GroundTag>, C>
{
    urd::index<::tyr::formalism::planning::Action<::tyr::GroundTag>> index;
    ygg::RowIndex<::tyr::formalism::planning::Action<::tyr::LiftedTag>> row;
    urd::child<::tyr::formalism::planning::ConjunctiveCondition<::tyr::GroundTag>, C> condition;
    std::vector<urd::child<::tyr::formalism::planning::ConditionalEffect<::tyr::GroundTag>, C>> effects;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"row", &builder::row>, urd::field<"condition", &builder::condition>, urd::field<"effects", &builder::effects>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::Members<&builder::row, &builder::condition, &builder::effects> {}; }
};

}  // namespace urd

#endif
