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
#include "search_engine/sequential.hpp"
#include "tyr/planning/algorithms/concepts.hpp"

namespace tyr::planning::gbfs_lazy
{

template<TaskKind Kind>
SearchResult<Kind> find_solution(Task<Kind>& task, SuccessorGenerator<Kind>& successor_generator, Heuristic<Kind>& heuristic, const Options<Kind>& options)
{
    if (options.num_search_workers == 0)
        throw std::invalid_argument("gbfs_lazy::find_solution(...): num_search_workers must be greater than zero.");
    if (options.num_search_workers > 1)
    {
        using Search = ::tyr::planning::detail::LazyGBFSPolicy<Kind, ParallelSearch>;
        using Execution = ::tyr::planning::detail::ParallelExecutionPolicy<Kind, typename Search::SuccessorMetadata, RandomDistHashTag>;
        return ::tyr::planning::detail::SearchEngine<Kind, Search, Execution>::find_solution(task, successor_generator, heuristic, options);
    }

    using Search = ::tyr::planning::detail::LazyGBFSPolicy<Kind, SequentialSearch>;
    using Execution = ::tyr::planning::detail::SequentialExecutionPolicy<Kind>;
    return ::tyr::planning::detail::SearchEngine<Kind, Search, Execution>::find_solution(task, successor_generator, heuristic, options);
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

template class SearchEngine<LiftedTag, LazyGBFSPolicy<LiftedTag, SequentialSearch>, SequentialExecutionPolicy<LiftedTag>>;
template class SearchEngine<GroundTag, LazyGBFSPolicy<GroundTag, SequentialSearch>, SequentialExecutionPolicy<GroundTag>>;

using LiftedParallelGBFS = LazyGBFSPolicy<LiftedTag, ParallelSearch>;
using GroundParallelGBFS = LazyGBFSPolicy<GroundTag, ParallelSearch>;
template class SearchEngine<LiftedTag,
                            LiftedParallelGBFS,
                            ParallelExecutionPolicy<LiftedTag, typename LiftedParallelGBFS::SuccessorMetadata, RandomDistHashTag>>;
template class SearchEngine<GroundTag,
                            GroundParallelGBFS,
                            ParallelExecutionPolicy<GroundTag, typename GroundParallelGBFS::SuccessorMetadata, RandomDistHashTag>>;

}
