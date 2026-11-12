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
#include "tyr/datalog/lifted/kckp/consistency_graph.hpp"
#include "tyr/datalog/rule_evaluation.hpp"
#include "tyr/datalog/statistics/rule.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_view.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <cassert>
#include <type_traits>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/segmented.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/std_set.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <variant>
#include <vector>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/formalism/row_view.hpp>

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

template<>
struct RuleWorkspace<LiftedTag>
{
    struct Common
    {
        explicit Common(const StaticConsistencyGraph& static_consistency_graph_);

        void initialize_iteration(const AssignmentSets& assignment_sets);

        void clear() noexcept;

        /// KCKP
        const StaticConsistencyGraph* static_consistency_graph;
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
        explicit Iteration(const ConstRuleWorkspace<LiftedTag>& cws, const Common& common);

        void clear() noexcept;

        /// Head updates
        PredicateHeadUpdates head_updates;

        /// KCKP
        kckp::Workspace kckp_workspace;

        /// Hyperedge anchors
        urd::Store<urd::sequence<kckp::Vertex>> anchor_vertices;

        /// Heads of the current binding that the rule updates
        urd::Store<urd::sequence<formalism::datalog::PredicateRowIndex<formalism::FluentTag>>> head_bindings;
    };

    struct Solve
    {
        Solve();

        void clear() noexcept;

#ifndef NDEBUG
        /// In debug mode, we accumulate all bindings to verify the correctness of delta-kckp
        urd::Store<urd::set<urd::flat_vector<urd::index<formalism::Object>>>> seen_bindings;
#endif

        ApplicabilityCache applicability_cache;

        /// Statistics
        RuleWorkerStatistics statistics;
    };

    struct Worker
    {
        explicit Worker(const ConstRuleWorkspace<LiftedTag>& cws, const Common& common);

        void clear() noexcept;

        formalism::datalog::Builder builder;
        urd::flat_vector<urd::index<formalism::Object>> binding;

        Iteration iteration;
        Solve solve;
    };

    explicit RuleWorkspace(const ConstRuleWorkspace<LiftedTag>& cws);
    RuleWorkspace(const RuleWorkspace& other) = delete;
    RuleWorkspace& operator=(const RuleWorkspace& other) = delete;
    RuleWorkspace(RuleWorkspace&& other) = delete;
    RuleWorkspace& operator=(RuleWorkspace&& other) = delete;
    ~RuleWorkspace() = default;

    void clear() noexcept;

    Common common;

    urd::Store<urd::sequence<Worker>, urd::stable_refs> worker;
};

using LiftedRuleWorkerT = typename RuleWorkspace<LiftedTag>::Worker;

template<>
struct ConstRuleWorkspace<LiftedTag>
{
public:
    [[nodiscard]] auto get_rule() const noexcept { return rule; }
    [[nodiscard]] auto get_nullary_condition() const noexcept { return nullary_condition; }
    [[nodiscard]] auto get_conflicting_overapproximation_condition() const noexcept { return conflicting_overapproximation_condition; }
    [[nodiscard]] auto get_hyperedge_condition() const noexcept { return hyperedge_condition; }
    [[nodiscard]] const auto& get_static_consistency_graph() const noexcept { return static_consistency_graph; }

    ConstRuleWorkspace(formalism::datalog::RuleView<LiftedTag> rule_, formalism::datalog::Repository& repository, kckp::Graph compatibility_graph);

private:
    formalism::datalog::RuleView<LiftedTag> rule;
    formalism::datalog::ConjunctiveConditionView<GroundTag> nullary_condition;
    formalism::datalog::ConjunctiveConditionView<LiftedTag> unary_overapproximation_condition;
    formalism::datalog::ConjunctiveConditionView<LiftedTag> binary_overapproximation_condition;
    formalism::datalog::ConjunctiveConditionView<LiftedTag> conflicting_overapproximation_condition;
    formalism::datalog::ConjunctiveConditionView<LiftedTag> hyperedge_condition;

    StaticConsistencyGraph static_consistency_graph;
};

/**
 * Implementations
 */

inline bool supports_inner_parallelism() noexcept { return true; }

}

#endif
