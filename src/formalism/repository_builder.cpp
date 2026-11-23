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

// The builder's construction and destruction, which touch the storages of all its types (src/formalism/repository.cpp).

#include "tyr/formalism/interning.hpp"
#include "tyr/formalism/repository.hpp"

#include <yggdrasil/formalism/builder_instantiations.hpp>
#include <yggdrasil/formalism/repository_impl.hpp>
#include <yggdrasil/formalism/repository_instantiations.hpp>

YGG_FORMALISM_BUILDER_MEMBERS(, tyr::formalism::Builder)
