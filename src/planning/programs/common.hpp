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
#include "tyr/formalism/write_context.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/lifted/programs/translation_context.hpp"

#include <memory>
#include <stdexcept>
#include <urd/elements/flat.hpp>
#include <utility>

namespace tyr::planning
{
extern urd::flat_string create_applicability_name(formalism::planning::ActionView<LiftedTag> action);

extern urd::flat_string create_triggered_name(formalism::planning::ActionView<LiftedTag> action,
                                              formalism::planning::ConditionalEffectView<LiftedTag> cond_eff);

extern urd::flat_string create_applicability_name(formalism::planning::AxiomView<LiftedTag> axiom);

/// The fluent predicate that holds where the action (the axiom) is applicable, and its atom over the action's (the
/// axiom's) parameters, interned into the context's repository.
std::pair<formalism::datalog::PredicateView<formalism::FluentTag>, bool>
create_applicability_predicate(formalism::planning::ActionView<LiftedTag> action,
                               formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context);
std::pair<formalism::datalog::AtomView<LiftedTag, formalism::FluentTag>, bool>
create_applicability_atom(formalism::planning::ActionView<LiftedTag> action,
                          formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context);
std::pair<formalism::datalog::PredicateView<formalism::FluentTag>, bool>
create_applicability_predicate(formalism::planning::AxiomView<LiftedTag> axiom,
                               formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context);
std::pair<formalism::datalog::AtomView<LiftedTag, formalism::FluentTag>, bool>
create_applicability_atom(formalism::planning::AxiomView<LiftedTag> axiom,
                          formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context);

/// The translation context with its Datalog symbols copied into the copy context's repository.
TranslationContext<LiftedTag> remap_translation_context(const TranslationContext<LiftedTag>& source,
                                                        formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context);

/// Translates the task's predicates (with its derived predicates as fluent ones where `with_derived`), functions, objects,
/// atoms and function term values into `program`, and records the predicate and function mappings in `translation_context`.
void translate_task_symbols(formalism::planning::TaskView<LiftedTag> task,
                            bool with_derived,
                            TranslationContext<LiftedTag>& translation_context,
                            formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context,
                            urd::data<formalism::datalog::Program<LiftedTag>>& program);

/// The RPG heuristics charge each action rule a unit cost.
inline void require_unit_cost(CostMode cost_mode)
{
    if (cost_mode != CostMode::UNIT)
        throw std::invalid_argument("GENERAL action costs are unsupported in the RPG heuristics; use CostMode::UNIT.");
}

}

#endif
