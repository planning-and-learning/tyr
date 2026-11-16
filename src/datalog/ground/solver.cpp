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
#include "scheduler_impl.hpp"
#include "tyr/datalog/cost_buckets.hpp"
#include "tyr/datalog/ground/contexts/program.hpp"
#include "tyr/datalog/ground/rule_instance.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/rule_evaluation.hpp"

#include <concepts>
#include <ranges>
#include <utility>

namespace tyr::datalog
{

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
class SolverPolicy<GroundTag, AP, TP, CP>
{
public:
    explicit SolverPolicy(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx) : m_ctx(&ctx) {}

    // Ground programs currently reject negative fluent bodies, so every supported program has one stratum.
    bool next_stratum() noexcept { return std::exchange(m_pending_stratum, false); }

    /// Fires every ready rule, each once: its conditions are all committed, so it fires with their aggregated cost, its
    /// support. Its heads go to the cost buckets; none is committed here, so no rule becomes ready while they fire.
    bool generate_updates(CostBuckets& cost_buckets, PendingPredicateAchievers<GroundTag>& pending_achievers)
    {
        auto& ready = scheduler().ready();
        for (const auto rule : ready)
            fire(rule, cost_buckets, pending_achievers);
        ready.clear();
        return false;
    }

    Scheduler<GroundTag>& scheduler() noexcept { return m_ctx->out().scheduler(); }

private:
    void fire(formalism::datalog::RuleView<GroundTag> rule, CostBuckets& pending_heads, PendingPredicateAchievers<GroundTag>& pending_achievers)
    {
        auto& out = m_ctx->out();
        auto& scratch = scheduler().scratch();
        scratch.clear_updates();
        auto input = make_rule_update_input(RuleInstance<GroundTag>(rule),
                                            FactSets { m_ctx->in().facts().fact_sets, out.facts().fact_sets },
                                            out.annotations(),
                                            out.annotation_policy(),
                                            out.cost_policy());
        // The rule's heads as the keys of their rows, which key the predicate annotations.
        const auto heads = input.rule_instance.get_heads() | std::views::transform([&](auto head) { return input.rule_instance.resolve(head).index(); });
        const auto result =
            fire_propositional_update(heads, input, scratch.predicate_updates, scratch.delta_annotations, scheduler().support(rule));
        if (result.status == RuleUpdateStatus::Completed)
            ++scheduler().statistics().num_rules_fired;

        reduce_predicate_head_updates(scratch.predicate_updates,
                                      out.annotation_policy(),
                                      out.facts().fact_sets,
                                      scratch.delta_annotations,
                                      out.annotations(),
                                      pending_heads,
                                      pending_achievers);
    }

    ProgramExecutionContext<GroundTag, AP, TP, CP>* m_ctx;
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
compute_model(ProgramExecutionContext<GroundTag, MinCostAnnotationWithAchieversPolicy<GroundTag, MaxAggregation>, TerminationPolicy<MaxAggregation>, RuleCostPolicy>& ctx);
template void
compute_model(ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<SumAggregation>, NoTerminationPolicy, RuleCostOverridePolicy<GroundTag>>& ctx);
template void compute_model(
    ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<SumAggregation>, TerminationPolicy<SumAggregation>, RuleCostOverridePolicy<GroundTag>>& ctx);
template void
compute_model(ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<MaxAggregation>, NoTerminationPolicy, RuleCostOverridePolicy<GroundTag>>& ctx);
template void compute_model(
    ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<MaxAggregation>, TerminationPolicy<MaxAggregation>, RuleCostOverridePolicy<GroundTag>>& ctx);
template void compute_model(ProgramExecutionContext<GroundTag,
                                                    MinCostAnnotationWithAchieversPolicy<GroundTag, MaxAggregation>,
                                                    TerminationPolicy<MaxAggregation>,
                                                    RuleCostOverridePolicy<GroundTag>>& ctx);

template void execute_model(ProgramExecutionContext<GroundTag, NoAnnotationPolicy, NoTerminationPolicy, RuleCostPolicy>& ctx,
                            ygg::ExecutionContext& execution_context);
template void execute_model(ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<SumAggregation>, NoTerminationPolicy, RuleCostPolicy>& ctx,
                            ygg::ExecutionContext& execution_context);
template void execute_model(ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<SumAggregation>, TerminationPolicy<SumAggregation>, RuleCostPolicy>& ctx,
                            ygg::ExecutionContext& execution_context);
template void execute_model(ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<MaxAggregation>, NoTerminationPolicy, RuleCostPolicy>& ctx,
                            ygg::ExecutionContext& execution_context);
template void execute_model(ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<MaxAggregation>, TerminationPolicy<MaxAggregation>, RuleCostPolicy>& ctx,
                            ygg::ExecutionContext& execution_context);
template void
execute_model(ProgramExecutionContext<GroundTag, MinCostAnnotationWithAchieversPolicy<GroundTag, MaxAggregation>, TerminationPolicy<MaxAggregation>, RuleCostPolicy>& ctx,
              ygg::ExecutionContext& execution_context);
template void
execute_model(ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<SumAggregation>, NoTerminationPolicy, RuleCostOverridePolicy<GroundTag>>& ctx,
              ygg::ExecutionContext& execution_context);
template void execute_model(
    ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<SumAggregation>, TerminationPolicy<SumAggregation>, RuleCostOverridePolicy<GroundTag>>& ctx,
    ygg::ExecutionContext& execution_context);
template void
execute_model(ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<MaxAggregation>, NoTerminationPolicy, RuleCostOverridePolicy<GroundTag>>& ctx,
              ygg::ExecutionContext& execution_context);
template void execute_model(
    ProgramExecutionContext<GroundTag, MinCostAnnotationPolicy<MaxAggregation>, TerminationPolicy<MaxAggregation>, RuleCostOverridePolicy<GroundTag>>& ctx,
    ygg::ExecutionContext& execution_context);
template void execute_model(ProgramExecutionContext<GroundTag,
                                                    MinCostAnnotationWithAchieversPolicy<GroundTag, MaxAggregation>,
                                                    TerminationPolicy<MaxAggregation>,
                                                    RuleCostOverridePolicy<GroundTag>>& ctx,
                            ygg::ExecutionContext& execution_context);
}
