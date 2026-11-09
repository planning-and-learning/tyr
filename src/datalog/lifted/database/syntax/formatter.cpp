/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/syntax/formatter.hpp"

namespace tyr::datalog::database
{
std::string explain(const QueryPlan& plan)
{
    fmt::memory_buffer out;
    detail::format_query_plan(std::back_inserter(out), plan);
    return fmt::to_string(out);
}
}
