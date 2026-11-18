/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/semantics/incremental/projection.hpp"

namespace tyr::datalog::database::incremental
{
bool ProjectionEvaluator::add(std::span<const ygg::Value> row)
{
    const bool inserted = m_result->insert(row).second;
    const auto slot = m_result->slot(row);
    if (inserted)
    {
        m_supports.insert_or_assign(slot, size_t { 1 });
        return true;
    }
    auto& count = m_supports.at(slot);  // a held row has its support, zero where it is pending removal
    if (count == std::numeric_limits<size_t>::max())
        throw std::overflow_error("Incremental projection: support count overflow.");
    ++count;
    return false;
}
}

namespace tyr::datalog::database::incremental
{
void ProjectionEvaluator::remove(std::span<const ygg::Value> row)
{
    const auto slot = m_result->slot(row);
    const auto found = slot.is_none() ? m_supports.end() : m_supports.find(slot);
    if (found == m_supports.end() || found->second == 0)
        throw std::invalid_argument("Incremental projection: removal has no supporting row.");
    if (--m_supports.at(slot) == 0)
        m_pending_removals.push_back(slot);
}
}

namespace tyr::datalog::database::incremental
{
ProjectionEvaluator::ProjectionEvaluator(ygg::FreeScratchRelations& scratch, ProjectionPlan plan) :
    m_plan(std::move(plan)),
    m_result(scratch.draw(m_plan.output_columns())),
    m_delta(scratch, m_plan.output_columns())
{
}
}

namespace tyr::datalog::database::incremental
{
ygg::FreeRelationBuilderView ProjectionEvaluator::get_result() const&
{
    detail::require_initialized(m_initialized);
    return m_result->view();
}
}

namespace tyr::datalog::database::incremental
{
const Delta& ProjectionEvaluator::get_delta() const&
{
    detail::require_initialized(m_initialized);
    return m_delta;
}
}

namespace tyr::datalog::database::incremental
{
size_t ProjectionEvaluator::memory_usage() const noexcept
{ return urd::memory_usage(m_result->relation) + m_delta.memory_usage() + urd::memory_usage(m_supports) + urd::memory_usage(m_pending_removals); }
}
