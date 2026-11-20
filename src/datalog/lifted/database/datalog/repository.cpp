/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/datalog/repository.hpp"

#include "tyr/datalog/lifted/database/datalog/builder.hpp"
#include "tyr/datalog/lifted/database/datalog/construction.hpp"
#include "tyr/datalog/lifted/database/datalog/detail/repository_instantiations.hpp"

#include <yggdrasil/formalism/builder_instantiations.hpp>
#include <yggdrasil/formalism/symbol_context_instantiations.hpp>

namespace tyr::datalog::database::datalog
{
// Numbered as yggdrasil's layers are, from the one process-wide counter, so that no two contexts share a number.
DatalogRepository::DatalogRepository() : m_index(ygg::next_layer()) {}

DatalogRepository::~DatalogRepository() = default;
}

TYR_DATALOG_PROGRAM_INTERNS()

// The repository's symbols, which the library's other sources only declare (yggdrasil/formalism/symbol_context.hpp).
YGG_FORMALISM_SYMBOL_CONTEXT(, tyr::datalog::database::datalog::DatalogRepository, 5)
YGG_FORMALISM_BUILDER(, tyr::datalog::database::datalog::DatalogBuilder, 5)
