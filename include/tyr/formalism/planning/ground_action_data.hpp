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

#ifndef TYR_FORMALISM_PLANNING_GROUND_ACTION_DATA_HPP_
#define TYR_FORMALISM_PLANNING_GROUND_ACTION_DATA_HPP_

#include "tyr/formalism/binding_index.hpp"
#include "tyr/formalism/function_index.hpp"
#include "tyr/formalism/planning/action_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/ground_action_index.hpp"
#include "tyr/formalism/planning/ground_conditional_effect_index.hpp"
#include "tyr/formalism/planning/ground_conjunctive_condition_index.hpp"
#include "tyr/formalism/planning/ground_conjunctive_effect_index.hpp"

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{
using namespace ::tyr;

template<>
struct Data<::tyr::formalism::planning::GroundAction>
{
    ygg::Index<::tyr::formalism::planning::GroundAction> index;
    ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::planning::Action>> binding;
    ygg::Index<::tyr::formalism::planning::GroundConjunctiveCondition> condition;
    ygg::IndexList<::tyr::formalism::planning::GroundConditionalEffect> effects;

    Data() = default;
    Data(ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::planning::Action>> binding_,
         ygg::Index<::tyr::formalism::planning::GroundConjunctiveCondition> condition_,
         ygg::IndexList<::tyr::formalism::planning::GroundConditionalEffect> effects_) :
        index(),
        binding(binding_),
        condition(condition_),
        effects(std::move(effects_))
    {
    }
    // Python constructor
    template<typename C>
    Data(::ygg::View<ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::planning::Action>>, C> binding_,
         ::ygg::View<ygg::Index<::tyr::formalism::planning::GroundConjunctiveCondition>, C> condition_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::GroundConditionalEffect>, C>>& effects_) :
        index(),
        binding(),
        condition(),
        effects()
    {
        set(binding_, binding);
        set(condition_, condition);
        set(effects_, effects);
    }
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(binding);
        ygg::clear(condition);
        ygg::clear(effects);
    }

    auto cista_members() const noexcept { return std::tie(index, binding, condition, effects); }
    // Have to include effects because row only binds objects to non-effect quantified variables.
    auto identifying_members() const noexcept { return std::tie(binding, condition, effects); }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::GroundAction>);
}

#endif
