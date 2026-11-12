/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/datalog/detail/program.hpp"
#include "tyr/datalog/lifted/database/datalog/detail/strata.hpp"

#include <optional>
#include <urd/memory.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/flat_set.hpp>
#include <urd/store.hpp>
#include <utility>

/// An aggregate's table holds a row (group, value) per group of its body's table; absent groups read as the fallback.
namespace tyr::datalog::database::datalog::detail
{
AggregateStratum::AggregateStratum(StratumInfo stratum, AggregateTable aggregate) :
    Stratum(std::move(stratum)),
    m_aggregate(std::move(aggregate)),
    m_groups(m_aggregate.group.size())
{
}

std::optional<ygg::uint_t> AggregateStratum::value_of(const Group& state) const
{
    if (state.count == 0)
        return std::nullopt;
    switch (m_aggregate.function)
    {
        case AggregateFunction::COUNT:
            return ygg::to_uint_t(state.count);
        case AggregateFunction::SUM:
            return static_cast<ygg::uint_t>(state.sum);
        case AggregateFunction::MIN:
            return state.values.begin()->first;
        case AggregateFunction::MAX:
            return state.values.rbegin()->first;
    }
    return std::nullopt;
}

std::pair<ygg::uint_t, AggregateStratum::Group&> AggregateStratum::group_of(std::span<const ygg::uint_t> row)
{
    m_key.clear();
    for (const auto column : m_aggregate.group)
        m_key.push_back(row[column]);
    const auto position = m_groups.insert(m_key);
    if (position == m_states.size())
        m_states.emplace_back();
    return { position, m_states[position] };
}

void AggregateStratum::change_group(Group& state, std::span<const ygg::uint_t> row, bool add) const
{
    state.count = add ? state.count + 1 : state.count - 1;
    if (m_aggregate.function == AggregateFunction::COUNT)
        return;
    const auto value = row[m_aggregate.value];
    if (m_aggregate.function == AggregateFunction::SUM)
        state.sum = add ? state.sum + value : state.sum - value;
    else if (add)
        ++state.values[value];
    else if (const auto it = state.values.find(value); --it->second == 0)
        state.values.erase(it);
}

std::span<const ygg::uint_t> AggregateStratum::group_row(std::span<const ygg::uint_t> group, ygg::uint_t value)
{
    m_row.assign(group.begin(), group.end());
    m_row.push_back(value);
    return m_row;
}

void AggregateStratum::evaluate(CompiledProgram& program, Executor&)
{
    auto& table = *program.tables[m_aggregate.table];
    table.clear();
    m_groups.clear();
    m_states.clear();
    program.tables[m_aggregate.source]->current.for_each([&](std::span<const ygg::uint_t> row) { change_group(group_of(row).second, row, true); });
    for (ygg::uint_t g = 0; g < m_groups.size(); ++g)
        if (const auto value = value_of(m_states[g]))
            table.current.insert(group_row(m_groups[g], *value));
}

void AggregateStratum::update(CompiledProgram& program, Executor&)
{
    auto& table = *program.tables[m_aggregate.table];
    auto& source = *program.tables[m_aggregate.source];
    auto touched = urd::Store<urd::sequence<std::pair<ygg::uint_t, std::optional<ygg::uint_t>>>>();
    auto is_touched = urd::Store<urd::set<ygg::uint_t>, urd::sorted>();
    const auto apply = [&](std::span<const ygg::uint_t> row, bool add)
    {
        auto [position, state] = group_of(row);
        if (is_touched.insert(position).second)
            touched.emplace_back(position, value_of(state));
        change_group(state, row, add);
    };
    source.removed.for_each([&](std::span<const ygg::uint_t> row) { apply(row, false); });
    source.added.for_each([&](std::span<const ygg::uint_t> row) { apply(row, true); });
    for (const auto& [g, before] : touched)
    {
        const auto after = value_of(m_states[g]);
        const auto group = m_groups[g];
        if (before)
            table.current.erase(group_row(group, *before));
        if (after)
            table.current.insert(group_row(group, *after));
        // An absent group reads as the fallback value, so the change sets hold it in place of the absent row.
        const auto old_value = before ? before : table.fallback;
        const auto new_value = after ? after : table.fallback;
        if (old_value == new_value)
            continue;
        if (old_value)
            table.removed.insert(group_row(group, *old_value));
        if (new_value)
            table.added.insert(group_row(group, *new_value));
    }
}

size_t AggregateStratum::state_memory_usage() const noexcept
{ return m_groups.memory_usage() + urd::memory_usage(m_states) + urd::memory_usage(m_key) + urd::memory_usage(m_row); }
}  // namespace tyr::datalog::database::datalog::detail
