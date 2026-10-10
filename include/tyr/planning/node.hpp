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

#ifndef TYR_PLANNING_NODE_HPP_
#define TYR_PLANNING_NODE_HPP_

#include "tyr/formalism/binding_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/planning/state_view.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/task.hpp"

#include <concepts>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/semantics/comparison.hpp>

namespace tyr::planning
{
template<TaskKind Kind, StateViewConcept<Kind> State = StateView<Kind>>
class Node : public ygg::comparison::Mixin<Node<Kind, State>>
{
public:
    using StateType = State;

    Node(State state, ygg::float_t metric) noexcept(std::is_nothrow_move_constructible_v<State>) : m_state(std::move(state)), m_metric(metric) {}

    const State& get_state() const noexcept { return m_state; }
    ygg::float_t get_metric() const noexcept { return m_metric; }

    auto pack() const noexcept(noexcept(m_state.pack()))
        requires requires(const State& state) {
            { state.pack() } -> std::same_as<PackedStateView<Kind>>;
        }
    {
        return PackedNode<Kind>(m_state.pack(), m_metric);
    }

    auto identifying_members() const noexcept
        requires ygg::Identifiable<State>
    {
        return std::tie(m_state, m_metric);
    }

private:
    State m_state;
    ygg::float_t m_metric;
};

template<TaskKind Kind>
class PackedNode : public ygg::comparison::Mixin<PackedNode<Kind>>
{
public:
    using TaskType = Task<Kind>;

    PackedNode(PackedStateView<Kind> state, ygg::float_t metric) noexcept : m_state(std::move(state)), m_metric(metric) {}

    const PackedStateView<Kind>& get_state() const noexcept { return m_state; }
    ygg::float_t get_metric() const noexcept { return m_metric; }

    Node<Kind> unpack() const { return Node<Kind>(m_state.unpack(), m_metric); }

    auto identifying_members() const noexcept { return std::tie(m_state, m_metric); }

private:
    PackedStateView<Kind> m_state;
    ygg::float_t m_metric;
};

template<TaskKind Kind, StateViewConcept<Kind> State = StateView<Kind>>
using NodeList = std::vector<Node<Kind, State>>;

template<TaskKind Kind>
using PackedNodeList = std::vector<PackedNode<Kind>>;

template<TaskKind Kind,
         StateViewConcept<Kind> State = StateView<Kind>,
         ygg::formalism::RelationBindingViewConcept<formalism::planning::Action<LiftedTag>, formalism::ObjectTag> Binding =
             formalism::planning::ActionBindingView>
struct LabeledNode
{
    using StateType = State;

    Binding label;
    Node<Kind, State> node;

    auto pack() const noexcept(noexcept(node.pack()))
        requires std::same_as<Binding, formalism::planning::ActionBindingView> && requires(const Node<Kind, State>& value) { value.pack(); }
    {
        return PackedLabeledNode<Kind> { label, node.pack() };
    }
};

template<TaskKind Kind>
struct PackedLabeledNode
{
    formalism::planning::ActionBindingView label;
    PackedNode<Kind> node;

    LabeledNode<Kind> unpack() const { return { label, node.unpack() }; }
};

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
