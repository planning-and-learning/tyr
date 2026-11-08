/*
 * Copyright (C) 2025-2026 Dominik Drexler
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */


#ifndef TYR_FORMALISM_DETAIL_REPOSITORY_INSTANTIATIONS_HPP_
#define TYR_FORMALISM_DETAIL_REPOSITORY_INSTANTIATIONS_HPP_

/// Extern declarations of intern for every symbol type of the datalog and planning repositories, instantiated in the
/// library (src/formalism/repository.cpp). Include only from sources that intern many of these types; see yggdrasil's
/// docs/TEMPLATE_RULES.md, rule 9.

#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"

#include <yggdrasil/formalism/intern_instantiations.hpp>

/// The instantiations (PREFIX extern: declarations); the two repositories share 34 types.
#define TYR_FORMALISM_INTERNS(PREFIX) \
    YGG_FORMALISM_INTERNS(PREFIX,                                                                                     \
                          67,                                                                                         \
                          true,                                                                                       \
                          boost::mp11::mp_unique<boost::mp11::mp_append<tyr::formalism::datalog::SymbolRepositoryTypes, \
                                                                        tyr::formalism::planning::SymbolRepositoryTypes>>)

TYR_FORMALISM_INTERNS(extern)

#endif
