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

#ifndef TYR_FORMALISM_UNIFICATION_MATCH_STATE_HPP_
#define TYR_FORMALISM_UNIFICATION_MATCH_STATE_HPP_

#include "tyr/formalism/unification/substitution.hpp"

namespace tyr::formalism::unification
{

template<typename T>
struct MatchState
{
    SubstitutionFunction<T> sigma;
    SubstitutionFunction<T> counted;
};

using TermMatchState = MatchState<Term>;

}

#endif