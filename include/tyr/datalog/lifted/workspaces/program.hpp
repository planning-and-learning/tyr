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

#ifndef TYR_DATALOG_LIFTED_WORKSPACES_PROGRAM_HPP_
#define TYR_DATALOG_LIFTED_WORKSPACES_PROGRAM_HPP_

#include "tyr/datalog/cost_buckets.hpp"
#include "tyr/datalog/lifted/policies/annotation_types.hpp"
#include "tyr/datalog/lifted/policies/cost.hpp"
#include "tyr/datalog/lifted/policies/numeric_support.hpp"
#include "tyr/datalog/lifted/rule_scheduler.hpp"
#include "tyr/datalog/lifted/workspaces/facts.hpp"
#include "tyr/datalog/lifted/workspaces/rule.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/policies/annotation_concept.hpp"
#include "tyr/datalog/policies/cost_concept.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/datalog/policies/termination_concept.hpp"
#include "tyr/datalog/programs/program.hpp"
#include "tyr/datalog/statistics/program.hpp"
#include "tyr/datalog/workspaces/program.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <cassert>
#include <concepts>
#include <optional>
#include <utility>
#include <vector>
#include <yggdrasil/core/dependent_false.hpp>

namespace tyr::datalog
{

template<AnnotationPolicyConcept<LiftedTag> AP, TerminationPolicyConcept<LiftedTag> TP, RuleCostPolicyConcept<LiftedTag> CP>
struct ProgramWorkspace<LiftedTag, AP, TP, CP>
{
    const ConstProgramWorkspace<LiftedTag>& const_workspace;
    const ::tyr::formalism::datalog::Repository& program_repository;

private:
    ::tyr::formalism::datalog::RepositoryPtr m_workspace_repository;

public:
    ::tyr::formalism::datalog::Repository& workspace_repository;

    FactsWorkspace<LiftedTag> facts;

    AP annotation_policy;
    PredicateAnnotations<LiftedTag> annotations;
    FunctionAnnotations<LiftedTag> numeric_annotations;
    ConcurrentPredicateAnnotations delta_annotations;
    ConcurrentFunctionAnnotations delta_numeric_annotations;
    std::optional<NumericSupportSelector<LiftedTag>> numeric_support_selector;

    TP tp;
    CP cost_policy;

    std::vector<std::unique_ptr<RuleWorkspace<LiftedTag, ::tyr::formalism::PredicateTag>>> predicate_rules;
    std::vector<std::unique_ptr<RuleWorkspace<LiftedTag, ::tyr::formalism::FunctionTag>>> function_rules;

    template<::tyr::formalism::RelationKind R>
    auto& get_rules() noexcept
    {
        if constexpr (std::same_as<R, ::tyr::formalism::PredicateTag>)
            return predicate_rules;
        else if constexpr (std::same_as<R, ::tyr::formalism::FunctionTag>)
            return function_rules;
        else
            static_assert(ygg::dependent_false<R>::value, "Missing case");
    }

    template<::tyr::formalism::RelationKind R>
    const auto& get_rules() const noexcept
    {
        if constexpr (std::same_as<R, ::tyr::formalism::PredicateTag>)
            return predicate_rules;
        else if constexpr (std::same_as<R, ::tyr::formalism::FunctionTag>)
            return function_rules;
        else
            static_assert(ygg::dependent_false<R>::value, "Missing case");
    }

    ::tyr::formalism::planning::Builder planning_builder;
    ::tyr::formalism::datalog::Builder datalog_builder;

    ygg::IndexList<::tyr::formalism::Object> binding;

    RuleSchedulerStrata schedulers;

    CostBuckets cost_buckets;

    ProgramStatistics statistics;

    explicit ProgramWorkspace(const Program<LiftedTag>& program, AP annotation_policy = AP(), TP tp = TP(), CP cost_policy = CP());
    ProgramWorkspace(Program<LiftedTag>&&, AP = AP(), TP = TP(), CP = CP()) = delete;

    /// Discards all evaluation-local views before clearing their repository. Call only while no
    /// evaluation is active.
    void reset_evaluation();

    void clear_costs() { cost_policy.clear(); }

    const auto& get_numeric_support_selector() const noexcept
    {
        assert(numeric_support_selector.has_value());
        return *numeric_support_selector;
    }

    template<typename Callback>
    void for_each_numeric_support(const NumericSupport<LiftedTag>& support, Callback&& callback) const
    {
        std::forward<Callback>(callback)(support.get_key(), support.get_interval(), support.get_cost());
    }
};

template<>
struct ConstProgramWorkspace<LiftedTag>
{
    ConstFactsWorkspace<LiftedTag> facts;

    std::vector<std::optional<ConstRuleWorkspace<LiftedTag, ::tyr::formalism::PredicateTag>>> predicate_rules;
    std::vector<std::optional<ConstRuleWorkspace<LiftedTag, ::tyr::formalism::FunctionTag>>> function_rules;

    template<::tyr::formalism::RelationKind R>
    auto& get_rules() noexcept
    {
        if constexpr (std::same_as<R, ::tyr::formalism::PredicateTag>)
            return predicate_rules;
        else if constexpr (std::same_as<R, ::tyr::formalism::FunctionTag>)
            return function_rules;
        else
            static_assert(ygg::dependent_false<R>::value, "Missing case");
    }

    template<::tyr::formalism::RelationKind R>
    const auto& get_rules() const noexcept
    {
        if constexpr (std::same_as<R, ::tyr::formalism::PredicateTag>)
            return predicate_rules;
        else if constexpr (std::same_as<R, ::tyr::formalism::FunctionTag>)
            return function_rules;
        else
            static_assert(ygg::dependent_false<R>::value, "Missing case");
    }

    explicit ConstProgramWorkspace(Program<LiftedTag>& program);
};

}

#endif
