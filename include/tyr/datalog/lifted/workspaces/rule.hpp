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
#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/lifted/consistency_graph.hpp"
#include "tyr/datalog/rule_evaluation.hpp"
#include "tyr/datalog/statistics/rule.hpp"
#include "tyr/formalism/binding_index.hpp"
#include "tyr/formalism/datalog/atom_index.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/object_index.hpp"

#include <cassert>
#include <deque>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/core/closed_interval.hpp>

namespace tyr::datalog
{

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

template<formalism::RelationKind R>
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
        RuleHeadUpdatesT<R> head_updates;

        /// KCKP
        kckp::Workspace kckp_workspace;
    };

    struct Solve
    {
        Solve();

        void clear() noexcept;

#ifndef NDEBUG
        /// In debug mode, we accumulate all bindings to verify the correctness of delta-kckp
        ygg::UnorderedSet<ygg::IndexList<formalism::Object>> seen_bindings;
#endif

#ifndef TYR_ENABLE_SEMI_NAIVE
        ygg::UnorderedSet<formalism::datalog::RuleBindingView<R>> seen_pending_rule_bindings;
#endif
        std::vector<formalism::datalog::RuleBindingView<R>> pending_rule_bindings;

        RuleEvaluationWorkspace rule_evaluation_workspace;
        ApplicabilityCache applicability_cache;

        /// Statistics
        RuleWorkerStatistics statistics;
    };

    struct Worker
    {
        explicit Worker(const ConstRuleWorkspace<LiftedTag, R>& cws, const Common& common);

        void clear() noexcept;

        formalism::datalog::Builder builder;
        ygg::IndexList<formalism::Object> binding;

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

template<formalism::RelationKind R>
struct ConstRuleWorkspace<LiftedTag, R>
{
public:
    auto get_rule() const noexcept { return rule; }
    auto get_nullary_condition() const noexcept { return nullary_condition; }
    const auto& get_lifted_effects() const noexcept { return lifted_effects; }
    const auto& get_nullary_effects() const noexcept { return nullary_effects; }
    auto get_conflicting_overapproximation_condition() const noexcept { return conflicting_overapproximation_condition; }
    const auto& get_static_consistency_graph() const noexcept { return static_consistency_graph; }

    ConstRuleWorkspace(formalism::datalog::RuleView<LiftedTag, R> rule, formalism::datalog::Repository& repository, kckp::Graph compatibility_graph);

private:
    formalism::datalog::RuleView<LiftedTag, R> rule;
    formalism::datalog::ConjunctiveConditionView<GroundTag> nullary_condition;
    formalism::datalog::NumericEffectOperatorViewList<LiftedTag, formalism::FluentTag> lifted_effects;
    formalism::datalog::NumericEffectOperatorViewList<GroundTag, formalism::FluentTag> nullary_effects;
    formalism::datalog::ConjunctiveConditionView<LiftedTag> unary_overapproximation_condition;
    formalism::datalog::ConjunctiveConditionView<LiftedTag> binary_overapproximation_condition;
    formalism::datalog::ConjunctiveConditionView<LiftedTag> conflicting_overapproximation_condition;

    StaticConsistencyGraph static_consistency_graph;
};

/**
 * Implementations
 */

inline PredicateHeadUpdates make_head_updates(formalism::datalog::AtomView<LiftedTag, formalism::FluentTag>) { return {}; }

inline FunctionHeadUpdates make_head_updates(formalism::datalog::NumericEffectOperatorView<LiftedTag, formalism::FluentTag>) { return {}; }

inline bool supports_inner_parallelism(formalism::datalog::AtomView<LiftedTag, formalism::FluentTag>) noexcept { return true; }
inline bool supports_inner_parallelism(formalism::datalog::NumericEffectOperatorView<LiftedTag, formalism::FluentTag>) noexcept { return false; }

template<formalism::RelationKind R>
RuleWorkspace<LiftedTag, R>::Common::Common(const StaticConsistencyGraph& static_consistency_graph) :
    static_consistency_graph(static_consistency_graph),
    kckp(static_consistency_graph.get_graph(), static_consistency_graph.get_partitioned_adjacency_layout()),
    statistics()
{
}

template<formalism::RelationKind R>
void RuleWorkspace<LiftedTag, R>::Common::clear() noexcept
{
    kckp.reset();
}

template<formalism::RelationKind R>
void RuleWorkspace<LiftedTag, R>::Common::initialize_iteration(const AssignmentSets& assignment_sets)
{
    kckp.update([&](auto& delta_graph, auto& full_graph)
                { static_consistency_graph.initialize_dynamic_consistency_graphs(assignment_sets, kckp.get_graph_layout(), delta_graph, full_graph); });
}

template<formalism::RelationKind R>
RuleWorkspace<LiftedTag, R>::Iteration::Iteration(const ConstRuleWorkspace<LiftedTag, R>& cws, const Common& common) :
    head_updates(make_head_updates(cws.get_rule().get_head())),
    kckp_workspace(common.kckp.get_graph_layout())
{
}

template<formalism::RelationKind R>
void RuleWorkspace<LiftedTag, R>::Iteration::clear() noexcept
{
    head_updates.clear();
}

template<formalism::RelationKind R>
RuleWorkspace<LiftedTag, R>::Solve::Solve() :
#ifndef NDEBUG
    seen_bindings(),
#endif
#ifndef TYR_ENABLE_SEMI_NAIVE
    seen_pending_rule_bindings(),
#endif
    pending_rule_bindings(),
    rule_evaluation_workspace(),
    applicability_cache(),
    statistics()
{
}

template<formalism::RelationKind R>
void RuleWorkspace<LiftedTag, R>::Solve::clear() noexcept
{
#ifndef NDEBUG
    seen_bindings.clear();
#endif
#ifndef TYR_ENABLE_SEMI_NAIVE
    seen_pending_rule_bindings.clear();
#endif
    pending_rule_bindings.clear();
    rule_evaluation_workspace.selector.clear();
    rule_evaluation_workspace.exact_supports.clear();
    applicability_cache.clear();
}

template<formalism::RelationKind R>
RuleWorkspace<LiftedTag, R>::Worker::Worker(const ConstRuleWorkspace<LiftedTag, R>& cws, const Common& common) :
    builder(),
    binding(),
    iteration(cws, common),
    solve()
{
}

template<formalism::RelationKind R>
void RuleWorkspace<LiftedTag, R>::Worker::clear() noexcept
{
    iteration.clear();
    solve.clear();
}

template<formalism::RelationKind R>
RuleWorkspace<LiftedTag, R>::RuleWorkspace(const ConstRuleWorkspace<LiftedTag, R>& cws_) : common(cws_.get_static_consistency_graph()), worker()
{
    worker.emplace_back(cws_, common);

#if defined(TYR_ENABLE_INNER_PARALLELISM) && defined(TYR_ENABLE_SEMI_NAIVE)
    // Only propositional heads use partitionable delta KCKP; numeric effects require full KCKP enumeration.
    if (supports_inner_parallelism(cws_.get_rule().get_head()) && cws_.get_rule().get_arity() > 2)
        worker.emplace_back(cws_, common);
#endif
}

template<formalism::RelationKind R>
void RuleWorkspace<LiftedTag, R>::clear() noexcept
{
    common.clear();
    for (auto& w : worker)
        w.clear();
}

}

#endif
