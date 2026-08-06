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

#ifndef TYR_PLANNING_ALGORITHMS_IW_PRUNING_STRATEGY_HPP_
#define TYR_PLANNING_ALGORITHMS_IW_PRUNING_STRATEGY_HPP_

#include "tyr/planning/algorithms/iw/novelty_table.hpp"
#include "tyr/planning/algorithms/iw/utils.hpp"
#include "tyr/planning/algorithms/strategies/pruning.hpp"

#include <memory>
#include <mutex>
#include <stdexcept>

namespace tyr::planning::iw
{

template<TaskKind Kind>
class NoveltyPruningStrategy : public PruningStrategy<Kind>
{
public:
    explicit NoveltyPruningStrategy(ygg::uint_t max_arity) : m_max_arity(max_arity), m_root_state(std::make_shared<SharedRootState>())
    {
        if (max_arity > MaxArity)
            throw std::invalid_argument("NoveltyPruningStrategy(...): max_arity exceeds iw::MaxArity.");
    }

    static std::shared_ptr<NoveltyPruningStrategy<Kind>> create(ygg::uint_t max_arity) { return std::make_shared<NoveltyPruningStrategy<Kind>>(max_arity); }

    [[nodiscard]] PruningStrategyPtr<Kind> make_worker(ygg::Index<Worker>) const override
    {
        m_root_state->enable_thread_safety();
        return std::shared_ptr<NoveltyPruningStrategy>(new NoveltyPruningStrategy(m_max_arity, m_root_state, m_novelty_table.make_worker()));
    }

    ygg::uint_t get_max_arity() const noexcept { return m_max_arity; }

    const DynamicNoveltyTable<MaxArity>& get_novelty_table() const noexcept { return m_novelty_table; }

    DynamicNoveltyTable<MaxArity>& get_novelty_table() noexcept { return m_novelty_table; }

    bool should_prune_state(const StateView<Kind>& state) override
    {
        if (m_max_arity == 0)
            return !m_root_state->initialize_or_matches(state);

        return !m_novelty_table.insert(state, m_max_arity);
    }

    bool should_prune_successor_state(const StateView<Kind>& state, const StateView<Kind>& succ_state, bool is_new_succ) override
    {
        if (m_max_arity == 0)
        {
            static_cast<void>(succ_state);
            if (!is_new_succ)
                return false;

            return !m_root_state->matches(state);
        }

        return is_new_succ && !m_novelty_table.insert(state, succ_state, m_max_arity);
    }

private:
    struct SharedRootState
    {
        bool initialize_or_matches(const StateView<Kind>& state)
        {
            const auto lock = lock_if_shared();
            if (!repository)
            {
                repository = state.get_state_repository();
                index = state.get_index();
            }
            return matches_unlocked(state);
        }

        bool matches(const StateView<Kind>& state) const
        {
            const auto lock = lock_if_shared();
            return repository && matches_unlocked(state);
        }

        /// Worker construction happens before search threads start.
        void enable_thread_safety() const noexcept { thread_safe = true; }

    private:
        std::unique_lock<std::mutex> lock_if_shared() const
        {
            auto lock = std::unique_lock(mutex, std::defer_lock);
            if (thread_safe)
                lock.lock();
            return lock;
        }

        bool matches_unlocked(const StateView<Kind>& state) const
        {
            return repository.get() == state.get_state_repository().get() && index == state.get_index();
        }

        // Justification: protects the one-time repository/index publication and subsequent reads shared by arity-zero workers; without it, workers could
        // initialize different roots or race while observing the root identity.
        mutable std::mutex mutex;
        mutable bool thread_safe { false };
        std::shared_ptr<StateRepository<Kind>> repository;
        ygg::Index<State<Kind>> index = ygg::Index<State<Kind>>::max();
    };

    NoveltyPruningStrategy(ygg::uint_t max_arity, std::shared_ptr<SharedRootState> root_state, DynamicNoveltyTable<MaxArity> novelty_table) :
        m_max_arity(max_arity),
        m_root_state(std::move(root_state)),
        m_novelty_table(std::move(novelty_table))
    {
    }

    ygg::uint_t m_max_arity;
    std::shared_ptr<SharedRootState> m_root_state;
    DynamicNoveltyTable<MaxArity> m_novelty_table;
};

}

#endif
