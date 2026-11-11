/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_DETAIL_REPOSITORY_INSTANTIATIONS_HPP_
#define TYR_DATALOG_DATABASE_DETAIL_REPOSITORY_INSTANTIATIONS_HPP_

/// Extern declarations of intern for every symbol type of the database's repositories (queries, relations),
/// instantiated in the library (syntax/query_repository.cpp). Include only from sources that intern many of these
/// types; see docs/TEMPLATE_RULES.md, rule 9.

#include "tyr/datalog/lifted/database/semantics/relation_view.hpp"
#include "tyr/datalog/lifted/database/syntax/query_repository.hpp"
#include "yggdrasil/formalism/intern_instantiations.hpp"

/// The instantiations (PREFIX extern: declarations).
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage): it expands to explicit instantiations, declarations, which no function gives
#define TYR_DATABASE_INTERNS(PREFIX) \
    YGG_FORMALISM_INTERNS(PREFIX, 16, false, boost::mp11::mp_append<tyr::datalog::database::QuerySymbolTypes, ygg::FreeRelationRepository::SymbolTypes>)

TYR_DATABASE_INTERNS(extern)

#endif
