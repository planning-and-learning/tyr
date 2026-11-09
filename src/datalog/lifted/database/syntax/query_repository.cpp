/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/syntax/query_repository.hpp"

#include "tyr/datalog/lifted/database/detail/repository_instantiations.hpp"

namespace tyr::datalog::database
{
QueryRepository::QueryRepository() : m_index(s_next_index.fetch_add(1, std::memory_order_relaxed)) {}

QueryRepository::~QueryRepository() = default;
}

TYR_DATABASE_INTERNS()
