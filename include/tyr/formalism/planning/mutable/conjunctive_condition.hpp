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

#ifndef TYR_FORMALISM_PLANNING_MUTABLE_CONJUNCTIVE_CONDITION_HPP_
#define TYR_FORMALISM_PLANNING_MUTABLE_CONJUNCTIVE_CONDITION_HPP_

#include <yggdrasil/semantics/comparison.hpp>
#include <yggdrasil/semantics/hash.hpp>
#include "tyr/formalism/planning/mutable/literal.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/unification/structure_traits.hpp"
#include "tyr/formalism/unification/structure_traits_impl.hpp"

#include <cstddef>
#include <tuple>
#include <utility>
#include <vector>

namespace tyr::formalism::planning
{
struct MutableConjunctiveCondition : ygg::comparison::Mixin<MutableConjunctiveCondition>
{
    size_t num_parent_variables;
    size_t num_variables;
    MutableLiteralList<StaticTag> static_literals;
    MutableLiteralList<FluentTag> fluent_literals;

    MutableConjunctiveCondition() = default;
    MutableConjunctiveCondition(size_t num_parent_variables,
                                size_t num_variables,
                                MutableLiteralList<StaticTag> static_literals,
                                MutableLiteralList<FluentTag> fluent_literals) :
        num_parent_variables(num_parent_variables),
        num_variables(num_variables),
        static_literals(std::move(static_literals)),
        fluent_literals(std::move(fluent_literals))
    {
    }
    MutableConjunctiveCondition(size_t num_parent_variables, ConjunctiveConditionView element) :
        num_parent_variables(num_parent_variables),
        num_variables(element.get_arity()),
        static_literals(),
        fluent_literals()
    {
        for (const auto& literal : element.get_literals<StaticTag>())
            static_literals.emplace_back(literal);
        for (const auto& literal : element.get_literals<FluentTag>())
            fluent_literals.emplace_back(literal);
    }

    auto identifying_members() const noexcept { return std::tie(num_parent_variables, num_variables, static_literals, fluent_literals); }
};

using MutableConjunctiveConditionList = std::vector<MutableConjunctiveCondition>;
}

namespace tyr::formalism::unification
{
template<>
struct structure_traits<tyr::formalism::planning::MutableConjunctiveCondition>
{
    using Type = tyr::formalism::planning::MutableConjunctiveCondition;

    template<typename F>
    static bool zip_terms(const Type& lhs, const Type& rhs, F&& f)
    {
        if (lhs.num_parent_variables != rhs.num_parent_variables)
            return false;

        if (lhs.num_variables != rhs.num_variables)
            return false;

        return tyr::formalism::unification::zip_terms(lhs.static_literals, rhs.static_literals, f)
               && tyr::formalism::unification::zip_terms(lhs.fluent_literals, rhs.fluent_literals, f);
    }

    template<typename F>
    static Type transform_terms(const Type& value, F&& f)
    {
        return Type(value.num_parent_variables,
                    value.num_variables,
                    tyr::formalism::unification::transform_terms(value.static_literals, f),
                    tyr::formalism::unification::transform_terms(value.fluent_literals, f));
    }
};
}

#endif
