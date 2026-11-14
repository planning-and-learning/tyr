/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/syntax/query_repository.hpp"

#include "tyr/datalog/lifted/database/detail/repository_instantiations.hpp"

#include <yggdrasil/formalism/builder_instantiations.hpp>
#include <yggdrasil/formalism/symbol_context_instantiations.hpp>

namespace tyr::datalog::database
{
// Numbered as yggdrasil's layers are, from the one process-wide counter, so that no two contexts share a number.
QueryRepository::QueryRepository() : m_index(ygg::next_layer()) {}

QueryRepository::~QueryRepository() = default;
}

TYR_DATABASE_INTERNS()

// The repository's symbols, which the library's other sources only declare (yggdrasil/formalism/symbol_context.hpp).
YGG_FORMALISM_SYMBOL_CONTEXT(, tyr::datalog::database::QueryRepository, 14)
YGG_FORMALISM_BUILDER(, tyr::datalog::database::QueryBuilder, 14)
