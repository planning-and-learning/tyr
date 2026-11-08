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


#ifndef TYR_PLANNING_GROUND_MATCH_TREE_DETAIL_REPOSITORY_INSTANTIATIONS_HPP_
#define TYR_PLANNING_GROUND_MATCH_TREE_DETAIL_REPOSITORY_INSTANTIATIONS_HPP_

/// Extern declarations of intern for every symbol type of the action and axiom match tree repositories, instantiated
/// in the library (src/planning/ground/match_tree.cpp). See yggdrasil's docs/TEMPLATE_RULES.md, rule 9.

#include "tyr/planning/ground/match_tree/declarations.hpp"

#include <yggdrasil/formalism/intern_instantiations.hpp>

/// The instantiations (PREFIX extern: declarations).
#define TYR_MATCH_TREE_INTERNS(PREFIX)                                                                                          \
    YGG_FORMALISM_INTERNS(PREFIX,                                                                                               \
                          10,                                                                                                   \
                          false,                                                                                                \
                          boost::mp11::mp_unique<boost::mp11::mp_append<                                                        \
                              tyr::planning::match_tree::RepositoryTypes<tyr::formalism::planning::Action<tyr::GroundTag>>,     \
                              tyr::planning::match_tree::RepositoryTypes<tyr::formalism::planning::Axiom<tyr::GroundTag>>>>)

TYR_MATCH_TREE_INTERNS(extern)

#endif
