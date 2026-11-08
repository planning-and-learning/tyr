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

#ifndef TYR_PLANNING_SUCCESSOR_GENERATOR_HPP_
#define TYR_PLANNING_SUCCESSOR_GENERATOR_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/successor_generator_concepts.hpp"

#include <concepts>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>
#include <urd/repr/slots/slots.hpp>
#include <urd/store.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::planning
{

/// The binding is present exactly when status is APPLICABLE and borrows the task repository.
struct ActionBindingResult
{
    ActionBindingStatus status = ActionBindingStatus::INAPPLICABLE;
    std::optional<formalism::planning::ActionRowView> binding;
};

/// The node successors are generated from, borrowed for one call: a registered node, or a node over a state builder.
/// A registered source lets its repository store a registered successor as a delta to it.
template<TaskKind Kind>
class SourceNode
{
public:
    SourceNode(const Node<Kind>& node) noexcept :  // NOLINT(google-explicit-constructor): every node is a source
        m_data(&node.get_state().data()),
        m_repository(&node.get_state().context()),
        m_registered(&node.get_state()),
        m_metric(node.get_metric())
    {
    }
    SourceNode(const Node<Kind, BuilderStateView<Kind>>& node) noexcept :  // NOLINT(google-explicit-constructor)
        m_data(&node.get_state().data()),
        m_repository(&node.get_state().context()),
        m_registered(nullptr),
        m_metric(node.get_metric())
    {
    }

    [[nodiscard]] const urd::data<State<Kind>>& data() const noexcept { return *m_data; }
    [[nodiscard]] const StateRepository<Kind>& get_state_repository() const noexcept { return *m_repository; }
    [[nodiscard]] const Task<Kind>& get_task() const noexcept { return *m_repository->get_task(); }
    /// The registered state, or null for a builder.
    [[nodiscard]] const StateView<Kind>* registered() const noexcept { return m_registered; }
    [[nodiscard]] ygg::float_t get_metric() const noexcept { return m_metric; }

private:
    const urd::data<State<Kind>>* m_data;
    const StateRepository<Kind>* m_repository;
    const StateView<Kind>* m_registered;
    ygg::float_t m_metric;
};

}

#endif
