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

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/grounding_context.hpp"
#include "tyr/formalism/planning/declarations.hpp"

#include <memory>
#include <optional>
#include <stdexcept>
#include <urd/elements/builder.hpp>
#include <urd/elements/flat.hpp>
#include <utility>

namespace tyr::planning
{
extern urd::flat_string create_applicability_name(formalism::ActionView<LiftedTag> action);

extern urd::flat_string create_triggered_name(formalism::ActionView<LiftedTag> action, formalism::ConditionalEffectView<LiftedTag> cond_eff);

extern urd::flat_string create_applicability_name(formalism::AxiomView<LiftedTag> axiom);

/// The derived predicate that holds where the action (the axiom) is applicable, and its atom over the action's (the
/// axiom's) parameters, interned into the context's repository.
std::pair<formalism::PredicateView<formalism::DerivedTag>, bool>
create_applicability_predicate(formalism::ActionView<LiftedTag> action, formalism::GroundingContext<formalism::Builder, formalism::Repository>& context);
std::pair<formalism::AtomView<LiftedTag, formalism::DerivedTag>, bool>
create_applicability_atom(formalism::ActionView<LiftedTag> action, formalism::GroundingContext<formalism::Builder, formalism::Repository>& context);
std::pair<formalism::PredicateView<formalism::DerivedTag>, bool>
create_applicability_predicate(formalism::AxiomView<LiftedTag> axiom, formalism::GroundingContext<formalism::Builder, formalism::Repository>& context);
std::pair<formalism::AtomView<LiftedTag, formalism::DerivedTag>, bool>
create_applicability_atom(formalism::AxiomView<LiftedTag> axiom, formalism::GroundingContext<formalism::Builder, formalism::Repository>& context);

/// A layer over `parent`, which stays frozen from here on: the layer's deleter holds the parent, so each layer keeps
/// its chain alive (domain, task, program, transform layers, per-worker workspace).
formalism::RepositoryPtr layer_over(std::shared_ptr<const formalism::Repository> parent);

/// Lists the task's predicates (its derived ones too), functions, objects, static and fluent atoms and function term
/// values in `program`, as the task's records: a program is a layer over the task's repository. The lists are dense: each
/// record sits at the position of its index (a program never drops a predicate; unused ones have empty facts).
template<TaskKind Kind>
void translate_task_symbols(formalism::TaskView<Kind> task, urd::builder<formalism::Program<Kind>, formalism::Repository>& program);

/// The literal of an FDR fact's atom with the polarity, interned into the context's repository; none for a fact without
/// an atom (a variable's none value).
std::optional<formalism::LiteralView<GroundTag, formalism::FluentTag>>
create_fact_literal(formalism::FDRFactView<formalism::FluentTag> fact,
                    bool polarity,
                    formalism::GroundingContext<formalism::Builder, formalism::Repository>& context);

/// The RPG heuristics charge each action rule a unit cost.
inline void require_unit_cost(CostMode cost_mode)
{
    if (cost_mode != CostMode::UNIT)
        throw std::invalid_argument("GENERAL action costs are unsupported in the RPG heuristics; use CostMode::UNIT.");
}

}

#endif
