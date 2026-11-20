/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_DATALOG_DETAIL_REPOSITORY_INSTANTIATIONS_HPP_
#define TYR_DATALOG_DATABASE_DATALOG_DETAIL_REPOSITORY_INSTANTIATIONS_HPP_

/// The interning of every Datalog record type, instantiated in the library (datalog/repository.cpp). Include only from
/// sources that intern many of these types; see rule 9 of the template rules
/// (https://planning-and-learning.github.io/brokkr/template_rules.html).

#include "tyr/datalog/lifted/database/datalog/repository.hpp"
#include "yggdrasil/formalism/intern_instantiations.hpp"

/// The instantiations (PREFIX extern: declarations).
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage): it expands to explicit instantiations, declarations, which no function gives
#define TYR_DATALOG_PROGRAM_INTERNS(PREFIX) YGG_FORMALISM_INTERNS(PREFIX, 5, false, tyr::datalog::database::datalog::DatalogSymbolTypes)

TYR_DATALOG_PROGRAM_INTERNS(extern)

#endif
