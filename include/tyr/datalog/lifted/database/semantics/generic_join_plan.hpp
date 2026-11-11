/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_GENERIC_JOIN_PLAN_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_GENERIC_JOIN_PLAN_HPP_

#include "tyr/datalog/lifted/database/semantics/plans.hpp"

#include <cstddef>
#include <span>
#include <urd/elements/flat.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>

namespace tyr::datalog::database
{
/// Prepared full natural join. Variable order controls execution; output order
/// independently preserves the caller's schema. Projection is a separate operator.
class GenericJoinPlan
{
    urd::flat_vector<urd::flat_vector<ygg::ColumnLayout>> m_inputs;
    urd::flat_vector<ygg::ColumnLayout> m_output;
    urd::flat_vector<urd::index<ygg::Column>> m_order;
    urd::flat_vector<urd::flat_vector<ColumnSlice>> m_keys;
    urd::flat_vector<urd::flat_vector<size_t>> m_occurrences;
    urd::flat_vector<ColumnSlice> m_output_positions;

public:
    GenericJoinPlan() = default;
    GenericJoinPlan(std::span<const urd::Store<urd::sequence<ygg::ColumnLayout>>> inputs,
                    std::span<const urd::index<ygg::Column>> variable_order,
                    std::span<const urd::index<ygg::Column>> output_order);

    [[nodiscard]] size_t input_count() const noexcept { return m_inputs.size(); }
    [[nodiscard]] std::span<const ygg::ColumnLayout> input_columns(size_t input) const
    {
        const auto& columns = m_inputs.at(input);
        return { columns.data(), columns.size() };
    }
    [[nodiscard]] std::span<const ygg::ColumnLayout> output_columns() const noexcept { return { m_output.data(), m_output.size() }; }
    [[nodiscard]] std::span<const urd::index<ygg::Column>> variable_order() const noexcept { return { m_order.data(), m_order.size() }; }
    [[nodiscard]] std::span<const ColumnSlice> input_keys(size_t input) const
    {
        const auto& keys = m_keys.at(input);
        return { keys.data(), keys.size() };
    }
    [[nodiscard]] std::span<const size_t> variable_inputs(size_t depth) const
    {
        const auto& occurrences = m_occurrences.at(depth);
        return { occurrences.data(), occurrences.size() };
    }
    [[nodiscard]] ColumnSlice output_position(size_t depth) const { return m_output_positions.at(depth); }

    static constexpr auto cista_members() noexcept
    {
        return urd::Members<&GenericJoinPlan::m_inputs,
                            &GenericJoinPlan::m_output,
                            &GenericJoinPlan::m_order,
                            &GenericJoinPlan::m_keys,
                            &GenericJoinPlan::m_occurrences,
                            &GenericJoinPlan::m_output_positions> {};
    }

    [[nodiscard]] bool matches(const GenericJoinPlan& other) const;
};
}  // namespace tyr::datalog::database

#endif
