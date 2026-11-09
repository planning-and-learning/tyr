/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/semantics/search/backward_frontier.hpp"

namespace tyr::datalog::database
{
void BackwardFrontier::start(std::span<const VertexSet::handle_type> targets)
{
    m_level.clear();
    m_frontier.clear();
    for (const auto target : targets)
    {
        level_slot(target) = 0;
        m_frontier.push_back(target);
    }
    m_depth = 0;
    m_exhausted = false;
}
}
