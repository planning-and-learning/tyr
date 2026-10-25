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

#ifndef TYR_FORMALISM_PLANNING_MUTABLE_CONJUNCTIVE_EFFECT_HPP_
#define TYR_FORMALISM_PLANNING_MUTABLE_CONJUNCTIVE_EFFECT_HPP_

#include "tyr/formalism/planning/mutable/literal.hpp"
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
struct MutableConjunctiveEffect : ygg::comparison::Mixin<MutableConjunctiveEffect>
{
    size_t num_parent_variables = 0;
    size_t num_variables = 0;
    MutableLiteralList<FluentTag> literals;

    MutableConjunctiveEffect() = default;
    MutableConjunctiveEffect(size_t num_parent_variables_, size_t num_variables_, MutableLiteralList<FluentTag> literals_) :
        num_parent_variables(num_parent_variables_),
        num_variables(num_variables_),
        literals(std::move(literals_))
    {
    }
    MutableConjunctiveEffect(size_t num_parent_variables_, size_t num_variables_, ConjunctiveEffectView<LiftedTag> element) :
        num_parent_variables(num_parent_variables_),
        num_variables(num_variables_),
        literals()
    {
        for (const auto& literal : element.get_literals())
            literals.emplace_back(literal);
    }

    static constexpr auto identifying_members() noexcept
    { return ygg::Members<&MutableConjunctiveEffect::num_parent_variables, &MutableConjunctiveEffect::num_variables, &MutableConjunctiveEffect::literals> {}; }
};

using MutableConjunctiveEffectList = std::vector<MutableConjunctiveEffect>;
}

namespace tyr::formalism::unification
{
template<>
struct structure_traits<planning::MutableConjunctiveEffect>
{
    using Type = planning::MutableConjunctiveEffect;

    template<typename F>
    static bool zip_terms(const Type& lhs, const Type& rhs, F f)
    {
        if (lhs.num_parent_variables != rhs.num_parent_variables)
            return false;

        if (lhs.num_variables != rhs.num_variables)
            return false;

        return unification::zip_terms(lhs.literals, rhs.literals, f);
    }

    template<typename F>
    static Type transform_terms(const Type& value, F f)
    { return Type(value.num_parent_variables, value.num_variables, unification::transform_terms(value.literals, f)); }
};
}

#endif
