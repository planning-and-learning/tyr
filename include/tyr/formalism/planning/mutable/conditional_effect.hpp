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

#ifndef TYR_FORMALISM_PLANNING_MUTABLE_CONDITIONAL_EFFECT_HPP_
#define TYR_FORMALISM_PLANNING_MUTABLE_CONDITIONAL_EFFECT_HPP_

#include "tyr/formalism/planning/mutable/conjunctive_condition.hpp"
#include "tyr/formalism/planning/mutable/conjunctive_effect.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/unification/structure_traits.hpp"
#include "tyr/formalism/unification/structure_traits_impl.hpp"

#include <cstddef>
#include <tuple>
#include <utility>
#include <vector>
#include <yggdrasil/semantics/comparison.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::formalism::planning
{
struct MutableConditionalEffect : ygg::comparison::Mixin<MutableConditionalEffect>
{
    size_t num_parent_variables = 0;
    size_t num_variables = 0;
    MutableConjunctiveCondition condition;
    MutableConjunctiveEffect effect;

    MutableConditionalEffect() = default;
    MutableConditionalEffect(size_t num_parent_variables_, size_t num_variables_, MutableConjunctiveCondition condition_, MutableConjunctiveEffect effect_) :
        num_parent_variables(num_parent_variables_),
        num_variables(num_variables_),
        condition(std::move(condition_)),
        effect(std::move(effect_))
    {
    }
    MutableConditionalEffect(size_t num_parent_variables_, ConditionalEffectView<LiftedTag> element) :
        num_parent_variables(num_parent_variables_),
        num_variables(element.get_arity()),
        condition(num_parent_variables_, element.get_condition()),
        effect(num_parent_variables_, num_variables, element.get_effect())
    {
    }

    static constexpr auto identifying_members() noexcept
    {
        return ygg::Members<&MutableConditionalEffect::num_parent_variables,
                            &MutableConditionalEffect::num_variables,
                            &MutableConditionalEffect::condition,
                            &MutableConditionalEffect::effect> {};
    }
};

using MutableConditionalEffectList = std::vector<MutableConditionalEffect>;
}

namespace tyr::formalism::unification
{
template<>
struct structure_traits<planning::MutableConditionalEffect>
{
    using Type = planning::MutableConditionalEffect;

    template<typename F>
    static bool zip_terms(const Type& lhs, const Type& rhs, F f)
    {
        if (lhs.num_parent_variables != rhs.num_parent_variables)
            return false;

        if (lhs.num_variables != rhs.num_variables)
            return false;

        return unification::zip_terms(lhs.condition, rhs.condition, f) && unification::zip_terms(lhs.effect, rhs.effect, f);
    }

    template<typename F>
    static Type transform_terms(const Type& value, F f)
    {
        return Type(value.num_parent_variables,
                    value.num_variables,
                    unification::transform_terms(value.condition, f),
                    unification::transform_terms(value.effect, f));
    }
};
}

#endif
