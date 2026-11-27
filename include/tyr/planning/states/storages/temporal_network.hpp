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

#ifndef TYR_PLANNING_STATES_STORAGES_TEMPORAL_NETWORK_HPP_
#define TYR_PLANNING_STATES_STORAGES_TEMPORAL_NETWORK_HPP_

#include <cstddef>
#include <limits>
#include <span>
#include <urd/elements/concepts.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

/// A simple temporal network in minimal form: timepoints t_0 (the origin, time 0), t_1, ... and, for each ordered pair, the
/// tightest upper bound d(i, j) on t_j - t_i that its constraints imply (infinity: unbounded). Kept closed (all-pairs
/// shortest paths) under each new constraint, so that consistency, the earliest and latest time of a timepoint, and the
/// projection onto a subset of timepoints (exact for simple temporal networks) are reads. The distances are a pooled urd
/// sequence, row by row.
class TemporalNetwork
{
public:
    /// The separation of interfering happenings (PDDL 2.1's epsilon).
    static constexpr ygg::float_t epsilon = static_cast<ygg::float_t>(0.001);
    static constexpr ygg::float_t infinity = std::numeric_limits<ygg::float_t>::infinity();

    /// The network of the origin alone.
    TemporalNetwork() { clear(); }

    /// Back to the origin alone; buffers kept.
    void clear()
    {
        m_size = 1;
        m_distances.assign(1, 0);
    }

    [[nodiscard]] std::size_t size() const noexcept { return m_size; }

    /// The tightest upper bound on t_j - t_i.
    [[nodiscard]] ygg::float_t distance(std::size_t i, std::size_t j) const noexcept { return m_distances[(i * m_size) + j]; }

    /// The earliest time of t_i (its lower bound relative to the origin).
    [[nodiscard]] ygg::float_t earliest(std::size_t i) const noexcept { return -distance(i, 0); }

    /// The latest time of t_i (infinity: unbounded).
    [[nodiscard]] ygg::float_t latest(std::size_t i) const noexcept { return distance(0, i); }

    /// The distances row by row (what a state stores).
    [[nodiscard]] std::span<const ygg::float_t> distances() const noexcept { return m_distances; }

    /// Takes a stored network of `size` timepoints (a minimal one, row by row).
    void assign(std::size_t size, std::span<const ygg::float_t> distances)
    {
        m_size = size;
        m_distances.assign(distances.begin(), distances.end());
    }

    /// Takes a stored network of `size` timepoints from its packed form, each distance `convert(packed)`.
    template<typename Packed, typename Convert>
    void assign(std::size_t size, std::span<const Packed> packed, Convert convert)
    {
        m_size = size;
        m_distances.clear();
        for (const auto value : packed)
            m_distances.push_back(convert(value));
    }

    /// A new timepoint, unconstrained but not before the origin (time never runs backwards): its index.
    std::size_t add_timepoint()
    {
        const auto old_size = m_size;
        const auto new_size = m_size + 1;
        m_scratch.assign(new_size * new_size, infinity);
        for (std::size_t i = 0; i < old_size; ++i)
            for (std::size_t j = 0; j < old_size; ++j)
                m_scratch[(i * new_size) + j] = m_distances[(i * old_size) + j];
        m_scratch[(old_size * new_size) + old_size] = 0;
        m_distances.swap(m_scratch);
        m_size = new_size;
        // t_new >= t_0: t_0 - t_new <= 0. A fresh timepoint keeps the network consistent.
        static_cast<void>(constrain(old_size, 0, 0));
        return old_size;
    }

    /// Adds t_j - t_i <= bound and closes the network again (O(n^2)); false, with the network unchanged, if that is
    /// inconsistent.
    [[nodiscard]] bool constrain(std::size_t i, std::size_t j, ygg::float_t bound)
    {
        if (bound >= distance(i, j))
            return true;
        // A negative cycle through the new edge: t_i - t_j <= d(j, i) and t_j - t_i <= bound with d(j, i) + bound < 0.
        if (distance(j, i) + bound < -tolerance(bound))
            return false;
        for (std::size_t a = 0; a < m_size; ++a)
        {
            const auto to_i = distance(a, i);
            if (to_i == infinity)
                continue;
            for (std::size_t b = 0; b < m_size; ++b)
            {
                const auto from_j = distance(j, b);
                if (from_j == infinity)
                    continue;
                auto& d = m_distances[(a * m_size) + b];
                const auto through = to_i + bound + from_j;
                if (through < d)
                    d = through;
            }
        }
        return true;
    }

    /// Requires lower <= t_j - t_i <= upper (either may be infinite); false, with the network possibly tightened by the
    /// first bound, if inconsistent: callers drop the network then.
    [[nodiscard]] bool constrain_between(std::size_t i, std::size_t j, ygg::float_t lower, ygg::float_t upper)
    { return constrain(i, j, upper) && (lower == -infinity || constrain(j, i, -lower)); }

    /// Keeps only the timepoints `kept` (the origin first), in that order: the minimal network restricted to them, which
    /// implies exactly what the full one implies about them.
    void project(std::span<const std::size_t> kept)
    {
        const auto new_size = kept.size();
        m_scratch.resize(new_size * new_size);
        for (std::size_t a = 0; a < new_size; ++a)
            for (std::size_t b = 0; b < new_size; ++b)
                m_scratch[(a * new_size) + b] = distance(kept[a], kept[b]);
        m_distances.swap(m_scratch);
        m_size = new_size;
    }

    static constexpr auto identifying_members() noexcept { return urd::Members<&TemporalNetwork::m_size, &TemporalNetwork::m_distances> {}; }

private:
    static ygg::float_t tolerance(ygg::float_t bound) noexcept { return ygg::FloatTolerance<ygg::float_t>::tolerance(bound, 0); }

    std::size_t m_size = 1;
    urd::Store<urd::sequence<ygg::float_t>> m_distances;
    urd::Store<urd::sequence<ygg::float_t>> m_scratch;
};

}

#endif
