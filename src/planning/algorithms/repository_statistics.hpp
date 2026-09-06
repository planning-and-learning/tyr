/*
 * Copyright (C) 2026 Dominik Drexler
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

#ifndef TYR_SRC_PLANNING_ALGORITHMS_REPOSITORY_STATISTICS_HPP_
#define TYR_SRC_PLANNING_ALGORITHMS_REPOSITORY_STATISTICS_HPP_

#include "tyr/formalism/planning/repository.hpp"
#include "tyr/planning/algorithms/statistics.hpp"
#include "tyr/planning/declarations.hpp"

namespace tyr::planning::detail
{

template<TaskKind Kind>
void snapshot_state_repository_statistics(const StateRepository<Kind>& repository, Statistics& statistics)
{
    statistics.set_num_registered_states(repository.num_states());
    statistics.set_state_storage_memory_usage(repository.memory_usage());
}

inline void snapshot_task_repository_statistics(const formalism::planning::Repository& repository, Statistics& statistics)
{
    statistics.set_action_bindings_memory_usage(repository.memory_usage<formalism::RelationBinding<formalism::planning::Action<LiftedTag>>>());
    statistics.set_predicate_bindings_memory_usage(
        repository.memory_usage<formalism::RelationBinding<formalism::Predicate<formalism::StaticTag>>>()
        + repository.memory_usage<formalism::RelationBinding<formalism::Predicate<formalism::FluentTag>>>()
        + repository.memory_usage<formalism::RelationBinding<formalism::Predicate<formalism::DerivedTag>>>());
    statistics.set_axiom_bindings_memory_usage(repository.memory_usage<formalism::RelationBinding<formalism::planning::Axiom<LiftedTag>>>());
    statistics.set_function_bindings_memory_usage(
        repository.memory_usage<formalism::RelationBinding<formalism::Function<formalism::StaticTag>>>()
        + repository.memory_usage<formalism::RelationBinding<formalism::Function<formalism::FluentTag>>>()
        + repository.memory_usage<formalism::RelationBinding<formalism::Function<formalism::AuxiliaryTag>>>());
}

}

#endif
