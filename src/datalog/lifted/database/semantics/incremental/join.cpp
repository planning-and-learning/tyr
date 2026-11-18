/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/semantics/incremental/join.hpp"

namespace tyr::datalog::database::incremental
{
JoinEvaluator::JoinEvaluator(ygg::FreeScratchRelations& scratch, JoinPlan plan) :
    m_plan(std::move(plan)),
    m_left_operand(scratch.draw(m_plan.left_columns())),
    m_right_operand(scratch.draw(m_plan.right_columns())),
    m_result(scratch.draw(m_plan.output_columns())),
    m_delta(scratch, m_plan.output_columns())
{
}
}

namespace tyr::datalog::database::incremental
{
ygg::FreeRelationBuilderView JoinEvaluator::get_result() const&
{
    detail::require_initialized(m_initialized);
    return m_result->view();
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
    return urd::memory_usage(m_left_operand->relation) + urd::memory_usage(m_right_operand->relation) + urd::memory_usage(m_result->relation)
           + m_delta.memory_usage() + urd::memory_usage(m_left_index) + urd::memory_usage(m_right_index);
}
}
