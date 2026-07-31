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

#ifndef TYR_PLANNING_LIFTED_STATE_REPOSITORY_HPP_
#define TYR_PLANNING_LIFTED_STATE_REPOSITORY_HPP_

#include "tyr/planning/lifted/state_builder.hpp"
#include "tyr/planning/lifted/state_data.hpp"
#include "tyr/planning/lifted/state_view.hpp"
#include "tyr/planning/state_index.hpp"
#include "tyr/planning/state_storage/config.hpp"

#include <yggdrasil/containers/indexed_hash_set.hpp>
#include <yggdrasil/containers/shared_object_pool.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/execution/onetbb.hpp>

#if defined(TYR_STATE_STORAGE_HASHSET)
#include "tyr/planning/lifted/state_storage/hash_set/atom.hpp"
#include "tyr/planning/lifted/state_storage/hash_set/fact.hpp"
#include "tyr/planning/state_storage/hash_set/numeric.hpp"
#elif defined(TYR_STATE_STORAGE_TREE)
#include "tyr/planning/lifted/state_storage/tree_compression/atom.hpp"
#include "tyr/planning/lifted/state_storage/tree_compression/fact.hpp"
#include "tyr/planning/state_storage/tree_compression/numeric.hpp"
#endif

#include <memory>
#include <valla/valla.hpp>
#include <vector>

namespace tyr::planning
{

template<>
class StateRepository<LiftedTag> : public std::enable_shared_from_this<StateRepository<LiftedTag>>
{
    friend class StateRepositoryFactory<LiftedTag>;

private:
    StateRepository(ygg::uint_t index, TaskPtr<LiftedTag> task, AxiomEvaluatorPtr<LiftedTag> axiom_evaluator);

public:
    StateView<LiftedTag> get_initial_state();

    StateView<LiftedTag> get_registered_state(ygg::Index<State<LiftedTag>> state_index);

    StateView<LiftedTag> create_state(
        const std::vector<ygg::Data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>>& fluent_facts,
        const std::vector<std::pair<ygg::Index<::tyr::formalism::planning::GroundFunctionTerm<::tyr::formalism::FluentTag>>, ygg::float_t>>& fterm_values);

    StateView<LiftedTag>
    create_state(const std::vector<::tyr::formalism::planning::FDRFactView<::tyr::formalism::FluentTag>>& fluent_facts,
                 const std::vector<::tyr::formalism::planning::GroundFunctionTermViewValuePair<::tyr::formalism::FluentTag>>& fterm_values);

    ygg::SharedObjectPoolPtr<ygg::Builder<State<LiftedTag>>> get_state_builder();

    StateView<LiftedTag> register_state(ygg::SharedObjectPoolPtr<ygg::Builder<State<LiftedTag>>> state);

    size_t memory_usage() const noexcept;

    const auto& get_task() const noexcept { return m_task; }
    const auto& get_axiom_evaluator() const noexcept { return m_axiom_evaluator; }
    const auto& get_execution_context() const noexcept { return m_execution_context; }
    auto get_index() const noexcept { return m_index; }

    size_t num_states() const noexcept { return m_packed_states.size(); }

private:
    ygg::uint_t m_index;
    TaskPtr<LiftedTag> m_task;
    ygg::ExecutionContextPtr m_execution_context;

    StateStorageContext<LiftedTag, StateStoragePolicyTag> m_context;
    FactStorageBackend<LiftedTag, StateStoragePolicyTag> m_fluent_backend;
    AtomStorageBackend<LiftedTag, StateStoragePolicyTag> m_derived_backend;
    NumericStorageBackend<LiftedTag, StateStoragePolicyTag> m_numeric_backend;

    ygg::IndexedHashSet<State<LiftedTag>> m_packed_states;
    ygg::SharedObjectPool<ygg::Builder<State<LiftedTag>>> m_state_builder_pool;

    AxiomEvaluatorPtr<LiftedTag> m_axiom_evaluator;
};

}

#endif
