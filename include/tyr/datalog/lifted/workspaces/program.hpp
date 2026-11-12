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
#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/lifted/kckp/workspaces/program.hpp"
#include "tyr/datalog/lifted/scheduler.hpp"
#include "tyr/datalog/lifted/workspaces/facts.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/policies/annotation_concepts.hpp"
#include "tyr/datalog/policies/annotation_types.hpp"
#include "tyr/datalog/policies/cost.hpp"
#include "tyr/datalog/policies/cost_concepts.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/datalog/policies/termination_concepts.hpp"
#include "tyr/datalog/statistics/program.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <cassert>
#include <memory>
#include <optional>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <yggdrasil/core/family.hpp>

namespace tyr::datalog
{

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
struct ProgramWorkspace<LiftedTag, AP, TP, CP>
{
    const ConstProgramWorkspace<LiftedTag>* const_workspace;
    const formalism::datalog::Repository* program_repository;
    /// Holds what evaluation derives; the program's repository is its parent.
    formalism::datalog::RepositoryPtr workspace_repository;

    FactsWorkspace<LiftedTag> facts;

    AP annotation_policy;
    PredicateAnnotations<> annotations;
    PredicateAnnotations<true> delta_annotations;

    TP tp;
    CP cost_policy;
    CostBuckets cost_buckets;
    PendingPredicateAchievers pending_achievers;

    /// The evaluation engine's own state (KCKP: assignment sets and rule workspaces).
    KCKPWorkspace engine;

    formalism::planning::Builder planning_builder;
    formalism::datalog::Builder datalog_builder;

    urd::flat_vector<urd::index<formalism::Object>> binding;

    urd::Store<urd::sequence<Scheduler<LiftedTag>>> schedulers;

    ProgramStatistics statistics;

    explicit ProgramWorkspace(const Program<LiftedTag>& program, AP annotation_policy_ = AP(), TP tp_ = TP(), CP cost_policy_ = CP());
    ProgramWorkspace(Program<LiftedTag>&&, AP = AP(), TP = TP(), CP = CP()) = delete;

    /// Discards all evaluation-local views before clearing their repository. Call only while no
    /// evaluation is active.
    void reset_evaluation();

    void clear_costs() { cost_policy.clear(); }
};

template<>
struct ConstProgramWorkspace<LiftedTag>
{
    ConstFactsWorkspace<LiftedTag> facts;
    /// The evaluation engine's shared state (KCKP: static assignment sets and const rule workspaces).
    ConstKCKPWorkspace engine;

    explicit ConstProgramWorkspace(Program<LiftedTag>& program);
};

}

#endif
