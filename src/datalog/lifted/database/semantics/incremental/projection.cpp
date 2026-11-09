/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/semantics/incremental/projection.hpp"

namespace tyr::datalog::database::incremental
{
ygg::RowPool::row_type ProjectionEvaluator::project_row(const ygg::RowPool::row_type& input, Workspace& workspace) const
{
    auto& row = workspace.row;
    row.reset(m_result.widths());
    database::detail::copy_fields(row, 0, input, m_plan.positions());
    return row.row();
}
}

namespace tyr::datalog::database::incremental
{
bool ProjectionEvaluator::add(const ygg::RowPool::row_type& row)
{
    const auto index = m_result.insert(row);
    if (index == m_supports.size())
    {
        m_supports.push_back(1);
        return true;
    }
    auto& count = m_supports[index];
    if (count == std::numeric_limits<size_t>::max())
        throw std::overflow_error("Incremental projection: support count overflow.");
    ++count;
    return false;
}
}

namespace tyr::datalog::database::incremental
{
void ProjectionEvaluator::remove(const ygg::RowPool::row_type& row)
{
    const auto index = m_result.find(row);
    if (!index || m_supports[*index] == 0)
        throw std::invalid_argument("Incremental projection: removal has no supporting row.");
    if (--m_supports[*index] == 0)
        m_pending_removals.push_back(*index);
}
}

namespace tyr::datalog::database::incremental
{
ProjectionEvaluator::ProjectionEvaluator(const ygg::FreeRelationRepository& layer, ProjectionPlan plan) :
    m_layer(&layer),
    m_plan(std::move(plan)),
    m_result(m_plan.output_columns()),
    m_delta(layer, m_plan.output_columns())
{
}
}

namespace tyr::datalog::database::incremental
{
ygg::FreeBuilderRelationView ProjectionEvaluator::get_result() const&
{
    detail::require_initialized(m_initialized);
    return m_layer->view(m_result);
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
{ return m_result.memory_usage() + m_delta.memory_usage() + m_supports.capacity() * sizeof(size_t) + m_pending_removals.capacity() * sizeof(ygg::uint_t); }
}
