/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/semantics/plans.hpp"

#include <algorithm>
#include <stdexcept>

namespace
{
/// The position of the column labeled `label`; std::out_of_range where there is none.
size_t column_position(std::span<const ygg::ColumnLayout> columns, urd::index<ygg::Column> label)
{
    const auto found = std::ranges::find(columns, label, &ygg::ColumnLayout::label);
    if (found == columns.end())
        throw std::out_of_range("Columns: unknown column label.");
    return static_cast<size_t>(found - columns.begin());
}
}  // namespace

namespace tyr::datalog::database
{
ProjectionPlan::ProjectionPlan(std::span<const ygg::ColumnLayout> input, std::span<const urd::index<ygg::Column>> columns)
{
    ygg::validate_columns(input);
    m_input.set(input.begin(), input.end());
    for (const auto label : columns)
    {
        const auto position = column_position(input, label);
        m_output.push_back(input[position]);
        m_positions.push_back(ColumnSlice { position });
    }
    ygg::validate_columns(output_columns());
}

JoinPlan::JoinPlan(std::span<const ygg::ColumnLayout> lhs, std::span<const ygg::ColumnLayout> rhs)
{
    ygg::validate_columns(lhs);
    ygg::validate_columns(rhs);
    m_lhs.set(lhs.begin(), lhs.end());
    m_rhs.set(rhs.begin(), rhs.end());
    m_output.set(lhs.begin(), lhs.end());
    for (size_t position = 0; position < rhs.size(); ++position)
    {
        const auto& column = rhs[position];
        const auto it = std::ranges::find(lhs, column.label, &ygg::ColumnLayout::label);
        if (it == lhs.end())
        {
            m_output.push_back(column);
            m_rhs_payload.push_back(ColumnSlice { position });
        }
        else
        {
            if (it->type != column.type)
                throw std::invalid_argument("Relational operation: common columns must have the same type.");
            m_lhs_keys.push_back(ColumnSlice { static_cast<size_t>(it - lhs.begin()) });
            m_rhs_keys.push_back(ColumnSlice { position });
        }
    }
}

SetPlan::SetPlan(std::span<const ygg::ColumnLayout> lhs, std::span<const ygg::ColumnLayout> rhs)
{
    ygg::validate_columns(lhs);
    if (!std::ranges::equal(lhs, rhs))
        throw std::invalid_argument("Relational operation: input schemas must have the same column order.");
    m_columns.set(lhs.begin(), lhs.end());
}

SelectEqualPlan::SelectEqualPlan(std::span<const ygg::ColumnLayout> input, urd::index<ygg::Column> lhs, urd::index<ygg::Column> rhs) :
    m_lhs(column_position(input, lhs)),
    m_rhs(column_position(input, rhs))
{
    ygg::validate_columns(input);
    if (input[m_lhs].type != input[m_rhs].type)
        throw std::invalid_argument("Relational operation: selected columns must have the same type.");
    m_columns.set(input.begin(), input.end());
}

SelectValuePlan::SelectValuePlan(std::span<const ygg::ColumnLayout> input, urd::index<ygg::Column> column, ygg::TypeId type, ygg::Value value) :
    m_position(column_position(input, column)),
    m_value(value)
{
    ygg::validate_columns(input);
    if (input[m_position].type != type)
        throw std::invalid_argument("Relational operation: selected value has an incompatible column type.");
    m_columns.set(input.begin(), input.end());
}
}  // namespace tyr::datalog::database
