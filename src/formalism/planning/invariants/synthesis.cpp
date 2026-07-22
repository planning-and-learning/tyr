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
#include "tyr/formalism/planning/invariants/formatter.hpp"
#include "tyr/formalism/planning/invariants/invariant.hpp"
#include "tyr/formalism/planning/mutable/action.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <algorithm>
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
InvariantList synthesize_invariants(DomainView domain)
{
    auto ops = MutableActionList {};
    for (const auto& action : domain.get_actions())
        ops.emplace_back(action);

    auto queue = make_initial_candidates(domain.get_predicates<FluentTag>());

    auto accepted = InvariantList {};
    auto seen = gtl::flat_hash_set<Invariant, ygg::Hash<Invariant>, ygg::EqualTo<Invariant>> {};

    while (!queue.empty())
    {
        auto candidate = std::move(queue.back());
        queue.pop_back();

        candidate = Invariant(candidate.num_rigid_variables, candidate.num_counted_variables, std::move(candidate.atoms));

        normalize_invariant(candidate);

        if (!seen.insert(candidate).second)
            continue;

        auto result = prove_invariant(candidate, ops);

        if (result.status == ProofStatus::Proven)
        {
            accepted.push_back(std::move(candidate));
        }
        else if (result.status == ProofStatus::UnbalancedAddEffect)
        {
            auto refined = refine_candidate(candidate, *result.threat, ops, domain.get_predicates<FluentTag>());

            queue.insert(queue.end(), std::make_move_iterator(refined.begin()), std::make_move_iterator(refined.end()));
        }
    }

    return accepted;
}
}
