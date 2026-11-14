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

// The members of the planning repository, which the library's other sources only declare
// (yggdrasil/formalism/repository.hpp). Compiled on its own, outside the unity batches (src/CMakeLists.txt).

#include "tyr/formalism/planning/repository.hpp"

#include <yggdrasil/formalism/repository_instantiations.hpp>

YGG_FORMALISM_REPOSITORY(, tyr::formalism::planning::Repository, 57, 8, 0)
