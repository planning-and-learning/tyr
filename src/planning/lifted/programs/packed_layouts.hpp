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

#ifndef TYR_SRC_PLANNING_LIFTED_PROGRAMS_PACKED_LAYOUTS_HPP_
#define TYR_SRC_PLANNING_LIFTED_PROGRAMS_PACKED_LAYOUTS_HPP_

#include "tyr/analysis/declarations.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/lifted/programs/translation_context.hpp"

#include <optional>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>

namespace tyr::planning
{

/// If ground bindings are packed, the task's variable domains, computed into storage, that define the binding layouts.
/// Otherwise nullptr.
const analysis::TaskVariableDomains* packed_layout_domains(formalism::planning::TaskView<LiftedTag> task,
                                                           std::optional<analysis::TaskVariableDomains>& storage);

/// If ground bindings are packed, packs the Datalog bindings of the task's static predicates by their facts, and those of its
/// fluent and derived predicates and fluent functions
/// by their argument domains, extended by the goal's objects so that goal atoms stay encodable. Otherwise a no-op.
/// Call before the first binding of these relations is created.
void set_packed_binding_layouts(formalism::planning::TaskView<LiftedTag> task,
                                const analysis::TaskVariableDomains* domains,
                                const P2DTranslationContext<LiftedTag>& p2d,
                                formalism::datalog::Repository& repository);

using AppPredicateToActionMapping = ygg::UnorderedMap<formalism::datalog::PredicateView<formalism::FluentTag>, formalism::planning::ActionView<LiftedTag>>;
using AppPredicateToAxiomsMapping =
    ygg::UnorderedMap<formalism::datalog::PredicateView<formalism::FluentTag>, std::vector<formalism::planning::AxiomView<LiftedTag>>>;

/// If ground bindings are packed, packs the Datalog bindings of applicability predicates by the parameter domains of their actions. Otherwise a no-op.
void set_packed_binding_layouts(const analysis::TaskVariableDomains* domains,
                                const AppPredicateToActionMapping& predicate_to_actions,
                                formalism::datalog::Repository& repository);

/// If ground bindings are packed, packs the Datalog bindings of applicability predicates by the parameter domains of their axioms. Otherwise a no-op.
void set_packed_binding_layouts(const analysis::TaskVariableDomains* domains,
                                const AppPredicateToAxiomsMapping& predicate_to_axioms,
                                formalism::datalog::Repository& repository);

}

#endif
