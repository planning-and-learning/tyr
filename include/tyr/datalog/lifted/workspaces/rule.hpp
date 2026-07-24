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
    ygg::Index<::tyr::formalism::Predicate<::tyr::formalism::FluentTag>> relation;
    using Row = ygg::Index<::tyr::formalism::Row>;

    ygg::UnorderedSet<Row> rows;
    std::vector<Row> sorted_rows;

    PredicateHeadIteration() = default;
    explicit PredicateHeadIteration(ygg::Index<::tyr::formalism::Predicate<::tyr::formalism::FluentTag>> relation) : relation(relation), rows(), sorted_rows()
    {
    }

    void clear() noexcept
    {
        rows.clear();
        sorted_rows.clear();
    }

    const std::vector<Row>& get_sorted_rows()
    {
        sorted_rows.assign(rows.begin(), rows.end());
        std::sort(sorted_rows.begin(), sorted_rows.end());
        return sorted_rows;
    }
};

struct FunctionHeadUpdate : ygg::comparison::Mixin<FunctionHeadUpdate>
{
    ygg::Index<::tyr::formalism::Row> row;
    ygg::ClosedInterval<ygg::float_t> interval;
    Cost cost;

    FunctionHeadUpdate(ygg::Index<::tyr::formalism::Row> row, ygg::ClosedInterval<ygg::float_t> interval, Cost cost) : row(row), interval(interval), cost(cost)
    {
    }

    auto identifying_members() const noexcept { return std::tie(row, interval, cost); }
};

struct FunctionHeadIteration
{
    ygg::Index<::tyr::formalism::Function<::tyr::formalism::FluentTag>> relation;
    ygg::UnorderedSet<FunctionHeadUpdate> updates;
    std::vector<FunctionHeadUpdate> sorted_updates;

    FunctionHeadIteration() = default;
    explicit FunctionHeadIteration(ygg::Index<::tyr::formalism::Function<::tyr::formalism::FluentTag>> relation) :
        relation(relation),
        updates(),
        sorted_updates()
    {
    }

    void clear() noexcept
    {
        updates.clear();
        sorted_updates.clear();
    }

    const std::vector<FunctionHeadUpdate>& get_sorted_updates()
    {
        sorted_updates.assign(updates.begin(), updates.end());
        std::sort(sorted_updates.begin(), sorted_updates.end());
        return sorted_updates;
    }
};

using RuleHeadIteration = std::variant<PredicateHeadIteration, FunctionHeadIteration>;

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

template<>
struct RuleWorkspace<LiftedTag>
{
    template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
    struct Instance
    {
        struct Common
        {
            explicit Common(const ::tyr::formalism::datalog::Repository& program_repository,
                            const ::tyr::formalism::datalog::Repository& workspace_repository,
                            const StaticConsistencyGraph& static_consistency_graph);

            void initialize_iteration(const StaticConsistencyGraph& static_consistency_graph, const AssignmentSets& assignment_sets);

            void clear() noexcept;

            /// Program repository to ground witnesses for which ground entities must already exist and we can simply call find.
            const ::tyr::formalism::datalog::Repository& program_repository;
            const ::tyr::formalism::datalog::Repository& workspace_repository;

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
            explicit Iteration(::tyr::formalism::datalog::RepositoryFactory& factory, const ConstRuleWorkspace<LiftedTag>& cws, const Common& common);

            void clear() noexcept;

            /// Merge stage into rule execution context
            ::tyr::formalism::datalog::Repository workspace_overlay_repository;

            /// Heads
            RuleHeadIteration head;

            // Propositional annotations are keyed by heads in workspace_overlay_repository.
            SelectedPredicateAnnotations<LiftedTag> and_annot;
            SelectedFunctionAnnotations<LiftedTag> numeric_and_annot;

            /// KPKC
            kpkc::Workspace kpkc_workspace;
        };

        struct Solve
        {
            explicit Solve(::tyr::formalism::datalog::RepositoryFactory& factory,
                           const ::tyr::formalism::datalog::Repository& program_repository,
                           const ::tyr::formalism::datalog::Repository& workspace_repository,
                           const AndAP& and_ap);

            void clear() noexcept;

            AndAP and_ap;

            /// Persistent memory
            ::tyr::formalism::datalog::Repository program_overlay_repository;

            /// In debug mode, we accumulate all bindings to verify the correctness of delta-kpkc
            ygg::UnorderedSet<ygg::IndexList<::tyr::formalism::Object>> seen_bindings_dbg;

            ygg::UnorderedSet<::tyr::formalism::datalog::RuleBindingView> pending_rule_bindings;
            std::vector<::tyr::formalism::datalog::RuleBindingView> pending_rule_binding_scratch;

            const std::vector<::tyr::formalism::datalog::RuleBindingView>& get_sorted_pending_rule_bindings();

            NumericSupportSelectorWorkspace<LiftedTag> numeric_support_selector_workspace;
            std::vector<NumericSupport<LiftedTag>> numeric_support_scratch;
            std::vector<NumericSupport<LiftedTag>> witness_support_scratch;
            ApplicabilityCache applicability_cache;

            /// Statistics
            RuleWorkerStatistics statistics;
        };

        struct Worker
        {
            explicit Worker(::tyr::formalism::datalog::RepositoryFactory& factory,
                            const ::tyr::formalism::datalog::Repository& program_repository,
                            const ::tyr::formalism::datalog::Repository& workspace_repository,
                            const ConstRuleWorkspace<LiftedTag>& cws,
                            const Common& common,
                            const AndAP& and_ap);

            void clear() noexcept;

            ::tyr::formalism::datalog::Builder builder;
            ygg::IndexList<::tyr::formalism::Object> binding;

            Iteration iteration;
            Solve solve;
        };

        Instance(::tyr::formalism::datalog::RepositoryFactory& factory,
                 const ::tyr::formalism::datalog::Repository& program_repository,
                 const ::tyr::formalism::datalog::Repository& workspace_repository,
                 const ConstRuleWorkspace<LiftedTag>& cws,
                 const AndAP& and_ap);
        Instance(const Instance& other) = delete;
        Instance& operator=(const Instance& other) = delete;
        Instance(Instance&& other) = delete;
        Instance& operator=(Instance&& other) = delete;

        void clear() noexcept;

        Common common;

        std::deque<Worker> worker;
    };
};

template<>
struct ConstRuleWorkspace<LiftedTag>
{
public:
    auto get_rule() const noexcept { return rule; }
    auto get_witness_rule() const noexcept { return witness_rule; }
    auto get_nullary_condition() const noexcept { return nullary_condition; }
    auto get_unary_overapproximation_rule() const noexcept { return unary_overapproximation_rule; }
    auto get_binary_overapproximation_rule() const noexcept { return binary_overapproximation_rule; }
    auto get_static_binary_overapproximation_rule() const noexcept { return static_binary_overapproximation_rule; }
    auto get_conflicting_overapproximation_rule() const noexcept { return conflicting_overapproximation_rule; }
    const auto& get_static_consistency_graph() const noexcept { return static_consistency_graph; }

    ConstRuleWorkspace(::tyr::formalism::datalog::RuleView rule,
                       ::tyr::formalism::datalog::Repository& repository,
                       const analysis::VariableDomainList& parameter_domains,
                       size_t num_objects,
                       size_t num_fluent_predicates,
                       const TaggedAssignmentSets<::tyr::formalism::StaticTag>& static_assignment_sets);

private:
    ::tyr::formalism::datalog::RuleView rule;
    ::tyr::formalism::datalog::RuleView witness_rule;
    ::tyr::formalism::datalog::GroundConjunctiveConditionView nullary_condition;
    ::tyr::formalism::datalog::RuleView unary_overapproximation_rule;
    ::tyr::formalism::datalog::RuleView binary_overapproximation_rule;
    ::tyr::formalism::datalog::RuleView static_binary_overapproximation_rule;
    ::tyr::formalism::datalog::RuleView conflicting_overapproximation_rule;

    StaticConsistencyGraph static_consistency_graph;
};

/**
 * Implementations
 */

template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
RuleWorkspace<LiftedTag>::Instance<AndAP>::Common::Common(const ::tyr::formalism::datalog::Repository& program_repository,
                                                          const ::tyr::formalism::datalog::Repository& workspace_repository,
                                                          const StaticConsistencyGraph& static_consistency_graph) :
    program_repository(program_repository),
    workspace_repository(workspace_repository),
    kpkc(static_consistency_graph),
    statistics()
{
}

template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
void RuleWorkspace<LiftedTag>::Instance<AndAP>::Common::clear() noexcept
{
    kpkc.reset();
}

template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
void RuleWorkspace<LiftedTag>::Instance<AndAP>::Common::initialize_iteration(const StaticConsistencyGraph& static_consistency_graph,
                                                                             const AssignmentSets& assignment_sets)
{
    kpkc.set_next_assignment_sets(static_consistency_graph, assignment_sets);
}

template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
RuleWorkspace<LiftedTag>::Instance<AndAP>::Iteration::Iteration(::tyr::formalism::datalog::RepositoryFactory& factory,
                                                                const ConstRuleWorkspace<LiftedTag>& cws,
                                                                const Common& common) :
    workspace_overlay_repository(factory.create(&common.workspace_repository)),
    head(visit(
        [](auto&& head) -> RuleHeadIteration
        {
            using Head = std::decay_t<decltype(head)>;

            if constexpr (std::is_same_v<Head, ::tyr::formalism::datalog::AtomView<::tyr::formalism::FluentTag>>)
            {
                return PredicateHeadIteration(head.get_predicate().get_index());
            }
            else
            {
                return visit([](auto&& effect) -> RuleHeadIteration { return FunctionHeadIteration(effect.get_fterm().get_function().get_index()); },
                             head.get_variant());
            }
        },
        cws.get_rule().get_head())),
    and_annot(),
    numeric_and_annot(),
    kpkc_workspace(common.kpkc.get_graph_layout())
{
}

template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
void RuleWorkspace<LiftedTag>::Instance<AndAP>::Iteration::clear() noexcept
{
    workspace_overlay_repository.clear();
    std::visit([](auto& arg) { arg.clear(); }, head);
    and_annot.clear();
    numeric_and_annot.clear();
}

template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
RuleWorkspace<LiftedTag>::Instance<AndAP>::Solve::Solve(::tyr::formalism::datalog::RepositoryFactory& factory,
                                                        const ::tyr::formalism::datalog::Repository& program_repository,
                                                        const ::tyr::formalism::datalog::Repository& workspace_repository,
                                                        const AndAP& and_ap) :
    and_ap(and_ap),
    program_overlay_repository(factory.create(&program_repository)),
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

template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
void RuleWorkspace<LiftedTag>::Instance<AndAP>::Solve::clear() noexcept
{
    program_overlay_repository.clear();
    seen_bindings_dbg.clear();
    pending_rule_bindings.clear();
    pending_rule_binding_scratch.clear();
    numeric_support_selector_workspace.clear();
    numeric_support_scratch.clear();
    witness_support_scratch.clear();
    applicability_cache.clear();
    and_ap.clear_achievers();
}

template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
const std::vector<::tyr::formalism::datalog::RuleBindingView>& RuleWorkspace<LiftedTag>::Instance<AndAP>::Solve::get_sorted_pending_rule_bindings()
{
    pending_rule_binding_scratch.assign(pending_rule_bindings.begin(), pending_rule_bindings.end());
    // DeltaKPKC emits canonical bindings within a worker. This sort only stabilizes unordered
    // worker-merge traversal, so RuleBindingView identity is sufficient and avoids object lookup.
    std::sort(pending_rule_binding_scratch.begin(), pending_rule_binding_scratch.end());
    return pending_rule_binding_scratch;
}

template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
RuleWorkspace<LiftedTag>::Instance<AndAP>::Worker::Worker(::tyr::formalism::datalog::RepositoryFactory& factory,
                                                          const ::tyr::formalism::datalog::Repository& program_repository,
                                                          const ::tyr::formalism::datalog::Repository& workspace_repository,
                                                          const ConstRuleWorkspace<LiftedTag>& cws,
                                                          const Common& common,
                                                          const AndAP& and_ap) :
    builder(),
    binding(),
    iteration(factory, cws, common),
    solve(factory, program_repository, workspace_repository, and_ap)
{
}

template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
void RuleWorkspace<LiftedTag>::Instance<AndAP>::Worker::clear() noexcept
{
    iteration.clear();
    solve.clear();
}

template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
RuleWorkspace<LiftedTag>::Instance<AndAP>::Instance(::tyr::formalism::datalog::RepositoryFactory& factory_,
                                                    const ::tyr::formalism::datalog::Repository& program_repository_,
                                                    const ::tyr::formalism::datalog::Repository& workspace_repository_,
                                                    const ConstRuleWorkspace<LiftedTag>& cws_,
                                                    const AndAP& and_ap_) :
    common(program_repository_, workspace_repository_, cws_.get_static_consistency_graph()),
    worker()
{
    worker.emplace_back(factory_, program_repository_, workspace_repository_, cws_, common, and_ap_);

#if defined(TYR_ENABLE_INNER_PARALLELISM) && defined(TYR_ENABLE_SEMI_NAIVE)
    // Only propositional heads use partitionable delta KPKC; numeric effects require full KPKC enumeration.
    if (std::holds_alternative<PredicateHeadIteration>(worker.front().iteration.head) && cws_.get_rule().get_arity() > 2)
        worker.emplace_back(factory_, program_repository_, workspace_repository_, cws_, common, and_ap_);
#endif
}

template<AndAnnotationPolicyConcept<LiftedTag> AndAP>
void RuleWorkspace<LiftedTag>::Instance<AndAP>::clear() noexcept
{
    common.clear();
    for (auto& w : worker)
        w.clear();
}

}

#endif
