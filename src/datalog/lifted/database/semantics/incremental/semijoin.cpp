/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/semantics/incremental/semijoin.hpp"

#include <ranges>

namespace tyr::datalog::database::incremental
{
bool SemijoinEvaluator::matched(const ygg::RowPool::row_type& lhs_row) const
{
    const auto lhs_keys = m_plan.lhs_keys();
    if (lhs_keys.empty() || !m_rhs_index)  // key columns have an index
        return !m_rhs.empty();
    m_key.clear();
    for (const auto key : lhs_keys)
        m_key.push_back(lhs_row.get(key.position));
    return m_rhs.index(*m_rhs_index).contains(std::span<const ygg::Value>(m_key));
}
}
