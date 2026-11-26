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

#include "tyr/declarations.hpp"
#include "tyr/formalism/atom_data.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/action_effects.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/invariant_builder.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <urd/elements/builder.hpp>
#include <urd/elements/data.hpp>
#include <vector>

namespace tyr::formalism::invariant
{

/// Whether one application of the action can add two atoms of one instance of the candidate.
bool is_operator_too_heavy(ActionView<LiftedTag> op, const urd::builder<Invariant, Repository>& inv);

/// Whether the add effect's atom can be added without some delete effect of the action deleting an atom of the same
/// instance of the candidate. The add effect is at `path` in the action; a delete balances it only if it happens whenever
/// the add does: its path is a prefix of the add's (nature picks one outcome per choice, a delete in another one may not
/// happen).
bool is_add_effect_unbalanced(ActionView<LiftedTag> op,
                              ConditionalEffectView<LiftedTag> effect,
                              OutcomePath path,
                              const urd::data<Atom<LiftedTag, FluentTag>>& add_atom,
                              const urd::builder<Invariant, Repository>& inv);

/// The add effect that threatens a candidate: its action (by position), its conditional effect (one of the action's or of an
/// outcome of its choices), and its literal (by position in the effect's literal list).
struct Threat
{
    std::size_t op_index;
    ConditionalEffectView<LiftedTag> effect;
    std::size_t add_index;
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

ProofResult prove_invariant(const urd::builder<Invariant, Repository>& inv, const std::vector<ActionView<LiftedTag>>& ops);

/// Whether a delete literal of the effect is overridden by an add literal of the same atom in it: PDDL's add-after-delete
/// semantics adds that atom, so the delete balances nothing (literals with the same condition share one conditional
/// effect, and Fast Downward's parser drops such a delete).
bool is_overridden_delete(ConditionalEffectView<LiftedTag> effect, LiteralView<LiftedTag, FluentTag> literal);

}

#endif
