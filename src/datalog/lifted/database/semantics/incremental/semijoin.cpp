/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/semantics/incremental/semijoin.hpp"

#include <ranges>

namespace tyr::datalog::database::incremental
{
bool SemijoinEvaluator::matched(const ygg::RowPool::row_type& left_row) const
{
    const auto left_keys = m_plan.left_keys();
    const auto right_keys = m_plan.right_keys();
    if (left_keys.empty())
        return !m_right_operand.empty();
    const auto current = m_layer->view(m_right_operand);
    for (const auto [first, last] = m_right_index.equal_range(database::detail::join_key_hash(left_row, left_keys));
         const auto position : std::ranges::subrange(first, last) | std::views::values)
        if (database::detail::join_keys_equal(current.row(position).get_values(), right_keys, left_row, left_keys))
            return true;
    return false;
}
}
