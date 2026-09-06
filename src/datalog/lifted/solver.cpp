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
#include "tyr/algorithms/kckp/delta_kckp.hpp"
#include "tyr/datalog/applicability.hpp"
#include "tyr/datalog/applicability_lifted.hpp"
#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/lifted/assignment_sets.hpp"
#include "tyr/datalog/lifted/consistency_graph.hpp"
#include "tyr/datalog/lifted/contexts/program.hpp"
#include "tyr/datalog/lifted/contexts/rule.hpp"
#include "tyr/datalog/lifted/contexts/stratum.hpp"
#include "tyr/datalog/lifted/rule_instance.hpp"
#include "tyr/datalog/lifted/scheduler.hpp"
#include "tyr/datalog/lifted/workspaces/facts.hpp"
#include "tyr/datalog/lifted/workspaces/program.hpp"
#include "tyr/datalog/lifted/workspaces/rule.hpp"
#include "tyr/datalog/policies/aggregation.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/datalog/rule_evaluation.hpp"
#include "tyr/declarations.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/grounder.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"

#include <assert.h>
#include <concepts>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/parallel_for_each.h>
#include <optional>
#include <span>
#include <utility>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/core/chrono.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/core/types.hpp>

namespace tyr::datalog
{
namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
class SolverPolicy<LiftedTag, AP, TP, CP>
{
public:
    explicit SolverPolicy(ProgramExecutionContext<LiftedTag, AP, TP, CP>& ctx) : m_ctx(ctx), m_program_stopwatch(ctx.out().statistics().total_time)
    {
        ++ctx.out().statistics().num_executions;
    }

    bool next_stratum()
    {
        auto& schedulers = m_ctx.out().schedulers();
        if (m_next_stratum == schedulers.size())
            return false;
        m_stratum.emplace(schedulers[m_next_stratum++], m_ctx);
        return true;
    }

    bool generate_updates(CostBuckets& cost_buckets, PendingPredicateAchievers& pending_achievers)
    {
        auto& ctx = stratum();
        run_active_rules<f::PredicateTag>(ctx);
        run_active_rules<f::FunctionTag>(ctx);
        reduce_worker_results<f::PredicateTag>(ctx, cost_buckets, pending_achievers);
        return reduce_worker_results<f::FunctionTag>(ctx, cost_buckets, pending_achievers);
    }

    Scheduler<LiftedTag>& scheduler() { return stratum().out().scheduler(); }

private:
    static void create_nullary_binding(ygg::IndexList<f::Object>& binding) { binding.clear(); }

    static void
    create_general_binding(std::span<const kckp::Vertex> clique, const StaticConsistencyGraph& consistency_graph, ygg::IndexList<f::Object>& binding)
    {
        const auto k = clique.size();
        const auto& layout = consistency_graph.get_graph_layout();

        binding.resize(k);

        for (ygg::uint_t p = 0; p < k; ++p)
        {
            const auto vertex = clique[p].index;
            assert(layout.vertex_to_partition[vertex] == p);
            binding[p] = ygg::Index<f::Object>(layout.vertex_labels[vertex]);
        }
    }

    void insert_nullary_update(fd::AtomView<LiftedTag, f::FluentTag> head_atom,
                               RuleUpdateInput<LiftedTag, f::PredicateTag, AP, CP>& input,
                               RuleWorkerExecutionContext<f::PredicateTag, AP, TP, CP>& ctx)
    {
        auto& out = ctx.out();
        const auto head = fd::ground_binding(head_atom, out.ground_context()).first;
        insert_propositional_update(head, input, out.head_updates(), out.delta_annotations());
    }

    void insert_nullary_update(fd::NumericEffectOperatorView<LiftedTag, f::FluentTag>,
                               RuleUpdateInput<LiftedTag, f::FunctionTag, AP, CP>& input,
                               RuleWorkerExecutionContext<f::FunctionTag, AP, TP, CP>& ctx)
    {
        [[maybe_unused]] const auto& in = ctx.in();
        auto& out = ctx.out();
        assert(is_applicable(in.cws_rule().get_rule(), ApplicabilityContext { in.fact_sets(), out.ground_context() }));
        insert_numeric_update(input, out.head_updates(), out.delta_numeric_annotations());
    }

    template<f::RelationKind R>
    void generate_nullary_case(RuleExecutionContext<R, AP, TP, CP>& rctx)
    {
        auto wrctx = rctx.get_rule_worker_execution_context();

        auto& in = wrctx.in();
        auto& out = wrctx.out();
        auto input = make_rule_update_input(RuleInstance<LiftedTag, R> { in.cws_rule(), out.ground_context() },
                                            in.numeric_support_selector(),
                                            in.annotations(),
                                            out.rule_evaluation_workspace(),
                                            in.numeric_annotations(),
                                            in.annotation_policy(),
                                            in.cost_policy());
        ++out.statistics().num_executions;
        ++out.statistics().num_generated_rules;

        create_nullary_binding(out.ground_context().binding);

        const auto nullary_condition = in.cws_rule().get_nullary_condition();
        const auto conflicting_condition = in.cws_rule().get_rule().get_body();
        const auto applicability_context = ApplicabilityContext { in.fact_sets(), out.ground_context() };
        auto& applicability_cache = out.applicability_cache();

        const auto statically_applicable = [&]()
        { return applicability_cache.static_nullary && is_applicable(conflicting_condition.template get_literals<f::StaticTag>(), applicability_context); };
        const auto dynamically_applicable = [&]()
        {
            if (!applicability_cache.dynamic_nullary)
                applicability_cache.dynamic_nullary = is_dynamically_applicable(nullary_condition, in.fact_sets());
            return applicability_cache.dynamic_nullary && is_applicable(conflicting_condition, applicability_context);
        };

        // Note: we never go through the consistency graph, and hence, have to check validity on the entire rule body.
        if (statically_applicable() && dynamically_applicable())
            insert_nullary_update(in.cws_rule().get_rule().get_head(), input, wrctx);
    }

    [[maybe_unused]] static bool ensure_novel_binding(const ygg::IndexList<f::Object>& binding, ygg::UnorderedSet<ygg::IndexList<f::Object>>& set)
    {
        return set.insert(binding).second;
    }

    template<typename Callback>
    void for_each_relevant_clique(fd::AtomView<LiftedTag, f::FluentTag>,
                                  kckp::DeltaKCKP& algorithm,
                                  Callback&& callback,
                                  kckp::Workspace& workspace,
                                  [[maybe_unused]] bool enumerate_all)
    {
#ifdef TYR_ENABLE_SEMI_NAIVE
        if (!enumerate_all)
        {
            algorithm.for_each_delta_clique(std::forward<Callback>(callback), workspace);
            return;
        }
#endif
        algorithm.for_each_clique(std::forward<Callback>(callback), workspace);
    }

    template<typename Callback>
    void
    for_each_relevant_clique(fd::NumericEffectOperatorView<LiftedTag, f::FluentTag>, kckp::DeltaKCKP& algorithm, Callback&& callback, kckp::Workspace& workspace, bool)
    {
        algorithm.for_each_clique(std::forward<Callback>(callback), workspace);
    }

    inline bool require_novel_binding(fd::AtomView<LiftedTag, f::FluentTag>) noexcept
    {
#ifdef TYR_ENABLE_SEMI_NAIVE
        return true;
#else
        return false;
#endif
    }

    inline bool require_novel_binding(fd::NumericEffectOperatorView<LiftedTag, f::FluentTag>) noexcept { return false; }

    bool try_generate_parallel(fd::AtomView<LiftedTag, f::FluentTag>, [[maybe_unused]] RuleExecutionContext<f::PredicateTag, AP, TP, CP>& rctx)
    {
#if defined(TYR_ENABLE_INNER_PARALLELISM) && defined(TYR_ENABLE_SEMI_NAIVE)
        constexpr size_t kNumStripes = 2;
        constexpr size_t kParallelThreshold = 1024;
        auto& kckp_algorithm = rctx.out().kckp();
        if (kckp_algorithm.get_iteration() > 1 && rctx.in().cws_rule().get_rule().get_arity() > 2 && !rctx.stratum_in().is_single_threaded())
        {
            const auto& delta_edges = kckp_algorithm.materialize_delta_edges();
            if (delta_edges.size() >= kParallelThreshold)
            {
                assert(rctx.out().workers().size() == kNumStripes);
                oneapi::tbb::parallel_for(size_t(0),
                                          kNumStripes,
                                          [&](size_t stripe)
                                          {
                                              auto wrctx = rctx.get_rule_worker_execution_context(stripe);
                                              auto& out = wrctx.out();
                                              auto& kckp_workspace = out.kckp_workspace();
                                              ++out.statistics().num_executions;

                                              for (auto edge_index = stripe; edge_index < delta_edges.size(); edge_index += kNumStripes)
                                              {
                                                  if (!kckp_algorithm.seed_from_anchor(delta_edges[edge_index], kckp_workspace))
                                                      continue;
                                                  kckp_algorithm.template complete_from_seed<kckp::Edge>([&](auto&& clique)
                                                                                                         { process_clique(wrctx, clique, true); },
                                                                                                         0,
                                                                                                         kckp_workspace);
                                              }
                                          });
                return true;
            }
        }
#endif
        return false;
    }

    bool try_generate_parallel(fd::NumericEffectOperatorView<LiftedTag, f::FluentTag>, RuleExecutionContext<f::FunctionTag, AP, TP, CP>&) { return false; }

    template<f::RelationKind R>
    bool is_current_binding_dynamically_applicable(RuleWorkerExecutionContext<R, AP, TP, CP>& wrctx, const ApplicabilityContext& applicability_context)
    {
        auto& in = wrctx.in();
        auto& applicability_cache = wrctx.out().applicability_cache();
        if (!applicability_cache.dynamic_nullary)
            applicability_cache.dynamic_nullary = is_dynamically_applicable(in.cws_rule().get_nullary_condition(), in.fact_sets());
        return applicability_cache.dynamic_nullary && is_applicable(in.cws_rule().get_conflicting_overapproximation_condition(), applicability_context);
    }

    bool try_process_predicate_binding(fd::AtomView<LiftedTag, f::FluentTag> head_atom,
                                       RuleWorkerExecutionContext<f::PredicateTag, AP, TP, CP>& wrctx,
                                       RuleUpdateInput<LiftedTag, f::PredicateTag, AP, CP>& input,
                                       const ApplicabilityContext& applicability_context)
    {
        auto& in = wrctx.in();
        auto& out = wrctx.out();
        auto head = fd::try_ground_binding(head_atom, out.ground_context());
        if (head && in.fact_sets().template get<f::FluentTag>().predicate.contains(*head))
        {
            if constexpr (AP::records_propositional_achievers)
            {
                if (!is_current_binding_dynamically_applicable(wrctx, applicability_context))
                    return false;
                assert(is_applicable(in.cws_rule().get_rule(), applicability_context));
                return insert_propositional_update(*head, input, out.head_updates(), out.delta_annotations()).is_handled();
            }
            return true;
        }

        if (!is_current_binding_dynamically_applicable(wrctx, applicability_context))
            return false;

        if (!head)
            head = fd::ground_binding(head_atom, out.ground_context()).first;
        assert(is_applicable(in.cws_rule().get_rule(), applicability_context));
        return insert_propositional_update(*head, input, out.head_updates(), out.delta_annotations()).is_handled();
    }

    void process_clique_head(fd::AtomView<LiftedTag, f::FluentTag> head_atom,
                             RuleWorkerExecutionContext<f::PredicateTag, AP, TP, CP>& wrctx,
                             RuleUpdateInput<LiftedTag, f::PredicateTag, AP, CP>& input,
                             const ApplicabilityContext& applicability_context)
    {
        auto& out = wrctx.out();
        if (try_process_predicate_binding(head_atom, wrctx, input, applicability_context))
            return;

        ++out.statistics().num_pending_rules;
        const auto rule_binding = input.rule_instance.witness_key();
#ifdef TYR_ENABLE_SEMI_NAIVE
        out.pending_rule_bindings().push_back(rule_binding);
#else
        if (out.seen_pending_rule_bindings().emplace(rule_binding).second)
            out.pending_rule_bindings().push_back(rule_binding);
#endif
    }

    void process_clique_head(fd::NumericEffectOperatorView<LiftedTag, f::FluentTag>,
                             RuleWorkerExecutionContext<f::FunctionTag, AP, TP, CP>& wrctx,
                             RuleUpdateInput<LiftedTag, f::FunctionTag, AP, CP>& input,
                             const ApplicabilityContext& applicability_context)
    {
        [[maybe_unused]] const auto& in = wrctx.in();
        auto& out = wrctx.out();
        if (!is_current_binding_dynamically_applicable(wrctx, applicability_context))
            return;

        assert(is_applicable(in.cws_rule().get_rule(), ApplicabilityContext { in.fact_sets(), out.ground_context() }));
        insert_numeric_update(input, out.head_updates(), out.delta_numeric_annotations());
    }

    template<f::RelationKind R>
    void process_clique(RuleWorkerExecutionContext<R, AP, TP, CP>& wrctx, std::span<const kckp::Vertex> clique, [[maybe_unused]] bool require_novel_binding)
    {
        auto& in = wrctx.in();
        auto& out = wrctx.out();
        auto input = make_rule_update_input(RuleInstance<LiftedTag, R> { in.cws_rule(), out.ground_context() },
                                            in.numeric_support_selector(),
                                            in.annotations(),
                                            out.rule_evaluation_workspace(),
                                            in.numeric_annotations(),
                                            in.annotation_policy(),
                                            in.cost_policy());

        create_general_binding(clique, in.cws_rule().get_static_consistency_graph(), out.ground_context().binding);

#ifndef NDEBUG
        const auto novel_binding = ensure_novel_binding(out.ground_context().binding, out.seen_bindings());
        assert((!require_novel_binding || novel_binding) && "Delta-KCKP generated duplicate binding.");
#endif

        ++out.statistics().num_generated_rules;

        const auto conflicting_condition = in.cws_rule().get_conflicting_overapproximation_condition();
        const auto applicability_context = ApplicabilityContext { in.fact_sets(), out.ground_context() };
        auto& applicability_cache = out.applicability_cache();

        const auto statically_applicable = [&]()
        { return applicability_cache.static_nullary && is_applicable(conflicting_condition.template get_literals<f::StaticTag>(), applicability_context); };

        if (!statically_applicable())
            return;

        process_clique_head(in.cws_rule().get_rule().get_head(), wrctx, input, applicability_context);
    }

    template<f::RelationKind R>
    void generate_general_case(RuleExecutionContext<R, AP, TP, CP>& rctx, bool enumerate_all)
    {
        auto& rule_out = rctx.out();
        auto& kckp_algorithm = rule_out.kckp();
        const auto head = rctx.in().cws_rule().get_rule().get_head();
        if (!enumerate_all && try_generate_parallel(head, rctx))
            return;

        auto wrctx = rctx.get_rule_worker_execution_context();
        auto& out = wrctx.out();
        auto& kckp_workspace = out.kckp_workspace();
        ++out.statistics().num_executions;

        for_each_relevant_clique(
            head,
            kckp_algorithm,
            [&](auto&& clique) { process_clique(wrctx, clique, require_novel_binding(head) && !enumerate_all); },
            kckp_workspace,
            enumerate_all);
    }

    template<f::RelationKind R>
    void generate(RuleExecutionContext<R, AP, TP, CP>& rctx, bool enumerate_all)
    {
        const auto arity = rctx.in().cws_rule().get_rule().get_arity();

        if (arity == 0)
            generate_nullary_case(rctx);
        else
            generate_general_case(rctx, enumerate_all);
    }

    void process_pending_rule_bindings(RuleExecutionContext<f::PredicateTag, AP, TP, CP>& rctx)
    {
        for (auto& worker : rctx.out().workers())
        {
            auto wrctx = RuleWorkerExecutionContext(rctx, worker);

            auto& in = wrctx.in();
            auto& out = wrctx.out();
            const auto applicability_context = ApplicabilityContext { in.fact_sets(), out.ground_context() };

            auto& pending = out.pending_rule_bindings();
            assert(pending.empty() || out.applicability_cache().static_nullary);
            std::erase_if(pending,
                          [&](const auto pending_binding)
                          {
                              out.ground_context().binding.clear();
                              ygg::extend(pending_binding.get_objects(), out.ground_context().binding);
                              auto input = make_rule_update_input(RuleInstance<LiftedTag, f::PredicateTag> { in.cws_rule(), out.ground_context() },
                                                                  in.numeric_support_selector(),
                                                                  in.annotations(),
                                                                  out.rule_evaluation_workspace(),
                                                                  in.numeric_annotations(),
                                                                  in.annotation_policy(),
                                                                  in.cost_policy());

                              assert(is_applicable(in.cws_rule().get_conflicting_overapproximation_condition().template get_literals<f::StaticTag>(),
                                                   applicability_context));
                              return try_process_predicate_binding(in.cws_rule().get_rule().get_head(), wrctx, input, applicability_context);
                          });
        }
    }

    void process_pending_rule_bindings(RuleExecutionContext<f::FunctionTag, AP, TP, CP>&) {}

    template<f::RelationKind R>
    /// Parallel phase: recheck pending rule bindings and generate new ground witnesses for all active rules.
    void run_active_rules(StratumExecutionContext<AP, TP, CP>& ctx)
    {
        auto& program_out = ctx.out().program();
        const auto& rule_scheduler = ctx.out().scheduler().template get<R>();
        const auto& active_rules = rule_scheduler.get_active_rules();

        const auto program_stopwatch = ygg::StopwatchScope(program_out.statistics().parallel_time);

        const auto process_rule = [&](auto&& rule_index)
        {
            // Numeric effects and state-dependent costs are non-idempotent as the numeric envelope evolves.
            const auto enumerate_all = std::same_as<R, f::FunctionTag> || rule_scheduler.requires_full_enumeration(rule_index);
            auto rctx = ctx.get_rule_execution_context(rule_index);
            auto& rule_out = rctx.out();

            const auto total_time = ygg::StopwatchScope(rule_out.statistics().total_time);
            ++rule_out.statistics().num_executions;

            rctx.clear_iteration();  ///< Clear iteration before process_pending_rule_bindings/generate

            {
                const auto initialize_time = ygg::StopwatchScope(rule_out.statistics().initialize_time);

                rctx.initialize();  ///< Initialize before process_pending_rule_bindings/generate

                const auto nullary_condition = rctx.in().cws_rule().get_nullary_condition();
                const auto static_nullary =
                    is_applicable(nullary_condition.template get_literals<f::StaticTag>(), rctx.get_rule_worker_execution_context().in().fact_sets());
                for (auto& worker : rule_out.workers())
                    worker.solve.applicability_cache.static_nullary = static_nullary;
            }

            {
                const auto process_pending_time = ygg::StopwatchScope(rule_out.statistics().process_pending_time);

#ifdef TYR_ENABLE_SEMI_NAIVE
                if (enumerate_all)
                {
                    // Full enumeration reconstructs the pending set; retaining it would duplicate old bindings.
                    for (auto& worker : rule_out.workers())
                        worker.solve.pending_rule_bindings.clear();
                }
                else
                {
                    process_pending_rule_bindings(rctx);
                }
#else
                process_pending_rule_bindings(rctx);
#endif
            }

            {
                const auto process_generate_time = ygg::StopwatchScope(rule_out.statistics().process_generate_time);

                generate(rctx, enumerate_all);
            }
        };

        // With one requested Datalog thread, run sequentially to pin the execution order to the scheduler's
        // sorted rule order (TBB task order is unspecified even at concurrency 1).
        if (ctx.in().is_single_threaded())
        {
            for (const auto rule_index : active_rules)
                process_rule(rule_index);
        }
        else
        {
            oneapi::tbb::parallel_for_each(active_rules.begin(), active_rules.end(), process_rule);
        }
    }

    template<f::RelationKind R>
    /// Sequential phase: reduce worker annotations and bucket their already-canonical heads by cost.
    bool reduce_worker_results(StratumExecutionContext<AP, TP, CP>& ctx, CostBuckets& cost_buckets, PendingPredicateAchievers& pending_achievers)
    {
        auto& program_out = ctx.out().program();
        auto& scheduler = ctx.out().scheduler();
        auto& annotation_policy = program_out.annotation_policy();
        auto annotation_improved = false;

        for (const auto rule_index : ctx.out().scheduler().template get<R>().get_active_rules())
        {
            const auto i = ygg::uint_t(rule_index);
            const auto& ws_rule = program_out.template get_rules<R>()[i];

            for (auto& worker : ws_rule->worker)
            {
                if constexpr (std::same_as<R, f::PredicateTag>)
                    reduce_predicate_head_updates(worker.iteration.head_updates,
                                                  annotation_policy,
                                                  program_out.facts().fact_sets.predicate,
                                                  program_out.delta_annotations(),
                                                  program_out.annotations(),
                                                  cost_buckets,
                                                  pending_achievers);
                else
                    annotation_improved |= reduce_function_head_updates(worker.iteration.head_updates,
                                                                        annotation_policy,
                                                                        program_out.delta_numeric_annotations(),
                                                                        program_out.numeric_annotations(),
                                                                        cost_buckets,
                                                                        scheduler,
                                                                        ctx.out().program_context());
            }
        }
        return annotation_improved;
    }

    StratumExecutionContext<AP, TP, CP>& stratum()
    {
        assert(m_stratum);
        return *m_stratum;
    }

    ProgramExecutionContext<LiftedTag, AP, TP, CP>& m_ctx;
    ygg::StopwatchScope<std::chrono::nanoseconds> m_program_stopwatch;
    std::optional<StratumExecutionContext<AP, TP, CP>> m_stratum;
    size_t m_next_stratum = 0;
};

template void compute_model(ProgramExecutionContext<LiftedTag, NoAnnotationPolicy, NoTerminationPolicy>& ctx);
template void compute_model(ProgramExecutionContext<LiftedTag, MinCostAnnotationPolicy<SumAggregation>, NoTerminationPolicy>& ctx);
template void compute_model(ProgramExecutionContext<LiftedTag, MinCostAnnotationPolicy<SumAggregation>, TerminationPolicy<SumAggregation>>& ctx);
template void compute_model(ProgramExecutionContext<LiftedTag, MinCostAnnotationPolicy<MaxAggregation>, NoTerminationPolicy>& ctx);
template void compute_model(ProgramExecutionContext<LiftedTag, MinCostAnnotationPolicy<MaxAggregation>, TerminationPolicy<MaxAggregation>>& ctx);
template void compute_model(ProgramExecutionContext<LiftedTag, MinCostAnnotationWithAchieversPolicy<MaxAggregation>, TerminationPolicy<MaxAggregation>>& ctx);
template void
compute_model(ProgramExecutionContext<LiftedTag, MinCostAnnotationPolicy<SumAggregation>, NoTerminationPolicy, RuleCostOverridePolicy<LiftedTag>>& ctx);
template void compute_model(
    ProgramExecutionContext<LiftedTag, MinCostAnnotationPolicy<SumAggregation>, TerminationPolicy<SumAggregation>, RuleCostOverridePolicy<LiftedTag>>& ctx);
template void
compute_model(ProgramExecutionContext<LiftedTag, MinCostAnnotationPolicy<MaxAggregation>, NoTerminationPolicy, RuleCostOverridePolicy<LiftedTag>>& ctx);
template void compute_model(
    ProgramExecutionContext<LiftedTag, MinCostAnnotationPolicy<MaxAggregation>, TerminationPolicy<MaxAggregation>, RuleCostOverridePolicy<LiftedTag>>& ctx);
template void compute_model(ProgramExecutionContext<LiftedTag,
                                                    MinCostAnnotationWithAchieversPolicy<MaxAggregation>,
                                                    TerminationPolicy<MaxAggregation>,
                                                    RuleCostOverridePolicy<LiftedTag>>& ctx);
}
