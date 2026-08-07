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

#include "tyr/planning/state_repository.hpp"

#include "state_repository.hpp"
#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/planning/ground/axiom_evaluator.hpp"
#include "tyr/planning/ground/state_data.hpp"
#include "tyr/planning/ground/state_repository.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/axiom_evaluator.hpp"
#include "tyr/planning/lifted/state_data.hpp"
#include "tyr/planning/lifted/state_repository.hpp"
#include "tyr/planning/lifted/task.hpp"

namespace tyr::planning::detail
{

template<>
struct StateRepositoryPolicy<GroundTag>
{
    template<bool ThreadSafe>
    static StateStorageContext<GroundTag, StateStoragePolicyTag, ThreadSafe> make_storage_context(const Task<GroundTag>& task)
    {
        return StateStorageContext<GroundTag, StateStoragePolicyTag, ThreadSafe>(task);
    }

    static void insert_initial_fluent_facts(const Task<GroundTag>& task, ygg::Builder<State<GroundTag>>& state)
    {
        for (const auto fact : task.get_task().get_fluent_facts())
            state.set(fact.get_data());
    }

    static void prepare_builder(const Task<GroundTag>& task, ygg::Builder<State<GroundTag>>& state)
    {
        state.clear();
        state.resize_fluent_facts(task.get_task().get_fluent_variables().size());
        state.resize_derived_atoms(task.get_task().template get_atoms<::tyr::formalism::DerivedTag>().size());
    }
};

template<>
struct StateRepositoryPolicy<LiftedTag>
{
    template<bool ThreadSafe>
    static StateStorageContext<LiftedTag, StateStoragePolicyTag, ThreadSafe> make_storage_context(const Task<LiftedTag>&)
    {
        return {};
    }

    static void insert_initial_fluent_facts(const Task<LiftedTag>& task, ygg::Builder<State<LiftedTag>>& state)
    {
        for (const auto atom : task.get_task().template get_atoms<::tyr::formalism::FluentTag>())
            state.set(task.get_fdr_context()->get_fact(atom).get_data());
    }

    static void prepare_builder(const Task<LiftedTag>&, ygg::Builder<State<LiftedTag>>& state) { state.clear(); }
};

}

namespace tyr::planning
{

template class StateRepository<GroundTag>;
template class StateRepository<LiftedTag>;

template StateView<GroundTag> materialize_state(const StateView<GroundTag>& source, StateRepository<GroundTag>& target);
template StateView<LiftedTag> materialize_state(const StateView<LiftedTag>& source, StateRepository<LiftedTag>& target);

static_assert(StateRepositoryConcept<StateRepository<GroundTag>, GroundTag>);
static_assert(StateRepositoryConcept<StateRepository<LiftedTag>, LiftedTag>);

}
