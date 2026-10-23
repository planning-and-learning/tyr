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
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/declarations.hpp"

#include <cassert>
#include <deque>
#include <span>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/core/closed_interval.hpp>

namespace tyr::datalog
{

/// The last lookup of a head atom: its objects and the binding they ground to, if interned.
struct HeadLookup
{
    bool valid = false;
    ygg::IndexList<formalism::Object> objects;
    std::optional<formalism::datalog::PredicateBindingIndex<formalism::FluentTag>> head;
};

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

        void initialize_iteration(const AssignmentSets& assignment_sets, const std::vector<std::vector<ygg::Index<formalism::Object>>>& delta_objects);

        void clear() noexcept;

        /// KCKP
        const StaticConsistencyGraph& static_consistency_graph;
        kckp::DeltaKCKP kckp;
        std::vector<uint64_t> dirty_vertices;  ///< Scratch of the incremental consistency graph update.

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

        /// Hyperedge anchors
        std::vector<kckp::Vertex> anchor_vertices;

        /// Heads of the current binding that the rule updates
        std::vector<formalism::datalog::PredicateBindingIndex<formalism::FluentTag>> head_bindings;

        /// Per head atom. Consecutive cliques often bind a head's variables to the same objects.
        /// Cleared per rule execution because the workspace repository is cleared between program executions.
        std::vector<HeadLookup> head_lookups;
    };

    struct Solve
    {
        Solve();

        void clear() noexcept;

#ifndef NDEBUG
        /// In debug mode, we accumulate all bindings to verify the correctness of delta-kckp
        ygg::UnorderedSet<ygg::IndexList<formalism::Object>> seen_bindings;
#endif

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
        /// Scratch of the forward checking of static literals.
        std::vector<ygg::uint_t> restrict_key;
        std::vector<ygg::uint_t> restrict_bits;
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
using LiftedRuleWorkerT = typename RuleWorkspace<LiftedTag, R>::Worker;

template<formalism::RelationKind R>
struct ConstRuleWorkspace<LiftedTag, R>
{
public:
    /// A literal of the conflicting condition and the partitions of its variables, checkable once they are all bound.
    struct ConflictingLiteralCheck
    {
        std::vector<ygg::uint_t> partitions;
        std::optional<formalism::datalog::LiteralView<LiftedTag, formalism::StaticTag>> static_literal;
        std::optional<formalism::datalog::LiteralView<LiftedTag, formalism::FluentTag>> fluent_literal;
    };

    struct VertexKeyHash
    {
        size_t operator()(const std::vector<ygg::uint_t>& key) const noexcept
        {
            auto hash = size_t(0x9e3779b97f4a7c15);
            for (const auto vertex : key)
                hash = (hash ^ vertex) * size_t(0x100000001b3) + (hash >> 29);
            return hash;
        }
    };

    /// A positive static conflicting literal seen from one of its partitions: given vertices of its other partitions
    /// (in `others` order), the bits in the target partition of the vertices that complete an atom of the literal.
    struct StaticLiteralIndex
    {
        ygg::uint_t target;
        std::vector<ygg::uint_t> others;
        /// With at most two other partitions, keyed by their vertices packed into one word; otherwise by the vertices.
        ygg::UnorderedMap<uint64_t, std::vector<ygg::uint_t>> packed_targets;
        ygg::UnorderedMap<std::vector<ygg::uint_t>, std::vector<ygg::uint_t>, VertexKeyHash, std::equal_to<>> targets;

        static uint64_t pack(std::span<const ygg::uint_t> vertices) noexcept
        {
            auto key = uint64_t(0);
            for (const auto vertex : vertices)
                key = (key << 32) | vertex;
            return key;
        }

        const std::vector<ygg::uint_t>* find(const std::vector<ygg::uint_t>& key) const noexcept
        {
            if (others.size() <= 2)
            {
                const auto it = packed_targets.find(pack(key));
                return it == packed_targets.end() ? nullptr : &it->second;
            }
            const auto it = targets.find(key);
            return it == targets.end() ? nullptr : &it->second;
        }
    };

    auto get_rule() const noexcept { return rule; }
    auto get_nullary_condition() const noexcept { return nullary_condition; }
    auto get_conflicting_overapproximation_condition() const noexcept { return conflicting_overapproximation_condition; }
    auto get_hyperedge_condition() const noexcept { return hyperedge_condition; }
    const auto& get_static_consistency_graph() const noexcept { return static_consistency_graph; }
    const auto& get_conflicting_literal_checks() const noexcept { return conflicting_literal_checks; }
    const auto& get_static_literal_indexes() const noexcept { return static_literal_indexes; }
    /// The function terms of the body's numeric constraints, by position: their bindings determine a constraint's value.
    const auto& get_numeric_constraint_static_terms() const noexcept { return numeric_constraint_static_terms; }
    const auto& get_numeric_constraint_fluent_terms() const noexcept { return numeric_constraint_fluent_terms; }

    ConstRuleWorkspace(formalism::datalog::RuleView<LiftedTag, R> rule,
                       formalism::datalog::Repository& repository,
                       kckp::Graph compatibility_graph,
                       const PredicateFactSets<formalism::StaticTag>& static_facts);

private:
    formalism::datalog::RuleView<LiftedTag, R> rule;
    formalism::datalog::ConjunctiveConditionView<GroundTag> nullary_condition;
    formalism::datalog::ConjunctiveConditionView<LiftedTag> unary_overapproximation_condition;
    formalism::datalog::ConjunctiveConditionView<LiftedTag> binary_overapproximation_condition;
    formalism::datalog::ConjunctiveConditionView<LiftedTag> conflicting_overapproximation_condition;
    formalism::datalog::ConjunctiveConditionView<LiftedTag> hyperedge_condition;

    StaticConsistencyGraph static_consistency_graph;

    std::vector<ConflictingLiteralCheck> conflicting_literal_checks;
    std::vector<StaticLiteralIndex> static_literal_indexes;
    std::vector<std::vector<formalism::datalog::FunctionTermView<LiftedTag, formalism::StaticTag>>> numeric_constraint_static_terms;
    std::vector<std::vector<formalism::datalog::FunctionTermView<LiftedTag, formalism::FluentTag>>> numeric_constraint_fluent_terms;
};

/**
 * Implementations
 */

inline PredicateHeadUpdates make_head_updates(formalism::PredicateTag) { return {}; }

inline FunctionHeadUpdates make_head_updates(formalism::FunctionTag) { return {}; }

inline bool supports_inner_parallelism(formalism::PredicateTag) noexcept { return true; }
inline bool supports_inner_parallelism(formalism::FunctionTag) noexcept { return false; }

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
void RuleWorkspace<LiftedTag, R>::Common::initialize_iteration(const AssignmentSets& assignment_sets, const std::vector<std::vector<ygg::Index<formalism::Object>>>& delta_objects)
{
    // After its first update, the rule has seen every fact but those of the delta: it listens to the predicates of its
    // positive fluent literals and runs in the iteration after they change.
    const auto delta = ConsistencyGraphDelta { delta_objects, dirty_vertices };
    const auto* incremental = kckp.get_iteration() == 0 ? nullptr : &delta;
    kckp.update([&](auto& delta_graph, auto& full_graph)
                { static_consistency_graph.initialize_dynamic_consistency_graphs(assignment_sets, kckp.get_graph_layout(), delta_graph, full_graph, incremental); });
}

template<formalism::RelationKind R>
RuleWorkspace<LiftedTag, R>::Iteration::Iteration(const ConstRuleWorkspace<LiftedTag, R>&, const Common& common) :
    head_updates(make_head_updates(R {})),
    kckp_workspace(common.kckp.get_graph_layout())
{
}

template<formalism::RelationKind R>
void RuleWorkspace<LiftedTag, R>::Iteration::clear() noexcept
{
    head_updates.clear();
    for (auto& lookup : head_lookups)
        lookup.valid = false;
}

template<formalism::RelationKind R>
RuleWorkspace<LiftedTag, R>::Solve::Solve() :
#ifndef NDEBUG
    seen_bindings(),
#endif
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
    if (supports_inner_parallelism(R {}) && cws_.get_rule().get_arity() > 2)
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
