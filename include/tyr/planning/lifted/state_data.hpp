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

#ifndef TYR_PLANNING_LIFTED_STATE_DATA_HPP_
#define TYR_PLANNING_LIFTED_STATE_DATA_HPP_

#include "tyr/planning/state_storage/config.hpp"

#if defined(TYR_STATE_STORAGE_HASHSET)
#include "tyr/planning/lifted/state_storage/hash_set/atom.hpp"
#include "tyr/planning/lifted/state_storage/hash_set/fact.hpp"
#include "tyr/planning/state_storage/hash_set/numeric.hpp"
#elif defined(TYR_STATE_STORAGE_TREE)
#include "tyr/planning/lifted/state_storage/tree_compression/atom.hpp"
#include "tyr/planning/lifted/state_storage/tree_compression/fact.hpp"
#include "tyr/planning/state_storage/tree_compression/numeric.hpp"
#endif

#include "tyr/planning/state_data.hpp"

#endif
