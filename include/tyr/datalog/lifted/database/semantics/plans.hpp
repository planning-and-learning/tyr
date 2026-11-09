/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_PLANS_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_PLANS_HPP_

#include "yggdrasil/formalism/relation_view.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

#include <algorithm>
#include <urd/elements/flat.hpp>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <tuple>
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
    urd::flat_vector<ygg::ColumnLayout> m_input;
    urd::flat_vector<ygg::ColumnLayout> m_output;
    urd::flat_vector<ColumnSlice> m_positions;

public:
    ProjectionPlan() = default;
    ProjectionPlan(std::span<const ygg::ColumnLayout> input, std::span<const urd::index<ygg::Column>> columns);

    [[nodiscard]] std::span<const ygg::ColumnLayout> input_columns() const noexcept { return { m_input.data(), m_input.size() }; }
    [[nodiscard]] std::span<const ygg::ColumnLayout> output_columns() const noexcept { return { m_output.data(), m_output.size() }; }
    [[nodiscard]] std::span<const ColumnSlice> positions() const noexcept { return { m_positions.data(), m_positions.size() }; }

    static constexpr auto cista_members() noexcept
    { return urd::Members<&ProjectionPlan::m_input, &ProjectionPlan::m_output, &ProjectionPlan::m_positions> {}; }
};

/// Natural join (and semijoin, antijoin) keys and payload: common labels must have the same type. The output is the lhs
/// columns followed by the rhs-only ones. The smaller hash-build side is selected from current input sizes at runtime.
class JoinPlan
{
    urd::flat_vector<ygg::ColumnLayout> m_lhs;
    urd::flat_vector<ygg::ColumnLayout> m_rhs;
    urd::flat_vector<ygg::ColumnLayout> m_output;
    urd::flat_vector<ColumnSlice> m_lhs_keys;
    urd::flat_vector<ColumnSlice> m_rhs_keys;
    urd::flat_vector<ColumnSlice> m_rhs_payload;

public:
    JoinPlan() = default;
    JoinPlan(std::span<const ygg::ColumnLayout> lhs, std::span<const ygg::ColumnLayout> rhs);

    [[nodiscard]] std::span<const ygg::ColumnLayout> lhs_columns() const noexcept { return { m_lhs.data(), m_lhs.size() }; }
    [[nodiscard]] std::span<const ygg::ColumnLayout> rhs_columns() const noexcept { return { m_rhs.data(), m_rhs.size() }; }
    [[nodiscard]] std::span<const ygg::ColumnLayout> output_columns() const noexcept { return { m_output.data(), m_output.size() }; }
    [[nodiscard]] std::span<const ColumnSlice> lhs_keys() const noexcept { return { m_lhs_keys.data(), m_lhs_keys.size() }; }
    [[nodiscard]] std::span<const ColumnSlice> rhs_keys() const noexcept { return { m_rhs_keys.data(), m_rhs_keys.size() }; }
    [[nodiscard]] std::span<const ColumnSlice> rhs_payload() const noexcept { return { m_rhs_payload.data(), m_rhs_payload.size() }; }

    static constexpr auto cista_members() noexcept
    { return urd::Members<&JoinPlan::m_lhs, &JoinPlan::m_rhs, &JoinPlan::m_output, &JoinPlan::m_lhs_keys, &JoinPlan::m_rhs_keys, &JoinPlan::m_rhs_payload> {}; }
};

/// Union and difference: both inputs have the same columns in the same order (project one first to align them).
class SetPlan
{
    urd::flat_vector<ygg::ColumnLayout> m_columns;

public:
    SetPlan() = default;
    SetPlan(std::span<const ygg::ColumnLayout> lhs, std::span<const ygg::ColumnLayout> rhs);

    /// The columns of both inputs and of the output.
    [[nodiscard]] std::span<const ygg::ColumnLayout> output_columns() const noexcept { return { m_columns.data(), m_columns.size() }; }

    static constexpr auto cista_members() noexcept { return urd::Members<&SetPlan::m_columns> {}; }
};

/// The rows whose columns `lhs` and `rhs` (of the same type) hold equal values.
class SelectEqualPlan
{
    urd::flat_vector<ygg::ColumnLayout> m_columns;
    size_t m_lhs = 0;
    size_t m_rhs = 0;

public:
    SelectEqualPlan() = default;
    SelectEqualPlan(std::span<const ygg::ColumnLayout> input, urd::index<ygg::Column> lhs, urd::index<ygg::Column> rhs);

    /// The columns of the input and of the output.
    [[nodiscard]] std::span<const ygg::ColumnLayout> output_columns() const noexcept { return { m_columns.data(), m_columns.size() }; }
    [[nodiscard]] size_t lhs_position() const noexcept { return m_lhs; }
    [[nodiscard]] size_t rhs_position() const noexcept { return m_rhs; }

    static constexpr auto cista_members() noexcept { return urd::Members<&SelectEqualPlan::m_columns, &SelectEqualPlan::m_lhs, &SelectEqualPlan::m_rhs> {}; }
};

/// The rows whose column `column` holds `value` (canonical, encode_value) of type `type`, the column's.
class SelectValuePlan
{
    urd::flat_vector<ygg::ColumnLayout> m_columns;
    size_t m_position = 0;
    ygg::Value m_value = 0;

public:
    SelectValuePlan() = default;
    SelectValuePlan(std::span<const ygg::ColumnLayout> input, urd::index<ygg::Column> column, ygg::TypeId type, ygg::Value value);

    /// The columns of the input and of the output.
    [[nodiscard]] std::span<const ygg::ColumnLayout> output_columns() const noexcept { return { m_columns.data(), m_columns.size() }; }
    [[nodiscard]] size_t position() const noexcept { return m_position; }
    [[nodiscard]] ygg::Value value() const noexcept { return m_value; }

    static constexpr auto cista_members() noexcept { return urd::Members<&SelectValuePlan::m_columns, &SelectValuePlan::m_position, &SelectValuePlan::m_value> {}; }
};

}  // namespace tyr::datalog::database

#endif
