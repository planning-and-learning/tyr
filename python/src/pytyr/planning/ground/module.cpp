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

#include "module.hpp"

#include "../planning.hpp"
#include "astar_eager/module.hpp"
#include "brfs/module.hpp"
#include "gbfs_lazy/module.hpp"
#include "iw/module.hpp"
#include "siw/module.hpp"

namespace tyr::planning
{

void bind_ground_module_definitions(nb::module_& m)
{
    auto astar_eager_module = m.def_submodule("astar_eager");
    astar_eager::bind_ground_module_definitions(astar_eager_module);

    auto brfs_module = m.def_submodule("brfs");
    brfs::bind_ground_module_definitions(brfs_module);

    auto gbfs_lazy_module = m.def_submodule("gbfs_lazy");
    gbfs_lazy::bind_ground_module_definitions(gbfs_lazy_module);

    auto iw_module = m.def_submodule("iw");
    iw::bind_ground_module_definitions(iw_module);

    auto siw_module = m.def_submodule("siw");
    siw::bind_ground_module_definitions(siw_module);

    nb::class_<Task<GroundTag>>(m, "Task")  //
        .def("get_formalism_task", &Task<GroundTag>::get_formalism_task, nb::rv_policy::reference_internal)
        .def("get_repository", &Task<GroundTag>::get_repository)
        .def("get_task", &Task<GroundTag>::get_task, nb::keep_alive<0, 1>())
        .def("get_fdr_context", &Task<GroundTag>::get_fdr_context, nb::rv_policy::reference_internal);

    bind_index<ygg::Index<State<GroundTag>>>(m, "StateIndex");
    bind_state<GroundTag>(m, "State");
    bind_state_builder<GroundTag>(m, "StateBuilder");
    bind_node<GroundTag>(m, "Node");
    bind_labeled_node<GroundTag>(m, "LabeledNode");
    bind_plan<GroundTag>(m, "Plan");
    bind_axiom_evaluator<GroundTag>(m, "AxiomEvaluator");
    bind_axiom_evaluator_factory<GroundTag>(m, "AxiomEvaluatorFactory");
    bind_state_repository<GroundTag>(m, "StateRepository");
    bind_state_repository_factory<GroundTag>(m, "StateRepositoryFactory");
    bind_successor_generator<GroundTag>(m, "SuccessorGenerator");
    bind_successor_generator_factory<GroundTag>(m, "SuccessorGeneratorFactory");
    bind_search_result<GroundTag>(m, "SearchResult");
    bind_goal_strategy<GroundTag>(m, "GoalStrategy");
    bind_conjunctive_goal_strategy<GroundTag>(m, "ConjunctiveGoalStrategy");
    bind_exhaustive_goal_strategy<GroundTag>(m, "ExhaustiveGoalStrategy");
    bind_pruning_strategy<GroundTag>(m, "PruningStrategy");
    bind_heuristic<GroundTag>(m, "Heuristic");
    bind_blind_heuristic<GroundTag>(m, "BlindHeuristic");
    bind_rpg_max_heuristic<GroundTag>(m, "MaxRPGHeuristic");
    bind_rpg_add_heuristic<GroundTag>(m, "AddRPGHeuristic");
    bind_rpg_ff_heuristic<GroundTag>(m, "FFRPGHeuristic");
    bind_lmcut_heuristic<GroundTag>(m, "LMCutHeuristic");
    bind_goal_count_heuristic<GroundTag>(m, "GoalCountHeuristic");
}

}  // namespace tyr::planning
