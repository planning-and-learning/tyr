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
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/action_executor.hpp"
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
class PredicateFactSets;
}

namespace tyr::planning
{
extern ::cista::offset::string create_applicability_name(formalism::planning::ActionView<LiftedTag> action);

extern ::cista::offset::string create_triggered_name(formalism::planning::ActionView<LiftedTag> action, formalism::planning::ConditionalEffectView<LiftedTag> cond_eff);

extern ::cista::offset::string create_applicability_name(formalism::planning::AxiomView<LiftedTag> axiom);

/// Whether the conditional effect has an effect rule, i.e., variables or a condition; the others fire with their action.
bool has_effect_rule(formalism::planning::ConditionalEffectView<LiftedTag> effect);

/// Per binding of an action, the k and extension of each atom of the predicate of its k-th effect rule (the rule's last head), whose
/// objects are those of the binding followed by the extension.
using EffectAtoms = ygg::UnorderedMap<ygg::IndexList<formalism::Object>, std::vector<std::pair<size_t, ygg::IndexList<formalism::Object>>>>;

EffectAtoms group_effect_atoms(formalism::planning::ActionView<LiftedTag> action,
                               std::span<const ygg::Index<formalism::datalog::Rule<LiftedTag, formalism::PredicateTag>>> effect_rules,
                               const formalism::datalog::Repository& repository,
                               const datalog::PredicateFactSets<formalism::FluentTag>& facts);

/// All effects of an action binding that fire, in the order of the action's effects: those without an effect rule, and those of
/// its effect atoms (by k, as in EffectAtoms).
void collect_firing_effects(formalism::planning::ActionView<LiftedTag> action,
                            std::span<const std::pair<size_t, ygg::IndexList<formalism::Object>>> atoms,
                            std::vector<ActionExecutor::FiringEffect>& out);

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
