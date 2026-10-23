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

#ifndef TYR_SRC_PLANNING_PROGRAMS_COMMON_HPP_
#define TYR_SRC_PLANNING_PROGRAMS_COMMON_HPP_

#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/lifted/programs/translation_context.hpp"

#include <cista/containers/string.h>
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/core/types.hpp>

namespace tyr::datalog
{
template<formalism::FactKind T>
class PredicateFactSet;
}

namespace tyr::planning
{
extern ::cista::offset::string create_applicability_name(formalism::planning::ActionView<LiftedTag> action);

extern ::cista::offset::string create_triggered_name(formalism::planning::ActionView<LiftedTag> action, formalism::planning::ConditionalEffectView<LiftedTag> cond_eff);

extern ::cista::offset::string create_applicability_name(formalism::planning::AxiomView<LiftedTag> axiom);

/// Whether the conditional effect has an effect predicate, i.e., variables or a condition; the others fire with their action.
bool has_effect_predicate(formalism::planning::ConditionalEffectView<LiftedTag> effect);

/// The atoms of an effect predicate per binding of its action, i.e., per prefix of the action's arity; an atom's objects are the
/// completed binding of the effect.
using EffectAtomsByBinding =
    ygg::UnorderedMap<ygg::IndexList<formalism::Object>, std::vector<formalism::datalog::PredicateBindingView<formalism::FluentTag>>>;

EffectAtomsByBinding group_effect_atoms(const datalog::PredicateFactSet<formalism::FluentTag>& atoms, size_t action_arity);

/// Calls `apply` with each completed binding of an effect in `binding`, which holds the action binding before and after: the
/// objects of its atoms with that prefix, or the action binding itself for an effect without predicate (`atoms` is null).
template<typename Apply>
void for_each_effect_binding(const EffectAtomsByBinding* atoms, ygg::IndexList<formalism::Object>& binding, Apply&& apply)
{
    if (!atoms)
    {
        apply();
        return;
    }
    const auto it = atoms->find(binding);
    if (it == atoms->end())
        return;
    const auto action_arity = binding.size();
    for (const auto atom : it->second)
    {
        binding.clear();
        ygg::extend(atom.get_objects(), binding);
        apply();
    }
    binding.resize(action_arity);
}

/// The translation context with its Datalog symbols copied into the copy context's repository.
TranslationContext<LiftedTag> remap_translation_context(const TranslationContext<LiftedTag>& source, formalism::datalog::CopyContext& context);

/// The RPG heuristics charge each action rule a unit cost.
inline void require_unit_cost(CostMode cost_mode)
{
    if (cost_mode != CostMode::UNIT)
        throw std::invalid_argument("GENERAL action costs are unsupported in the RPG heuristics; use CostMode::UNIT.");
}

}

#endif
