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

#include "tyr/formalism/planning/invariants/synthesis.hpp"

#include "normalization.hpp"
#include "refinement.hpp"
#include "tyr/formalism/planning/expression_arity.hpp"
#include "tyr/formalism/planning/grounder.hpp"
#include "tyr/formalism/planning/interning.hpp"
#include "tyr/formalism/planning/mutable/action.hpp"
#include "tyr/formalism/planning/mutable/formatter.hpp"
#include "tyr/formalism/planning/mutable/invariant.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <algorithm>
#include <deque>
#include <gtl/phmap.hpp>
#include <map>
#include <optional>
#include <variant>
#include <vector>
#include <yggdrasil/semantics/comparators.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::formalism::planning::invariant
{
namespace
{
/// Fast Downward's default limit on generated candidates (--invariant-generation-max-candidates); counting candidates
/// keeps the limit deterministic, unlike a time limit.
constexpr size_t max_candidates = 100'000;

/// PDDL's add-after-delete semantics: an atom that one effect both deletes and adds under the same condition is added, so
/// the delete must not balance anything. Literals with the same condition share one conditional effect, and the delete is
/// dropped there, as Fast Downward's parser does.
MutableAction without_overridden_deletes(MutableAction action)
{
    for (auto& effect : action.effects)
    {
        auto& literals = effect.effect.literals;
        auto added = MutableAtomList<FluentTag> {};
        for (const auto& literal : literals)
            if (literal.polarity)
                added.push_back(literal.atom);
        std::erase_if(literals,
                      [&](const MutableLiteral<FluentTag>& literal) { return !literal.polarity && std::ranges::find(added, literal.atom) != added.end(); });
    }
    return action;
}

InvariantView intern(const MutableInvariant& invariant, Builder& builder, Repository& repository)
{
    auto data = checkout<Invariant>(builder);
    data->num_rigid_variables = invariant.num_rigid_variables;
    data->num_counted_variables = invariant.num_counted_variables;
    for (const auto& atom : invariant.atoms)
    {
        auto atom_data = checkout<Atom<LiftedTag, FluentTag>>(builder);
        atom_data->predicate = atom.predicate.get_index();
        for (const auto& term : atom.terms)
            atom_data->terms.push_back(term);
        data->atoms.push_back(insert(repository, *atom_data).first.get_index());
    }
    return insert(repository, *data).first;
}

MutableInvariantList synthesize_mutable_invariants(DomainView domain)
{
    auto ops = MutableActionList {};
    for (const auto& action : domain.get_actions())
        ops.push_back(without_overridden_deletes(MutableAction(action)));

    // Breadth-first search with a closed list (Helmert 2009, Section 5). A candidate is refined on the first unbalanced
    // add effect in domain order, so its successors depend only on the candidate, and the result is deterministic.
    auto queue = std::deque<MutableInvariant> {};
    auto seen = gtl::flat_hash_set<MutableInvariant, ygg::Hash<MutableInvariant>, ygg::EqualTo<MutableInvariant>> {};

    const auto enqueue = [&](MutableInvariant candidate)
    {
        candidate = MutableInvariant(candidate.num_rigid_variables, candidate.num_counted_variables, std::move(candidate.atoms));
        normalize_invariant(candidate);
        if (seen.size() >= max_candidates)
            return;
        if (seen.insert(candidate).second)
            queue.push_back(std::move(candidate));
    };

    for (auto& candidate : make_initial_candidates(domain.get_predicates<FluentTag>()))
        enqueue(std::move(candidate));

    auto accepted = MutableInvariantList {};

    while (!queue.empty())
    {
        auto candidate = std::move(queue.front());
        queue.pop_front();

        auto result = prove_invariant(candidate, ops);

        if (result.status == ProofStatus::Proven)
        {
            accepted.push_back(std::move(candidate));
        }
        else if (result.status == ProofStatus::UnbalancedAddEffect)
        {
            for (auto& refined : refine_candidate(candidate, *result.threat, ops, domain.get_predicates<FluentTag>()))
                enqueue(std::move(refined));
        }
    }

    return accepted;
}
}  // namespace

InvariantViewList synthesize_invariants(DomainView domain, Repository& repository)
{
    auto builder = Builder();
    auto result = InvariantViewList {};
    for (const auto& invariant : synthesize_mutable_invariants(domain))
        result.push_back(intern(invariant, builder, repository));
    return result;
}
}
