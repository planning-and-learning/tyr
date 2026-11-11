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

#ifndef TYR_PLANNING_NODE_CONCEPTS_HPP_
#define TYR_PLANNING_NODE_CONCEPTS_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_view_concepts.hpp"

#include <concepts>
#include <type_traits>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::planning
{

template<TaskKind Kind, StateViewConcept<Kind> State = StateView<Kind>>
class Node;

/// A node and the action binding that reached it: by default an interned binding; a borrowed one (BorrowedActionRowView,
/// runir's rule evaluation) before it is materialized.
template<TaskKind Kind,
         StateViewConcept<Kind> State = StateView<Kind>,
         ygg::RowViewConcept<formalism::planning::Action<LiftedTag>> Binding = formalism::planning::ActionRowView>
struct LabeledNode;

template<TaskKind Kind, StateViewConcept<Kind> State = StateView<Kind>>
using NodeList = urd::Store<urd::sequence<Node<Kind, State>>>;

template<TaskKind Kind>
using PackedNodeList = urd::Store<urd::sequence<PackedNode<Kind>>>;

template<TaskKind Kind,
         StateViewConcept<Kind> State = StateView<Kind>,
         ygg::RowViewConcept<formalism::planning::Action<LiftedTag>> Binding = formalism::planning::ActionRowView>
using LabeledNodeList = urd::Store<urd::sequence<LabeledNode<Kind, State, Binding>>>;

template<TaskKind Kind>
using PackedLabeledNodeList = urd::Store<urd::sequence<PackedLabeledNode<Kind>>>;

}

#endif
