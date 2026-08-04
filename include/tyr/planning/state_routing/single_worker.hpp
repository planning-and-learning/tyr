/*
 * Copyright (C) 2026 Dominik Drexler
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

#ifndef TYR_PLANNING_STATE_ROUTING_SINGLE_WORKER_HPP_
#define TYR_PLANNING_STATE_ROUTING_SINGLE_WORKER_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/state_routing/dist_hash.hpp"
#include "tyr/planning/successor_generator.hpp"

#include <cassert>
#include <cstdint>
#include <optional>
#include <utility>

namespace tyr::planning
{

template<TaskKind Kind, typename Metadata>
struct RoutedSuccessor
{
    LabeledNode<Kind> labeled_node;
    Metadata metadata;
};

template<TaskKind Kind, DistHashKind HashKind, typename Metadata>
class SingleWorkerStateRouter
{
    using Builder = ygg::Builder<State<Kind>>;
    using LocalState = ygg::SharedObjectPoolPtr<Builder>;

    struct PendingSuccessor
    {
        LocalState state;
        PendingActionResult action_result;
        ::tyr::formalism::planning::ActionBindingView action;
        Metadata metadata;

        PendingSuccessor(LocalState state_, PendingActionResult action_result_, ::tyr::formalism::planning::ActionBindingView action_, Metadata metadata_) :
            state(std::move(state_)),
            action_result(action_result_),
            action(action_),
            metadata(std::move(metadata_))
        {
        }

        PendingSuccessor(const PendingSuccessor&) = delete;
        PendingSuccessor& operator=(const PendingSuccessor&) = delete;
        PendingSuccessor(PendingSuccessor&&) noexcept = default;
        PendingSuccessor& operator=(PendingSuccessor&&) noexcept = default;
    };

public:
    explicit SingleWorkerStateRouter(uint64_t seed = 0) noexcept : m_dist_hash(seed) {}

    void send(LocalState state, PendingActionResult action_result, ::tyr::formalism::planning::ActionBindingView action, Metadata metadata)
    {
        assert(!m_pending);
        const auto owner = m_dist_hash.owner(*state, 1);
        assert(owner == ygg::Index<Worker>(0));
        static_cast<void>(owner);
        m_pending.emplace(std::move(state), action_result, action, std::move(metadata));
    }

    RoutedSuccessor<Kind, Metadata> receive(SuccessorGenerator<Kind>& successor_generator)
    {
        assert(m_pending);
        auto pending = std::move(*m_pending);
        m_pending.reset();

        auto node = successor_generator.finalize_successor_state(std::move(pending.state), pending.action_result);
        return RoutedSuccessor<Kind, Metadata> { LabeledNode<Kind> { pending.action, std::move(node) }, std::move(pending.metadata) };
    }

private:
    DistHash<Kind, HashKind> m_dist_hash;
    std::optional<PendingSuccessor> m_pending;
};

}

#endif
