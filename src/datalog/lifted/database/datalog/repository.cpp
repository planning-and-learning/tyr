/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/datalog/repository.hpp"

#include "tyr/datalog/lifted/database/datalog/detail/repository_instantiations.hpp"

namespace tyr::datalog::database::datalog
{
DatalogRepository::DatalogRepository() : m_index(s_next_index.fetch_add(1, std::memory_order_relaxed)) {}

DatalogRepository::~DatalogRepository() = default;
}

TYR_DATALOG_PROGRAM_INTERNS()
