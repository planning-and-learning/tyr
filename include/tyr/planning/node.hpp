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

#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/planning/ground/state_view.hpp"
#include "tyr/planning/lifted/state_view.hpp"
#include "tyr/planning/state_index.hpp"
#include "tyr/planning/state_view.hpp"
#include "tyr/planning/task.hpp"

#include <concepts>
#include <ranges>
#include <tuple>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/semantics/comparison.hpp>

namespace tyr::planning
{
template<StateViewConcept State>
class Node : public ygg::comparison::Mixin<Node<State>>
{
public:
    using StateType = State;
    using KindType = typename State::KindType;
    using TaskType = typename State::TaskType;

    Node(State state, ygg::float_t metric) noexcept : m_state(std::move(state)), m_metric(metric) {}

    const State& get_state() const noexcept { return m_state; }
    ygg::float_t get_metric() const noexcept { return m_metric; }

    auto pack() const noexcept
        requires requires(const State& state) {
            { state.pack() } -> std::same_as<PackedStateView<KindType>>;
        }
    {
        return PackedNode<KindType>(m_state.pack(), m_metric);
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

    Node<StateView<Kind>> unpack() const { return Node(m_state.unpack(), m_metric); }

    auto identifying_members() const noexcept { return std::tie(m_state, m_metric); }

private:
    PackedStateView<Kind> m_state;
    ygg::float_t m_metric;
};

template<StateViewConcept State>
using NodeList = std::vector<Node<State>>;

template<TaskKind Kind>
using PackedNodeList = std::vector<PackedNode<Kind>>;

template<StateViewConcept State>
struct LabeledNode
{
    using StateType = State;
    using KindType = typename State::KindType;
    using TaskType = typename State::TaskType;

    formalism::planning::ActionBindingView label;
    Node<State> node;

    auto pack() const noexcept
        requires requires(const Node<State>& value) { value.pack(); }
    {
        return PackedLabeledNode<KindType> { label, node.pack() };
    }
};

template<TaskKind Kind>
struct PackedLabeledNode
{
    formalism::planning::ActionBindingView label;
    PackedNode<Kind> node;

    LabeledNode<StateView<Kind>> unpack() const { return { label, node.unpack() }; }
};

template<StateViewConcept State>
using LabeledNodeList = std::vector<LabeledNode<State>>;

template<TaskKind Kind>
using PackedLabeledNodeList = std::vector<PackedLabeledNode<Kind>>;

template<typename T>
concept NodeConcept = requires(const T& cn) {
    requires StateViewConcept<typename T::StateType>;
    { cn.get_state() } -> std::same_as<const typename T::StateType&>;
    { cn.get_metric() } -> std::same_as<ygg::float_t>;
};

}

#endif
