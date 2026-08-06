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

#ifndef TYR_PLANNING_ALGORITHMS_IW_NOVELTY_TABLE_HPP_
#define TYR_PLANNING_ALGORITHMS_IW_NOVELTY_TABLE_HPP_

#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/planning/algorithms/iw/utils.hpp"
#include "tyr/planning/ground/state_view.hpp"
#include "tyr/planning/lifted/state_view.hpp"
#include "tyr/planning/state_view.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <limits>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <vector>
#include <yggdrasil/containers/segmented_bit_vector.hpp>
#include <yggdrasil/core/types.hpp>

namespace tyr::planning::iw
{

using AtomIndexList = std::vector<ygg::uint_t>;
using TupleIndex = uint64_t;

namespace detail
{

template<size_t Arity>
struct NoveltyTableStorage
{
    // Justification: protects all novelty bits while one state is tested and inserted, so concurrent workers cannot both report the same tuple as novel.
    // A lock is required across every arity lane to preserve exact state-level novelty; shard only if profiling justifies a different algorithm.
    mutable std::mutex mutex;
    std::array<ygg::SegmentedBitVector<>, Arity + 1> seen_by_arity;
};

}

template<size_t Arity>
inline bool validate_tuple(std::span<const ygg::uint_t> tuple)
{
    if (tuple.size() > Arity)
        return false;

    for (auto i = size_t { 1 }; i < tuple.size(); ++i)
        if (tuple[i - 1] >= tuple[i])
            return false;

    return true;
}

template<size_t K>
inline TupleIndex choose(ygg::uint_t n)
{
    static_assert(K > 0);

    if constexpr (K == 1)
    {
        return n;
    }
    else
    {
        if (n < K)
            return 0;

        auto result = choose<K - 1>(n - 1);
        if (result > std::numeric_limits<TupleIndex>::max() / n)
            throw std::overflow_error("iw::choose(...): tuple index overflow.");

        result *= n;
        result /= K;
        return result;
    }
}

template<size_t Arity, size_t K = 1>
inline TupleIndex rank_tuple_impl(std::span<const ygg::uint_t> tuple)
{
    if constexpr (K > Arity)
    {
        return 0;
    }
    else
    {
        if (tuple.size() < K)
            return 0;

        const auto summand = choose<K>(tuple[K - 1]);
        const auto suffix = rank_tuple_impl<Arity, K + 1>(tuple);
        if (summand > std::numeric_limits<TupleIndex>::max() - suffix)
            throw std::overflow_error("iw::rank_tuple(...): tuple index overflow.");

        return summand + suffix;
    }
}

/// @brief Rank a canonical tuple of atom indices using the combinadic number system.
///
/// The tuple must be strictly increasing. The rank is independent of the
/// currently known number of atoms, hence old ranks remain stable when new atoms
/// are encountered.
template<size_t Arity>
inline TupleIndex rank_tuple(std::span<const ygg::uint_t> tuple)
{
    assert(validate_tuple<Arity>(tuple));
    return rank_tuple_impl<Arity>(tuple);
}

template<size_t Arity, typename Callback>
void for_each_tuple(const AtomIndexList& atoms,
                    size_t tuple_size,
                    size_t atom_pos,
                    size_t tuple_pos,
                    std::array<ygg::uint_t, Arity>& tuple,
                    Callback&& callback)
    requires(Arity > 0)
{
    if (tuple_pos == tuple_size)
    {
        callback(std::span<const ygg::uint_t>(tuple.data(), tuple_size));
        return;
    }

    const auto remaining = tuple_size - tuple_pos;
    for (auto i = atom_pos; i + remaining <= atoms.size(); ++i)
    {
        tuple[tuple_pos] = atoms[i];
        for_each_tuple<Arity>(atoms, tuple_size, i + 1, tuple_pos + 1, tuple, callback);
    }
}

template<size_t Arity, typename Callback>
void for_each_tuple(const AtomIndexList& atoms, size_t max_arity, std::array<ygg::uint_t, Arity>& tuple, Callback&& callback)
    requires(Arity > 0)
{
    assert(max_arity <= Arity);
    for (auto tuple_size = size_t { 1 }; tuple_size <= std::min({ Arity, max_arity, atoms.size() }); ++tuple_size)
        for_each_tuple<Arity>(atoms, tuple_size, 0, 0, tuple, callback);
}

template<size_t Arity, typename Callback>
void for_each_tuple(const AtomIndexList& atoms, std::array<ygg::uint_t, Arity>& tuple, Callback&& callback)
    requires(Arity > 0)
{
    for_each_tuple<Arity>(atoms, Arity, tuple, callback);
}

template<size_t Arity, typename Callback>
void for_each_tuple_with_added_atoms(const AtomIndexList& added_atoms,
                                     const AtomIndexList& kept_atoms,
                                     size_t num_added,
                                     size_t num_kept,
                                     size_t added_pos,
                                     size_t added_tuple_pos,
                                     std::array<ygg::uint_t, Arity>& added_tuple,
                                     std::array<ygg::uint_t, Arity>& kept_tuple,
                                     std::array<ygg::uint_t, Arity>& tuple,
                                     Callback&& callback)
    requires(Arity > 0)
{
    if (added_tuple_pos == num_added)
    {
        for_each_tuple<Arity>(kept_atoms,
                              num_kept,
                              0,
                              0,
                              kept_tuple,
                              [&](std::span<const ygg::uint_t> kept)
                              {
                                  std::ranges::merge(added_tuple.begin(), added_tuple.begin() + num_added, kept.begin(), kept.end(), tuple.begin());
                                  callback(std::span<const ygg::uint_t>(tuple.data(), num_added + num_kept));
                              });
        return;
    }

    const auto remaining = num_added - added_tuple_pos;
    for (auto i = added_pos; i + remaining <= added_atoms.size(); ++i)
    {
        added_tuple[added_tuple_pos] = added_atoms[i];
        for_each_tuple_with_added_atoms<
            Arity>(added_atoms, kept_atoms, num_added, num_kept, i + 1, added_tuple_pos + 1, added_tuple, kept_tuple, tuple, callback);
    }
}

template<size_t Arity, typename Callback>
void for_each_tuple_with_added_atoms(const AtomIndexList& added_atoms,
                                     const AtomIndexList& kept_atoms,
                                     size_t max_arity,
                                     std::array<ygg::uint_t, Arity>& added_tuple,
                                     std::array<ygg::uint_t, Arity>& kept_tuple,
                                     std::array<ygg::uint_t, Arity>& tuple,
                                     Callback&& callback)
    requires(Arity > 0)
{
    assert(max_arity <= Arity);
    for (auto tuple_size = size_t { 1 }; tuple_size <= std::min({ Arity, max_arity, added_atoms.size() + kept_atoms.size() }); ++tuple_size)
    {
        for (auto num_added = size_t { 1 }; num_added <= std::min(tuple_size, added_atoms.size()); ++num_added)
        {
            const auto num_kept = tuple_size - num_added;
            if (num_kept <= kept_atoms.size())
                for_each_tuple_with_added_atoms<Arity>(added_atoms, kept_atoms, num_added, num_kept, 0, 0, added_tuple, kept_tuple, tuple, callback);
        }
    }
}

template<size_t Arity, typename Callback>
void for_each_tuple_with_added_atoms(const AtomIndexList& added_atoms,
                                     const AtomIndexList& kept_atoms,
                                     std::array<ygg::uint_t, Arity>& added_tuple,
                                     std::array<ygg::uint_t, Arity>& kept_tuple,
                                     std::array<ygg::uint_t, Arity>& tuple,
                                     Callback&& callback)
    requires(Arity > 0)
{
    for_each_tuple_with_added_atoms<Arity>(added_atoms, kept_atoms, Arity, added_tuple, kept_tuple, tuple, callback);
}

template<size_t Arity>
class DynamicNoveltyTable
{
public:
    DynamicNoveltyTable() : m_storage(std::make_shared<detail::NoveltyTableStorage<Arity>>()) {}
    DynamicNoveltyTable(const DynamicNoveltyTable&) = delete;
    DynamicNoveltyTable& operator=(const DynamicNoveltyTable&) = delete;
    DynamicNoveltyTable(DynamicNoveltyTable&&) noexcept = default;
    DynamicNoveltyTable& operator=(DynamicNoveltyTable&&) noexcept = default;

    [[nodiscard]] DynamicNoveltyTable make_worker() const
    {
        // Worker construction happens before search threads start; sequential IW keeps the lock-free fast path.
        m_thread_safe = true;
        return DynamicNoveltyTable(m_storage, true);
    }

    ygg::uint_t get_arity() const noexcept { return Arity; }

    size_t get_num_bits(ygg::uint_t arity) const
    {
        assert(arity <= Arity);
        const auto lock = lock_if_shared();
        return m_storage->seen_by_arity[arity].size();
    }

    void clear()
    {
        const auto lock = lock_if_shared();
        for (auto& seen : m_storage->seen_by_arity)
            seen.clear();
    }

    void validate_max_arity(ygg::uint_t max_arity) const
    {
        if (max_arity > Arity)
            throw std::invalid_argument("DynamicNoveltyTable::validate_max_arity(...): max_arity exceeds table arity.");
    }

    bool test(std::span<const ygg::uint_t> tuple) const
    {
        assert(validate_tuple<Arity>(tuple));
        const auto lock = lock_if_shared();
        return test_unlocked(tuple);
    }

    bool insert(std::span<const ygg::uint_t> tuple)
    {
        assert(validate_tuple<Arity>(tuple));
        const auto lock = lock_if_shared();
        return insert_unlocked(tuple);
    }

    template<TaskKind Kind>
    bool insert(const StateView<Kind>& state, ygg::uint_t max_arity)
    {
        validate_max_arity(max_arity);

        collect_atoms(state, m_atoms);
        const auto lock = lock_if_shared();

        if constexpr (Arity == 0)
        {
            return insert_unlocked(std::span<const ygg::uint_t> {});
        }
        else
        {
            if (max_arity == 0)
                return insert_unlocked(std::span<const ygg::uint_t> {});

            auto novel = false;
            for_each_tuple<Arity>(m_atoms, max_arity, m_tuple, [&](std::span<const ygg::uint_t> tuple) { novel = insert_unlocked(tuple) || novel; });

            return novel;
        }
    }

    template<TaskKind Kind>
    bool insert(const StateView<Kind>& state)
    {
        return insert(state, Arity);
    }

    template<TaskKind Kind>
    bool insert(const StateView<Kind>& src, const StateView<Kind>& dst, ygg::uint_t max_arity)
    {
        validate_max_arity(max_arity);

        if constexpr (Arity == 0)
        {
            static_cast<void>(src);
            static_cast<void>(dst);
            return false;
        }
        else
        {
            if (max_arity == 0)
            {
                static_cast<void>(src);
                static_cast<void>(dst);
                return false;
            }

            collect_atoms(src, m_src_atoms);
            collect_atoms(dst, m_atoms);

            m_added_atoms.clear();
            std::ranges::set_difference(m_atoms, m_src_atoms, std::back_inserter(m_added_atoms));
            if (m_added_atoms.empty())
                return false;

            m_kept_atoms.clear();
            std::ranges::set_intersection(m_atoms, m_src_atoms, std::back_inserter(m_kept_atoms));

            const auto lock = lock_if_shared();
            auto novel = false;
            for_each_tuple_with_added_atoms<Arity>(m_added_atoms,
                                                   m_kept_atoms,
                                                   max_arity,
                                                   m_added_tuple,
                                                   m_kept_tuple,
                                                   m_tuple,
                                                   [&](std::span<const ygg::uint_t> tuple) { novel = insert_unlocked(tuple) || novel; });

            return novel;
        }
    }

    template<TaskKind Kind>
    bool insert(const StateView<Kind>& src, const StateView<Kind>& dst)
    {
        return insert(src, dst, Arity);
    }

private:
    explicit DynamicNoveltyTable(std::shared_ptr<detail::NoveltyTableStorage<Arity>> storage, bool thread_safe) :
        m_storage(std::move(storage)),
        m_thread_safe(thread_safe)
    {
    }

    std::unique_lock<std::mutex> lock_if_shared() const
    {
        auto lock = std::unique_lock(m_storage->mutex, std::defer_lock);
        if (m_thread_safe)
            lock.lock();
        return lock;
    }

    bool test_unlocked(std::span<const ygg::uint_t> tuple) const
    {
        const auto rank = rank_tuple<Arity>(tuple);
        const auto& seen = m_storage->seen_by_arity[tuple.size()];
        return rank < seen.size() && seen[rank];
    }

    bool insert_unlocked(std::span<const ygg::uint_t> tuple)
    {
        const auto rank = rank_tuple<Arity>(tuple);
        auto& seen = m_storage->seen_by_arity[tuple.size()];
        if (rank >= seen.size())
            seen.resize(rank + 1, false);

        const auto novel = !seen[rank];
        seen[rank] = true;
        return novel;
    }

    template<TaskKind Kind>
    void collect_atoms(const StateView<Kind>& state, AtomIndexList& out)
    {
        out.clear();
        for (const auto fact : state.get_fluent_facts_view())
        {
            if (const auto atom = fact.get_atom())
                out.push_back(ygg::uint_t(atom->get_index()));
        }

        std::ranges::sort(out);
        assert(std::ranges::adjacent_find(out) == out.end());
    }

    std::shared_ptr<detail::NoveltyTableStorage<Arity>> m_storage;
    mutable bool m_thread_safe { false };
    AtomIndexList m_atoms;
    AtomIndexList m_src_atoms;
    AtomIndexList m_added_atoms;
    AtomIndexList m_kept_atoms;
    std::array<ygg::uint_t, Arity> m_added_tuple;
    std::array<ygg::uint_t, Arity> m_kept_tuple;
    std::array<ygg::uint_t, Arity> m_tuple;
};

}

#endif
