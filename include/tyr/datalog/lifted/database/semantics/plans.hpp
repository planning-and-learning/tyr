/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_PLANS_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_PLANS_HPP_

#include "yggdrasil/formalism/relation_view.hpp"

#include <algorithm>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <tuple>
#include <urd/elements/flat.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>

namespace tyr::datalog::database
{

/// A field: its column's position in the row, independent of its column label. Renaming columns preserves compiled keys
/// and reusable join indexes. Copying one copies its bits: the plan checked the columns alike.
struct ColumnSlice
{
    size_t position;

    bool operator==(const ColumnSlice&) const = default;

    static constexpr auto identifying_members() noexcept { return urd::Members<&ColumnSlice::position> {}; }

    static constexpr auto cista_members() noexcept { return urd::Members<&ColumnSlice::position> {}; }
};

/// A prepared operation is built from its inputs' columns, which it validates (and owns), and resolves positions once.
/// Reuse it with any inputs of the same ordered columns, whatever their rows or identity; the operations only assert
/// (debug builds) that their inputs' and output's columns are the plan's.

/// Keeps the input's columns `columns` in that order (each an input label, once).
class ProjectionPlan
{
    urd::flat_vector<urd::data<ygg::ColumnLayout>> m_input;
    urd::flat_vector<urd::data<ygg::ColumnLayout>> m_output;
    urd::flat_vector<ColumnSlice> m_positions;

public:
    ProjectionPlan() = default;
    ProjectionPlan(std::span<const urd::data<ygg::ColumnLayout>> input, std::span<const urd::index<ygg::Column>> columns);

    [[nodiscard]] std::span<const urd::data<ygg::ColumnLayout>> input_columns() const noexcept { return { m_input.data(), m_input.size() }; }

    [[nodiscard]] std::span<const urd::data<ygg::ColumnLayout>> output_columns() const noexcept { return { m_output.data(), m_output.size() }; }

    [[nodiscard]] std::span<const ColumnSlice> positions() const noexcept { return { m_positions.data(), m_positions.size() }; }

    static constexpr auto cista_members() noexcept
    { return urd::Members<&ProjectionPlan::m_input, &ProjectionPlan::m_output, &ProjectionPlan::m_positions> {}; }
};

/// Natural join (and semijoin, antijoin) keys and payload: common labels must have the same type. The output is the left
/// columns followed by the right-only ones. The smaller hash-build side is selected from current input sizes at runtime.
class JoinPlan
{
    urd::flat_vector<urd::data<ygg::ColumnLayout>> m_left;
    urd::flat_vector<urd::data<ygg::ColumnLayout>> m_right;
    urd::flat_vector<urd::data<ygg::ColumnLayout>> m_output;
    urd::flat_vector<ColumnSlice> m_left_keys;
    urd::flat_vector<ColumnSlice> m_right_keys;
    urd::flat_vector<ColumnSlice> m_right_payload;

public:
    JoinPlan() = default;
    JoinPlan(std::span<const urd::data<ygg::ColumnLayout>> left, std::span<const urd::data<ygg::ColumnLayout>> right);

    [[nodiscard]] std::span<const urd::data<ygg::ColumnLayout>> left_columns() const noexcept { return { m_left.data(), m_left.size() }; }

    [[nodiscard]] std::span<const urd::data<ygg::ColumnLayout>> right_columns() const noexcept { return { m_right.data(), m_right.size() }; }

    [[nodiscard]] std::span<const urd::data<ygg::ColumnLayout>> output_columns() const noexcept { return { m_output.data(), m_output.size() }; }

    [[nodiscard]] std::span<const ColumnSlice> left_keys() const noexcept { return { m_left_keys.data(), m_left_keys.size() }; }

    [[nodiscard]] std::span<const ColumnSlice> right_keys() const noexcept { return { m_right_keys.data(), m_right_keys.size() }; }

    [[nodiscard]] std::span<const ColumnSlice> right_payload() const noexcept { return { m_right_payload.data(), m_right_payload.size() }; }

    static constexpr auto cista_members() noexcept
    {
        return urd::
            Members<&JoinPlan::m_left, &JoinPlan::m_right, &JoinPlan::m_output, &JoinPlan::m_left_keys, &JoinPlan::m_right_keys, &JoinPlan::m_right_payload> {};
    }
};

/// Union and difference: both inputs have the same columns in the same order (project one first to align them).
class SetPlan
{
    urd::flat_vector<urd::data<ygg::ColumnLayout>> m_columns;

public:
    SetPlan() = default;
    SetPlan(std::span<const urd::data<ygg::ColumnLayout>> left, std::span<const urd::data<ygg::ColumnLayout>> right);

    /// The columns of both inputs and of the output.
    [[nodiscard]] std::span<const urd::data<ygg::ColumnLayout>> output_columns() const noexcept { return { m_columns.data(), m_columns.size() }; }

    static constexpr auto cista_members() noexcept { return urd::Members<&SetPlan::m_columns> {}; }
};

/// The rows whose columns `left` and `right` (of the same type) hold equal values.
class SelectEqualPlan
{
    urd::flat_vector<urd::data<ygg::ColumnLayout>> m_columns;
    size_t m_left = 0;
    size_t m_right = 0;

public:
    SelectEqualPlan() = default;
    SelectEqualPlan(std::span<const urd::data<ygg::ColumnLayout>> input, urd::index<ygg::Column> left, urd::index<ygg::Column> right);

    /// The columns of the input and of the output.
    [[nodiscard]] std::span<const urd::data<ygg::ColumnLayout>> output_columns() const noexcept { return { m_columns.data(), m_columns.size() }; }

    [[nodiscard]] size_t left_position() const noexcept { return m_left; }

    [[nodiscard]] size_t right_position() const noexcept { return m_right; }

    static constexpr auto cista_members() noexcept { return urd::Members<&SelectEqualPlan::m_columns, &SelectEqualPlan::m_left, &SelectEqualPlan::m_right> {}; }
};

/// The rows whose column `column` holds `value` (canonical, encode_value) of type `type`, the column's.
class SelectValuePlan
{
    urd::flat_vector<urd::data<ygg::ColumnLayout>> m_columns;
    size_t m_position = 0;
    ygg::Value m_value = 0;

public:
    SelectValuePlan() = default;
    SelectValuePlan(std::span<const urd::data<ygg::ColumnLayout>> input, urd::index<ygg::Column> column, ygg::TypeId type, ygg::Value value);

    /// The columns of the input and of the output.
    [[nodiscard]] std::span<const urd::data<ygg::ColumnLayout>> output_columns() const noexcept { return { m_columns.data(), m_columns.size() }; }

    [[nodiscard]] size_t position() const noexcept { return m_position; }

    [[nodiscard]] ygg::Value value() const noexcept { return m_value; }

    static constexpr auto cista_members() noexcept
    { return urd::Members<&SelectValuePlan::m_columns, &SelectValuePlan::m_position, &SelectValuePlan::m_value> {}; }
};

}  // namespace tyr::datalog::database

#endif
