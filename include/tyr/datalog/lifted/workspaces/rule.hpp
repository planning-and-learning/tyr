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

#ifndef TYR_DATALOG_LIFTED_WORKSPACES_RULE_HPP_
#define TYR_DATALOG_LIFTED_WORKSPACES_RULE_HPP_

#include "tyr/datalog/lifted/consistency_graph.hpp"
#include "tyr/datalog/lifted/delta_kpkc.hpp"
#include "tyr/datalog/lifted/policies/numeric_support.hpp"
#include "tyr/datalog/policies/annotation_concept.hpp"
#include "tyr/datalog/statistics/rule.hpp"
#include "tyr/datalog/workspaces/rule.hpp"
#include "tyr/formalism/binding_index.hpp"
#include "tyr/formalism/datalog/builder.hpp"
#include "tyr/formalism/datalog/ground_atom_index.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/object_index.hpp"

#include <algorithm>
#include <cassert>
#include <deque>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/semantics/comparison.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::datalog
{

struct PredicateHeadIteration
{
    using Binding = ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag>;

    ygg::UnorderedSet<Binding> bindings;
    std::vector<Binding> sorted_bindings;

    void clear() noexcept
    {
        bindings.clear();
        sorted_bindings.clear();
    }

    const std::vector<Binding>& get_sorted_bindings()
    {
        sorted_bindings.assign(bindings.begin(), bindings.end());
        std::sort(sorted_bindings.begin(), sorted_bindings.end(), canonical_binding_less<Binding>);
        return sorted_bindings;
    }
};

struct FunctionHeadUpdate : ygg::comparison::Mixin<FunctionHeadUpdate>
{
    ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> binding;
    ygg::ClosedInterval<ygg::float_t> interval;
    Cost cost;

    FunctionHeadUpdate(::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> binding,
                       ygg::ClosedInterval<ygg::float_t> interval,
                       Cost cost) :
        binding(binding),
        interval(interval),
        cost(cost)
    {
    }

    auto identifying_members() const noexcept { return std::tie(binding, interval, cost); }
};

struct FunctionHeadIteration
{
    ygg::UnorderedSet<FunctionHeadUpdate> updates;
    std::vector<FunctionHeadUpdate> sorted_updates;

    void clear() noexcept
    {
        updates.clear();
        sorted_updates.clear();
    }

    const std::vector<FunctionHeadUpdate>& get_sorted_updates()
    {
        sorted_updates.assign(updates.begin(), updates.end());
        std::sort(sorted_updates.begin(),
                  sorted_updates.end(),
                  [](const auto& lhs, const auto& rhs)
                  {
                      if (canonical_binding_less(lhs.binding, rhs.binding))
                          return true;
                      if (canonical_binding_less(rhs.binding, lhs.binding))
                          return false;
                      return ygg::Less<> {}(std::tuple(lower(lhs.interval), upper(lhs.interval), lhs.cost),
                                            std::tuple(lower(rhs.interval), upper(rhs.interval), rhs.cost));
                  });
        return sorted_updates;
    }
};

template<::tyr::formalism::RelationKind R>
struct RuleHeadIteration;

template<>
struct RuleHeadIteration<::tyr::formalism::PredicateTag>
{
    using type = PredicateHeadIteration;
};

template<>
struct RuleHeadIteration<::tyr::formalism::FunctionTag>
{
    using type = FunctionHeadIteration;
};

template<::tyr::formalism::RelationKind R>
using RuleHeadIterationT = typename RuleHeadIteration<R>::type;

struct ApplicabilityCache
{
    bool static_nullary = false;
    bool dynamic_nullary = false;

    void clear() noexcept
    {
        static_nullary = false;
        dynamic_nullary = false;
    }
};

template<::tyr::formalism::RelationKind R>
struct RuleWorkspace<LiftedTag, R>
{
    template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
    struct Instance
    {
        struct Common
        {
            explicit Common(const StaticConsistencyGraph& static_consistency_graph);

            void initialize_iteration(const StaticConsistencyGraph& static_consistency_graph, const AssignmentSets& assignment_sets);

            void clear() noexcept;

            /// KPKC
            kpkc::DeltaKPKC kpkc;

            /// Statistics
            RuleStatistics statistics;
        };

        /// @brief Each iteration consists of
        /// - generate all k-cliques
        /// - ground witnesses
        /// - annotate witnesses
        struct Iteration
        {
            explicit Iteration(const ConstRuleWorkspace<LiftedTag, R>& cws, const Common& common);

            void clear() noexcept;

            /// Heads
            RuleHeadIterationT<R> head;

            // Annotations are worker-local, while their heads are interned in the shared workspace repository.
            DeltaPredicateAnnotations<LiftedTag> and_annot;
            DeltaFunctionAnnotations<LiftedTag> numeric_and_annot;

            /// KPKC
            kpkc::Workspace kpkc_workspace;
        };

        struct Solve
        {
            explicit Solve(const AndAP& and_ap);

            void clear() noexcept;

            AndAP and_ap;

            /// In debug mode, we accumulate all bindings to verify the correctness of delta-kpkc
            ygg::UnorderedSet<ygg::IndexList<::tyr::formalism::Object>> seen_bindings_dbg;

            ygg::UnorderedSet<::tyr::formalism::datalog::RuleBindingView<R>> pending_rule_bindings;
            std::vector<::tyr::formalism::datalog::RuleBindingView<R>> pending_rule_binding_scratch;

            const std::vector<::tyr::formalism::datalog::RuleBindingView<R>>& get_sorted_pending_rule_bindings();

            NumericSupportSelectorWorkspace<LiftedTag> numeric_support_selector_workspace;
            std::vector<NumericSupport<LiftedTag>> numeric_support_scratch;
            std::vector<NumericSupport<LiftedTag>> witness_support_scratch;
            ApplicabilityCache applicability_cache;

            /// Statistics
            RuleWorkerStatistics statistics;
        };

        struct Worker
        {
            explicit Worker(const ConstRuleWorkspace<LiftedTag, R>& cws, const Common& common, const AndAP& and_ap);

            void clear() noexcept;

            ::tyr::formalism::datalog::Builder builder;
            ygg::IndexList<::tyr::formalism::Object> binding;

            Iteration iteration;
            Solve solve;
        };

        Instance(const ConstRuleWorkspace<LiftedTag, R>& cws, const AndAP& and_ap);
        Instance(const Instance& other) = delete;
        Instance& operator=(const Instance& other) = delete;
        Instance(Instance&& other) = delete;
        Instance& operator=(Instance&& other) = delete;

        void clear() noexcept;

        Common common;

        std::deque<Worker> worker;
    };
};

template<::tyr::formalism::RelationKind R>
struct ConstRuleWorkspace<LiftedTag, R>
{
public:
    auto get_rule() const noexcept { return rule; }
    auto get_witness_rule() const noexcept { return witness_rule; }
    auto get_nullary_condition() const noexcept { return nullary_condition; }
    auto get_conflicting_overapproximation_rule() const noexcept { return conflicting_overapproximation_rule; }
    const auto& get_static_consistency_graph() const noexcept { return static_consistency_graph; }
    Cost get_pre_evaluated_metric_cost() const noexcept { return pre_evaluated_metric_cost; }
    const auto& get_runtime_metric_effects() const noexcept { return runtime_metric_effects; }

    ConstRuleWorkspace(::tyr::formalism::datalog::RuleView<R> rule,
                       ::tyr::formalism::datalog::Repository& repository,
                       const analysis::VariableDomainList& parameter_domains,
                       size_t num_objects,
                       size_t num_fluent_predicates,
                       const TaggedFactSets<::tyr::formalism::StaticTag>& static_fact_sets,
                       const TaggedAssignmentSets<::tyr::formalism::StaticTag>& static_assignment_sets);

private:
    ::tyr::formalism::datalog::RuleView<R> rule;
    ::tyr::formalism::datalog::RuleView<R> witness_rule;
    ::tyr::formalism::datalog::GroundConjunctiveConditionView nullary_condition;
    ::tyr::formalism::datalog::RuleView<R> unary_overapproximation_rule;
    ::tyr::formalism::datalog::RuleView<R> binary_overapproximation_rule;
    ::tyr::formalism::datalog::RuleView<R> static_binary_overapproximation_rule;
    ::tyr::formalism::datalog::RuleView<R> conflicting_overapproximation_rule;

    Cost pre_evaluated_metric_cost;
    ::tyr::formalism::datalog::NumericEffectOperatorViewList<::tyr::formalism::FluentTag> runtime_metric_effects;

    StaticConsistencyGraph static_consistency_graph;
};

/**
 * Implementations
 */

inline PredicateHeadIteration make_head_iteration(::tyr::formalism::datalog::AtomView<::tyr::formalism::FluentTag>) { return {}; }

inline FunctionHeadIteration make_head_iteration(::tyr::formalism::datalog::NumericEffectOperatorView<::tyr::formalism::FluentTag>) { return {}; }

inline bool supports_inner_parallelism(::tyr::formalism::datalog::AtomView<::tyr::formalism::FluentTag>) noexcept { return true; }
inline bool supports_inner_parallelism(::tyr::formalism::datalog::NumericEffectOperatorView<::tyr::formalism::FluentTag>) noexcept { return false; }

template<::tyr::formalism::RelationKind R>
template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
RuleWorkspace<LiftedTag, R>::Instance<AndAP>::Common::Common(const StaticConsistencyGraph& static_consistency_graph) :
    kpkc(static_consistency_graph),
    statistics()
{
}

template<::tyr::formalism::RelationKind R>
template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
void RuleWorkspace<LiftedTag, R>::Instance<AndAP>::Common::clear() noexcept
{
    kpkc.reset();
}

template<::tyr::formalism::RelationKind R>
template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
void RuleWorkspace<LiftedTag, R>::Instance<AndAP>::Common::initialize_iteration(const StaticConsistencyGraph& static_consistency_graph,
                                                                                const AssignmentSets& assignment_sets)
{
    kpkc.set_next_assignment_sets(static_consistency_graph, assignment_sets);
}

template<::tyr::formalism::RelationKind R>
template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
RuleWorkspace<LiftedTag, R>::Instance<AndAP>::Iteration::Iteration(const ConstRuleWorkspace<LiftedTag, R>& cws, const Common& common) :
    head(make_head_iteration(cws.get_rule().get_head())),
    and_annot(),
    numeric_and_annot(),
    kpkc_workspace(common.kpkc.get_graph_layout())
{
}

template<::tyr::formalism::RelationKind R>
template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
void RuleWorkspace<LiftedTag, R>::Instance<AndAP>::Iteration::clear() noexcept
{
    head.clear();
    and_annot.clear();
    numeric_and_annot.clear();
}

template<::tyr::formalism::RelationKind R>
template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
RuleWorkspace<LiftedTag, R>::Instance<AndAP>::Solve::Solve(const AndAP& and_ap) :
    and_ap(and_ap),
    seen_bindings_dbg(),
    pending_rule_bindings(),
    pending_rule_binding_scratch(),
    numeric_support_selector_workspace(),
    numeric_support_scratch(),
    witness_support_scratch(),
    applicability_cache(),
    statistics()
{
}

template<::tyr::formalism::RelationKind R>
template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
void RuleWorkspace<LiftedTag, R>::Instance<AndAP>::Solve::clear() noexcept
{
    seen_bindings_dbg.clear();
    pending_rule_bindings.clear();
    pending_rule_binding_scratch.clear();
    numeric_support_selector_workspace.clear();
    numeric_support_scratch.clear();
    witness_support_scratch.clear();
    applicability_cache.clear();
    and_ap.clear_achievers();
}

template<::tyr::formalism::RelationKind R>
template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
const std::vector<::tyr::formalism::datalog::RuleBindingView<R>>& RuleWorkspace<LiftedTag, R>::Instance<AndAP>::Solve::get_sorted_pending_rule_bindings()
{
    pending_rule_binding_scratch.assign(pending_rule_bindings.begin(), pending_rule_bindings.end());
    std::sort(pending_rule_binding_scratch.begin(), pending_rule_binding_scratch.end(), canonical_binding_less<::tyr::formalism::datalog::RuleBindingView<R>>);
    return pending_rule_binding_scratch;
}

template<::tyr::formalism::RelationKind R>
template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
RuleWorkspace<LiftedTag, R>::Instance<AndAP>::Worker::Worker(const ConstRuleWorkspace<LiftedTag, R>& cws, const Common& common, const AndAP& and_ap) :
    builder(),
    binding(),
    iteration(cws, common),
    solve(and_ap)
{
}

template<::tyr::formalism::RelationKind R>
template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
void RuleWorkspace<LiftedTag, R>::Instance<AndAP>::Worker::clear() noexcept
{
    iteration.clear();
    solve.clear();
}

template<::tyr::formalism::RelationKind R>
template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
RuleWorkspace<LiftedTag, R>::Instance<AndAP>::Instance(const ConstRuleWorkspace<LiftedTag, R>& cws_, const AndAP& and_ap_) :
    common(cws_.get_static_consistency_graph()),
    worker()
{
    worker.emplace_back(cws_, common, and_ap_);

#if defined(TYR_ENABLE_INNER_PARALLELISM) && defined(TYR_ENABLE_SEMI_NAIVE)
    // Only propositional heads use partitionable delta KPKC; numeric effects require full KPKC enumeration.
    if (supports_inner_parallelism(cws_.get_rule().get_head()) && cws_.get_rule().get_arity() > 2)
        worker.emplace_back(cws_, common, and_ap_);
#endif
}

template<::tyr::formalism::RelationKind R>
template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
void RuleWorkspace<LiftedTag, R>::Instance<AndAP>::clear() noexcept
{
    common.clear();
    for (auto& w : worker)
        w.clear();
}

}

#endif
