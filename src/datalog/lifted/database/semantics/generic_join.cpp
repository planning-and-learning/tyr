/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/semantics/generic_join.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

#include <algorithm>
#include <span>
#include <stdexcept>
#include <vector>

namespace tyr::datalog::database
{
GenericJoinPlan::GenericJoinPlan(std::span<const urd::Store<urd::sequence<ygg::ColumnLayout>>> inputs,
                                 std::span<const urd::index<ygg::Column>> variable_order,
                                 std::span<const urd::index<ygg::Column>> output_order) :
    m_inputs(inputs.size()),
    m_order(variable_order.begin(), variable_order.end()),
    m_keys(inputs.size()),
    m_occurrences(variable_order.size())
{
    for (size_t i = 0; i < inputs.size(); ++i)
        m_inputs[i].set(inputs[i].begin(), inputs[i].end());
    urd::Store<urd::sequence<ygg::ColumnLayout>> combined;
    for (const auto& input : inputs)
    {
        ygg::validate_columns(input);
        for (const auto column : input)
        {
            const auto found = std::ranges::find(combined, column.label, &ygg::ColumnLayout::label);
            if (found == combined.end())
                combined.push_back(column);
            else if (found->type != column.type)
                throw std::invalid_argument("Generic join: common columns must have the same type.");
        }
    }
    const auto require_order = [&](std::span<const urd::index<ygg::Column>> order)
    {
        if (order.size() != combined.size())
            throw std::invalid_argument("Generic join: orders must contain every output column exactly once.");
        for (size_t i = 0; i < order.size(); ++i)
            if (std::ranges::find(combined, order[i], &ygg::ColumnLayout::label) == combined.end()
                || std::ranges::find(order.first(i), order[i]) != order.first(i).end())
                throw std::invalid_argument("Generic join: orders must contain every output column exactly once.");
    };
    require_order(variable_order);
    require_order(output_order);
    urd::Store<urd::sequence<ygg::ColumnLayout>> output;
    for (const auto label : output_order)
        output.push_back(*std::ranges::find(combined, label, &ygg::ColumnLayout::label));
    m_output.set(output.begin(), output.end());
    for (size_t depth = 0; depth < variable_order.size(); ++depth)
    {
        const auto label = variable_order[depth];
        m_output_positions.push_back(ColumnSlice { static_cast<size_t>(std::ranges::find(output, label, &ygg::ColumnLayout::label) - output.begin()) });
        for (size_t input = 0; input < inputs.size(); ++input)
        {
            const auto found = std::ranges::find(inputs[input], label, &ygg::ColumnLayout::label);
            if (found != inputs[input].end())
            {
                m_keys[input].push_back(ColumnSlice { static_cast<size_t>(found - inputs[input].begin()) });
                m_occurrences[depth].push_back(input);
            }
        }
    }
}

bool GenericJoinPlan::matches(const GenericJoinPlan& other) const
{ return m_inputs == other.m_inputs && m_order == other.m_order && m_output == other.m_output; }

namespace detail
{
void count_generic_join_work(size_t GenericJoinWork::* member) noexcept
{
    ++(generic_join_work().*member);
}
}
}

namespace tyr::datalog::database::detail
{
bool GenericJoinTrie::contains(const ygg::RowPool::row_type& row, std::span<const ColumnSlice> keys) const
{
    const auto* node = this;
    for (const auto key : keys)
    {
        node = node->find(row.get(key.position));
        if (!node)
            return false;
    }
    return node->rows != 0;
}
}

namespace tyr::datalog::database::detail
{
bool GenericJoinTrie::insert(const ygg::RowPool::row_type& row, std::span<const ColumnSlice> keys)
{
    if (keys.empty())
    {
        const bool inserted = rows == 0;
        rows = 1;
        return inserted;
    }
    const auto value = row.get(keys.front().position);
    auto found = children.find(value);
    if (found == children.end())
        found = children.emplace(value, std::make_unique<GenericJoinTrie>()).first;
    if (!found->second->insert(row, keys.subspan(1)))
        return false;
    ++rows;
    return true;
}
}

namespace tyr::datalog::database::detail
{
bool GenericJoinTrie::erase(const ygg::RowPool::row_type& row, std::span<const ColumnSlice> keys)
{
    if (keys.empty())
    {
        const bool erased = rows != 0;
        rows = 0;
        return erased;
    }
    const auto value = row.get(keys.front().position);
    const auto found = children.find(value);
    if (found == children.end() || !found->second->erase(row, keys.subspan(1)))
        return false;
    --rows;
    if (found->second->rows == 0)
        children.erase(found);
    return true;
}
}

namespace tyr::datalog::database
{
void GenericJoinWorkspace::enumerate(size_t depth, urd::builder<ygg::Relation<ygg::Schema>>& out)
{
    detail::count_generic_join_work(&detail::GenericJoinWork::prefixes);
    if (depth == m_plan.variable_order().size())
    {
        out.insert(m_row.row());
        return;
    }
    const auto count = m_plan.input_count();
    const auto cursors = std::span(m_cursors);
    const auto current = cursors.subspan(depth * count, count);
    const auto next = cursors.subspan((depth + 1) * count, count);
    const auto inputs = m_plan.variable_inputs(depth);
    const auto source = *std::ranges::min_element(inputs, {}, [&](size_t input) { return current[input]->children.size(); });
    for (const auto& [key, child] : current[source]->children)
    {
        detail::count_generic_join_work(&detail::GenericJoinWork::candidate_keys);
        std::ranges::copy(current, next.begin());
        bool present = true;
        for (const auto input : inputs)
        {
            if (input != source)
                detail::count_generic_join_work(&detail::GenericJoinWork::probes);
            next[input] = input == source ? child.get() : current[input]->find(key);
            if (!next[input])
            {
                present = false;
                break;
            }
        }
        if (present)
        {
            const auto output = m_plan.output_position(depth);
            m_row.set(output.position, key);
            enumerate(depth + 1, out);
        }
    }
}
}
