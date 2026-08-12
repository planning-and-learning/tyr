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

#include "tyr/datalog/solver.hpp"

#include "../solver_impl.hpp"
#include "scheduler.cpp"
#include "tyr/datalog/cost_buckets.hpp"
#include "tyr/datalog/ground/contexts/program.hpp"
#include "tyr/datalog/ground/rule_instance.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/rule_evaluation.hpp"

#include <cassert>
#include <concepts>
#include <limits>
#include <utility>

namespace tyr::datalog
{
namespace f = ::tyr::formalism;

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
class SolverPolicy<GroundTag, AP, TP, CP>
{
public:
    explicit SolverPolicy(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx) : m_ctx(ctx) {}

    // Ground programs currently reject negative fluent bodies, so every supported program has one stratum.
    bool next_stratum() noexcept { return std::exchange(m_pending_stratum, false); }

    bool generate_updates(CostBuckets& cost_buckets, PendingPredicateAchievers& pending_achievers)
    {
        while (scheduler().next_cost() != std::numeric_limits<Cost>::max())
            process_rule_frontier(cost_buckets, pending_achievers, scheduler().next_cost());
        return false;
    }

    Scheduler<GroundTag>& scheduler() noexcept { return m_ctx.out().scheduler(); }

private:
    template<f::RelationKind R>
    void process_next_rule(CostBuckets& pending_heads, PendingPredicateAchievers& pending_achievers)
    {
        auto& out = m_ctx.out();
        auto entry = scheduler().template pop<R>();
        assert(entry);

        if (!scheduler().claim(*entry))
            return;

        auto& scratch = scheduler().scratch();
        scratch.clear_updates();
        auto selector = out.numeric_support_selector();
        auto input = make_rule_update_input(RuleInstance<GroundTag, R>(entry->rule),
                                            selector,
                                            out.annotations(),
                                            scratch.rule_evaluation,
                                            out.numeric_annotations(),
                                            out.annotation_policy(),
                                            out.cost_policy());
        const auto result = [&]
        {
            if constexpr (std::same_as<R, f::PredicateTag>)
            {
                const auto head = input.rule_instance.resolve(input.rule_instance.get_head());
                return insert_propositional_update(head, input, scratch.predicate_updates, scratch.delta_annotations, entry->cost);
            }
            else
            {
                return insert_numeric_update(input, scratch.function_updates, scratch.delta_numeric_annotations, entry->cost);
            }
        }();
        if (result.status == RuleUpdateStatus::Unavailable || result.status == RuleUpdateStatus::QueueLabelChanged)
        {
            ++scheduler().statistics().num_stale_queue_pops;
            if (result.queue_label)
                scheduler().enqueue(entry->rule, *result.queue_label);
            return;
        }

        if (result.status == RuleUpdateStatus::Completed)
            ++scheduler().statistics().num_rules_fired;

        if constexpr (std::same_as<R, f::PredicateTag>)
            reduce_predicate_head_updates(scratch.predicate_updates,
                                          out.annotation_policy(),
                                          out.facts().fact_sets.predicate,
                                          scratch.delta_annotations,
                                          out.annotations(),
                                          pending_heads,
                                          pending_achievers);
        else
            reduce_function_head_updates(scratch.function_updates,
                                         out.annotation_policy(),
                                         scratch.delta_numeric_annotations,
                                         out.numeric_annotations(),
                                         pending_heads,
                                         scheduler(),
                                         m_ctx);
    }

    void process_rule_frontier(CostBuckets& pending_heads, PendingPredicateAchievers& pending_achievers, Cost cost)
    {
        while (scheduler().next_cost() == cost)
        {
            if (scheduler().template next_cost<f::PredicateTag>() <= scheduler().template next_cost<f::FunctionTag>())
                process_next_rule<f::PredicateTag>(pending_heads, pending_achievers);
            else
                process_next_rule<f::FunctionTag>(pending_heads, pending_achievers);
        }
    }

    ProgramExecutionContext<GroundTag, AP, TP, CP>& m_ctx;
    bool m_pending_stratum = true;
};

template void compute_model(ProgramExecutionContext<GroundTag, NoAnnotationPolicy, NoTerminationPolicy, RuleCostPolicy>& ctx);
template void compute_model(ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<SumAggregation>, NoTerminationPolicy, RuleCostPolicy>& ctx);
template void
compute_model(ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<SumAggregation>, TerminationPolicy<SumAggregation>, RuleCostPolicy>& ctx);
template void compute_model(ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<MaxAggregation>, NoTerminationPolicy, RuleCostPolicy>& ctx);
template void
compute_model(ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<MaxAggregation>, TerminationPolicy<MaxAggregation>, RuleCostPolicy>& ctx);
template void
compute_model(ProgramExecutionContext<GroundTag, MinCostAnnotationWithAchieversPolicy<MaxAggregation>, TerminationPolicy<MaxAggregation>, RuleCostPolicy>& ctx);
template void
compute_model(ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<SumAggregation>, NoTerminationPolicy, RuleCostOverridePolicy<GroundTag>>& ctx);
template void compute_model(
    ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<SumAggregation>, TerminationPolicy<SumAggregation>, RuleCostOverridePolicy<GroundTag>>& ctx);
template void
compute_model(ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<MaxAggregation>, NoTerminationPolicy, RuleCostOverridePolicy<GroundTag>>& ctx);
template void compute_model(
    ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<MaxAggregation>, TerminationPolicy<MaxAggregation>, RuleCostOverridePolicy<GroundTag>>& ctx);
template void compute_model(ProgramExecutionContext<GroundTag,
                                                    MinCostAnnotationWithAchieversPolicy<MaxAggregation>,
                                                    TerminationPolicy<MaxAggregation>,
                                                    RuleCostOverridePolicy<GroundTag>>& ctx);
}
