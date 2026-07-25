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

#include "tyr/datalog/lifted/bottom_up.hpp"

#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/formatter.hpp"
#include "tyr/datalog/lifted/applicability.hpp"
#include "tyr/datalog/lifted/assignment_sets.hpp"
#include "tyr/datalog/lifted/consistency_graph.hpp"
#include "tyr/datalog/lifted/delta_kpkc.hpp"
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
#include "tyr/formalism/datalog/merge.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"

#include <algorithm>
#include <assert.h>
#include <fmt/ostream.h>
#include <iostream>
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

template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
constexpr bool records_propositional_achievers =
    requires(const AndAP& and_ap, ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> head) { and_ap.find_achievers(head); };

static void create_nullary_binding(ygg::IndexList<f::Object>& binding) { binding.clear(); }

static void create_general_binding(std::span<const kpkc::Vertex> clique, const StaticConsistencyGraph& consistency_graph, ygg::IndexList<f::Object>& binding)
{
    const auto k = clique.size();

    binding.resize(k);

    for (ygg::uint_t p = 0; p < k; ++p)
    {
        const auto& vertex = consistency_graph.get_vertex(clique[p].index);
        assert(ygg::uint_t(vertex.get_parameter_index()) == p);

        binding[p] = vertex.get_object_index();
    }
}

template<AndAnnotationPolicyConcept<LiftedTag> AndAP, RuleCostPolicyConcept<LiftedTag> CP>
struct RuleUpdateInput
{
    fd::RuleView rule;
    fd::ConjunctiveConditionView witness_condition;
    const NumericSupportSelector<LiftedTag>& numeric_support_selector;
    NumericSupportSelectorWorkspace<LiftedTag>& numeric_support_selector_workspace;
    std::vector<NumericSupport<LiftedTag>>& numeric_support_scratch;
    std::vector<NumericSupport<LiftedTag>>& witness_support_scratch;
    Cost current_cost;
    const SelectedPredicateAnnotations<LiftedTag>& program_and_annot;
    const SelectedFunctionAnnotations<LiftedTag>& program_numeric_and_annot;
    const FactSets& fact_sets;
    const AndAP& and_ap;
    const CP& cost_policy;
    fd::GrounderContext& solve_context;
    fd::GrounderContext& iteration_context;

    AndAnnotationContext<LiftedTag> make_annotation_context(std::optional<fd::RuleBindingView> rule_binding,
                                                            Cost metric_effect_cost,
                                                            std::span<const NumericSupport<LiftedTag>> numeric_supports = {}) const
    {
        return AndAnnotationContext<LiftedTag> { current_cost,
                                                 numeric_supports,
                                                 witness_support_scratch,
                                                 rule,
                                                 rule_binding,
                                                 metric_effect_cost,
                                                 witness_condition,
                                                 numeric_support_selector,
                                                 numeric_support_selector_workspace,
                                                 program_and_annot,
                                                 program_numeric_and_annot,
                                                 solve_context,
                                                 iteration_context };
    }
};

template<typename In, typename Out>
static auto make_rule_update_input(const In& in, Out& out, const NumericSupportSelector<LiftedTag>& numeric_support_selector)
{
    return RuleUpdateInput<std::decay_t<decltype(in.and_ap())>, std::decay_t<decltype(in.cost_policy())>> { in.cws_rule().get_rule(),
                                                                                                            in.cws_rule().get_witness_rule().get_body(),
                                                                                                            numeric_support_selector,
                                                                                                            out.numeric_support_selector_workspace(),
                                                                                                            out.numeric_support_scratch(),
                                                                                                            out.witness_support_scratch(),
                                                                                                            in.cost_buckets().current_cost(),
                                                                                                            in.and_annot(),
                                                                                                            in.numeric_and_annot(),
                                                                                                            in.fact_sets(),
                                                                                                            in.and_ap(),
                                                                                                            in.cost_policy(),
                                                                                                            out.ground_context_solve(),
                                                                                                            out.ground_context_iteration() };
}

template<f::NumericEffectOpKind Op, AndAnnotationPolicyConcept<LiftedTag> AndAP, RuleCostPolicyConcept<LiftedTag> CP>
Cost metric_effect_delta(fd::NumericEffectView<Op, f::FluentTag> effect, const RuleUpdateInput<AndAP, CP>& input)
{
    return metric_effect_delta(
        Op {},
        [&] { return is_valid_binding(effect.get_fterm(), input.fact_sets, input.solve_context); },
        [&] { return is_valid_binding(effect.get_fexpr(), input.fact_sets, input.solve_context); });
}

static void append_numeric_supports(const std::vector<NumericSupportSelectorWorkspace<LiftedTag>::SelectionEntry>& selection,
                                    std::vector<NumericSupport<LiftedTag>>& supports)
{
    for (const auto& entry : selection)
        supports.push_back(NumericSupport<LiftedTag> { entry.key, entry.interval, entry.cost });
}

template<AndAnnotationPolicyConcept<LiftedTag> AndAP, RuleCostPolicyConcept<LiftedTag> CP>
static bool collect_expression_supports(fd::FunctionExpressionView expression,
                                        const RuleUpdateInput<AndAP, CP>& input,
                                        std::vector<NumericSupport<LiftedTag>>& supports,
                                        std::vector<NumericSupportSelectorWorkspace<LiftedTag>::SelectionEntry>& selection)
{
    const auto ground_expression = fd::ground(expression, input.iteration_context);
    const auto value =
        input.numeric_support_selector.evaluate_effect_expression(ygg::make_view(ground_expression, input.iteration_context.destination), selection);
    if (empty(value))
        return false;

    append_numeric_supports(selection, supports);
    return true;
}

template<f::NumericEffectOpKind Op, AndAnnotationPolicyConcept<LiftedTag> AndAP, RuleCostPolicyConcept<LiftedTag> CP>
static bool collect_numeric_head_supports(fd::NumericEffectView<Op, f::FluentTag> effect,
                                          fd::FunctionBindingView<f::FluentTag> program_head,
                                          const RuleUpdateInput<AndAP, CP>& input,
                                          std::vector<NumericSupport<LiftedTag>>& supports)
{
    auto& selection = input.numeric_support_selector_workspace.selection;
    selection.clear();

    if constexpr (!std::is_same_v<Op, f::Assign>)
    {
        if (empty(input.numeric_support_selector.select_fluent_interval(program_head, selection)))
            return false;
    }

    return collect_expression_supports(effect.get_fexpr(), input, supports, selection);
}

template<f::NumericEffectOpKind Op, AndAnnotationPolicyConcept<LiftedTag> AndAP, RuleCostPolicyConcept<LiftedTag> CP>
static bool collect_metric_effect_supports(fd::NumericEffectView<Op, f::FluentTag> effect,
                                           const RuleUpdateInput<AndAP, CP>& input,
                                           std::vector<NumericSupport<LiftedTag>>& supports)
{
    auto& selection = input.numeric_support_selector_workspace.selection;
    selection.clear();

    if constexpr (!std::is_same_v<Op, f::Increase> && !std::is_same_v<Op, f::Decrease>)
    {
        const auto binding = fd::ground(effect.get_fterm(), input.iteration_context).first.get_row();
        if (empty(input.numeric_support_selector.select_fluent_interval(binding, selection)))
            return false;
    }

    return collect_expression_supports(effect.get_fexpr(), input, supports, selection);
}

template<AndAnnotationPolicyConcept<LiftedTag> AndAP, RuleCostPolicyConcept<LiftedTag> CP>
static bool collect_metric_effect_supports(const RuleUpdateInput<AndAP, CP>& input, std::vector<NumericSupport<LiftedTag>>& supports)
{
    for (const auto metric_effect : input.rule.get_metric_effects())
        if (!ygg::visit([&](auto&& effect) { return collect_metric_effect_supports(effect, input, supports); }, metric_effect.get_variant()))
            return false;
    return true;
}

template<AndAnnotationPolicyConcept<LiftedTag> AndAP, RuleCostPolicyConcept<LiftedTag> CP>
Cost metric_effect_delta(const RuleUpdateInput<AndAP, CP>& input)
{
    auto delta = Cost(0);
    for (const auto metric_effect : input.rule.get_metric_effects())
    {
        const auto effect_delta = ygg::visit([&](auto&& effect) { return metric_effect_delta(effect, input); }, metric_effect.get_variant());
        if (effect_delta == std::numeric_limits<Cost>::max())
            return effect_delta;
        delta += effect_delta;
    }

    return delta;
}

template<AndAnnotationPolicyConcept<LiftedTag> AndAP, RuleCostPolicyConcept<LiftedTag> CP>
Cost metric_effect_cost(fd::RuleBindingView rule_binding, const RuleUpdateInput<AndAP, CP>& input)
{
    return reduce_cost(metric_effect_delta(input), input.cost_policy.get_cost(rule_binding));
}

template<AndAnnotationPolicyConcept<LiftedTag> AndAP, RuleCostPolicyConcept<LiftedTag> CP>
static void record_propositional_achiever(fd::PredicateBindingView<f::FluentTag> program_head, const RuleUpdateInput<AndAP, CP>& input)
    requires records_propositional_achievers<AndAP>
{
    const auto rule_binding = fd::ground_binding(input.rule, input.solve_context).first;
    const auto cost = metric_effect_cost(rule_binding, input);
    if (cost == std::numeric_limits<Cost>::max())
        return;
    auto& numeric_supports = input.numeric_support_scratch;
    numeric_supports.clear();
    if (!collect_metric_effect_supports(input, numeric_supports))
        return;
    const auto context = input.make_annotation_context(rule_binding, cost, numeric_supports);

    input.and_ap.record_achiever(program_head, context);
}

template<AndAnnotationPolicyConcept<LiftedTag> AndAP, RuleCostPolicyConcept<LiftedTag> CP>
static void record_propositional_achiever(fd::PredicateBindingView<f::FluentTag>, const RuleUpdateInput<AndAP, CP>&) noexcept
    requires(!records_propositional_achievers<AndAP>)
{
}

template<AndAnnotationPolicyConcept<LiftedTag> AndAP, RuleCostPolicyConcept<LiftedTag> CP>
static void insert_propositional_update(fd::PredicateBindingView<f::FluentTag> program_head,
                                        const RuleUpdateInput<AndAP, CP>& input,
                                        RuleHeadIteration& head_iteration,
                                        SelectedPredicateAnnotations<LiftedTag>& and_annot)
{
    auto rule_binding = std::optional<fd::RuleBindingView> {};
    auto cost = Cost(0);
    if constexpr (std::same_as<CP, RuleCostPolicy<LiftedTag>> && !records_propositional_achievers<AndAP>)
    {
        cost = metric_effect_delta(input);
    }
    else
    {
        rule_binding = fd::ground_binding(input.rule, input.solve_context).first;
        cost = metric_effect_cost(*rule_binding, input);
    }
    if (cost == std::numeric_limits<Cost>::max())
        return;
    auto& numeric_supports = input.numeric_support_scratch;
    numeric_supports.clear();
    if (!collect_metric_effect_supports(input, numeric_supports))
        return;
    const auto context = input.make_annotation_context(rule_binding, cost, numeric_supports);

    input.and_ap.record_achiever(program_head, context);

    std::get<PredicateHeadIteration>(head_iteration).rows.insert(program_head.get_index().row);

    input.and_ap.update_annotation(program_head, program_head, context, and_annot);
}

template<AndAnnotationPolicyConcept<LiftedTag> AndAP, RuleCostPolicyConcept<LiftedTag> CP>
static void insert_numeric_update(fd::NumericEffectOperatorView<f::FluentTag> head,
                                  const FactSets& fact_sets,
                                  const RuleUpdateInput<AndAP, CP>& input,
                                  RuleHeadIteration& head_iteration,
                                  SelectedFunctionAnnotations<LiftedTag>& numeric_and_annot)
{
    const auto interval = is_valid_binding(head, fact_sets, input.iteration_context);
    if (empty(interval))
        return;

    visit(
        [&](auto&& effect)
        {
            const auto program_head = fd::ground(effect.get_fterm(), input.iteration_context).first.get_row();
            const auto worker_head = fd::ground(effect.get_fterm(), input.solve_context).first;
            const auto rule_binding = fd::ground_binding(input.rule, input.solve_context).first;
            const auto rem_rule_cost = metric_effect_cost(rule_binding, input);
            if (rem_rule_cost == std::numeric_limits<Cost>::max())
                return;

            const auto effect_interval =
                rem_rule_cost == Cost(0) ? widen_free_growth(interval, fact_sets.get<f::FluentTag>().function[program_head]) : interval;

            const auto cost = reduce_cost(rem_rule_cost, input.cost_policy.get_cost(rule_binding, program_head, effect_interval));
            auto& numeric_supports = input.numeric_support_scratch;
            numeric_supports.clear();
            if (!collect_metric_effect_supports(input, numeric_supports) || !collect_numeric_head_supports(effect, program_head, input, numeric_supports))
                return;
            const auto context = input.make_annotation_context(rule_binding, cost, numeric_supports);

            input.and_ap.update_annotation(program_head, worker_head.get_row(), effect_interval, context, numeric_and_annot);

            std::get<FunctionHeadIteration>(head_iteration).updates.emplace(worker_head.get_row().get_index().row, effect_interval, input.current_cost + cost);
        },
        head.get_variant());
}

template<OrAnnotationPolicyConcept<LiftedTag> OrAP,
         AndAnnotationPolicyConcept<LiftedTag> AndAP,
         TerminationPolicyConcept<LiftedTag> TP,
         RuleCostPolicyConcept<LiftedTag> CP>
void generate_nullary_case(RuleExecutionContext<OrAP, AndAP, TP, CP>& rctx)
{
    auto wrctx = rctx.get_rule_worker_execution_context();

    const auto& in = wrctx.in();
    auto& out = wrctx.out();
    const auto& numeric_support_selector = in.numeric_support_selector();
    const auto input = make_rule_update_input(in, out, numeric_support_selector);
    ++out.statistics().num_executions;
    ++out.statistics().num_generated_rules;

    create_nullary_binding(out.ground_context_solve().binding);

    const auto nullary_condition = in.cws_rule().get_nullary_condition();
    const auto conflicting_condition = in.cws_rule().get_rule().get_body();
    auto& applicability_cache = out.applicability_cache();

    const auto statically_applicable = [&]()
    {
        return applicability_cache.static_nullary
               && is_valid_binding(conflicting_condition.template get_literals<f::StaticTag>(), in.fact_sets(), out.ground_context_iteration());
    };
    const auto dynamically_applicable = [&]()
    {
        if (!applicability_cache.dynamic_nullary)
            applicability_cache.dynamic_nullary = is_dynamically_applicable(nullary_condition, in.fact_sets());
        return applicability_cache.dynamic_nullary && is_valid_binding(conflicting_condition, in.fact_sets(), out.ground_context_iteration());
    };

    // Note: we never go through the consistency graph, and hence, have to check validity on the entire rule body.
    if (statically_applicable() && dynamically_applicable())
    {
        visit(
            [&](auto&& head)
            {
                using Head = std::decay_t<decltype(head)>;

                if constexpr (std::is_same_v<Head, fd::AtomView<f::FluentTag>>)
                {
                    const auto program_head = fd::ground_binding(head, out.ground_context_iteration()).first;
                    insert_propositional_update(program_head, input, out.head(), out.and_annot());
                }
                else
                {
                    assert(ensure_applicability(in.cws_rule().get_rule(), out.ground_context_iteration(), in.fact_sets()));

                    insert_numeric_update(head, in.fact_sets(), input, out.head(), out.numeric_and_annot());
                }
            },
            in.cws_rule().get_rule().get_head());
    }
}

[[maybe_unused]] static bool ensure_applicability(fd::RuleView rule, fd::GrounderContext& context, const FactSets& fact_sets)
{
    const auto ground_rule = ground(rule, context).first;

    const auto applicable = is_applicable(ground_rule, fact_sets);

    if (!applicable)
    {
        std::cout << "Delta-KPKC generated false positive." << std::endl;
        fmt::print(std::cout, "{}\n", rule);
        fmt::print(std::cout, "{}\n", ground_rule);
    }

    return applicable;
}

[[maybe_unused]] static bool ensure_novel_binding(const ygg::IndexList<f::Object>& binding, ygg::UnorderedSet<ygg::IndexList<f::Object>>& set)
{
    const auto inserted = set.insert(binding).second;

    if (!inserted)
    {
        std::cout << "Delta-KPKC generated duplicate binding." << std::endl;
    }

    return inserted;
}

template<OrAnnotationPolicyConcept<LiftedTag> OrAP,
         AndAnnotationPolicyConcept<LiftedTag> AndAP,
         TerminationPolicyConcept<LiftedTag> TP,
         RuleCostPolicyConcept<LiftedTag> CP>
void process_clique(RuleWorkerExecutionContext<OrAP, AndAP, TP, CP>& wrctx,
                    const NumericSupportSelector<LiftedTag>& numeric_support_selector,
                    std::span<const kpkc::Vertex> clique,
                    bool require_novel_binding)
{
    const auto& in = wrctx.in();
    auto& out = wrctx.out();
    const auto input = make_rule_update_input(in, out, numeric_support_selector);

    create_general_binding(clique, in.cws_rule().get_static_consistency_graph(), out.ground_context_solve().binding);

    assert(!require_novel_binding || ensure_novel_binding(out.ground_context_solve().binding, out.seen_bindings_dbg()));

    ++out.statistics().num_generated_rules;

    const auto nullary_condition = in.cws_rule().get_nullary_condition();
    const auto conflicting_condition = in.cws_rule().get_conflicting_overapproximation_rule().get_body();
    auto& applicability_cache = out.applicability_cache();

    const auto statically_applicable = [&]()
    {
        return applicability_cache.static_nullary
               && is_valid_binding(conflicting_condition.template get_literals<f::StaticTag>(), in.fact_sets(), out.ground_context_iteration());
    };
    const auto dynamically_applicable = [&]()
    {
        if (!applicability_cache.dynamic_nullary)
            applicability_cache.dynamic_nullary = is_dynamically_applicable(nullary_condition, in.fact_sets());
        return applicability_cache.dynamic_nullary && is_valid_binding(conflicting_condition, in.fact_sets(), out.ground_context_iteration());
    };

    if (!statically_applicable())
        return;

    visit(
        [&](auto&& head)
        {
            using Head = std::decay_t<decltype(head)>;

            if constexpr (std::is_same_v<Head, fd::AtomView<f::FluentTag>>)
            {
                const auto program_head = fd::ground_binding(head, out.ground_context_iteration()).first;
                if (in.fact_sets().template get<f::FluentTag>().predicate.contains(program_head))
                {
                    if constexpr (records_propositional_achievers<AndAP>)
                    {
                        if (dynamically_applicable())
                            record_propositional_achiever(program_head, input);
                    }
                    return;  ///< optimal cost proven
                }

                if (!dynamically_applicable())
                {
#ifdef TYR_ENABLE_SEMI_NAIVE
                    // Delta KPKC will not revisit this binding when only its nullary part becomes true.
                    ++out.statistics().num_pending_rules;

                    const auto rule_binding = fd::ground_binding(in.cws_rule().get_conflicting_overapproximation_rule(), out.ground_context_solve()).first;

                    out.pending_rule_bindings().emplace(rule_binding);
#endif
                    return;
                }

                assert(ensure_applicability(in.cws_rule().get_rule(), out.ground_context_iteration(), in.fact_sets()));

                insert_propositional_update(program_head, input, out.head(), out.and_annot());
            }
            else
            {
                if (!dynamically_applicable())
                    return;

                assert(ensure_applicability(in.cws_rule().get_rule(), out.ground_context_iteration(), in.fact_sets()));

                insert_numeric_update(head, in.fact_sets(), input, out.head(), out.numeric_and_annot());
            }
        },
        in.cws_rule().get_rule().get_head());
}

template<OrAnnotationPolicyConcept<LiftedTag> OrAP,
         AndAnnotationPolicyConcept<LiftedTag> AndAP,
         TerminationPolicyConcept<LiftedTag> TP,
         RuleCostPolicyConcept<LiftedTag> CP>
void generate_general_case(RuleExecutionContext<OrAP, AndAP, TP, CP>& rctx)
{
    auto& rule_out = rctx.out();
    auto& kpkc_algorithm = rule_out.kpkc();

    auto for_each_relevant_clique = [&](auto&& head, auto&& callback, auto& workspace)
    {
#ifdef TYR_ENABLE_SEMI_NAIVE
        using Head = std::decay_t<decltype(head)>;

        // Note: rules with numeric effects are non-idempotent, so we must consider all k-cliques, not just new ones.
        if constexpr (std::is_same_v<Head, fd::NumericEffectOperatorView<f::FluentTag>>)
            kpkc_algorithm.for_each_k_clique(std::forward<decltype(callback)>(callback), workspace);
        else
            kpkc_algorithm.for_each_new_k_clique(std::forward<decltype(callback)>(callback), workspace);
#else
        kpkc_algorithm.for_each_k_clique(std::forward<decltype(callback)>(callback), workspace);
#endif
    };

    visit(
        [&](auto&& head)
        {
#ifdef TYR_ENABLE_SEMI_NAIVE
            using Head = std::decay_t<decltype(head)>;
#endif

#if defined(TYR_ENABLE_INNER_PARALLELISM) && defined(TYR_ENABLE_SEMI_NAIVE)
            if constexpr (std::is_same_v<Head, fd::AtomView<f::FluentTag>>)
            {
                constexpr size_t kNumStripes = 2;
                constexpr size_t kParallelThreshold = 1024;

                if (kpkc_algorithm.get_iteration() > 1 && rctx.in().cws_rule().get_rule().get_arity() > 2
                    && oneapi::tbb::this_task_arena::max_concurrency() >= 2)
                {
                    const auto& delta_edges = kpkc_algorithm.materialize_delta_edges();

                    if (delta_edges.size() >= kParallelThreshold)
                    {
                        assert(rctx.out().workers().size() == kNumStripes);

                        oneapi::tbb::parallel_for(size_t(0),
                                                  kNumStripes,
                                                  [&](size_t stripe)
                                                  {
                                                      auto wrctx = rctx.get_rule_worker_execution_context(stripe);
                                                      const auto& numeric_support_selector = wrctx.in().numeric_support_selector();
                                                      auto& out = wrctx.out();
                                                      auto& kpkc_workspace = out.kpkc_workspace();
                                                      ++out.statistics().num_executions;

                                                      for (auto edge_index = stripe; edge_index < delta_edges.size(); edge_index += kNumStripes)
                                                      {
                                                          if (!kpkc_algorithm.seed_from_anchor(delta_edges[edge_index], kpkc_workspace))
                                                              continue;

                                                          kpkc_algorithm.template complete_from_seed<kpkc::Edge>(
                                                              [&](auto&& clique) { process_clique(wrctx, numeric_support_selector, clique, true); },
                                                              0,
                                                              kpkc_workspace);
                                                      }
                                                  });
                        return;
                    }
                }
            }
#endif

            auto wrctx = rctx.get_rule_worker_execution_context();
            const auto& numeric_support_selector = wrctx.in().numeric_support_selector();
            auto& out = wrctx.out();
            auto& kpkc_workspace = out.kpkc_workspace();
            ++out.statistics().num_executions;

#ifdef TYR_ENABLE_SEMI_NAIVE
            constexpr auto require_novel_binding = !std::is_same_v<Head, fd::NumericEffectOperatorView<f::FluentTag>>;
#else
            constexpr auto require_novel_binding = false;
#endif
            for_each_relevant_clique(
                head,
                [&](auto&& clique) { process_clique(wrctx, numeric_support_selector, clique, require_novel_binding); },
                kpkc_workspace);
        },
        rctx.in().cws_rule().get_rule().get_head());
}

template<OrAnnotationPolicyConcept<LiftedTag> OrAP,
         AndAnnotationPolicyConcept<LiftedTag> AndAP,
         TerminationPolicyConcept<LiftedTag> TP,
         RuleCostPolicyConcept<LiftedTag> CP>
void generate(RuleExecutionContext<OrAP, AndAP, TP, CP>& rctx)
{
    const auto arity = rctx.in().cws_rule().get_rule().get_arity();

    if (arity == 0)
        generate_nullary_case(rctx);
    else
        generate_general_case(rctx);
}

template<OrAnnotationPolicyConcept<LiftedTag> OrAP,
         AndAnnotationPolicyConcept<LiftedTag> AndAP,
         TerminationPolicyConcept<LiftedTag> TP,
         RuleCostPolicyConcept<LiftedTag> CP>
void process_pending_rule_bindings(RuleExecutionContext<OrAP, AndAP, TP, CP>& rctx)
{
    for (auto& worker : rctx.out().workers())
    {
        auto wrctx = RuleWorkerExecutionContext(rctx, worker);

        const auto& in = wrctx.in();
        auto& out = wrctx.out();
        const auto& numeric_support_selector = in.numeric_support_selector();
        const auto input = make_rule_update_input(in, out, numeric_support_selector);
        const auto nullary_condition = in.cws_rule().get_nullary_condition();
        const auto conflicting_condition = in.cws_rule().get_conflicting_overapproximation_rule().get_body();
        auto& applicability_cache = out.applicability_cache();
        const auto dynamically_applicable = [&]()
        {
            if (!applicability_cache.dynamic_nullary)
                applicability_cache.dynamic_nullary = is_dynamically_applicable(nullary_condition, in.fact_sets());
            return applicability_cache.dynamic_nullary && is_valid_binding(conflicting_condition, in.fact_sets(), out.ground_context_iteration());
        };

        auto& pending = out.pending_rule_bindings();
        assert(pending.empty() || applicability_cache.static_nullary);
        for (const auto pending_binding : out.sorted_pending_rule_bindings())
        {
            out.ground_context_solve().binding.clear();
            for (const auto object : pending_binding.get_objects())
                out.ground_context_solve().binding.push_back(object.get_index());

            assert(out.ground_context_solve().binding == out.ground_context_iteration().binding);
            assert(is_valid_binding(conflicting_condition.template get_literals<f::StaticTag>(), in.fact_sets(), out.ground_context_iteration()));

            auto erase_pending = false;
            visit(
                [&](auto&& head)
                {
                    using Head = std::decay_t<decltype(head)>;

                    if constexpr (std::is_same_v<Head, fd::AtomView<f::FluentTag>>)
                    {
                        const auto program_head = fd::ground_binding(head, out.ground_context_iteration()).first;

                        if (in.fact_sets().template get<f::FluentTag>().predicate.contains(program_head))  ///< optimal cost proven
                        {
                            if constexpr (records_propositional_achievers<AndAP>)
                            {
                                if (dynamically_applicable())
                                {
                                    assert(ensure_applicability(in.cws_rule().get_rule(), out.ground_context_iteration(), in.fact_sets()));

                                    record_propositional_achiever(program_head, input);
                                    erase_pending = true;
                                }
                            }
                            else
                            {
                                erase_pending = true;
                            }
                        }
                        else
                        {
                            if (dynamically_applicable())
                            {
                                assert(ensure_applicability(in.cws_rule().get_rule(), out.ground_context_iteration(), in.fact_sets()));

                                insert_propositional_update(program_head, input, out.head(), out.and_annot());
                                erase_pending = true;
                            }
                        }
                    }
                    else
                    {
                        throw std::logic_error("Numeric-head rules should not be stored as pending.");
                    }
                },
                in.cws_rule().get_rule().get_head());

            if (erase_pending)
                pending.erase(pending_binding);
        }
    }
}

template<OrAnnotationPolicyConcept<LiftedTag> OrAP,
         AndAnnotationPolicyConcept<LiftedTag> AndAP,
         TerminationPolicyConcept<LiftedTag> TP,
         RuleCostPolicyConcept<LiftedTag> CP>
/// Parallel phase: recheck pending rule bindings and generate new ground witnesses for all active rules.
void run_active_rules(StratumExecutionContext<OrAP, AndAP, TP, CP>& ctx)
{
    auto& program_out = ctx.out().program();
    const auto& active_rules = ctx.out().scheduler().get_active_rules();

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

#ifdef TYR_ENABLE_SEMI_NAIVE
        {
            const auto process_pending_time = ygg::StopwatchScope(rule_out.statistics().process_pending_time);

            process_pending_rule_bindings(rctx);
        }
#endif

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

template<OrAnnotationPolicyConcept<LiftedTag> OrAP,
         AndAnnotationPolicyConcept<LiftedTag> AndAP,
         TerminationPolicyConcept<LiftedTag> TP,
         RuleCostPolicyConcept<LiftedTag> CP>
/// Sequential phase: merge worker heads and annotations into the program and bucket them by cost.
void merge_worker_results(StratumExecutionContext<OrAP, AndAP, TP, CP>& ctx)
{
    auto& program_out = ctx.out().program();
    auto& cost_buckets = program_out.cost_buckets();

    for (const auto rule_index : ctx.out().scheduler().get_active_rules())
    {
        const auto i = ygg::uint_t(rule_index);
        auto merge_context = fd::MergeContext { program_out.datalog_builder(), program_out.workspace_repository() };
        const auto& ws_rule = program_out.rules()[i];

        for (auto& worker : ws_rule->worker)
        {
            std::visit(
                [&](auto& head_iteration)
                {
                    using HeadIteration = std::decay_t<decltype(head_iteration)>;

                    if constexpr (std::is_same_v<HeadIteration, PredicateHeadIteration>)
                    {
                        for (const auto worker_head_index : head_iteration.get_sorted_rows())
                        {
                            const auto worker_head =
                                ygg::make_view(ygg::Index<f::RelationBinding<f::Predicate<f::FluentTag>>> { head_iteration.relation, worker_head_index },
                                               worker.iteration.workspace_overlay_repository);

                            const auto program_head = fd::merge_d2d(worker_head, merge_context).first;

                            const auto cost_update =
                                program_out.or_ap().update_annotation(program_head, worker_head, worker.iteration.and_annot, program_out.and_annot());

                            cost_buckets.update(cost_update, program_head);
                        }
                    }
                    else
                    {
                        for (const auto& update : head_iteration.get_sorted_updates())
                        {
                            const auto worker_head =
                                ygg::make_view(ygg::Index<f::RelationBinding<f::Function<f::FluentTag>>> { head_iteration.relation, update.row },
                                               worker.solve.program_overlay_repository);

                            const auto program_head = fd::merge_d2d(worker_head, merge_context).first;
                            const auto* worker_annotation = worker.iteration.numeric_and_annot.find(worker_head, update.interval);
                            if (worker_annotation)
                                program_out.numeric_and_annot().insert(program_head, update.interval, *worker_annotation);

                            cost_buckets.insert(update.cost, program_head, update.interval);
                        }
                    }
                },
                worker.iteration.head);
        }
    }
}

template<OrAnnotationPolicyConcept<LiftedTag> OrAP,
         AndAnnotationPolicyConcept<LiftedTag> AndAP,
         TerminationPolicyConcept<LiftedTag> TP,
         RuleCostPolicyConcept<LiftedTag> CP>
/// Commit the cheapest bucket: insert its heads into the fact and assignment sets and notify the scheduler.
bool commit_current_bucket(StratumExecutionContext<OrAP, AndAP, TP, CP>& ctx)
{
    auto& program_out = ctx.out().program();
    auto& scheduler = ctx.out().scheduler();
    auto& facts = program_out.facts();
    auto& cost_buckets = program_out.cost_buckets();
    auto changed = false;

    for (const auto head : cost_buckets.get_current_bucket_sorted())
    {
        if (facts.fact_sets.predicate.insert(head))
        {
            changed = true;
            scheduler.on_generate(head.get_index().relation);
            facts.assignment_sets.predicate.insert(head);
        }
    }

    for (const auto& [head, interval] : cost_buckets.get_current_function_bucket_sorted())
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

template<OrAnnotationPolicyConcept<LiftedTag> OrAP,
         AndAnnotationPolicyConcept<LiftedTag> AndAP,
         TerminationPolicyConcept<LiftedTag> TP,
         RuleCostPolicyConcept<LiftedTag> CP>
void solve_bottom_up_for_stratum(StratumExecutionContext<OrAP, AndAP, TP, CP>& ctx)
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
        if (program_out.tp().check(FactSets { ctx.in().program().facts().fact_sets, program_out.facts().fact_sets }))
            return;

        scheduler.on_start_iteration();

        run_active_rules(ctx);

        cost_buckets.clear_current();  ///< Clear before merging to avoid handling current heads twice.
        merge_worker_results(ctx);

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

template<OrAnnotationPolicyConcept<LiftedTag> OrAP,
         AndAnnotationPolicyConcept<LiftedTag> AndAP,
         TerminationPolicyConcept<LiftedTag> TP,
         RuleCostPolicyConcept<LiftedTag> CP>
void solve_bottom_up(ProgramExecutionContext<LiftedTag, OrAP, AndAP, TP, CP>& ctx)
{
    auto& out = ctx.out();
    const auto program_stopwatch = ygg::StopwatchScope(out.statistics().total_time);
    ++out.statistics().num_executions;

    for (auto stratum_ctx : ctx.get_stratum_execution_contexts())
    {
        solve_bottom_up_for_stratum(stratum_ctx);
    }
}

template void
solve_bottom_up(ProgramExecutionContext<LiftedTag, NoOrAnnotationPolicy<LiftedTag>, NoAndAnnotationPolicy<LiftedTag>, NoTerminationPolicy<LiftedTag>>& ctx);
template void solve_bottom_up(
    ProgramExecutionContext<LiftedTag, OrAnnotationPolicy<LiftedTag>, AndAnnotationPolicy<LiftedTag, SumAggregation>, NoTerminationPolicy<LiftedTag>>& ctx);
template void solve_bottom_up(ProgramExecutionContext<LiftedTag,
                                                      OrAnnotationPolicy<LiftedTag>,
                                                      AndAnnotationPolicy<LiftedTag, SumAggregation>,
                                                      TerminationPolicy<LiftedTag, SumAggregation>>& ctx);
template void solve_bottom_up(
    ProgramExecutionContext<LiftedTag, OrAnnotationPolicy<LiftedTag>, AndAnnotationPolicy<LiftedTag, MaxAggregation>, NoTerminationPolicy<LiftedTag>>& ctx);
template void solve_bottom_up(ProgramExecutionContext<LiftedTag,
                                                      OrAnnotationPolicy<LiftedTag>,
                                                      AndAnnotationPolicy<LiftedTag, MaxAggregation>,
                                                      TerminationPolicy<LiftedTag, MaxAggregation>>& ctx);
template void solve_bottom_up(ProgramExecutionContext<LiftedTag,
                                                      NoOrAnnotationPolicy<LiftedTag>,
                                                      NoAndAnnotationPolicy<LiftedTag>,
                                                      NoTerminationPolicy<LiftedTag>,
                                                      RuleCostOverridePolicy<LiftedTag>>& ctx);
template void solve_bottom_up(ProgramExecutionContext<LiftedTag,
                                                      OrAnnotationPolicy<LiftedTag>,
                                                      AndAnnotationPolicy<LiftedTag, SumAggregation>,
                                                      NoTerminationPolicy<LiftedTag>,
                                                      RuleCostOverridePolicy<LiftedTag>>& ctx);
template void solve_bottom_up(ProgramExecutionContext<LiftedTag,
                                                      OrAnnotationPolicy<LiftedTag>,
                                                      AndAnnotationPolicy<LiftedTag, SumAggregation>,
                                                      TerminationPolicy<LiftedTag, SumAggregation>,
                                                      RuleCostOverridePolicy<LiftedTag>>& ctx);
template void solve_bottom_up(ProgramExecutionContext<LiftedTag,
                                                      OrAnnotationPolicy<LiftedTag>,
                                                      AndAnnotationPolicy<LiftedTag, MaxAggregation>,
                                                      NoTerminationPolicy<LiftedTag>,
                                                      RuleCostOverridePolicy<LiftedTag>>& ctx);
template void solve_bottom_up(ProgramExecutionContext<LiftedTag,
                                                      OrAnnotationPolicy<LiftedTag>,
                                                      AndAnnotationPolicy<LiftedTag, MaxAggregation>,
                                                      TerminationPolicy<LiftedTag, MaxAggregation>,
                                                      RuleCostOverridePolicy<LiftedTag>>& ctx);
template void solve_bottom_up(ProgramExecutionContext<LiftedTag,
                                                      OrAnnotationPolicy<LiftedTag>,
                                                      AchieverAndAnnotationPolicy<LiftedTag, MaxAggregation>,
                                                      TerminationPolicy<LiftedTag, MaxAggregation>,
                                                      RuleCostOverridePolicy<LiftedTag>>& ctx);
}
