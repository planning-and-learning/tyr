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

#ifndef TYR_PYTHON_PLANNING_PLANNING_HPP_
#define TYR_PYTHON_PLANNING_PLANNING_HPP_

#include "pytyr/planning/algorithms/search_result.hpp"
#include "pytyr/planning/algorithms/strategies/goal.hpp"
#include "pytyr/planning/algorithms/strategies/pruning.hpp"
#include "pytyr/planning/axioms/axiom_evaluator.hpp"
#include "pytyr/planning/heuristics/heuristics.hpp"
#include "pytyr/planning/module.hpp"
#include "pytyr/planning/nodes/nodes.hpp"
#include "pytyr/planning/plans/plans.hpp"
#include "pytyr/planning/states/states.hpp"
#include "pytyr/planning/successors/successor_generator.hpp"
#include "pytyr/planning/successors/successors.hpp"

#include <nanobind/nanobind.h>
#include <tyr/planning/declarations.hpp>
#include <tyr/planning/states/worker_state_index.hpp>
#include <tyr/planning/tasks/ground/task.hpp>
#include <tyr/planning/tasks/lifted/task.hpp>
#include <tyr/planning/tasks/task.hpp>
#include <tyr/planning/heuristics/lmcut.hpp>
#include <tyr/planning/heuristics/rpg_add.hpp>
#include <tyr/planning/heuristics/rpg_ff.hpp>
#include <tyr/planning/heuristics/rpg_max.hpp>

#include "pytyr/formalism/conversions.hpp"
#include "pytyr/module.hpp"
#include <nanobind/stl/chrono.h>
#include <nanobind/stl/function.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <tyr/planning/algorithms/astar_eager.hpp>
#include <tyr/planning/algorithms/astar_eager/event_handler.hpp>
#include <tyr/planning/algorithms/brfs.hpp>
#include <tyr/planning/algorithms/brfs/event_handler.hpp>
#include <tyr/planning/algorithms/gbfs_lazy.hpp>
#include <tyr/planning/algorithms/gbfs_lazy/event_handler.hpp>
#include <tyr/planning/algorithms/iw.hpp>
#include <tyr/planning/algorithms/iw/novelty_table.hpp>
#include <tyr/planning/algorithms/iw/pruning_strategy.hpp>
#include <tyr/planning/algorithms/serialized.hpp>
#include <tyr/planning/algorithms/serialized/event_handler.hpp>
#include <tyr/planning/algorithms/serialized/statistics.hpp>
#include <tyr/planning/algorithms/siw.hpp>
#include <tyr/planning/algorithms/siw/event_handler.hpp>
#include <tyr/planning/algorithms/siw/statistics.hpp>
#include <tyr/planning/algorithms/statistics.hpp>
#include <tyr/planning/algorithms/strategies/goal.hpp>
#include <tyr/planning/algorithms/strategies/pruning.hpp>
#include <tyr/planning/algorithms/search_space/search_node.hpp>
#include <tyr/planning/algorithms/state_routing/dist_hash.hpp>
#include <tyr/planning/axioms/axiom_evaluator.hpp>
#include <tyr/planning/tasks/ground/task.hpp>
#include <tyr/planning/tasks/lifted/task.hpp>
#include <tyr/planning/states/state_builder.hpp>
#include <tyr/planning/states/state_repository.hpp>
#include <tyr/planning/states/storages/policies/policy.hpp>
#include <tyr/planning/tasks/task.hpp>
#include <tyr/planning/states/worker_state_index.hpp>
#include <urd/python/bindings.hpp>
#include <urd/python/sequences/sequence.hpp>
#include <urd/python/values/dense_set.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <yggdrasil/execution/onetbb.hpp>
#include <yggdrasil/python/bindings.hpp>

namespace tyr::planning
{

/// The classes bound identically for both task kinds.
template<TaskKind Kind>
void bind_kind_definitions(nb::module_& m)
{
    bind_states<Kind>(m);
    bind_nodes<Kind>(m);
    bind_plans<Kind>(m);
    bind_successors<Kind>(m);
    bind_axiom_evaluator<Kind>(m, "AxiomEvaluator");
    bind_state_repository<Kind>(m, "StateRepository");
    bind_successor_generator<Kind>(m, "SuccessorGenerator");
    bind_search_result<Kind>(m, "SearchResult");
    bind_goal_strategy<Kind>(m, "GoalStrategy");
    bind_conjunctive_goal_strategy<Kind>(m, "ConjunctiveGoalStrategy");
    bind_exhaustive_goal_strategy<Kind>(m, "ExhaustiveGoalStrategy");
    bind_pruning_strategy<Kind>(m, "PruningStrategy");
    bind_heuristic<Kind>(m, "Heuristic");
    bind_blind_heuristic<Kind>(m, "BlindHeuristic");
    bind_task_heuristic<Kind, MaxRPGHeuristic<Kind>>(m, "MaxRPGHeuristic");
    bind_task_heuristic<Kind, AddRPGHeuristic<Kind>>(m, "AddRPGHeuristic");
    bind_task_heuristic<Kind, FFRPGHeuristic<Kind>>(m, "FFRPGHeuristic");
    bind_task_heuristic<Kind, LMCutHeuristic<Kind>>(m, "LMCutHeuristic");
    bind_goal_count_heuristic<Kind>(m, "GoalCountHeuristic");
}

}  // namespace tyr::planning

#endif
