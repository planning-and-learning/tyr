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

#include "tyr/planning/algorithms/astar_eager.hpp"

#include "search_engine/astar_eager.hpp"
#include "search_engine/sequential.hpp"
#include "tyr/planning/algorithms/concepts.hpp"

namespace tyr::planning::astar_eager
{

template<TaskKind Kind>
SearchResult<Kind> find_solution(Task<Kind>& task, SuccessorGenerator<Kind>& successor_generator, Heuristic<Kind>& heuristic, const Options<Kind>& options)
{
    using Search = detail::EagerAStarPolicy<Kind>;
    using Execution = detail::SequentialExecutionPolicy<Kind>;
    return detail::SearchEngine<Kind, Search, Execution, RandomDistHashTag>::find_solution(task, successor_generator, heuristic, options);
}

template SearchResult<LiftedTag> find_solution<LiftedTag>(Task<LiftedTag>& task,
                                                          SuccessorGenerator<LiftedTag>& successor_generator,
                                                          Heuristic<LiftedTag>& heuristic,
                                                          const Options<LiftedTag>& options);

template SearchResult<GroundTag> find_solution<GroundTag>(Task<GroundTag>& task,
                                                          SuccessorGenerator<GroundTag>& successor_generator,
                                                          Heuristic<GroundTag>& heuristic,
                                                          const Options<GroundTag>& options);

static_assert(SolverConcept<Solver<LiftedTag>, LiftedTag>);
static_assert(SolverConcept<Solver<GroundTag>, GroundTag>);

}

namespace tyr::planning::detail
{

template class SearchEngine<LiftedTag, EagerAStarPolicy<LiftedTag>, SequentialExecutionPolicy<LiftedTag>, RandomDistHashTag>;
template class SearchEngine<GroundTag, EagerAStarPolicy<GroundTag>, SequentialExecutionPolicy<GroundTag>, RandomDistHashTag>;

}
