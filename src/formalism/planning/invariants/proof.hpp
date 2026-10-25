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

#ifndef TYR_FORMALISM_PLANNING_INVARIANTS_PROOF_HPP_
#define TYR_FORMALISM_PLANNING_INVARIANTS_PROOF_HPP_

#include "tyr/formalism/planning/mutable/action.hpp"
#include "tyr/formalism/planning/mutable/atom.hpp"
#include "tyr/formalism/planning/mutable/conditional_effect.hpp"
#include "tyr/formalism/planning/mutable/invariant.hpp"

#include <cstdint>

namespace tyr::formalism::planning::invariant
{

bool is_operator_too_heavy(const MutableAction& op, const MutableInvariant& inv);

bool is_add_effect_unbalanced(const MutableAction& op,
                              const MutableConditionalEffect& effect,
                              const MutableAtom<FluentTag>& add_atom,
                              const MutableInvariant& inv);

struct Threat
{
    size_t op_index;
    size_t effect_index;
    size_t add_index;
};

enum class ProofStatus : std::uint8_t
{
    Proven,
    TooHeavy,
    UnbalancedAddEffect
};

struct ProofResult
{
    ProofStatus status = ProofStatus::Proven;
    std::optional<Threat> threat;
};

ProofResult prove_invariant(const MutableInvariant& inv, const MutableActionList& ops);
}

#endif