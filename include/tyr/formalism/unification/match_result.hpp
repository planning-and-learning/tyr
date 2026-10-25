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

#ifndef TYR_FORMALISM_UNIFICATION_MATCH_RESULT_HPP_
#define TYR_FORMALISM_UNIFICATION_MATCH_RESULT_HPP_

#include <cstdint>
#include <optional>

namespace tyr::formalism::unification
{

enum class MatchFailure : std::uint8_t
{
    none,
    structure_mismatch,
};

template<typename State>
struct MatchResult
{
    std::optional<State> state;
    MatchFailure failure = MatchFailure::none;

    [[nodiscard]] bool has_value() const noexcept { return state.has_value(); }

    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }

    [[nodiscard]] const State& value() const { return *state; }

    [[nodiscard]] State& value() { return *state; }
};

}

#endif