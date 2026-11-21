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

#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <urd/elements/flat.hpp>
#include <utility>

namespace tyr::planning
{
extern urd::flat_string create_applicability_name(formalism::planning::ActionView<LiftedTag> action);

extern urd::flat_string create_triggered_name(formalism::planning::ActionView<LiftedTag> action,
                                              formalism::planning::ConditionalEffectView<LiftedTag> cond_eff);

extern urd::flat_string create_applicability_name(formalism::planning::AxiomView<LiftedTag> axiom);

/// The derived predicate that holds where the action (the axiom) is applicable, and its atom over the action's (the
/// axiom's) parameters, interned into the context's repository.
std::pair<formalism::datalog::PredicateView<formalism::DerivedTag>, bool>
create_applicability_predicate(formalism::planning::ActionView<LiftedTag> action,
                               formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context);
std::pair<formalism::datalog::AtomView<LiftedTag, formalism::DerivedTag>, bool>
create_applicability_atom(formalism::planning::ActionView<LiftedTag> action,
                          formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context);
std::pair<formalism::datalog::PredicateView<formalism::DerivedTag>, bool>
create_applicability_predicate(formalism::planning::AxiomView<LiftedTag> axiom,
                               formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context);
std::pair<formalism::datalog::AtomView<LiftedTag, formalism::DerivedTag>, bool>
create_applicability_atom(formalism::planning::AxiomView<LiftedTag> axiom,
                          formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context);

/// A layer over `parent`, which stays frozen from here on: the layer's deleter holds the parent, so each layer keeps
/// its chain alive (domain, task, program, transform layers, per-worker workspace).
formalism::datalog::RepositoryPtr layer_over(std::shared_ptr<const formalism::datalog::Repository> parent, std::size_t num_objects);

/// Lists the task's predicates (its derived ones too), functions, objects, static and fluent atoms and function term
/// values in `program`, as the task's records: a program is a layer over the task's repository. The lists are dense: each
/// record sits at the position of its index (a program never drops a predicate; unused ones have empty facts).
template<TaskKind Kind>
void translate_task_symbols(formalism::planning::TaskView<Kind> task, urd::builder<formalism::datalog::Program<Kind>, formalism::datalog::Repository>& program);

/// The literal of an FDR fact's atom with the polarity, interned into the context's repository; none for a fact without
/// an atom (a variable's none value).
std::optional<formalism::datalog::LiteralView<GroundTag, formalism::FluentTag>>
create_fact_literal(formalism::planning::FDRFactView<formalism::FluentTag> fact,
                    bool polarity,
                    formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context);

/// The RPG heuristics charge each action rule a unit cost.
inline void require_unit_cost(CostMode cost_mode)
{
    if (cost_mode != CostMode::UNIT)
        throw std::invalid_argument("GENERAL action costs are unsupported in the RPG heuristics; use CostMode::UNIT.");
}

}

#endif
