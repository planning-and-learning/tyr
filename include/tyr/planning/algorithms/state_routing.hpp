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

#ifndef TYR_PLANNING_ALGORITHMS_STATE_ROUTING_HPP_
#define TYR_PLANNING_ALGORITHMS_STATE_ROUTING_HPP_

#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/ground/state_builder.hpp"
#include "tyr/planning/ground/state_repository.hpp"
#include "tyr/planning/lifted/state_builder.hpp"
#include "tyr/planning/lifted/state_repository.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/successor_generator.hpp"

#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <utility>
#include <yggdrasil/containers/unique_object_pool.hpp>
#include <yggdrasil/semantics/containers/dynamic_bitset_hash.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::planning
{

struct RandomDistHashTag
{
};

struct ZobristDistHashTag
{
};

template<typename T>
concept DistHashKind = std::same_as<T, RandomDistHashTag> || std::same_as<T, ZobristDistHashTag>;

template<TaskKind Kind, DistHashKind HashKind>
class DistHash;

template<TaskKind Kind>
class DistHash<Kind, RandomDistHashTag>
{
public:
    explicit DistHash(uint64_t seed = 0) noexcept : m_seed(seed) {}

    ygg::hash_t hash(const ygg::Builder<State<Kind>>& state) const noexcept
    {
        auto result = static_cast<ygg::hash_t>(m_seed);
        ygg::hash_combine(result, state.template get_atoms<::tyr::formalism::FluentTag>());
        ygg::hash_combine(result, state.get_numeric_variables());
        return result;
    }

    size_t owner(const ygg::Builder<State<Kind>>& state, size_t num_workers) const noexcept
    {
        assert(num_workers > 0);
        // The ordinary single-worker search path pays no state-hashing cost.
        return num_workers == 1 ? 0 : static_cast<size_t>(hash(state) % num_workers);
    }

private:
    uint64_t m_seed;
};

template<TaskKind Kind>
struct InternalStateID
{
    ygg::uint_t worker;
    ygg::Index<State<Kind>> state;
};

template<TaskKind Kind>
class StateTransferPool
{
public:
    using Builder = ygg::Builder<State<Kind>>;
    using LocalState = ygg::SharedObjectPoolPtr<Builder>;
    using TransferredState = ygg::UniqueObjectPoolPtr<Builder, true>;

    static_assert(std::is_nothrow_swappable_v<Builder>);

    [[nodiscard]] TransferredState export_state(LocalState state)
    {
        auto transferred = m_pool.get_or_allocate();
        using std::swap;
        swap(*transferred, *state);
        return transferred;
    }

    [[nodiscard]] LocalState import_state(StateRepository<Kind>& repository, TransferredState state)
    {
        auto local = repository.get_state_builder();
        using std::swap;
        swap(*local, *state);
        return local;
    }

private:
    // Handles retain the pool address and may be released by another worker.
    ygg::UniqueObjectPool<Builder, true> m_pool;
};

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
        assert(owner == 0);
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
