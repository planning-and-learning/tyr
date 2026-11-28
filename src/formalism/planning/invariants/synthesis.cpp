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
#include "tyr/formalism/planning/grounder.hpp"
#include "tyr/formalism/planning/invariant_builder.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/repository.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cstddef>
#include <fmt/format.h>
#include <map>
#include <optional>
#include <urd/config.hpp>
#include <urd/elements/equal_to.hpp>
#include <urd/elements/hash.hpp>
#include <urd/elements/less.hpp>
#include <urd/repr/queues/queue.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <variant>
#include <vector>

namespace tyr::formalism::invariant
{
namespace
{
/// Fast Downward's default limit on generated candidates (--invariant-generation-max-candidates); counting candidates
/// keeps the limit deterministic, unlike a time limit.
constexpr size_t max_candidates = 100'000;

/// The candidate interned (its held atoms first): its atoms' parameters its rigid, then its counted variables, named ?v0,
/// ?v1, ... in that order.
InvariantView intern(urd::builder<Invariant, Repository>& invariant, Builder& builder, Repository& repository)
{
    auto parameters = checkout<Parameters>(builder);
    for (std::size_t position = 0; position < invariant.num_rigid_variables + invariant.num_counted_variables; ++position)
    {
        auto variable = checkout<Variable>(builder);
        variable->name = fmt::format("?v{}", position);
        urd::append(parameters->variables, repository.insert(*variable, builder).first);
    }
    const auto parameters_index = repository.insert(*parameters, builder).first.index();
    for (auto& part : invariant.atoms)
        atom_of(part).parameters = parameters_index;
    return repository.insert(invariant, builder).first;
}

/// The invariants of the domain, as candidates held in `storage`.
std::vector<urd::builder<Invariant, Repository>> synthesize_candidates(DomainView domain, CandidateStorage& storage)
{
    auto ops = std::vector<ActionView<LiftedTag>>();
    for (const auto action : domain.actions())
        ops.push_back(action);

    // Breadth-first search with a closed list (Helmert 2009, Section 5). A candidate is refined on the first unbalanced
    // add effect in domain order, so its successors depend only on the candidate, and the result is deterministic.
    auto queue = urd::Store<urd::queue<urd::builder<Invariant, Repository>>, urd::fifo> {};
    auto seen = urd::Store<urd::set<urd::builder<Invariant, Repository>>> {};

    const auto enqueue = [&](urd::builder<Invariant, Repository> candidate)
    {
        normalize_invariant(candidate);
        if (seen.size() >= max_candidates)
            return;
        if (seen.insert(urd::clone(candidate)).second)
            queue.push(std::move(candidate));
    };

    for (auto& candidate : make_initial_candidates(domain.predicates<FluentTag>(), storage))
        enqueue(std::move(candidate));

    auto accepted = std::vector<urd::builder<Invariant, Repository>>();

    while (!queue.empty())
    {
        auto candidate = std::move(queue.front());
        queue.pop();

        auto result = prove_invariant(candidate, ops);

        if (result.status == ProofStatus::Proven)
        {
            accepted.push_back(std::move(candidate));
        }
        else if (result.status == ProofStatus::UnbalancedAddEffect)
        {
            // prove_invariant names the threatening add effect of an unbalanced candidate.
            URD_ASSUME(result.threat.has_value());
            for (auto& refined : refine_candidate(candidate, *result.threat, ops, storage))
                enqueue(std::move(refined));
        }
    }

    return accepted;
}
}  // namespace

InvariantViewList synthesize_invariants(DomainView domain, Repository& repository)
{
    auto storage = CandidateStorage { .repository = &repository, .atoms = {} };
    auto builder = Builder();
    auto result = InvariantViewList {};
    for (auto& invariant : synthesize_candidates(domain, storage))
        result.push_back(intern(invariant, builder, repository));
    return result;
}
}
