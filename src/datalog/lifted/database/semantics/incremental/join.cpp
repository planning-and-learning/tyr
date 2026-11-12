/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/semantics/incremental/join.hpp"

namespace tyr::datalog::database::incremental
{
JoinEvaluator::JoinEvaluator(const ygg::FreeRelationRepository& layer, JoinPlan plan) :
    m_layer(&layer),
    m_plan(std::move(plan)),
    m_lhs(m_plan.lhs_columns()),
    m_rhs(m_plan.rhs_columns()),
    m_result(m_plan.output_columns()),
    m_delta(layer, m_plan.output_columns())
{
    m_lhs_index = detail::key_index(m_lhs, m_plan.lhs_keys());
    m_rhs_index = detail::key_index(m_rhs, m_plan.rhs_keys());
}
}

namespace tyr::datalog::database::incremental
{
ygg::FreeBuilderRelationView JoinEvaluator::get_result() const&
{
    detail::require_initialized(m_initialized);
    return m_layer->view(m_result);
}
}

namespace tyr::datalog::database::incremental
{
const Delta& JoinEvaluator::get_delta() const&
{
    detail::require_initialized(m_initialized);
    return m_delta;
}
}

namespace tyr::datalog::database::incremental
{
size_t JoinEvaluator::memory_usage() const noexcept
{ return m_lhs.memory_usage() + m_rhs.memory_usage() + m_result.memory_usage() + m_delta.memory_usage() + urd::memory_usage(m_key); }
}
