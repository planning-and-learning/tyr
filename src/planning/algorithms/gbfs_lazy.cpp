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

#include "tyr/planning/algorithms/gbfs_lazy.hpp"

#include "search_engine/gbfs_lazy.hpp"
#include "search_engine/parallel.hpp"
#include "tyr/planning/algorithms/concepts.hpp"

namespace tyr::planning::gbfs_lazy
{

template<TaskKind Kind>
SearchResult<Kind> find_solution(Task<Kind>& task,
                                 StateRepository<Kind>& state_repository,
                                 AxiomEvaluator<Kind>& axiom_evaluator,
                                 SuccessorGenerator<Kind>& successor_generator,
                                 Heuristic<Kind>& heuristic,
                                 const Options<Kind>& options)
{
    return planning::detail::find_solution<planning::detail::LazyGBFSPolicy>(task,
                                                                             state_repository,
                                                                             axiom_evaluator,
                                                                             successor_generator,
                                                                             heuristic,
                                                                             options,
                                                                             "gbfs_lazy");
}

template SearchResult<LiftedTag> find_solution<LiftedTag>(Task<LiftedTag>& task,
                                                          StateRepository<LiftedTag>& state_repository,
                                                          AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                          SuccessorGenerator<LiftedTag>& successor_generator,
                                                          Heuristic<LiftedTag>& heuristic,
                                                          const Options<LiftedTag>& options);

template SearchResult<GroundTag> find_solution<GroundTag>(Task<GroundTag>& task,
                                                          StateRepository<GroundTag>& state_repository,
                                                          AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                          SuccessorGenerator<GroundTag>& successor_generator,
                                                          Heuristic<GroundTag>& heuristic,
                                                          const Options<GroundTag>& options);

static_assert(SolverConcept<Solver<LiftedTag>, LiftedTag>);
static_assert(SolverConcept<Solver<GroundTag>, GroundTag>);

}
