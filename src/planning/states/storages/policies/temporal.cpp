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

#include "tyr/planning/states/storages/policies/temporal.hpp"

#include "tyr/planning/states/state_data.hpp"
#include "tyr/planning/states/storages/policies/policy.hpp"
#include "tyr/planning/states/storages/temporal_parts.hpp"
#include "tyr/planning/states/storages/temporal_storage_data.hpp"

#include <bit>
#include <cmath>
#include <cstdint>
#include <span>

namespace tyr::planning
{

void unpack_temporal(std::span<const std::uint64_t> words, TemporalParts& parts)
{
    parts.clear();
    if (words.empty())
        return;
    auto at = std::size_t { 0 };
    const auto next = [&] { return words[at++]; };
    const auto number = [&] { return std::bit_cast<ygg::float_t>(next()); };
    const auto open_count = next();
    const auto role_count = next();
    parts.til_progress = static_cast<std::uint32_t>(next());
    const auto size = static_cast<std::size_t>(next());
    for (std::uint64_t i = 0; i < open_count; ++i)
    {
        auto open = OpenAction();
        const auto relation = next();
        const auto row = next();
        open.action = ActionRowKey(
            urd::detail::key_part<formalism::Action<LiftedTag>> { static_cast<urd::key_value_t<formalism::Action<LiftedTag>>>(relation) },
            urd::detail::key_part<ygg::Row<formalism::Action<LiftedTag>>> { static_cast<urd::key_value_t<ygg::Row<formalism::Action<LiftedTag>>>>(row) });
        open.start = static_cast<std::uint32_t>(next());
        open.min_duration = number();
        open.max_duration = number();
        parts.open.push_back(open);
    }
    for (std::uint64_t i = 0; i < role_count; ++i)
    {
        auto role = TemporalRole();
        role.kind = static_cast<TemporalRoleKind>(next());
        role.variable = static_cast<std::uint32_t>(next());
        role.timepoint = static_cast<std::uint32_t>(next());
        parts.roles.push_back(role);
    }
    parts.network.assign(size, words.subspan(at, size * size), [](std::uint64_t word) { return std::bit_cast<ygg::float_t>(word); });
}

template<TaskKind Kind, typename Tag, bool ThreadSafe>
StateComponentRepository<TemporalStorage<Kind>, Tag, ThreadSafe>::StateComponentRepository(Context& ctx) : m_temporals(&ctx.temporals)
{
}

template<TaskKind Kind, typename Tag, bool ThreadSafe>
auto StateComponentRepository<TemporalStorage<Kind>, Tag, ThreadSafe>::insert_built(const Unpacked& unpacked) -> std::pair<Key, bool>
{
    m_words.clear();
    pack_temporal(unpacked.parts, m_words);
    const auto [handle, inserted] = m_temporals->insert(m_words);
    return { Key(handle), inserted };
}

template<TaskKind Kind, typename Tag, bool ThreadSafe>
void StateComponentRepository<TemporalStorage<Kind>, Tag, ThreadSafe>::unpack(const Handle& packed, Unpacked& unpacked) const
{
    m_words.resize(m_temporals->row_size(packed));
    m_temporals->copy(packed, m_words);
    unpack_temporal(m_words, unpacked.parts);
}

template<TaskKind Kind, typename Tag, bool ThreadSafe>
TemporalParts StateComponentRepository<TemporalStorage<Kind>, Tag, ThreadSafe>::unpack(Key key) const
{
    auto words = urd::Store<urd::sequence<std::uint64_t>>(m_temporals->row_size(key.value));
    m_temporals->copy(key.value, words);
    auto parts = TemporalParts();
    unpack_temporal(words, parts);
    return parts;
}

template class StateComponentRepository<TemporalStorage<GroundTag>, StateStoragePolicyTag, true>;
template class StateComponentRepository<TemporalStorage<LiftedTag>, StateStoragePolicyTag, true>;

}
