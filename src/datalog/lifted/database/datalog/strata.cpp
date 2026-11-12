/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/datalog/detail/strata.hpp"

#include "tyr/datalog/lifted/database/datalog/detail/program.hpp"

#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::datalog::database::datalog::detail
{
urd::Store<urd::sequence<Version>>& Stratum::reads(const CompiledRule& rule, Version all)
{
    m_reads.assign(rule.literals.size(), all);
    return m_reads;
}
}  // namespace tyr::datalog::database::datalog::detail
