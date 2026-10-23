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
#include "tyr/formalism/datalog/expression_arity.hpp"
#include "tyr/formalism/datalog/grounder.hpp"
#include "tyr/formalism/datalog/interning.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"

#include <algorithm>
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

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
class SolverPolicy<LiftedTag, AP, TP, CP>
{
public:
    explicit SolverPolicy(ProgramExecutionContext<LiftedTag, AP, TP, CP>& ctx) :
        m_ctx(ctx),
        m_program_stopwatch(ctx.out().statistics().total_time),
        m_goal(ctx.out().goal_cutoff())
    {
        ++ctx.out().statistics().num_executions;
        initialize_goal_cutoff();
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
        ctx.out().program().advance_constraint_cache_epoch();
        begin_goal_cutoff_iteration(ctx);
        run_active_rules<formalism::PredicateTag>(ctx);
        run_active_rules<formalism::FunctionTag>(ctx);
        // The rules consumed the facts committed since they last ran; the commit phase collects the next ones.
        ctx.out().program().facts().reset_delta();
        reduce_worker_results<formalism::PredicateTag>(ctx, cost_buckets, pending_achievers);
        return reduce_worker_results<formalism::FunctionTag>(ctx, cost_buckets, pending_achievers);
    }

    Scheduler<LiftedTag>& scheduler() { return stratum().out().scheduler(); }

private:
    using GoalAtom = GoalCutoffWorkspace::GoalAtom;

    /// Enables the goal cutoff where stopping an iteration early keeps every witness: positive fluent goals without numeric
    /// constraints, propositional rules only, one stratum, and annotations without achievers.
    void initialize_goal_cutoff()
    {
        if constexpr (AP::stores_annotations && !AP::records_propositional_achievers
                      && std::same_as<CP, RuleCostPolicy> && !std::same_as<TP, NoTerminationPolicy>)
        {
            const auto& goal = m_ctx.out().tp().get_goal();
            if (!goal || !goal->get_numeric_constraints().empty() || m_ctx.out().schedulers().size() != 1
                || !m_ctx.in().template get_rules<formalism::FunctionTag>().empty())
                return;
            const auto fact_sets = FactSets { m_ctx.in().facts().fact_sets, m_ctx.out().facts().fact_sets };
            if (!is_applicable(goal->template get_literals<formalism::StaticTag>(), fact_sets))
                return;

            auto min_edge_cost = std::numeric_limits<Cost>::max();
            for (const auto& rule : m_ctx.in().template get_rules<formalism::PredicateTag>())
                min_edge_cost = std::min(min_edge_cost, Cost(rule->get_rule().get_cost()));

            m_goal.atoms.clear();
            m_goal.positions.clear();
            for (const auto literal : goal->template get_literals<formalism::FluentTag>())
                if (m_goal.positions.emplace(literal.get_atom().get_row().get_index(), m_goal.atoms.size()).second)
                    m_goal.atoms.push_back(literal.get_atom().get_row().get_index());
            m_goal.final.assign(m_goal.atoms.size(), false);
            m_min_edge_cost = min_edge_cost == std::numeric_limits<Cost>::max() ? Cost(0) : min_edge_cost;
            m_goal_cutoff_enabled = !m_goal.atoms.empty();
        }
    }

    /// The committed cost of the delta: later candidates contain an atom committed from now on, and buckets commit in
    /// nondecreasing cost order. 0, a valid lower bound, if the delta has no predicate atom.
    Cost get_delta_cost(StratumExecutionContext<AP, TP, CP>& ctx) const
    {
        const auto& program_out = ctx.out().program();
        for (const auto& set : program_out.facts().delta_fact_sets.predicate.get_sets())
            for (const auto atom : set.get_bindings())
                if (const auto* annotation = program_out.annotations().find(atom.get_index()))
                    return get_cost(*annotation);
        return Cost(0);
    }

    /// Marks the goal atom at position i final if its tentative cost cannot decrease any more.
    void observe_goal_atom(size_t i, Cost tentative_cost)
    {
        if (m_goal.final[i] || tentative_cost > m_goal_cutoff_bound)
            return;
        m_goal.final[i] = true;
        if (++m_num_final_goals == m_goal.atoms.size())
            m_goal_cutoff_stopped = true;
    }

    void begin_goal_cutoff_iteration(StratumExecutionContext<AP, TP, CP>& ctx)
    {
        m_goal_cutoff_stopped = false;
        if (!m_goal_cutoff_enabled || !ctx.in().is_single_threaded())
            return;

        m_goal_cutoff_bound = get_delta_cost(ctx) + m_min_edge_cost;
        std::fill(m_goal.final.begin(), m_goal.final.end(), false);
        m_num_final_goals = 0;
        const auto& program_out = ctx.out().program();
        const auto& facts = program_out.facts().fact_sets.predicate;
        for (size_t i = 0; i < m_goal.atoms.size(); ++i)
        {
            if (facts.contains(m_goal.atoms[i]))
                observe_goal_atom(i, Cost(0));
            else if (const auto* annotation = program_out.annotations().find(m_goal.atoms[i]))
                observe_goal_atom(i, get_cost(*annotation));
        }
    }

    /// Observes the tentative costs of the goal atoms among the heads of a processed binding.
    void observe_goal_heads(StratumExecutionContext<AP, TP, CP>& ctx, std::span<const GoalAtom> heads)
    {
        if (!m_goal_cutoff_enabled || m_goal_cutoff_stopped)
            return;
        for (const auto head : heads)
            if (const auto it = m_goal.positions.find(head); it != m_goal.positions.end())
                if (const auto* annotation = ctx.out().program().delta_annotations().find(head))
                    observe_goal_atom(it->second, get_cost(*annotation));
    }

    bool goal_cutoff_stopped() const noexcept { return m_goal_cutoff_stopped; }

    static void create_nullary_binding(ygg::IndexList<formalism::Object>& binding) { binding.clear(); }

    static void
    create_general_binding(std::span<const kckp::Vertex> clique, const StaticConsistencyGraph& consistency_graph, ygg::IndexList<formalism::Object>& binding)
    {
        const auto k = clique.size();
        const auto& layout = consistency_graph.get_graph_layout();

        binding.resize(k);

        for (ygg::uint_t p = 0; p < k; ++p)
        {
            const auto vertex = clique[p].index;
            assert(layout.vertex_to_partition[vertex] == p);
            binding[p] = ygg::Index<formalism::Object>(layout.vertex_labels[vertex]);
        }
    }

    void insert_nullary_update(formalism::PredicateTag,
                               RuleUpdateInput<LiftedTag, formalism::PredicateTag, AP, CP>& input,
                               RuleWorkerExecutionContext<formalism::PredicateTag, AP, TP, CP>& ctx)
    {
        auto& out = ctx.out();
        auto& heads = out.head_bindings();
        heads.clear();
        for (const auto head_atom : ctx.in().cws_rule().get_rule().get_heads())
            heads.push_back(formalism::datalog::ground_binding(head_atom, out.ground_context()).first.get_index());
        insert_propositional_update(std::span(std::as_const(heads)), input, out.head_updates(), out.delta_annotations());
    }

    void insert_nullary_update(formalism::FunctionTag,
                               RuleUpdateInput<LiftedTag, formalism::FunctionTag, AP, CP>& input,
                               RuleWorkerExecutionContext<formalism::FunctionTag, AP, TP, CP>& ctx)
    {
        [[maybe_unused]] const auto& in = ctx.in();
        auto& out = ctx.out();
        assert(is_applicable(in.cws_rule().get_rule(), ApplicabilityContext { in.fact_sets(), out.ground_context() }));
        insert_numeric_update(input, out.head_updates(), out.delta_numeric_annotations());
    }

    template<formalism::RelationKind R>
    void generate_nullary_case(RuleExecutionContext<R, AP, TP, CP>& rctx)
    {
        auto wrctx = rctx.get_rule_worker_execution_context();

        auto& in = wrctx.in();
        auto& out = wrctx.out();
        if (!in.cws_rule().get_numeric_constraint_fluent_terms().empty())
            out.rule_evaluation_workspace().constraint_cache = in.constraint_cache();
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

        const auto statically_applicable = [&]() {
            return applicability_cache.static_nullary
                   && is_applicable(conflicting_condition.template get_literals<formalism::StaticTag>(), applicability_context);
        };
        const auto dynamically_applicable = [&]()
        {
            if (!applicability_cache.dynamic_nullary)
                applicability_cache.dynamic_nullary = is_dynamically_applicable(nullary_condition, in.fact_sets());
            return applicability_cache.dynamic_nullary && is_applicable(conflicting_condition, applicability_context);
        };

        // Note: we never go through the consistency graph, and hence, have to check validity on the entire rule body.
        if (statically_applicable() && dynamically_applicable())
            insert_nullary_update(R {}, input, wrctx);
    }

    [[maybe_unused]] static bool ensure_novel_binding(const ygg::IndexList<formalism::Object>& binding,
                                                      ygg::UnorderedSet<ygg::IndexList<formalism::Object>>& set)
    {
        return set.insert(binding).second;
    }

    template<typename Callback, typename Prune>
    void for_each_relevant_clique(formalism::PredicateTag,
                                  kckp::DeltaKCKP& algorithm,
                                  Callback&& callback,
                                  kckp::Workspace& workspace,
                                  [[maybe_unused]] bool enumerate_all,
                                  Prune&& prune)
    {
#ifdef TYR_ENABLE_SEMI_NAIVE
        if (!enumerate_all)
        {
            algorithm.for_each_delta_clique(std::forward<Callback>(callback), workspace, prune);
            return;
        }
#endif
        algorithm.for_each_clique(std::forward<Callback>(callback), workspace, prune);
    }

    template<typename Callback, typename Prune>
    void for_each_relevant_clique(formalism::FunctionTag, kckp::DeltaKCKP& algorithm, Callback&& callback, kckp::Workspace& workspace, bool, Prune&&)
    {
        algorithm.for_each_clique(std::forward<Callback>(callback), workspace);
    }

    /// Prunes a partial clique once the goal cutoff stopped the iteration, or once the vertex just bound completes the variables
    /// of a literal of the conflicting condition (the literals the graph does not encode) that does not hold. Bindings
    /// rejected this way are generated again by the delta that makes them applicable, as when the leaf rejects them.
    /// Partitions are the rule's parameters.
    auto make_literal_prune(formalism::PredicateTag, RuleWorkerExecutionContext<formalism::PredicateTag, AP, TP, CP>& wrctx)
    {
        const auto& checks = wrctx.in().cws_rule().get_conflicting_literal_checks();
        const auto& layout = wrctx.in().cws_rule().get_static_consistency_graph().get_graph_layout();
        return [this, &wrctx, &layout, &checks](ygg::uint_t partition, size_t, const kckp::Workspace& workspace)
        {
            if (goal_cutoff_stopped())
                return true;
            for (const auto& check : checks)
            {
                if (std::ranges::find(check.partitions, partition) == check.partitions.end()
                    || !std::ranges::all_of(check.partitions, [&](auto p) { return workspace.partition_bits.test(p); }))
                    continue;
                auto& binding = wrctx.out().ground_context().binding;
                binding.resize(layout.num_partitions);
                for (const auto p : check.partitions)
                    binding[p] = ygg::Index<formalism::Object>(layout.vertex_labels[workspace.partial_solution[p].index]);
                const auto context = ApplicabilityContext { wrctx.in().fact_sets(), wrctx.out().ground_context() };
                if (check.static_literal ? !is_applicable(*check.static_literal, context) : !is_applicable(*check.fluent_literal, context))
                    return true;
            }
            return false;
        };
    }

    auto make_literal_prune(formalism::FunctionTag, RuleWorkerExecutionContext<formalism::FunctionTag, AP, TP, CP>&) { return kckp::NoPrune {}; }

    inline bool require_novel_binding(formalism::PredicateTag) noexcept
    {
#ifdef TYR_ENABLE_SEMI_NAIVE
        return true;
#else
        return false;
#endif
    }

    inline bool require_novel_binding(formalism::FunctionTag) noexcept { return false; }

    bool try_generate_parallel(formalism::PredicateTag, [[maybe_unused]] RuleExecutionContext<formalism::PredicateTag, AP, TP, CP>& rctx)
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
                                              auto prune = make_literal_prune(formalism::PredicateTag {}, wrctx);
                                              ++out.statistics().num_executions;

                                              for (auto edge_index = stripe; edge_index < delta_edges.size(); edge_index += kNumStripes)
                                              {
                                                  if (!kckp_algorithm.seed_from_anchor(delta_edges[edge_index], kckp_workspace))
                                                      continue;
                                                  kckp_algorithm.template complete_from_seed<kckp::Edge>([&](auto&& clique)
                                                                                                         { process_clique(wrctx, clique, true, true); },
                                                                                                         0,
                                                                                                         kckp_workspace,
                                                                                                         prune);
                                              }
                                          });
                return true;
            }
        }
#endif
        return false;
    }

    bool try_generate_parallel(formalism::FunctionTag, RuleExecutionContext<formalism::FunctionTag, AP, TP, CP>&) { return false; }

    /// With literals_checked, the literals of the conflicting condition hold already (checked by the literal prune).
    template<formalism::RelationKind R>
    bool is_current_binding_dynamically_applicable(RuleWorkerExecutionContext<R, AP, TP, CP>& wrctx,
                                                   const ApplicabilityContext& applicability_context,
                                                   bool literals_checked = false)
    {
        auto& in = wrctx.in();
        auto& applicability_cache = wrctx.out().applicability_cache();
        if (!applicability_cache.dynamic_nullary)
            applicability_cache.dynamic_nullary = is_dynamically_applicable(in.cws_rule().get_nullary_condition(), in.fact_sets());
        if (!applicability_cache.dynamic_nullary)
            return false;
        const auto condition = in.cws_rule().get_conflicting_overapproximation_condition();
        return literals_checked ? is_applicable(condition.get_numeric_constraints(), applicability_context) : is_applicable(condition, applicability_context);
    }

    /// Collects the heads the binding can update: committed facts are final, but still receive achievers.
    /// Returns whether some head is not yet interned.
    bool collect_predicate_heads(RuleWorkerExecutionContext<formalism::PredicateTag, AP, TP, CP>& wrctx, bool intern)
    {
        auto& in = wrctx.in();
        auto& out = wrctx.out();
        auto& ground_context = out.ground_context();
        auto& heads = out.head_bindings();
        heads.clear();
        auto missing = false;
        const auto head_atoms = in.cws_rule().get_rule().get_heads();
        auto& lookups = out.head_lookups();
        lookups.resize(head_atoms.size());
        auto lookup_it = lookups.begin();
        for (const auto head_atom : head_atoms)
        {
            auto& lookup = *lookup_it++;
            if (!lookup.valid || (intern && !lookup.head) || !grounds_to(head_atom.get_terms(), ground_context.binding, lookup.objects))
            {
                auto binding =
                    formalism::datalog::checkout<formalism::RelationBinding<formalism::Predicate<formalism::FluentTag>>>(ground_context.builder);
                binding->relation = head_atom.get_predicate().get_index();
                formalism::datalog::ground(head_atom.get_terms(), ground_context, binding->objects);
                formalism::canonicalize(*binding);
                const auto head = intern ? std::optional(formalism::datalog::insert(ground_context.destination, *binding).first)
                                         : ground_context.destination.find(*binding);
                lookup.valid = true;
                lookup.objects = binding->objects;
                lookup.head = head ? std::optional(head->get_index()) : std::nullopt;
            }
            const auto head = lookup.head;
            if (!head)
            {
                missing = true;
                continue;
            }
            if (!AP::records_propositional_achievers && in.fact_sets().template get<formalism::FluentTag>().predicate.contains(*head))
                continue;
            heads.push_back(*head);
        }
        return missing;
    }

    /// Whether the terms ground to objects under binding. objects is a grounding of the same terms, so constants match.
    static bool grounds_to(formalism::datalog::TermListView terms,
                           const ygg::IndexList<formalism::Object>& binding,
                           const ygg::IndexList<formalism::Object>& objects)
    {
        auto position = size_t(0);
        for (const auto term : terms)
        {
            const auto matches = visit(
                [&]<typename Alternative>(Alternative arg)
                {
                    if constexpr (std::same_as<Alternative, formalism::ParameterIndex>)
                        return binding[ygg::uint_t(arg)] == objects[position];
                    else
                        return true;
                },
                term.get_variant());
            if (!matches)
                return false;
            ++position;
        }
        return true;
    }

    bool try_process_predicate_binding(RuleWorkerExecutionContext<formalism::PredicateTag, AP, TP, CP>& wrctx,
                                       RuleUpdateInput<LiftedTag, formalism::PredicateTag, AP, CP>& input,
                                       const ApplicabilityContext& applicability_context,
                                       bool literals_checked)
    {
        [[maybe_unused]] auto& in = wrctx.in();
        auto& out = wrctx.out();
        const auto missing = collect_predicate_heads(wrctx, false);
        if (!missing && out.head_bindings().empty())
            return true;

        if (!is_current_binding_dynamically_applicable(wrctx, applicability_context, literals_checked))
            return false;

        // Heads are interned only for applicable bindings.
        if (missing)
            collect_predicate_heads(wrctx, true);
        assert(is_applicable(in.cws_rule().get_rule(), applicability_context));
        const auto handled =
            insert_propositional_update(std::span(std::as_const(out.head_bindings())), input, out.head_updates(), out.delta_annotations()).is_handled();
        observe_goal_heads(stratum(), std::span(std::as_const(out.head_bindings())));
        return handled;
    }

    void process_clique_head(formalism::PredicateTag,
                             RuleWorkerExecutionContext<formalism::PredicateTag, AP, TP, CP>& wrctx,
                             RuleUpdateInput<LiftedTag, formalism::PredicateTag, AP, CP>& input,
                             const ApplicabilityContext& applicability_context,
                             bool literals_checked)
    {
        // A rejected binding is generated again by the delta that makes it applicable (see generate_hyperedge_cliques).
        // Bindings rejected by a numeric constraint are retried because its fluent reads trigger a full enumeration of the rule.
        if (!try_process_predicate_binding(wrctx, input, applicability_context, literals_checked))
            ++wrctx.out().statistics().num_rejected_rules;
    }

    void process_clique_head(formalism::FunctionTag,
                             RuleWorkerExecutionContext<formalism::FunctionTag, AP, TP, CP>& wrctx,
                             RuleUpdateInput<LiftedTag, formalism::FunctionTag, AP, CP>& input,
                             const ApplicabilityContext& applicability_context,
                             bool literals_checked)
    {
        [[maybe_unused]] const auto& in = wrctx.in();
        auto& out = wrctx.out();
        if (!is_current_binding_dynamically_applicable(wrctx, applicability_context, literals_checked))
            return;

        assert(is_applicable(in.cws_rule().get_rule(), ApplicabilityContext { in.fact_sets(), out.ground_context() }));
        insert_numeric_update(input, out.head_updates(), out.delta_numeric_annotations());
    }

    /// literals_checked: the clique was completed under the literal prune (make_literal_prune).
    template<formalism::RelationKind R>
    void process_clique(RuleWorkerExecutionContext<R, AP, TP, CP>& wrctx, std::span<const kckp::Vertex> clique, bool require_novel_binding, bool literals_checked)
    {
        create_general_binding(clique, wrctx.in().cws_rule().get_static_consistency_graph(), wrctx.out().ground_context().binding);
        process_binding(wrctx, require_novel_binding, literals_checked);
    }

    /// Processes the binding in the worker's ground context.
    template<formalism::RelationKind R>
    void process_binding(RuleWorkerExecutionContext<R, AP, TP, CP>& wrctx, [[maybe_unused]] bool require_novel_binding, bool literals_checked = false)
    {
        auto& in = wrctx.in();
        auto& out = wrctx.out();
        if (!in.cws_rule().get_numeric_constraint_fluent_terms().empty())
            out.rule_evaluation_workspace().constraint_cache = in.constraint_cache();
        auto input = make_rule_update_input(RuleInstance<LiftedTag, R> { in.cws_rule(), out.ground_context() },
                                            in.numeric_support_selector(),
                                            in.annotations(),
                                            out.rule_evaluation_workspace(),
                                            in.numeric_annotations(),
                                            in.annotation_policy(),
                                            in.cost_policy());

        ++out.statistics().num_generated_rules;

        const auto conflicting_condition = in.cws_rule().get_conflicting_overapproximation_condition();
        const auto applicability_context = ApplicabilityContext { in.fact_sets(), out.ground_context() };
        auto& applicability_cache = out.applicability_cache();

        const auto statically_applicable = [&]() {
            return applicability_cache.static_nullary
                   && (literals_checked || is_applicable(conflicting_condition.template get_literals<formalism::StaticTag>(), applicability_context));
        };

        if (!statically_applicable())
            return;

#ifndef NDEBUG
        // Only applicable bindings count: a rejected binding is generated again once it becomes applicable.
        // The check only fills the nullary cache with the value the gating in run_active_rules established.
        if (is_current_binding_dynamically_applicable(wrctx, applicability_context))
        {
            const auto novel_binding = ensure_novel_binding(out.ground_context().binding, out.seen_bindings());
            assert((!require_novel_binding || novel_binding) && "Delta-KCKP generated duplicate binding.");
        }
#endif

        process_clique_head(R {}, wrctx, input, applicability_context, literals_checked);
    }

    template<formalism::RelationKind R>
    void generate_general_case(RuleExecutionContext<R, AP, TP, CP>& rctx, bool enumerate_all)
    {
        auto& rule_out = rctx.out();
        auto& kckp_algorithm = rule_out.kckp();
#ifdef TYR_ENABLE_SEMI_NAIVE
        // Before the edge deltas, as the retried bindings were: equal-cost witnesses keep their first derivation.
        if constexpr (std::same_as<R, formalism::PredicateTag>)
            if (!enumerate_all)
                generate_hyperedge_cliques(rctx);
#endif
        if (enumerate_all || !try_generate_parallel(R {}, rctx))
        {
            auto wrctx = rctx.get_rule_worker_execution_context();
            auto& out = wrctx.out();
            auto& kckp_workspace = out.kckp_workspace();
            ++out.statistics().num_executions;

            for_each_relevant_clique(
                R {},
                kckp_algorithm,
                [&](auto&& clique)
                {
                    if (!goal_cutoff_stopped())
                        process_clique(wrctx, clique, require_novel_binding(R {}) && !enumerate_all, std::same_as<R, formalism::PredicateTag>);
                },
                kckp_workspace,
                enumerate_all,
                make_literal_prune(R {}, wrctx));
        }
    }

    /// Edge anchors generate every clique with a delta vertex or edge. A new atom of hyperedge literal i generates
    /// the cliques without any, provided no literal before i has a new atom, so each binding is generated once,
    /// in the iteration it becomes applicable. generate_general_case calls this before the edge anchors, the order
    /// in which the retried bindings were processed, so that equal-cost witnesses keep their first derivation.
    void generate_hyperedge_cliques(RuleExecutionContext<formalism::PredicateTag, AP, TP, CP>& rctx)
    {
        const auto& cws_rule = rctx.in().cws_rule();
        const auto literals = cws_rule.get_hyperedge_condition().template get_literals<formalism::FluentTag>();
        auto& kckp_algorithm = rctx.out().kckp();
        if (literals.empty() || kckp_algorithm.get_iteration() <= 1)
            return;

        const auto& graph = cws_rule.get_static_consistency_graph();
        const auto& infos = graph.get_hyperedge_indexed_literals().infos;
        const auto& delta_fact_sets = rctx.stratum_out().program().facts().delta_fact_sets;
        auto wrctx = rctx.get_rule_worker_execution_context();
        auto& out = wrctx.out();
        const auto delta_sets = FactSets { rctx.stratum_in().program().facts().fact_sets, delta_fact_sets };
        const auto delta_context = ApplicabilityContext { delta_sets, out.ground_context() };
        auto prune = make_literal_prune(formalism::PredicateTag {}, wrctx);

        const auto& delta_objects = rctx.stratum_out().program().facts().delta_objects;
        for (size_t i = 0; i < literals.size(); ++i)
        {
            const auto& objects = delta_objects[ygg::uint_t(infos[i].predicate)];
            const auto atom_arity = infos[i].position_mappings.arity;
            for (size_t begin = 0; begin < objects.size(); begin += atom_arity)
            {
                if (!match_anchor(graph.get_graph_layout(), infos[i], std::span(objects).subspan(begin, atom_arity), out.anchor_vertices()))
                    continue;

                kckp_algorithm.for_each_hyperedge_clique(
                    out.anchor_vertices(),
                    [&](auto&& clique)
                    {
                        if (goal_cutoff_stopped())
                            return;
                        create_general_binding(clique, graph, out.ground_context().binding);
                        for (size_t j = 0; j < i; ++j)
                            if (is_applicable(literals[j], delta_context))
                                return;  // An earlier literal's new atom anchors this clique.
                        process_binding(wrctx, true);
                    },
                    out.kckp_workspace(),
                    prune);
            }
        }
    }

    /// The vertices of the atom's objects at the literal's parameters, if the atom matches its constants and repeated parameters.
    /// vertices is a per-worker scratch buffer, reused across anchors to avoid allocations.
    static bool match_anchor(const kckp::GraphLayout& layout,
                             const details::RuleToLiteralInfo<formalism::FluentTag>& info,
                             std::span<const ygg::Index<formalism::Object>> objects,
                             std::vector<kckp::Vertex>& vertices)
    {
        for (const auto& [position, object] : info.position_mappings.constant_positions)
            if (objects[position] != object)
                return false;

        vertices.clear();
        const auto& parameter_to_positions = info.position_mappings.parameter_to_positions;
        for (ygg::uint_t parameter = 0; parameter < parameter_to_positions.size(); ++parameter)
        {
            const auto& positions = parameter_to_positions[parameter];
            if (positions.empty())
                continue;
            const auto object = objects[positions.front()];
            if (!std::ranges::all_of(positions, [&](const auto position) { return objects[position] == object; }))
                return false;
            const auto vertex = layout.find_vertex(parameter, ygg::uint_t(object));
            if (!vertex)
                return false;
            vertices.push_back(*vertex);
        }
        return true;
    }

    template<formalism::RelationKind R>
    void generate(RuleExecutionContext<R, AP, TP, CP>& rctx, bool enumerate_all)
    {
        const auto arity = rctx.in().cws_rule().get_rule().get_arity();

        if (arity == 0)
            generate_nullary_case(rctx);
        else
            generate_general_case(rctx, enumerate_all);
    }


    template<formalism::RelationKind R>
    /// Parallel phase: generate new ground witnesses for all active rules.
    void run_active_rules(StratumExecutionContext<AP, TP, CP>& ctx)
    {
        auto& program_out = ctx.out().program();
        const auto& rule_scheduler = ctx.out().scheduler().template get<R>();
        const auto& active_rules = rule_scheduler.get_active_rules();

        const auto program_stopwatch = ygg::StopwatchScope(program_out.statistics().parallel_time);

        const auto process_rule = [&](auto&& rule_index)
        {
            // Numeric effects and state-dependent costs are non-idempotent as the numeric envelope evolves.
            const auto enumerate_all = std::same_as<R, formalism::FunctionTag> || rule_scheduler.requires_full_enumeration(rule_index);
            auto rctx = ctx.get_rule_execution_context(rule_index);
            auto& rule_out = rctx.out();

            const auto total_time = ygg::StopwatchScope(rule_out.statistics().total_time);
            ++rule_out.statistics().num_executions;

            rctx.clear_iteration();  ///< Clear iteration before generate
            if (goal_cutoff_stopped())
                return;

            {
                const auto initialize_time = ygg::StopwatchScope(rule_out.statistics().initialize_time);

                const auto wrctx = rctx.get_rule_worker_execution_context();
                const auto& fact_sets = wrctx.in().fact_sets();
                const auto nullary_condition = rctx.in().cws_rule().get_nullary_condition();
                const auto static_nullary = is_applicable(nullary_condition.template get_literals<formalism::StaticTag>(), fact_sets);
                const auto dynamic_nullary = static_nullary && is_dynamically_applicable(nullary_condition, fact_sets);
                if (!dynamic_nullary && rctx.in().cws_rule().get_rule().get_arity() > 0)
                {
                    // No binding is applicable while the nullary condition fails. Its atoms are listened to, so the rule
                    // runs again once it holds, and the reset graph then enumerates all cliques.
                    rule_out.kckp().reset();
                    return;
                }

                rctx.initialize();  ///< Initialize before generate

                for (auto& worker : rule_out.workers())
                    worker.solve.applicability_cache.static_nullary = static_nullary;
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

    template<formalism::RelationKind R>
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
                if constexpr (std::same_as<R, formalism::PredicateTag>)
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
    bool m_goal_cutoff_enabled = false;
    bool m_goal_cutoff_stopped = false;
    Cost m_min_edge_cost = Cost(0);
    Cost m_goal_cutoff_bound = Cost(0);
    GoalCutoffWorkspace& m_goal;
    size_t m_num_final_goals = 0;
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

template void execute_model(ProgramExecutionContext<LiftedTag, NoAnnotationPolicy, NoTerminationPolicy>& ctx, ygg::ExecutionContext& execution_context);
template void execute_model(ProgramExecutionContext<LiftedTag, MinCostAnnotationPolicy<SumAggregation>, NoTerminationPolicy>& ctx, ygg::ExecutionContext& execution_context);
template void execute_model(ProgramExecutionContext<LiftedTag, MinCostAnnotationPolicy<SumAggregation>, TerminationPolicy<SumAggregation>>& ctx, ygg::ExecutionContext& execution_context);
template void execute_model(ProgramExecutionContext<LiftedTag, MinCostAnnotationPolicy<MaxAggregation>, NoTerminationPolicy>& ctx, ygg::ExecutionContext& execution_context);
template void execute_model(ProgramExecutionContext<LiftedTag, MinCostAnnotationPolicy<MaxAggregation>, TerminationPolicy<MaxAggregation>>& ctx, ygg::ExecutionContext& execution_context);
template void execute_model(ProgramExecutionContext<LiftedTag, MinCostAnnotationWithAchieversPolicy<MaxAggregation>, TerminationPolicy<MaxAggregation>>& ctx, ygg::ExecutionContext& execution_context);
template void
execute_model(ProgramExecutionContext<LiftedTag, MinCostAnnotationPolicy<SumAggregation>, NoTerminationPolicy, RuleCostOverridePolicy<LiftedTag>>& ctx, ygg::ExecutionContext& execution_context);
template void execute_model(
    ProgramExecutionContext<LiftedTag, MinCostAnnotationPolicy<SumAggregation>, TerminationPolicy<SumAggregation>, RuleCostOverridePolicy<LiftedTag>>& ctx, ygg::ExecutionContext& execution_context);
template void
execute_model(ProgramExecutionContext<LiftedTag, MinCostAnnotationPolicy<MaxAggregation>, NoTerminationPolicy, RuleCostOverridePolicy<LiftedTag>>& ctx, ygg::ExecutionContext& execution_context);
template void execute_model(
    ProgramExecutionContext<LiftedTag, MinCostAnnotationPolicy<MaxAggregation>, TerminationPolicy<MaxAggregation>, RuleCostOverridePolicy<LiftedTag>>& ctx, ygg::ExecutionContext& execution_context);
template void execute_model(ProgramExecutionContext<LiftedTag,
                                                    MinCostAnnotationWithAchieversPolicy<MaxAggregation>,
                                                    TerminationPolicy<MaxAggregation>,
                                                    RuleCostOverridePolicy<LiftedTag>>& ctx, ygg::ExecutionContext& execution_context);
}
