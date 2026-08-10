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

#include "tyr/datalog/lifted/solver.hpp"

#include "tyr/algorithms/kckp/delta_kckp.hpp"
#include "tyr/datalog/applicability.hpp"
#include "tyr/datalog/applicability_lifted.hpp"
#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/formatter.hpp"
#include "tyr/datalog/lifted/assignment_sets.hpp"
#include "tyr/datalog/lifted/consistency_graph.hpp"
#include "tyr/datalog/lifted/policies/annotation.hpp"
#include "tyr/datalog/lifted/policies/numeric_support.hpp"
#include "tyr/datalog/lifted/rule_scheduler.hpp"
#include "tyr/datalog/lifted/workspaces/facts.hpp"
#include "tyr/datalog/lifted/workspaces/program.hpp"
#include "tyr/datalog/lifted/workspaces/rule.hpp"
#include "tyr/datalog/numeric_utils.hpp"
#include "tyr/datalog/policies/aggregation.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/declarations.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/grounder.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"

#include <algorithm>
#include <assert.h>
#include <memory>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/parallel_for_each.h>
#include <oneapi/tbb/task_arena.h>
#include <optional>
#include <span>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/chrono.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/formatting/formatter.hpp>
#include <yggdrasil/semantics/comparators.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

namespace tyr::datalog
{

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

template<f::RelationKind R, AnnotationPolicyConcept<LiftedTag> AP, RuleCostPolicyConcept<LiftedTag> CP>
struct RuleUpdateInput
{
    fd::RuleView<R> rule;
    Cost pre_evaluated_metric_cost;
    std::span<const fd::NumericEffectOperatorView<f::FluentTag>> runtime_metric_effects;
    fd::ConjunctiveConditionView witness_condition;
    const NumericSupportSelector<LiftedTag>& numeric_support_selector;
    NumericSupportSelectorWorkspace<LiftedTag>& numeric_support_selector_workspace;
    std::vector<NumericSupport<LiftedTag>>& effect_support_scratch;
    std::vector<NumericSupport<LiftedTag>>& witness_support_scratch;
    Cost current_cost;
    const PredicateAnnotations<LiftedTag>& annotations;
    const FunctionAnnotations<LiftedTag>& numeric_annotations;
    const FactSets& fact_sets;
    AP& annotation_policy;
    const CP& cost_policy;
    fd::GrounderContext& ground_context;

    AnnotationContext<LiftedTag, R> make_annotation_context(std::optional<fd::RuleBindingView<R>> rule_binding,
                                                            Cost metric_effect_cost,
                                                            std::span<const NumericSupport<LiftedTag>> numeric_supports = {}) const
    {
        return AnnotationContext<LiftedTag, R> { current_cost,
                                                 numeric_supports,
                                                 witness_support_scratch,
                                                 rule,
                                                 rule_binding,
                                                 metric_effect_cost,
                                                 witness_condition,
                                                 numeric_support_selector,
                                                 numeric_support_selector_workspace,
                                                 annotations,
                                                 numeric_annotations,
                                                 ground_context };
    }
};

template<f::RelationKind R, typename In, typename Out>
static auto make_rule_update_input(In& in, Out& out)
{
    return RuleUpdateInput<R, std::decay_t<decltype(in.annotation_policy())>, std::decay_t<decltype(in.cost_policy())>> {
        in.cws_rule().get_rule(),
        in.cws_rule().get_pre_evaluated_metric_cost(),
        in.cws_rule().get_runtime_metric_effects(),
        in.cws_rule().get_witness_rule().get_body(),
        in.numeric_support_selector(),
        out.numeric_support_selector_workspace(),
        out.effect_support_scratch(),
        out.witness_support_scratch(),
        in.cost_buckets().current_cost(),
        in.annotations(),
        in.numeric_annotations(),
        in.fact_sets(),
        in.annotation_policy(),
        in.cost_policy(),
        out.ground_context()
    };
}

template<f::RelationKind R, AnnotationPolicyConcept<LiftedTag> AP, RuleCostPolicyConcept<LiftedTag> CP>
std::optional<Cost> metric_effect_delta(fd::NumericEffectView<f::FluentTag> effect, const RuleUpdateInput<R, AP, CP>& input)
{
    const auto context = ApplicabilityContext { input.fact_sets, input.ground_context };
    return metric_effect_delta(
        effect.get_operator(),
        [&] { return evaluate(effect.get_fterm(), context); },
        [&] { return evaluate(effect.get_fexpr(), context); });
}

static void append_numeric_supports(const std::vector<NumericSupportSelectorWorkspace<LiftedTag>::SelectionEntry>& selection,
                                    std::vector<NumericSupport<LiftedTag>>& supports)
{
    for (const auto& entry : selection)
        supports.push_back(NumericSupport<LiftedTag> { entry.key, entry.interval, entry.cost });
}

template<f::RelationKind R, AnnotationPolicyConcept<LiftedTag> AP, RuleCostPolicyConcept<LiftedTag> CP>
static bool collect_expression_supports(fd::FunctionExpressionView expression,
                                        const RuleUpdateInput<R, AP, CP>& input,
                                        std::vector<NumericSupport<LiftedTag>>& supports,
                                        std::vector<NumericSupportSelectorWorkspace<LiftedTag>::SelectionEntry>& selection)
{
    const auto ground_expression = fd::ground(expression, input.ground_context);
    const auto value = input.numeric_support_selector.evaluate_effect_expression(ground_expression, selection);
    if (empty(value))
        return false;

    append_numeric_supports(selection, supports);
    return true;
}

template<f::RelationKind R, AnnotationPolicyConcept<LiftedTag> AP, RuleCostPolicyConcept<LiftedTag> CP>
static bool collect_numeric_head_supports(fd::NumericEffectView<f::FluentTag> effect,
                                          fd::FunctionBindingView<f::FluentTag> head,
                                          const RuleUpdateInput<R, AP, CP>& input,
                                          std::vector<NumericSupport<LiftedTag>>& supports)
{
    auto& selection = input.numeric_support_selector_workspace.selection;
    selection.clear();

    if (effect.get_operator() != f::NumericEffectOperatorKind::Assign)
    {
        if (empty(input.numeric_support_selector.select_fluent_interval(head, selection)))
            return false;
    }

    return collect_expression_supports(effect.get_fexpr(), input, supports, selection);
}

template<f::RelationKind R, AnnotationPolicyConcept<LiftedTag> AP, RuleCostPolicyConcept<LiftedTag> CP>
static bool collect_metric_effect_supports(fd::NumericEffectView<f::FluentTag> effect,
                                           const RuleUpdateInput<R, AP, CP>& input,
                                           std::vector<NumericSupport<LiftedTag>>& supports)
{
    auto& selection = input.numeric_support_selector_workspace.selection;
    selection.clear();

    if (effect.get_operator() != f::NumericEffectOperatorKind::Increase && effect.get_operator() != f::NumericEffectOperatorKind::Decrease)
    {
        const auto binding = fd::ground_binding(effect.get_fterm(), input.ground_context).first;
        if (empty(input.numeric_support_selector.select_fluent_interval(binding, selection)))
            return false;
    }

    return collect_expression_supports(effect.get_fexpr(), input, supports, selection);
}

template<f::RelationKind R, AnnotationPolicyConcept<LiftedTag> AP, RuleCostPolicyConcept<LiftedTag> CP>
static bool collect_metric_effect_supports(const RuleUpdateInput<R, AP, CP>& input, std::vector<NumericSupport<LiftedTag>>& supports)
{
    for (const auto& metric_effect : input.runtime_metric_effects)
        if (!ygg::visit([&](auto&& effect) { return collect_metric_effect_supports(effect, input, supports); }, metric_effect.get_variant()))
            return false;
    return true;
}

template<f::RelationKind R, AnnotationPolicyConcept<LiftedTag> AP, RuleCostPolicyConcept<LiftedTag> CP>
std::optional<Cost> metric_effect_delta(const RuleUpdateInput<R, AP, CP>& input)
{
    auto delta = input.pre_evaluated_metric_cost;
    for (const auto& metric_effect : input.runtime_metric_effects)
    {
        const auto effect_delta = ygg::visit([&](auto&& effect) { return metric_effect_delta(effect, input); }, metric_effect.get_variant());
        if (!effect_delta)
            return std::nullopt;
        delta += *effect_delta;
    }

    return delta;
}

template<f::RelationKind R, AnnotationPolicyConcept<LiftedTag> AP, RuleCostPolicyConcept<LiftedTag> CP>
std::optional<Cost> metric_effect_cost(fd::RuleBindingView<R> rule_binding, const RuleUpdateInput<R, AP, CP>& input)
{
    const auto delta = metric_effect_delta(input);
    return delta ? std::optional(reduce_cost(*delta, input.cost_policy.get_cost(rule_binding))) : std::nullopt;
}

template<AnnotationPolicyConcept<LiftedTag> AP, RuleCostPolicyConcept<LiftedTag> CP>
static void record_propositional_achiever(fd::PredicateBindingView<f::FluentTag> head, const RuleUpdateInput<f::PredicateTag, AP, CP>& input)
    requires AP::records_propositional_achievers
{
    const auto rule_binding = fd::ground_binding(input.rule, input.ground_context).first;
    const auto cost = metric_effect_cost(rule_binding, input);
    if (!cost)
        return;
    auto& numeric_supports = input.effect_support_scratch;
    numeric_supports.clear();
    if (!collect_metric_effect_supports(input, numeric_supports))
        return;
    const auto context = input.make_annotation_context(rule_binding, *cost, numeric_supports);

    input.annotation_policy.record_achiever(head, context);
}

template<AnnotationPolicyConcept<LiftedTag> AP, RuleCostPolicyConcept<LiftedTag> CP>
static void insert_propositional_update(fd::PredicateBindingView<f::FluentTag> head,
                                        const RuleUpdateInput<f::PredicateTag, AP, CP>& input,
                                        PredicateHeadIteration& head_iteration,
                                        DeltaPredicateAnnotations<LiftedTag>& delta_annotations)
{
    auto rule_binding = std::optional<fd::RuleBindingView<f::PredicateTag>> {};
    auto cost = std::optional<Cost> {};
    if constexpr (std::same_as<CP, RuleCostPolicy<LiftedTag>> && !AP::records_propositional_achievers)
    {
        cost = metric_effect_delta(input);
    }
    else
    {
        rule_binding = fd::ground_binding(input.rule, input.ground_context).first;
        cost = metric_effect_cost(*rule_binding, input);
    }
    if (!cost)
        return;
    auto& numeric_supports = input.effect_support_scratch;
    numeric_supports.clear();
    if (!collect_metric_effect_supports(input, numeric_supports))
        return;
    const auto context = input.make_annotation_context(rule_binding, *cost, numeric_supports);

    input.annotation_policy.record_achiever(head, context);

    head_iteration.insert(head);

    input.annotation_policy.update_annotation(head, context, delta_annotations);
}

template<AnnotationPolicyConcept<LiftedTag> AP, RuleCostPolicyConcept<LiftedTag> CP>
static void insert_numeric_update(fd::NumericEffectOperatorView<f::FluentTag> head,
                                  const FactSets& fact_sets,
                                  const RuleUpdateInput<f::FunctionTag, AP, CP>& input,
                                  FunctionHeadIteration& head_iteration,
                                  DeltaFunctionAnnotations<LiftedTag>& delta_numeric_annotations)
{
    const auto interval = evaluate(head, ApplicabilityContext { fact_sets, input.ground_context });
    if (empty(interval))
        return;

    visit(
        [&](auto&& effect)
        {
            const auto head = fd::ground_binding(effect.get_fterm(), input.ground_context).first;
            const auto rule_binding = fd::ground_binding(input.rule, input.ground_context).first;
            const auto rem_rule_cost = metric_effect_cost(rule_binding, input);
            if (!rem_rule_cost)
                return;

            const auto effect_interval = *rem_rule_cost == Cost(0) ? widen_free_growth(interval, fact_sets.get<f::FluentTag>().function[head]) : interval;

            const auto cost = reduce_cost(*rem_rule_cost, input.cost_policy.get_cost(rule_binding, head, effect_interval));
            auto& numeric_supports = input.effect_support_scratch;
            numeric_supports.clear();
            if (!collect_metric_effect_supports(input, numeric_supports) || !collect_numeric_head_supports(effect, head, input, numeric_supports))
                return;
            const auto context = input.make_annotation_context(rule_binding, cost, numeric_supports);

            input.annotation_policy.update_annotation(head, effect_interval, context, delta_numeric_annotations);

            head_iteration.insert(FunctionHeadUpdate(head, effect_interval, input.current_cost + cost));
        },
        head.get_variant());
}

template<typename In, typename Out, AnnotationPolicyConcept<LiftedTag> AP, RuleCostPolicyConcept<LiftedTag> CP>
void insert_nullary_update(fd::AtomView<f::FluentTag> head_atom, const RuleUpdateInput<f::PredicateTag, AP, CP>& input, const In&, Out& out)
{
    const auto head = fd::ground_binding(head_atom, out.ground_context()).first;
    insert_propositional_update(head, input, out.head_updates(), out.delta_annotations());
}

template<typename In, typename Out, AnnotationPolicyConcept<LiftedTag> AP, RuleCostPolicyConcept<LiftedTag> CP>
void insert_nullary_update(fd::NumericEffectOperatorView<f::FluentTag> head, const RuleUpdateInput<f::FunctionTag, AP, CP>& input, const In& in, Out& out)
{
    assert(is_applicable(in.cws_rule().get_rule(), ApplicabilityContext { in.fact_sets(), out.ground_context() }));
    insert_numeric_update(head, in.fact_sets(), input, out.head_updates(), out.delta_numeric_annotations());
}

template<f::RelationKind R, AnnotationPolicyConcept<LiftedTag> AP, TerminationPolicyConcept<LiftedTag> TP, RuleCostPolicyConcept<LiftedTag> CP>
void generate_nullary_case(RuleExecutionContext<R, AP, TP, CP>& rctx)
{
    auto wrctx = rctx.get_rule_worker_execution_context();

    auto& in = wrctx.in();
    auto& out = wrctx.out();
    const auto input = make_rule_update_input<R>(in, out);
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
void for_each_relevant_clique(fd::AtomView<f::FluentTag>, kckp::DeltaKCKP& algorithm, Callback&& callback, kckp::Workspace& workspace)
{
#ifdef TYR_ENABLE_SEMI_NAIVE
    algorithm.for_each_delta_clique(std::forward<Callback>(callback), workspace);
#else
    algorithm.for_each_clique(std::forward<Callback>(callback), workspace);
#endif
}

template<typename Callback>
void for_each_relevant_clique(fd::NumericEffectOperatorView<f::FluentTag>, kckp::DeltaKCKP& algorithm, Callback&& callback, kckp::Workspace& workspace)
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

template<AnnotationPolicyConcept<LiftedTag> AP, TerminationPolicyConcept<LiftedTag> TP, RuleCostPolicyConcept<LiftedTag> CP>
bool try_generate_parallel(fd::AtomView<f::FluentTag>, [[maybe_unused]] RuleExecutionContext<f::PredicateTag, AP, TP, CP>& rctx)
{
#if defined(TYR_ENABLE_INNER_PARALLELISM) && defined(TYR_ENABLE_SEMI_NAIVE)
    constexpr size_t kNumStripes = 2;
    constexpr size_t kParallelThreshold = 1024;
    auto& kckp_algorithm = rctx.out().kckp();
    if (kckp_algorithm.get_iteration() > 1 && rctx.in().cws_rule().get_rule().get_arity() > 2 && oneapi::tbb::this_task_arena::max_concurrency() >= 2)
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

template<AnnotationPolicyConcept<LiftedTag> AP, TerminationPolicyConcept<LiftedTag> TP, RuleCostPolicyConcept<LiftedTag> CP>
bool try_generate_parallel(fd::NumericEffectOperatorView<f::FluentTag>, RuleExecutionContext<f::FunctionTag, AP, TP, CP>&)
{
    return false;
}

template<AnnotationPolicyConcept<LiftedTag> AP, TerminationPolicyConcept<LiftedTag> TP, RuleCostPolicyConcept<LiftedTag> CP, typename DynamicallyApplicable>
void process_clique_head(fd::AtomView<f::FluentTag> head_atom,
                         RuleWorkerExecutionContext<f::PredicateTag, AP, TP, CP>& wrctx,
                         const RuleUpdateInput<f::PredicateTag, AP, CP>& input,
                         DynamicallyApplicable&& dynamically_applicable)
{
    auto& in = wrctx.in();
    auto& out = wrctx.out();
    auto head = fd::try_ground_binding(head_atom, out.ground_context());
    if (head && in.fact_sets().template get<f::FluentTag>().predicate.contains(*head))
    {
        if constexpr (AP::records_propositional_achievers)
            if (dynamically_applicable())
                record_propositional_achiever(*head, input);
        return;
    }

    if (!dynamically_applicable())
    {
        ++out.statistics().num_pending_rules;
        const auto rule_binding = fd::ground_binding(in.cws_rule().get_conflicting_overapproximation_rule(), out.ground_context()).first;
#ifdef TYR_ENABLE_SEMI_NAIVE
        out.pending_rule_bindings().push_back(rule_binding);
#else
        if (out.seen_pending_rule_bindings().emplace(rule_binding).second)
            out.pending_rule_bindings().push_back(rule_binding);
#endif
        return;
    }

    if (!head)
        head = fd::ground_binding(head_atom, out.ground_context()).first;
    assert(is_applicable(in.cws_rule().get_rule(), ApplicabilityContext { in.fact_sets(), out.ground_context() }));
    insert_propositional_update(*head, input, out.head_updates(), out.delta_annotations());
}

template<AnnotationPolicyConcept<LiftedTag> AP, TerminationPolicyConcept<LiftedTag> TP, RuleCostPolicyConcept<LiftedTag> CP, typename DynamicallyApplicable>
void process_clique_head(fd::NumericEffectOperatorView<f::FluentTag> head,
                         RuleWorkerExecutionContext<f::FunctionTag, AP, TP, CP>& wrctx,
                         const RuleUpdateInput<f::FunctionTag, AP, CP>& input,
                         DynamicallyApplicable&& dynamically_applicable)
{
    const auto& in = wrctx.in();
    auto& out = wrctx.out();
    if (!dynamically_applicable())
        return;

    assert(is_applicable(in.cws_rule().get_rule(), ApplicabilityContext { in.fact_sets(), out.ground_context() }));
    insert_numeric_update(head, in.fact_sets(), input, out.head_updates(), out.delta_numeric_annotations());
}

template<f::RelationKind R, AnnotationPolicyConcept<LiftedTag> AP, TerminationPolicyConcept<LiftedTag> TP, RuleCostPolicyConcept<LiftedTag> CP>
void process_clique(RuleWorkerExecutionContext<R, AP, TP, CP>& wrctx, std::span<const kckp::Vertex> clique, [[maybe_unused]] bool require_novel_binding)
{
    auto& in = wrctx.in();
    auto& out = wrctx.out();
    const auto input = make_rule_update_input<R>(in, out);

    create_general_binding(clique, in.cws_rule().get_static_consistency_graph(), out.ground_context().binding);

    assert((!require_novel_binding || ensure_novel_binding(out.ground_context().binding, out.seen_bindings())) && "Delta-KCKP generated duplicate binding.");

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

template<f::RelationKind R, AnnotationPolicyConcept<LiftedTag> AP, TerminationPolicyConcept<LiftedTag> TP, RuleCostPolicyConcept<LiftedTag> CP>
void generate_general_case(RuleExecutionContext<R, AP, TP, CP>& rctx)
{
    auto& rule_out = rctx.out();
    auto& kckp_algorithm = rule_out.kckp();
    const auto head = rctx.in().cws_rule().get_rule().get_head();
    if (try_generate_parallel(head, rctx))
        return;

    auto wrctx = rctx.get_rule_worker_execution_context();
    auto& out = wrctx.out();
    auto& kckp_workspace = out.kckp_workspace();
    ++out.statistics().num_executions;

    for_each_relevant_clique(head, kckp_algorithm, [&](auto&& clique) { process_clique(wrctx, clique, require_novel_binding(head)); }, kckp_workspace);
}

template<f::RelationKind R, AnnotationPolicyConcept<LiftedTag> AP, TerminationPolicyConcept<LiftedTag> TP, RuleCostPolicyConcept<LiftedTag> CP>
void generate(RuleExecutionContext<R, AP, TP, CP>& rctx)
{
    const auto arity = rctx.in().cws_rule().get_rule().get_arity();

    if (arity == 0)
        generate_nullary_case(rctx);
    else
        generate_general_case(rctx);
}

template<AnnotationPolicyConcept<LiftedTag> AP, TerminationPolicyConcept<LiftedTag> TP, RuleCostPolicyConcept<LiftedTag> CP>
void process_pending_rule_bindings(RuleExecutionContext<f::PredicateTag, AP, TP, CP>& rctx)
{
    for (auto& worker : rctx.out().workers())
    {
        auto wrctx = RuleWorkerExecutionContext(rctx, worker);

        auto& in = wrctx.in();
        auto& out = wrctx.out();
        const auto input = make_rule_update_input<f::PredicateTag>(in, out);
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

                          assert(is_applicable(conflicting_condition.template get_literals<f::StaticTag>(), applicability_context));

                          auto head = fd::try_ground_binding(in.cws_rule().get_rule().get_head(), out.ground_context());
                          if (head && in.fact_sets().template get<f::FluentTag>().predicate.contains(*head))
                          {
                              if constexpr (AP::records_propositional_achievers)
                              {
                                  if (!dynamically_applicable())
                                      return false;

                                  assert(is_applicable(in.cws_rule().get_rule(), applicability_context));
                                  record_propositional_achiever(*head, input);
                              }
                              return true;
                          }

                          if (!dynamically_applicable())
                              return false;

                          if (!head)
                              head = fd::ground_binding(in.cws_rule().get_rule().get_head(), out.ground_context()).first;
                          assert(is_applicable(in.cws_rule().get_rule(), applicability_context));
                          insert_propositional_update(*head, input, out.head_updates(), out.delta_annotations());
                          return true;
                      });
    }
}

template<AnnotationPolicyConcept<LiftedTag> AP, TerminationPolicyConcept<LiftedTag> TP, RuleCostPolicyConcept<LiftedTag> CP>
void process_pending_rule_bindings(RuleExecutionContext<f::FunctionTag, AP, TP, CP>&)
{
}

template<f::RelationKind R, AnnotationPolicyConcept<LiftedTag> AP, TerminationPolicyConcept<LiftedTag> TP, RuleCostPolicyConcept<LiftedTag> CP>
/// Parallel phase: recheck pending rule bindings and generate new ground witnesses for all active rules.
void run_active_rules(StratumExecutionContext<AP, TP, CP>& ctx)
{
    auto& program_out = ctx.out().program();
    const auto& active_rules = ctx.out().scheduler().template get<R>().get_active_rules();

    const auto program_stopwatch = ygg::StopwatchScope(program_out.statistics().parallel_time);

    const auto process_rule = [&](auto&& rule_index)
    {
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

            process_pending_rule_bindings(rctx);
        }

        {
            const auto process_generate_time = ygg::StopwatchScope(rule_out.statistics().process_generate_time);

            generate(rctx);
        }
    };

    // With a single-threaded arena, run sequentially to pin the execution order to the scheduler's
    // sorted rule order (TBB task order is unspecified even at concurrency 1).
    if (oneapi::tbb::this_task_arena::max_concurrency() == 1)
    {
        for (const auto rule_index : active_rules)
            process_rule(rule_index);
    }
    else
    {
        oneapi::tbb::parallel_for_each(active_rules.begin(), active_rules.end(), process_rule);
    }
}

template<typename ProgramOut>
void reduce_worker_heads(PredicateHeadIteration& head_iteration, ProgramOut& program_out, CostBuckets& cost_buckets)
{
    for (const auto head : head_iteration.bindings)
    {
        const auto cost_update = program_out.annotation_policy().update_annotation(head, program_out.delta_annotations(), program_out.annotations());
        cost_buckets.update(cost_update, head);
    }
}

template<typename ProgramOut>
void reduce_worker_heads(FunctionHeadIteration& head_iteration, ProgramOut& program_out, CostBuckets& cost_buckets)
{
    for (const auto& update : head_iteration.updates)
    {
        if (const auto* annotation = program_out.delta_numeric_annotations().find(update.binding, update.interval))
            program_out.numeric_annotations().insert(update.binding, update.interval, *annotation);
        cost_buckets.insert(update.cost, update.binding, update.interval);
    }
}

template<f::RelationKind R, AnnotationPolicyConcept<LiftedTag> AP, TerminationPolicyConcept<LiftedTag> TP, RuleCostPolicyConcept<LiftedTag> CP>
/// Sequential phase: reduce worker annotations and bucket their already-canonical heads by cost.
void reduce_worker_results(StratumExecutionContext<AP, TP, CP>& ctx)
{
    auto& program_out = ctx.out().program();
    auto& cost_buckets = program_out.cost_buckets();

    for (const auto rule_index : ctx.out().scheduler().template get<R>().get_active_rules())
    {
        const auto i = ygg::uint_t(rule_index);
        const auto& ws_rule = program_out.template get_rules<R>()[i];

        for (auto& worker : ws_rule->worker)
            reduce_worker_heads(worker.iteration.head_updates, program_out, cost_buckets);
    }
}

template<AnnotationPolicyConcept<LiftedTag> AP, TerminationPolicyConcept<LiftedTag> TP, RuleCostPolicyConcept<LiftedTag> CP>
/// Commit the cheapest bucket: insert its heads into the fact and assignment sets and notify the scheduler.
bool commit_current_bucket(StratumExecutionContext<AP, TP, CP>& ctx)
{
    auto& program_out = ctx.out().program();
    auto& scheduler = ctx.out().scheduler();
    auto& facts = program_out.facts();
    auto& cost_buckets = program_out.cost_buckets();
    auto changed = false;

    // Native bucket order avoids sorting overhead. It is deterministic for single-threaded
    // execution; equal-cost order under parallel execution is intentionally unspecified.
    for (const auto head : cost_buckets.get_current_bucket())
    {
        if (facts.fact_sets.predicate.insert(head))
        {
            changed = true;
            scheduler.on_generate(head.get_index().relation);
            facts.assignment_sets.predicate.insert(head);
        }
    }

    for (const auto& [head, interval] : cost_buckets.get_current_function_bucket())
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

template<AnnotationPolicyConcept<LiftedTag> AP, TerminationPolicyConcept<LiftedTag> TP, RuleCostPolicyConcept<LiftedTag> CP>
void compute_model_for_stratum(StratumExecutionContext<AP, TP, CP>& ctx)
{
    auto& out = ctx.out();
    auto& scheduler = out.scheduler();
    auto& program_out = out.program();
    auto& cost_buckets = program_out.cost_buckets();

    scheduler.activate_all();
    cost_buckets.clear();

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

        cost_buckets.clear_current();  ///< Clear before reducing to avoid handling current heads twice.
        reduce_worker_results<f::PredicateTag>(ctx);
        reduce_worker_results<f::FunctionTag>(ctx);

#ifdef TYR_ENABLE_SEMI_NAIVE
        if (!cost_buckets.advance_to_next_nonempty())
            return;  ///< All reachable heads are committed.
        commit_current_bucket(ctx);
        scheduler.on_finish_iteration();
#else
        auto changed = false;
        while (cost_buckets.advance_to_next_nonempty())
        {
            if (commit_current_bucket(ctx))
            {
                changed = true;
                break;
            }
            cost_buckets.clear_current();
        }
        if (!changed)
            return;  ///< All reachable heads are committed.
        scheduler.activate_all();
#endif
    }
}

template<AnnotationPolicyConcept<LiftedTag> AP, TerminationPolicyConcept<LiftedTag> TP, RuleCostPolicyConcept<LiftedTag> CP>
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

template void compute_model(ProgramExecutionContext<LiftedTag, NoAnnotationPolicy<LiftedTag>, NoTerminationPolicy<LiftedTag>>& ctx);
template void compute_model(ProgramExecutionContext<LiftedTag, MinCostAnnotationPolicy<LiftedTag, SumAggregation>, NoTerminationPolicy<LiftedTag>>& ctx);
template void
compute_model(ProgramExecutionContext<LiftedTag, MinCostAnnotationPolicy<LiftedTag, SumAggregation>, TerminationPolicy<LiftedTag, SumAggregation>>& ctx);
template void compute_model(ProgramExecutionContext<LiftedTag, MinCostAnnotationPolicy<LiftedTag, MaxAggregation>, NoTerminationPolicy<LiftedTag>>& ctx);
template void
compute_model(ProgramExecutionContext<LiftedTag, MinCostAnnotationPolicy<LiftedTag, MaxAggregation>, TerminationPolicy<LiftedTag, MaxAggregation>>& ctx);
template void
compute_model(ProgramExecutionContext<LiftedTag, NoAnnotationPolicy<LiftedTag>, NoTerminationPolicy<LiftedTag>, RuleCostOverridePolicy<LiftedTag>>& ctx);
template void compute_model(
    ProgramExecutionContext<LiftedTag, MinCostAnnotationPolicy<LiftedTag, SumAggregation>, NoTerminationPolicy<LiftedTag>, RuleCostOverridePolicy<LiftedTag>>&
        ctx);
template void compute_model(ProgramExecutionContext<LiftedTag,
                                                    MinCostAnnotationPolicy<LiftedTag, SumAggregation>,
                                                    TerminationPolicy<LiftedTag, SumAggregation>,
                                                    RuleCostOverridePolicy<LiftedTag>>& ctx);
template void compute_model(
    ProgramExecutionContext<LiftedTag, MinCostAnnotationPolicy<LiftedTag, MaxAggregation>, NoTerminationPolicy<LiftedTag>, RuleCostOverridePolicy<LiftedTag>>&
        ctx);
template void compute_model(ProgramExecutionContext<LiftedTag,
                                                    MinCostAnnotationPolicy<LiftedTag, MaxAggregation>,
                                                    TerminationPolicy<LiftedTag, MaxAggregation>,
                                                    RuleCostOverridePolicy<LiftedTag>>& ctx);
template void compute_model(ProgramExecutionContext<LiftedTag,
                                                    MinCostAnnotationWithAchieversPolicy<LiftedTag, MaxAggregation>,
                                                    TerminationPolicy<LiftedTag, MaxAggregation>,
                                                    RuleCostOverridePolicy<LiftedTag>>& ctx);
}
