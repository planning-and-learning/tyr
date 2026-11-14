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

// The members of the datalog repository, which the library's other sources only declare
// (yggdrasil/formalism/repository.hpp). Compiled on its own, outside the unity batches (src/CMakeLists.txt).

#include "tyr/formalism/datalog/repository.hpp"

#include <yggdrasil/formalism/free_relations.hpp>
#include <yggdrasil/formalism/repository_instantiations.hpp>

YGG_FORMALISM_REPOSITORY(, tyr::formalism::datalog::Repository, 42, 6, 0)

// The repository of free relations (yggdrasil/formalism/free_relations.hpp), which the lifted Datalog database holds:
// yggdrasil has no library of its own to instantiate it in.
YGG_FORMALISM_REPOSITORY(, ygg::FreeRelationRepository, 2, 1, 1)
YGG_FORMALISM_REPOSITORY_SCHEMA(, ygg::FreeRelationRepository)
