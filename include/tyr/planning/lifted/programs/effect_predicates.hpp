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

#ifndef TYR_PLANNING_LIFTED_PROGRAMS_EFFECT_PREDICATES_HPP_
#define TYR_PLANNING_LIFTED_PROGRAMS_EFFECT_PREDICATES_HPP_

#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/planning/views.hpp"

#include <cstddef>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::planning
{

/// The conditional effect of an effect predicate, whose atoms are the objects of an action binding followed by objects of the
/// effect's variables for which the effect fires, and the rule that derives them. The static consistency graph of the rule
/// relates the action's objects to the effect extensions that static facts allow.
struct EffectPredicate
{
    formalism::planning::ActionView<LiftedTag> action;
    size_t position;  ///< Among the action's effects.
    ygg::Index<formalism::datalog::Rule<LiftedTag, formalism::PredicateTag>> rule;
};

using EffectPredicateToEffectMapping = ygg::UnorderedMap<formalism::datalog::PredicateView<formalism::FluentTag>, EffectPredicate>;

/// Whether the conditional effect has an effect predicate: it has variables or a condition. The others fire with their action.
inline bool has_effect_predicate(formalism::planning::ConditionalEffectView<LiftedTag> effect)
{
    const auto condition = effect.get_condition();
    return !effect.get_variables().empty() || !condition.get_literals<formalism::StaticTag>().empty()
           || !condition.get_literals<formalism::FluentTag>().empty() || !condition.get_literals<formalism::DerivedTag>().empty()
           || !condition.get_numeric_constraints().empty();
}

/// The extension of an effect atom and the position of its effect.
struct EffectExtension
{
    size_t position;
    ygg::IndexList<formalism::Object> extension;
};

/// Per action binding, the extensions of its effect atoms.
using EffectExtensionsByBinding = ygg::UnorderedMap<ygg::IndexList<formalism::Object>, std::vector<EffectExtension>>;

/// Adds an atom of the effect predicate at `position` of an action with `action_arity` variables.
template<typename Objects>
void add_effect_atom(EffectExtensionsByBinding& result, size_t position, size_t action_arity, Objects objects)
{
    auto prefix = ygg::IndexList<formalism::Object> {};
    auto extension = ygg::IndexList<formalism::Object> {};
    for (size_t i = 0; i < objects.size(); ++i)
        (i < action_arity ? prefix : extension).push_back(objects[i].get_index());
    result[std::move(prefix)].push_back({ position, std::move(extension) });
}

}

#endif
