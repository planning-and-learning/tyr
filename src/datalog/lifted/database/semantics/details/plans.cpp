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
size_t column_position(std::span<const urd::data<ygg::ColumnLayout>> columns, urd::index<ygg::Column> label)
{
    const auto found = std::ranges::find(columns, label, &urd::data<ygg::ColumnLayout>::label);
    if (found == columns.end())
        throw std::out_of_range("Columns: unknown column label.");
    return static_cast<size_t>(found - columns.begin());
}
}  // namespace

namespace tyr::datalog::database
{
ProjectionPlan::ProjectionPlan(std::span<const urd::data<ygg::ColumnLayout>> input, std::span<const urd::index<ygg::Column>> columns)
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

JoinPlan::JoinPlan(std::span<const urd::data<ygg::ColumnLayout>> left, std::span<const urd::data<ygg::ColumnLayout>> right)
{
    ygg::validate_columns(left);
    ygg::validate_columns(right);
    m_left.set(left.begin(), left.end());
    m_right.set(right.begin(), right.end());
    m_output.set(left.begin(), left.end());
    for (size_t position = 0; position < right.size(); ++position)
    {
        const auto& column = right[position];
        const auto it = std::ranges::find(left, column.label, &urd::data<ygg::ColumnLayout>::label);
        if (it == left.end())
        {
            m_output.push_back(column);
            m_right_payload.push_back(ColumnSlice { position });
        }
        else
        {
            if (it->type != column.type)
                throw std::invalid_argument("Relational operation: common columns must have the same type.");
            m_left_keys.push_back(ColumnSlice { static_cast<size_t>(it - left.begin()) });
            m_right_keys.push_back(ColumnSlice { position });
        }
    }
}

SetPlan::SetPlan(std::span<const urd::data<ygg::ColumnLayout>> left, std::span<const urd::data<ygg::ColumnLayout>> right)
{
    ygg::validate_columns(left);
    if (!std::ranges::equal(left, right))
        throw std::invalid_argument("Relational operation: input schemas must have the same column order.");
    m_columns.set(left.begin(), left.end());
}

SelectEqualPlan::SelectEqualPlan(std::span<const urd::data<ygg::ColumnLayout>> input, urd::index<ygg::Column> left, urd::index<ygg::Column> right) :
    m_left(column_position(input, left)),
    m_right(column_position(input, right))
{
    ygg::validate_columns(input);
    if (input[m_left].type != input[m_right].type)
        throw std::invalid_argument("Relational operation: selected columns must have the same type.");
    m_columns.set(input.begin(), input.end());
}

SelectValuePlan::SelectValuePlan(std::span<const urd::data<ygg::ColumnLayout>> input, urd::index<ygg::Column> column, ygg::TypeId type, ygg::Value value) :
    m_position(column_position(input, column)),
    m_value(value)
{
    ygg::validate_columns(input);
    if (input[m_position].type != type)
        throw std::invalid_argument("Relational operation: selected value has an incompatible column type.");
    m_columns.set(input.begin(), input.end());
}
}  // namespace tyr::datalog::database
