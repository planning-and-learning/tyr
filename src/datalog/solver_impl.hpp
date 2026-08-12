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

#ifndef TYR_SRC_DATALOG_SOLVER_IMPL_HPP_
#define TYR_SRC_DATALOG_SOLVER_IMPL_HPP_

#include "tyr/datalog/cost_buckets.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/policies/annotation_concept.hpp"
#include "tyr/datalog/policies/annotation_types.hpp"
#include "tyr/datalog/policies/cost_concept.hpp"
#include "tyr/datalog/policies/termination_concept.hpp"
#include "tyr/datalog/solver.hpp"

namespace tyr::datalog
{

enum class SolverIterationTrigger
{
    AnnotationImproved,
    FactChanged
};

template<TaskKind Kind, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
class SolverPolicy;

template<TaskKind Kind, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
bool commit_head_bucket(ProgramExecutionContext<Kind, AP, TP, CP>& ctx, Scheduler<Kind>& scheduler, CostBuckets& cost_buckets, Cost cost)
{
    auto bucket = cost_buckets.take(cost);
    gtl::erase_if(bucket.predicate, [&](const auto fact) { return !ctx.out().facts().insert(fact); });
    gtl::erase_if(bucket.function, [&](const auto& entry) { return !ctx.out().facts().insert(entry.first, entry.second); });

    // Install the entire bucket before notifying rules. The scheduler controls the representation-specific
    // notification order.
    scheduler.on_generate(bucket, ctx);
    return !bucket.empty();
}

template<TaskKind Kind, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void compute_model_impl(ProgramExecutionContext<Kind, AP, TP, CP>& ctx)
{
    auto policy = SolverPolicy<Kind, AP, TP, CP>(ctx);
    while (policy.next_stratum())
    {
        auto cost_buckets = CostBuckets {};
        auto pending_achievers = PendingPredicateAchievers {};
        policy.begin_stratum();

        while (true)
        {
            if (ctx.out().tp().should_terminate(FactSets { ctx.in().facts().fact_sets, ctx.out().facts().fact_sets }))
                return;

            policy.begin_iteration();
            if (policy.generate_updates(cost_buckets, pending_achievers))
            {
                policy.finish_iteration(SolverIterationTrigger::AnnotationImproved);
                continue;
            }

            auto fact_changed = false;
            while (!cost_buckets.is_empty() && !fact_changed)
            {
                const auto cost = cost_buckets.min_cost();
                fact_changed = commit_head_bucket(ctx, policy.scheduler(), cost_buckets, cost);
                publish_pending_achievers(pending_achievers, pending_achievers.buckets.upper_bound(cost), ctx.out().annotation_policy());
            }

            if (!fact_changed)
            {
                publish_pending_achievers(pending_achievers, pending_achievers.buckets.end(), ctx.out().annotation_policy());
                break;
            }
            policy.finish_iteration(SolverIterationTrigger::FactChanged);
        }
    }
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void compute_model(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx)
{
    compute_model_impl(ctx);
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void compute_model(ProgramExecutionContext<LiftedTag, AP, TP, CP>& ctx)
{
    compute_model_impl(ctx);
}

}

#endif
