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

#ifndef TYR_PLANNING_ALGORITHMS_STATISTICS_HPP_
#define TYR_PLANNING_ALGORITHMS_STATISTICS_HPP_

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace tyr::planning
{

class Statistics
{
private:
    uint64_t m_num_generated;
    uint64_t m_num_expanded;
    uint64_t m_num_deadends;
    uint64_t m_num_pruned;
    uint64_t m_num_registered_states;

    size_t m_state_storage_memory_usage;
    size_t m_action_bindings_memory_usage;
    size_t m_predicate_bindings_memory_usage;
    size_t m_axiom_bindings_memory_usage;
    size_t m_function_bindings_memory_usage;

    std::chrono::nanoseconds m_idle_time;

    std::chrono::time_point<std::chrono::steady_clock> m_search_start_time_point;
    std::chrono::time_point<std::chrono::steady_clock> m_search_end_time_point;

public:
    Statistics() :
        m_num_generated(0),
        m_num_expanded(0),
        m_num_deadends(0),
        m_num_pruned(0),
        m_num_registered_states(0),
        m_state_storage_memory_usage(0),
        m_action_bindings_memory_usage(0),
        m_predicate_bindings_memory_usage(0),
        m_axiom_bindings_memory_usage(0),
        m_function_bindings_memory_usage(0),
        m_idle_time(0)
    {
    }

    void clear() noexcept
    {
        m_num_generated = 0;
        m_num_expanded = 0;
        m_num_deadends = 0;
        m_num_pruned = 0;
        m_num_registered_states = 0;
        m_state_storage_memory_usage = 0;
        m_action_bindings_memory_usage = 0;
        m_predicate_bindings_memory_usage = 0;
        m_axiom_bindings_memory_usage = 0;
        m_function_bindings_memory_usage = 0;
        m_idle_time = {};
        m_search_start_time_point = {};
        m_search_end_time_point = {};
    }

    /**
     * Setters
     */

    void increment_num_generated() { ++m_num_generated; }
    void increment_num_expanded() { ++m_num_expanded; }
    void increment_num_deadends() { ++m_num_deadends; }
    void increment_num_pruned() { ++m_num_pruned; }
    void add_idle_time(std::chrono::nanoseconds duration) noexcept { m_idle_time += duration; }

    void set_num_registered_states(uint64_t value) noexcept { m_num_registered_states = value; }
    void set_state_storage_memory_usage(size_t value) noexcept { m_state_storage_memory_usage = value; }
    void set_action_bindings_memory_usage(size_t value) noexcept { m_action_bindings_memory_usage = value; }
    void set_predicate_bindings_memory_usage(size_t value) noexcept { m_predicate_bindings_memory_usage = value; }
    void set_axiom_bindings_memory_usage(size_t value) noexcept { m_axiom_bindings_memory_usage = value; }
    void set_function_bindings_memory_usage(size_t value) noexcept { m_function_bindings_memory_usage = value; }

    /// Accumulate work while retaining the peak resource usage of sequentially composed searches.
    void add(const Statistics& other)
    {
        m_num_generated += other.m_num_generated;
        m_num_expanded += other.m_num_expanded;
        m_num_deadends += other.m_num_deadends;
        m_num_pruned += other.m_num_pruned;
        m_num_registered_states = std::max(m_num_registered_states, other.m_num_registered_states);
        m_state_storage_memory_usage = std::max(m_state_storage_memory_usage, other.m_state_storage_memory_usage);
        m_action_bindings_memory_usage = std::max(m_action_bindings_memory_usage, other.m_action_bindings_memory_usage);
        m_predicate_bindings_memory_usage = std::max(m_predicate_bindings_memory_usage, other.m_predicate_bindings_memory_usage);
        m_axiom_bindings_memory_usage = std::max(m_axiom_bindings_memory_usage, other.m_axiom_bindings_memory_usage);
        m_function_bindings_memory_usage = std::max(m_function_bindings_memory_usage, other.m_function_bindings_memory_usage);
        m_idle_time += other.m_idle_time;
    }

    void set_search_start_time_point(std::chrono::time_point<std::chrono::steady_clock> time_point) { m_search_start_time_point = time_point; }
    void set_search_end_time_point(std::chrono::time_point<std::chrono::steady_clock> time_point) { m_search_end_time_point = time_point; }

    /**
     * Getters
     */

    uint64_t get_num_generated() const { return m_num_generated; }
    uint64_t get_num_expanded() const { return m_num_expanded; }
    uint64_t get_num_deadends() const { return m_num_deadends; }
    uint64_t get_num_pruned() const { return m_num_pruned; }
    uint64_t get_num_registered_states() const noexcept { return m_num_registered_states; }

    size_t get_state_storage_memory_usage() const noexcept { return m_state_storage_memory_usage; }
    size_t get_action_bindings_memory_usage() const noexcept { return m_action_bindings_memory_usage; }
    size_t get_predicate_bindings_memory_usage() const noexcept { return m_predicate_bindings_memory_usage; }
    size_t get_axiom_bindings_memory_usage() const noexcept { return m_axiom_bindings_memory_usage; }
    size_t get_function_bindings_memory_usage() const noexcept { return m_function_bindings_memory_usage; }

    auto get_search_time() const { return m_search_end_time_point - m_search_start_time_point; }
    auto get_current_search_time() const { return std::chrono::steady_clock::now() - m_search_start_time_point; }
    auto get_idle_time() const noexcept { return m_idle_time; }
};

class ProgressStatistics
{
public:
    class Snapshot
    {
    private:
        uint64_t m_num_generated;
        uint64_t m_num_expanded;
        uint64_t m_num_deadends;
        uint64_t m_num_pruned;

    public:
        Snapshot(uint64_t num_generated, uint64_t num_expanded, uint64_t num_deadends, uint64_t num_pruned) :
            m_num_generated(num_generated),
            m_num_expanded(num_expanded),
            m_num_deadends(num_deadends),
            m_num_pruned(num_pruned)
        {
        }

        uint64_t get_num_generated() const { return m_num_generated; }
        uint64_t get_num_expanded() const { return m_num_expanded; }
        uint64_t get_num_deadends() const { return m_num_deadends; }
        uint64_t get_num_pruned() const { return m_num_pruned; }
    };

    void add_snapshot(const Statistics& statistics)
    {
        m_snapshots.push_back(
            Snapshot(statistics.get_num_generated(), statistics.get_num_expanded(), statistics.get_num_deadends(), statistics.get_num_pruned()));
    }

    void add_snap_shot(const Statistics& statistics) { add_snapshot(statistics); }
    void clear() noexcept { m_snapshots.clear(); }
    bool empty() const noexcept { return m_snapshots.empty(); }
    size_t size() const noexcept { return m_snapshots.size(); }

    const auto& get_snapshots() const { return m_snapshots; }

private:
    std::vector<Snapshot> m_snapshots;
};

}

#endif
