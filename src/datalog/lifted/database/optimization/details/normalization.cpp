/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/optimization/details/normalization.hpp"

namespace tyr::datalog::database::detail
{
QueryView<> Normalizer::project(QueryView<> arg, std::span<const urd::index<ygg::Column>> columns)
{
    if (std::ranges::equal(columns_of(arg), columns, {}, &ygg::ColumnLayout::label))
        return arg;
    if (is<QueryProjectTag>(arg))
        return project(as<QueryProjectTag>(arg).get_operand(), columns);
    if (is<QueryUnionTag>(arg))
    {
        const auto operation = as<QueryUnionTag>(arg);
        return union_(project(operation.get_left_operand(), columns), project(operation.get_right_operand(), columns));
    }
    if (is<QueryRenameTag>(arg))
    {
        const auto operation = as<QueryRenameTag>(arg);
        const auto inner = inverse(operation, columns);
        return rename(project(operation.get_operand(), inner), columns);
    }
    // Into the left operand of a semijoin or antijoin, keeping the labels it shares with the right operand.
    if (is<QuerySemijoinTag>(arg))
        return project_filter_join<QuerySemijoinTag>(arg, columns);
    if (is<QueryAntijoinTag>(arg))
        return project_filter_join<QueryAntijoinTag>(arg, columns);
    return finish_project(arg, columns);
}
}
