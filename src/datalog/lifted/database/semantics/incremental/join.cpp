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
    m_left_operand(m_plan.left_columns()),
    m_right_operand(m_plan.right_columns()),
    m_result(m_plan.output_columns()),
    m_delta(layer, m_plan.output_columns())
{
}
}

namespace tyr::datalog::database::incremental
{
ygg::FreeRelationBuilderView JoinEvaluator::get_result() const&
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
{
    return m_left_operand.memory_usage() + m_right_operand.memory_usage() + m_result.memory_usage() + m_delta.memory_usage() + urd::memory_usage(m_left_index)
           + urd::memory_usage(m_right_index);
}
}
