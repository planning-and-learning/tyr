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

#ifndef TYR_PLANNING_STATE_STORAGE_POLICY_HPP_
#define TYR_PLANNING_STATE_STORAGE_POLICY_HPP_

// The pools of the state storage policy the build chooses (TYR_STATE_STORAGE_POLICY, its header
// TYR_STATE_STORAGE_POLICY_HEADER), and only those: a state's storage keys are that policy's handles
// (urd::key_value_of, state_storage.hpp).
#include "tyr/planning/state_storage/numeric.hpp"

#include TYR_STATE_STORAGE_POLICY_HEADER

#endif
