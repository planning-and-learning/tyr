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

#include "tyr/planning/algorithms/brfs.hpp"

#include "search_engine/brfs.hpp"
#include "search_engine/parallel.hpp"
#include "tyr/planning/algorithms/concepts.hpp"
#include "tyr/planning/heuristics/blind.hpp"

namespace tyr::planning::brfs
{
template<TaskKind Kind>
SearchResult<Kind> find_solution(Task<Kind>& task,
                                 StateRepository<Kind>& state_repository,
                                 AxiomEvaluator<Kind>& axiom_evaluator,
                                 SuccessorGenerator<Kind>& successor_generator,
                                 const Options<Kind>& options)
{
    static_assert(Options<Kind>::cost_mode == CostMode::UNIT);

    auto heuristic = BlindHeuristic<Kind> {};
    return detail::find_solution<detail::BreadthFirstPolicy>(task, state_repository, axiom_evaluator, successor_generator, heuristic, options, "brfs");
}

template SearchResult<LiftedTag> find_solution<LiftedTag>(Task<LiftedTag>& task,
                                                          StateRepository<LiftedTag>& state_repository,
                                                          AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                          SuccessorGenerator<LiftedTag>& successor_generator,
                                                          const Options<LiftedTag>& options);

template SearchResult<GroundTag> find_solution<GroundTag>(Task<GroundTag>& task,
                                                          StateRepository<GroundTag>& state_repository,
                                                          AxiomEvaluator<GroundTag>& axiom_evaluator,
                                                          SuccessorGenerator<GroundTag>& successor_generator,
                                                          const Options<GroundTag>& options);

static_assert(SolverConcept<Solver<LiftedTag>, LiftedTag>);
static_assert(SolverConcept<Solver<GroundTag>, GroundTag>);

}
