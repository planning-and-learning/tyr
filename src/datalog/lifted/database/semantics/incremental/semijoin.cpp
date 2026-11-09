/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/semantics/incremental/semijoin.hpp"

namespace tyr::datalog::database::incremental
{
bool SemijoinEvaluator::matched(const ygg::RowPool::row_type& lhs_row) const
{
    const auto lhs_keys = m_plan.lhs_keys();
    const auto rhs_keys = m_plan.rhs_keys();
    if (lhs_keys.empty())
        return !m_rhs.empty();
    const auto current = m_layer->view(m_rhs);
    for (const auto position : m_rhs_index.values(database::detail::join_key_hash(lhs_row, lhs_keys)))
        if (database::detail::join_keys_equal(current.row(position).get_values(), rhs_keys, lhs_row, lhs_keys))
            return true;
    return false;
}
}
