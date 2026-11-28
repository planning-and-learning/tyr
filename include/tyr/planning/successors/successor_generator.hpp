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

#ifndef TYR_PLANNING_SUCCESSORS_SUCCESSOR_GENERATOR_HPP_
#define TYR_PLANNING_SUCCESSORS_SUCCESSOR_GENERATOR_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/nodes/node_view.hpp"
#include "tyr/planning/successors/applicability_decl.hpp"
#include "tyr/planning/successors/successor_generator_concepts.hpp"

#include <concepts>
#include <functional>
#include <optional>
#include <type_traits>
#include <urd/repr/slots/slots.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::planning
{

/// The binding is present exactly when status is APPLICABLE and borrows the task repository.
struct ActionBindingResult
{
    ActionBindingStatus status = ActionBindingStatus::INAPPLICABLE;
    std::optional<formalism::ActionRowView> binding;
};

/// What applicability reads of a node's state: its storages (the state's builder's, as a node holds it), the node's
/// metric its auxiliary value.
template<TaskKind Kind>
StateContext<Kind> state_context_of(const NodeBuilderView<Kind>& node)
{ return StateContext<Kind>(*node.context().get_task(), node.state(), node.metric()); }

/// Whether a node's state is registered: it has its key (a registered source lets its repository store a registered
/// successor as a delta to it).
template<TaskKind Kind>
bool is_registered(const NodeBuilderView<Kind>& node)
{ return !node.pack().state.is_none(); }

}

#endif
