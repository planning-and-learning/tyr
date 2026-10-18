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
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/binding_view.hpp>

namespace tyr::planning
{

template<TaskKind Kind, StateViewConcept<Kind> State = StateView<Kind>>
class Node;

template<TaskKind Kind,
         StateViewConcept<Kind> State = StateView<Kind>,
         ygg::formalism::RelationBindingViewConcept<formalism::planning::Action<LiftedTag>, formalism::ObjectTag> Binding =
             formalism::planning::ActionBindingView>
struct LabeledNode;

template<TaskKind Kind, StateViewConcept<Kind> State = StateView<Kind>>
using NodeList = std::vector<Node<Kind, State>>;

template<TaskKind Kind>
using PackedNodeList = std::vector<PackedNode<Kind>>;

template<TaskKind Kind,
         StateViewConcept<Kind> State = StateView<Kind>,
         ygg::formalism::RelationBindingViewConcept<formalism::planning::Action<LiftedTag>, formalism::ObjectTag> Binding =
             formalism::planning::ActionBindingView>
using LabeledNodeList = std::vector<LabeledNode<Kind, State, Binding>>;

template<TaskKind Kind>
using PackedLabeledNodeList = std::vector<PackedLabeledNode<Kind>>;

template<typename T, typename Kind>
concept NodeConcept = TaskKind<Kind> && requires(const std::remove_reference_t<T>& cn) {
    { cn.get_state() } -> StateViewConcept<Kind>;
    { cn.get_metric() } -> std::same_as<ygg::float_t>;
};

}

#endif
