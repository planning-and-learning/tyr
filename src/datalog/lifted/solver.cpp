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

#include "tyr/algorithms/kckp/delta_kckp.hpp"
#include "tyr/datalog/applicability.hpp"
#include "tyr/datalog/applicability_lifted.hpp"
#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/lifted/assignment_sets.hpp"
#include "tyr/datalog/lifted/consistency_graph.hpp"
#include "tyr/datalog/lifted/contexts/program.hpp"
#include "tyr/datalog/lifted/rule_instance.hpp"
#include "tyr/datalog/lifted/rule_scheduler.hpp"
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

#include <algorithm>
#include <assert.h>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/parallel_for_each.h>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/core/chrono.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/semantics/comparators.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::datalog
{
namespace
{
namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

static void create_nullary_binding(ygg::IndexList<f::Object>& binding) { binding.clear(); }

static void create_general_binding(std::span<const kckp::Vertex> clique, const StaticConsistencyGraph& consistency_graph, ygg::IndexList<f::Object>& binding)
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

template<f::RelationKind R, AnnotationPolicyConcept AP, RuleCostPolicyConcept CP>
struct RuleUpdateInput
{
    RuleInstance<LiftedTag, R> rule_instance;
    RuleEvaluationInput evaluation;
    RuleEvaluationWorkspace& workspace;
    const FunctionAnnotations<>& numeric_annotations;
    AP& annotation_policy;
    const CP& cost_policy;
};

template<f::RelationKind R, typename In, typename Out>
static auto make_rule_update_input(In& in, Out& out)
{
    return RuleUpdateInput<R, std::decay_t<decltype(in.annotation_policy())>, std::decay_t<decltype(in.cost_policy())>> {
        RuleInstance<LiftedTag, R> { in.cws_rule().get_rule(), out.ground_context() },
        RuleEvaluationInput { in.numeric_support_selector(), in.annotations() },
        out.rule_evaluation_workspace(),
        in.numeric_annotations(),
        in.annotation_policy(),
        in.cost_policy()
    };
}

template<AnnotationPolicyConcept AP, RuleCostPolicyConcept CP>
static bool insert_propositional_update(fd::PredicateBindingView<f::FluentTag> head,
                                        RuleUpdateInput<f::PredicateTag, AP, CP>& input,
                                        PredicateHeadIteration& head_iteration,
                                        [[maybe_unused]] PredicateAnnotations<true>& delta_annotations)
{
    auto candidate = evaluate_predicate_candidate(input.rule_instance, input.annotation_policy, input.cost_policy, input.evaluation, input.workspace);
    if (!candidate)
        return false;

    assert(candidate->head == head);
    if constexpr (AP::stores_annotations)
    {
        const auto can_update = input.annotation_policy.can_update(head, candidate->cost, input.evaluation.predicate_annotations, delta_annotations);
        if constexpr (AP::records_propositional_achievers)
        {
            auto witness = materialize_witness(input.rule_instance, *candidate);
            head_iteration.insert_achiever(head, witness);
            if (can_update)
                input.annotation_policy.try_update_candidate(head, std::move(witness), delta_annotations);
        }
        else if (can_update)
        {
            auto witness = materialize_witness(input.rule_instance, *candidate);
            input.annotation_policy.try_update_candidate(head, std::move(witness), delta_annotations);
        }
    }
    // A previously staged certificate may still need to become an available fact after a resumed solve.
    head_iteration.insert(head);
    return true;
}

template<AnnotationPolicyConcept AP, RuleCostPolicyConcept CP>
static bool insert_numeric_update(RuleUpdateInput<f::FunctionTag, AP, CP>& input,
                                  FunctionHeadIteration& head_iteration,
                                  [[maybe_unused]] FunctionAnnotations<true>& delta_numeric_annotations)
{
    auto candidate = evaluate_function_candidate(input.rule_instance, input.annotation_policy, input.cost_policy, input.evaluation, input.workspace);
    if (!candidate)
        return false;

    if constexpr (!AP::stores_annotations)
    {
        if (candidate->grows_fact)
            head_iteration.insert(FunctionHeadUpdate(candidate->head, candidate->interval, true));
    }
    else
    {
        auto staged = false;
        if (input.annotation_policy.can_update(candidate->head, candidate->interval, candidate->cost, input.numeric_annotations, delta_numeric_annotations))
        {
            auto witness = materialize_witness(input.rule_instance, *candidate);
            staged = input.annotation_policy.try_update_candidate(candidate->head, candidate->interval, std::move(witness), delta_numeric_annotations);
        }
        if (staged || candidate->grows_fact)
            head_iteration.insert(FunctionHeadUpdate(candidate->head, candidate->interval, candidate->grows_fact));
    }
    return true;
}

template<typename In, typename Out, AnnotationPolicyConcept AP, RuleCostPolicyConcept CP>
void insert_nullary_update(fd::AtomView<f::FluentTag> head_atom, RuleUpdateInput<f::PredicateTag, AP, CP>& input, const In&, Out& out)
{
    const auto head = fd::ground_binding(head_atom, out.ground_context()).first;
    insert_propositional_update(head, input, out.head_updates(), out.delta_annotations());
}

template<typename In, typename Out, AnnotationPolicyConcept AP, RuleCostPolicyConcept CP>
void insert_nullary_update(fd::NumericEffectOperatorView<f::FluentTag>, RuleUpdateInput<f::FunctionTag, AP, CP>& input, const In& in, Out& out)
{
    assert(is_applicable(in.cws_rule().get_rule(), ApplicabilityContext { in.fact_sets(), out.ground_context() }));
    insert_numeric_update(input, out.head_updates(), out.delta_numeric_annotations());
}

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void generate_nullary_case(RuleExecutionContext<R, AP, TP, CP>& rctx)
{
    auto wrctx = rctx.get_rule_worker_execution_context();

    auto& in = wrctx.in();
    auto& out = wrctx.out();
    auto input = make_rule_update_input<R>(in, out);
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
        insert_nullary_update(in.cws_rule().get_rule().get_head(), input, in, out);
}

[[maybe_unused]] static bool ensure_novel_binding(const ygg::IndexList<f::Object>& binding, ygg::UnorderedSet<ygg::IndexList<f::Object>>& set)
{
    return set.insert(binding).second;
}

template<typename Callback>
void for_each_relevant_clique(fd::AtomView<f::FluentTag>,
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
void for_each_relevant_clique(fd::NumericEffectOperatorView<f::FluentTag>, kckp::DeltaKCKP& algorithm, Callback&& callback, kckp::Workspace& workspace, bool)
{
    algorithm.for_each_clique(std::forward<Callback>(callback), workspace);
}

inline bool require_novel_binding(fd::AtomView<f::FluentTag>) noexcept
{
#ifdef TYR_ENABLE_SEMI_NAIVE
    return true;
#else
    return false;
#endif
}

inline bool require_novel_binding(fd::NumericEffectOperatorView<f::FluentTag>) noexcept { return false; }

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
bool try_generate_parallel(fd::AtomView<f::FluentTag>, [[maybe_unused]] RuleExecutionContext<f::PredicateTag, AP, TP, CP>& rctx)
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
            oneapi::tbb::parallel_for(
                size_t(0),
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
                        kckp_algorithm.template complete_from_seed<kckp::Edge>([&](auto&& clique) { process_clique(wrctx, clique, true); }, 0, kckp_workspace);
                    }
                });
            return true;
        }
    }
#endif
    return false;
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
bool try_generate_parallel(fd::NumericEffectOperatorView<f::FluentTag>, RuleExecutionContext<f::FunctionTag, AP, TP, CP>&)
{
    return false;
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP, typename DynamicallyApplicable>
void process_clique_head(fd::AtomView<f::FluentTag> head_atom,
                         RuleWorkerExecutionContext<f::PredicateTag, AP, TP, CP>& wrctx,
                         RuleUpdateInput<f::PredicateTag, AP, CP>& input,
                         DynamicallyApplicable&& dynamically_applicable)
{
    auto& in = wrctx.in();
    auto& out = wrctx.out();
    const auto retain_pending = [&]()
    {
        ++out.statistics().num_pending_rules;
        const auto rule_binding = fd::ground_binding(in.cws_rule().get_conflicting_overapproximation_rule(), out.ground_context()).first;
#ifdef TYR_ENABLE_SEMI_NAIVE
        out.pending_rule_bindings().push_back(rule_binding);
#else
        if (out.seen_pending_rule_bindings().emplace(rule_binding).second)
            out.pending_rule_bindings().push_back(rule_binding);
#endif
    };
    auto head = fd::try_ground_binding(head_atom, out.ground_context());
    if (head && in.fact_sets().template get<f::FluentTag>().predicate.contains(*head))
    {
        if constexpr (AP::stores_annotations)
        {
            if (!dynamically_applicable())
                retain_pending();
            else
            {
                assert(is_applicable(in.cws_rule().get_rule(), ApplicabilityContext { in.fact_sets(), out.ground_context() }));
                if (!insert_propositional_update(*head, input, out.head_updates(), out.delta_annotations()))
                    retain_pending();
            }
        }
        return;
    }

    if (!dynamically_applicable())
    {
        retain_pending();
        return;
    }

    if (!head)
        head = fd::ground_binding(head_atom, out.ground_context()).first;
    assert(is_applicable(in.cws_rule().get_rule(), ApplicabilityContext { in.fact_sets(), out.ground_context() }));
    if (!insert_propositional_update(*head, input, out.head_updates(), out.delta_annotations()))
        retain_pending();
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP, typename DynamicallyApplicable>
void process_clique_head(fd::NumericEffectOperatorView<f::FluentTag>,
                         RuleWorkerExecutionContext<f::FunctionTag, AP, TP, CP>& wrctx,
                         RuleUpdateInput<f::FunctionTag, AP, CP>& input,
                         DynamicallyApplicable&& dynamically_applicable)
{
    const auto& in = wrctx.in();
    auto& out = wrctx.out();
    if (!dynamically_applicable())
        return;

    assert(is_applicable(in.cws_rule().get_rule(), ApplicabilityContext { in.fact_sets(), out.ground_context() }));
    insert_numeric_update(input, out.head_updates(), out.delta_numeric_annotations());
}

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void process_clique(RuleWorkerExecutionContext<R, AP, TP, CP>& wrctx, std::span<const kckp::Vertex> clique, [[maybe_unused]] bool require_novel_binding)
{
    auto& in = wrctx.in();
    auto& out = wrctx.out();
    auto input = make_rule_update_input<R>(in, out);

    create_general_binding(clique, in.cws_rule().get_static_consistency_graph(), out.ground_context().binding);

#ifndef NDEBUG
    const auto novel_binding = ensure_novel_binding(out.ground_context().binding, out.seen_bindings());
    assert((!require_novel_binding || novel_binding) && "Delta-KCKP generated duplicate binding.");
#endif

    ++out.statistics().num_generated_rules;

    const auto nullary_condition = in.cws_rule().get_nullary_condition();
    const auto conflicting_condition = in.cws_rule().get_conflicting_overapproximation_rule().get_body();
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

    if (!statically_applicable())
        return;

    process_clique_head(in.cws_rule().get_rule().get_head(), wrctx, input, dynamically_applicable);
}

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
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

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void generate(RuleExecutionContext<R, AP, TP, CP>& rctx, bool enumerate_all)
{
    const auto arity = rctx.in().cws_rule().get_rule().get_arity();

    if (arity == 0)
        generate_nullary_case(rctx);
    else
        generate_general_case(rctx, enumerate_all);
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void process_pending_rule_bindings(RuleExecutionContext<f::PredicateTag, AP, TP, CP>& rctx)
{
    for (auto& worker : rctx.out().workers())
    {
        auto wrctx = RuleWorkerExecutionContext(rctx, worker);

        auto& in = wrctx.in();
        auto& out = wrctx.out();
        const auto nullary_condition = in.cws_rule().get_nullary_condition();
        const auto conflicting_condition = in.cws_rule().get_conflicting_overapproximation_rule().get_body();
        const auto applicability_context = ApplicabilityContext { in.fact_sets(), out.ground_context() };
        auto& applicability_cache = out.applicability_cache();
        const auto dynamically_applicable = [&]()
        {
            if (!applicability_cache.dynamic_nullary)
                applicability_cache.dynamic_nullary = is_dynamically_applicable(nullary_condition, in.fact_sets());
            return applicability_cache.dynamic_nullary && is_applicable(conflicting_condition, applicability_context);
        };

        auto& pending = out.pending_rule_bindings();
        assert(pending.empty() || applicability_cache.static_nullary);
        std::erase_if(pending,
                      [&](const auto pending_binding)
                      {
                          out.ground_context().binding.clear();
                          ygg::extend(pending_binding.get_objects(), out.ground_context().binding);
                          auto input = make_rule_update_input<f::PredicateTag>(in, out);

                          assert(is_applicable(conflicting_condition.template get_literals<f::StaticTag>(), applicability_context));

                          auto head = fd::try_ground_binding(in.cws_rule().get_rule().get_head(), out.ground_context());
                          if (head && in.fact_sets().template get<f::FluentTag>().predicate.contains(*head))
                          {
                              if constexpr (AP::stores_annotations)
                              {
                                  if (!dynamically_applicable())
                                      return false;

                                  assert(is_applicable(in.cws_rule().get_rule(), applicability_context));
                                  return insert_propositional_update(*head, input, out.head_updates(), out.delta_annotations());
                              }
                              return true;
                          }

                          if (!dynamically_applicable())
                              return false;

                          if (!head)
                              head = fd::ground_binding(in.cws_rule().get_rule().get_head(), out.ground_context()).first;
                          assert(is_applicable(in.cws_rule().get_rule(), applicability_context));
                          return insert_propositional_update(*head, input, out.head_updates(), out.delta_annotations());
                      });
    }
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void process_pending_rule_bindings(RuleExecutionContext<f::FunctionTag, AP, TP, CP>&)
{
}

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
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

template<AnnotationPolicyConcept AP>
void reduce_worker_heads(PredicateHeadIteration& head_iteration,
                         [[maybe_unused]] AP& annotation_policy,
                         [[maybe_unused]] const PredicateFactSets<f::FluentTag>& facts,
                         [[maybe_unused]] PredicateAnnotations<true>& delta_annotations,
                         [[maybe_unused]] PredicateAnnotations<>& annotations,
                         CostBuckets& cost_buckets,
                         [[maybe_unused]] PendingPredicateAchievers& pending_achievers)
{
    if constexpr (AP::records_propositional_achievers)
        for (auto& achiever : head_iteration.achievers)
            insert_pending_achiever(pending_achievers, std::move(achiever));

    for (const auto head : head_iteration.bindings)
    {
        if constexpr (AP::stores_annotations)
        {
            const auto* delta_annotation = delta_annotations.find(head);
            if (facts.contains(head))
            {
                assert((!delta_annotation || get_cost(*delta_annotation) >= annotations.fetch_cost(head)) && "A committed predicate label must be final.");
                continue;
            }

            if (!delta_annotation && !annotations.find(head))
                continue;
            cost_buckets.update(annotation_policy.commit_annotation(head, delta_annotations, annotations), head);
        }
        else
        {
            cost_buckets.insert(Cost(0), head);
        }
    }
}

template<AnnotationPolicyConcept AP>
bool reduce_worker_heads(FunctionHeadIteration& head_iteration,
                         [[maybe_unused]] AP& annotation_policy,
                         [[maybe_unused]] FunctionAnnotations<true>& delta_numeric_annotations,
                         [[maybe_unused]] FunctionAnnotations<>& numeric_annotations,
                         [[maybe_unused]] RuleSchedulerStratum& scheduler,
                         CostBuckets& cost_buckets)
{
    auto annotation_improved = false;
    for (const auto& update : head_iteration.updates)
    {
        if constexpr (AP::stores_annotations)
        {
            const auto* annotation = delta_numeric_annotations.find(update.binding, update.interval);
            auto improved = false;
            if (annotation)
            {
                improved = annotation_policy.commit_annotation(update.binding, update.interval, delta_numeric_annotations, numeric_annotations)
                               .is_strict_improvement();
                annotation = numeric_annotations.find(update.binding, update.interval);
            }
            else
                annotation = numeric_annotations.find(update.binding, update.interval);
            // FunctionAnnotations retains the cheapest certificate for each exact interval.
            // FactSets determines whether that certificate is available; CostBuckets schedules only hull growth.
            if (annotation && update.grows_fact)
                cost_buckets.insert(get_cost(*annotation), update.binding, update.interval);
            else if (improved)
            {
                annotation_improved = true;
                scheduler.on_generate(update.binding.get_index().relation);
            }
        }
        else
        {
            if (update.grows_fact)
                cost_buckets.insert(Cost(0), update.binding, update.interval);
        }
    }
    return annotation_improved;
}

template<f::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
/// Sequential phase: reduce worker annotations and bucket their already-canonical heads by cost.
bool reduce_worker_results(StratumExecutionContext<AP, TP, CP>& ctx, PendingPredicateAchievers& pending_achievers)
{
    auto& program_out = ctx.out().program();
    auto& scheduler = ctx.out().scheduler();
    auto& cost_buckets = program_out.cost_buckets();
    auto& annotation_policy = program_out.annotation_policy();
    auto annotation_improved = false;

    for (const auto rule_index : ctx.out().scheduler().template get<R>().get_active_rules())
    {
        const auto i = ygg::uint_t(rule_index);
        const auto& ws_rule = program_out.template get_rules<R>()[i];

        for (auto& worker : ws_rule->worker)
        {
            if constexpr (std::same_as<R, f::PredicateTag>)
                reduce_worker_heads(worker.iteration.head_updates,
                                    annotation_policy,
                                    program_out.facts().fact_sets.predicate,
                                    program_out.delta_annotations(),
                                    program_out.annotations(),
                                    cost_buckets,
                                    pending_achievers);
            else
                annotation_improved |= reduce_worker_heads(worker.iteration.head_updates,
                                                           annotation_policy,
                                                           program_out.delta_numeric_annotations(),
                                                           program_out.numeric_annotations(),
                                                           scheduler,
                                                           cost_buckets);
        }
    }
    return annotation_improved;
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
/// Commit a cost bucket: insert its heads into the fact and assignment sets and notify the scheduler.
bool commit_bucket(StratumExecutionContext<AP, TP, CP>& ctx, Cost cost)
{
    auto& program_out = ctx.out().program();
    auto& scheduler = ctx.out().scheduler();
    auto& facts = program_out.facts();
    auto bucket = program_out.cost_buckets().take(cost);
    auto changed = false;

    // Native bucket order avoids sorting overhead. It is deterministic for single-threaded
    // execution; equal-cost order under parallel execution is intentionally unspecified.
    for (const auto head : bucket.predicate)
    {
        if (facts.fact_sets.predicate.insert(head))
        {
            changed = true;
            scheduler.on_generate(head.get_index().relation);
            facts.assignment_sets.predicate.insert(head);
        }
    }

    for (const auto& [head, interval] : bucket.function)
    {
        if (facts.fact_sets.function.insert(head, interval))
        {
            changed = true;
            scheduler.on_generate(head.get_index().relation);
            facts.assignment_sets.function.insert(head, interval);
        }
    }

    program_out.rebuild_numeric_support_selector(ctx.in().program().facts().fact_sets);
    return changed;
}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void compute_model_for_stratum(StratumExecutionContext<AP, TP, CP>& ctx)
{
    auto& out = ctx.out();
    auto& scheduler = out.scheduler();
    auto& program_out = out.program();
    auto& cost_buckets = program_out.cost_buckets();

    scheduler.activate_all();
    cost_buckets.clear();
    auto pending_achievers = PendingPredicateAchievers {};

    while (true)
    {
        // Facts are committed in cost order, so the goal cost is proven minimal once the goal holds.
        if (program_out.tp().should_terminate(FactSets { ctx.in().program().facts().fact_sets, program_out.facts().fact_sets }))
            return;

        scheduler.on_start_iteration();

        program_out.delta_annotations().clear();
        program_out.delta_numeric_annotations().clear();

        run_active_rules<f::PredicateTag>(ctx);
        run_active_rules<f::FunctionTag>(ctx);

        reduce_worker_results<f::PredicateTag>(ctx, pending_achievers);
        const auto numeric_annotation_improved = reduce_worker_results<f::FunctionTag>(ctx, pending_achievers);

        if (numeric_annotation_improved)
        {
            scheduler.on_finish_iteration();
            continue;
        }

#ifdef TYR_ENABLE_SEMI_NAIVE
        if (cost_buckets.is_empty())
        {
            publish_pending_achievers(pending_achievers, pending_achievers.buckets.end(), program_out.annotation_policy());
            return;  ///< All reachable heads are committed.
        }
        const auto cost = cost_buckets.min_cost();
        commit_bucket(ctx, cost);
        publish_pending_achievers(pending_achievers, pending_achievers.buckets.upper_bound(cost), program_out.annotation_policy());
        scheduler.on_finish_iteration();
#else
        auto fact_changed = false;
        while (!cost_buckets.is_empty())
        {
            const auto cost = cost_buckets.min_cost();
            const auto bucket_changed = commit_bucket(ctx, cost);
            publish_pending_achievers(pending_achievers, pending_achievers.buckets.upper_bound(cost), program_out.annotation_policy());
            if (bucket_changed)
            {
                fact_changed = true;
                break;
            }
        }
        if (!fact_changed)
        {
            publish_pending_achievers(pending_achievers, pending_achievers.buckets.end(), program_out.annotation_policy());
            return;  ///< All reachable heads are committed.
        }
        scheduler.activate_all();
#endif
    }
}

}

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void compute_model(ProgramExecutionContext<LiftedTag, AP, TP, CP>& ctx)
{
    auto& out = ctx.out();
    const auto program_stopwatch = ygg::StopwatchScope(out.statistics().total_time);
    ++out.statistics().num_executions;

    for (auto stratum_ctx : ctx.get_stratum_execution_contexts())
    {
        compute_model_for_stratum(stratum_ctx);
    }
}

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
