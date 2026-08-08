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

#include "tyr/algorithms/kckp/delta_kckp.hpp"
#include "tyr/datalog/lifted/consistency_graph.hpp"
#include "tyr/datalog/lifted/policies/numeric_support.hpp"
#include "tyr/datalog/statistics/rule.hpp"
#include "tyr/datalog/workspaces/rule.hpp"
#include "tyr/formalism/binding_index.hpp"
#include "tyr/formalism/datalog/builder.hpp"
#include "tyr/formalism/datalog/ground_atom_index.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/object_index.hpp"

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

    ygg::UnorderedSet<Binding> seen_bindings;
    std::vector<Binding> bindings;

    void clear() noexcept
    {
        seen_bindings.clear();
        bindings.clear();
    }

    void insert(Binding binding)
    {
        if (seen_bindings.emplace(binding).second)
            bindings.push_back(binding);
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
    ygg::UnorderedSet<FunctionHeadUpdate> seen_updates;
    std::vector<FunctionHeadUpdate> updates;

    void clear() noexcept
    {
        seen_updates.clear();
        updates.clear();
    }

    void insert(FunctionHeadUpdate update)
    {
        if (seen_updates.emplace(update).second)
            updates.push_back(std::move(update));
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
    struct Common
    {
        explicit Common(const StaticConsistencyGraph& static_consistency_graph);

        void initialize_iteration(const AssignmentSets& assignment_sets);

        void clear() noexcept;

        /// KCKP
        const StaticConsistencyGraph& static_consistency_graph;
        kckp::DeltaKCKP kckp;

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

        /// Head updates
        RuleHeadIterationT<R> head_updates;

        /// KCKP
        kckp::Workspace kckp_workspace;
    };

    struct Solve
    {
        Solve();

        void clear() noexcept;

#ifndef NDEBUG
        /// In debug mode, we accumulate all bindings to verify the correctness of delta-kckp
        ygg::UnorderedSet<ygg::IndexList<::tyr::formalism::Object>> seen_bindings;
#endif

#ifndef TYR_ENABLE_SEMI_NAIVE
        ygg::UnorderedSet<::tyr::formalism::datalog::RuleBindingView<R>> seen_pending_rule_bindings;
#endif
        std::vector<::tyr::formalism::datalog::RuleBindingView<R>> pending_rule_bindings;

        NumericSupportSelectorWorkspace<LiftedTag> numeric_support_selector_workspace;
        std::vector<NumericSupport<LiftedTag>> effect_support_scratch;
        std::vector<NumericSupport<LiftedTag>> witness_support_scratch;
        ApplicabilityCache applicability_cache;

        /// Statistics
        RuleWorkerStatistics statistics;
    };

    struct Worker
    {
        explicit Worker(const ConstRuleWorkspace<LiftedTag, R>& cws, const Common& common);

        void clear() noexcept;

        ::tyr::formalism::datalog::Builder builder;
        ygg::IndexList<::tyr::formalism::Object> binding;

        Iteration iteration;
        Solve solve;
    };

    explicit RuleWorkspace(const ConstRuleWorkspace<LiftedTag, R>& cws);
    RuleWorkspace(const RuleWorkspace& other) = delete;
    RuleWorkspace& operator=(const RuleWorkspace& other) = delete;
    RuleWorkspace(RuleWorkspace&& other) = delete;
    RuleWorkspace& operator=(RuleWorkspace&& other) = delete;

    void clear() noexcept;

    Common common;

    std::deque<Worker> worker;
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
                       kckp::Graph compatibility_graph,
                       std::vector<ygg::Index<::tyr::formalism::Object>> vertex_objects,
                       const TaggedFactSets<::tyr::formalism::StaticTag>& static_fact_sets);

private:
    ::tyr::formalism::datalog::RuleView<R> rule;
    ::tyr::formalism::datalog::RuleView<R> witness_rule;
    ::tyr::formalism::datalog::GroundConjunctiveConditionView nullary_condition;
    ::tyr::formalism::datalog::RuleView<R> unary_overapproximation_rule;
    ::tyr::formalism::datalog::RuleView<R> binary_overapproximation_rule;
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
RuleWorkspace<LiftedTag, R>::Common::Common(const StaticConsistencyGraph& static_consistency_graph) :
    static_consistency_graph(static_consistency_graph),
    kckp(static_consistency_graph.get_graph(), static_consistency_graph.get_partitioned_adjacency_layout()),
    statistics()
{
}

template<::tyr::formalism::RelationKind R>
void RuleWorkspace<LiftedTag, R>::Common::clear() noexcept
{
    kckp.reset();
}

template<::tyr::formalism::RelationKind R>
void RuleWorkspace<LiftedTag, R>::Common::initialize_iteration(const AssignmentSets& assignment_sets)
{
    kckp.update([&](auto& delta_graph, auto& full_graph)
                { static_consistency_graph.initialize_dynamic_consistency_graphs(assignment_sets, kckp.get_graph_layout(), delta_graph, full_graph); });
}

template<::tyr::formalism::RelationKind R>
RuleWorkspace<LiftedTag, R>::Iteration::Iteration(const ConstRuleWorkspace<LiftedTag, R>& cws, const Common& common) :
    head_updates(make_head_iteration(cws.get_rule().get_head())),
    kckp_workspace(common.kckp.get_graph_layout())
{
}

template<::tyr::formalism::RelationKind R>
void RuleWorkspace<LiftedTag, R>::Iteration::clear() noexcept
{
    head_updates.clear();
}

template<::tyr::formalism::RelationKind R>
RuleWorkspace<LiftedTag, R>::Solve::Solve() :
#ifndef NDEBUG
    seen_bindings(),
#endif
#ifndef TYR_ENABLE_SEMI_NAIVE
    seen_pending_rule_bindings(),
#endif
    pending_rule_bindings(),
    numeric_support_selector_workspace(),
    effect_support_scratch(),
    witness_support_scratch(),
    applicability_cache(),
    statistics()
{
}

template<::tyr::formalism::RelationKind R>
void RuleWorkspace<LiftedTag, R>::Solve::clear() noexcept
{
#ifndef NDEBUG
    seen_bindings.clear();
#endif
#ifndef TYR_ENABLE_SEMI_NAIVE
    seen_pending_rule_bindings.clear();
#endif
    pending_rule_bindings.clear();
    numeric_support_selector_workspace.clear();
    effect_support_scratch.clear();
    witness_support_scratch.clear();
    applicability_cache.clear();
}

template<::tyr::formalism::RelationKind R>
RuleWorkspace<LiftedTag, R>::Worker::Worker(const ConstRuleWorkspace<LiftedTag, R>& cws, const Common& common) :
    builder(),
    binding(),
    iteration(cws, common),
    solve()
{
}

template<::tyr::formalism::RelationKind R>
void RuleWorkspace<LiftedTag, R>::Worker::clear() noexcept
{
    iteration.clear();
    solve.clear();
}

template<::tyr::formalism::RelationKind R>
RuleWorkspace<LiftedTag, R>::RuleWorkspace(const ConstRuleWorkspace<LiftedTag, R>& cws_) : common(cws_.get_static_consistency_graph()), worker()
{
    worker.emplace_back(cws_, common);

#if defined(TYR_ENABLE_INNER_PARALLELISM) && defined(TYR_ENABLE_SEMI_NAIVE)
    // Only propositional heads use partitionable delta KCKP; numeric effects require full KCKP enumeration.
    if (supports_inner_parallelism(cws_.get_rule().get_head()) && cws_.get_rule().get_arity() > 2)
        worker.emplace_back(cws_, common);
#endif
}

template<::tyr::formalism::RelationKind R>
void RuleWorkspace<LiftedTag, R>::clear() noexcept
{
    common.clear();
    for (auto& w : worker)
        w.clear();
}

}

#endif
