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

#ifndef TYR_FORMALISM_PLANNING_MUTABLE_ACTION_HPP_
#define TYR_FORMALISM_PLANNING_MUTABLE_ACTION_HPP_

#include <yggdrasil/semantics/comparison.hpp>
#include <yggdrasil/semantics/hash.hpp>
#include "tyr/formalism/planning/mutable/conditional_effect.hpp"
#include "tyr/formalism/planning/mutable/conjunctive_condition.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/unification/structure_traits.hpp"
#include "tyr/formalism/unification/structure_traits_impl.hpp"

#include <cstddef>
#include <tuple>
#include <utility>
#include <vector>

namespace tyr::formalism::planning
{
struct MutableAction : ygg::comparison::Mixin<MutableAction>
{
    size_t num_variables;
    MutableConjunctiveCondition condition;
    MutableConditionalEffectList effects;

    MutableAction() = default;
    MutableAction(size_t num_variables, MutableConjunctiveCondition condition, MutableConditionalEffectList effects) :
        num_variables(num_variables),
        condition(std::move(condition)),
        effects(std::move(effects))
    {
    }
    MutableAction(ActionView element) : num_variables(element.get_arity()), condition(num_variables, element.get_condition()), effects()
    {
        for (const auto& effect : element.get_effects())
            effects.emplace_back(num_variables, effect);
    }

    auto identifying_members() const noexcept { return std::tie(num_variables, condition, effects); }
};

using MutableActionList = std::vector<MutableAction>;
}

namespace tyr::formalism::unification
{
template<>
struct structure_traits<tyr::formalism::planning::MutableAction>
{
    using Type = tyr::formalism::planning::MutableAction;

    template<typename F>
    static bool zip_terms(const Type& lhs, const Type& rhs, F&& f)
    {
        if (lhs.num_variables != rhs.num_variables)
            return false;

        return tyr::formalism::unification::zip_terms(lhs.condition, rhs.condition, f) && tyr::formalism::unification::zip_terms(lhs.effects, rhs.effects, f);
    }

    template<typename F>
    static Type transform_terms(const Type& value, F&& f)
    {
        return Type(value.num_variables,
                    tyr::formalism::unification::transform_terms(value.condition, f),
                    tyr::formalism::unification::transform_terms(value.effects, f));
    }
};
}

#endif
