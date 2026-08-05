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

#include "tyr/planning/lifted/state_repository.hpp"

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_context.hpp"  // for Binary...
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/lifted/axiom_evaluator.hpp"  // for AxiomE...
#include "tyr/planning/lifted/state_data.hpp"
#include "tyr/planning/lifted/task.hpp"       // for Lifted...
#include "tyr/planning/state_repository.hpp"  // for StateR...
#include "tyr/planning/state_storage/config.hpp"
#include "tyr/planning/task_utils.hpp"  // for create...

#include <yggdrasil/containers/indexed_hash_set.hpp>
#include <yggdrasil/core/config.hpp>

#if defined(TYR_STATE_STORAGE_HASHSET)
#include "tyr/planning/lifted/state_storage/hash_set/atom.hpp"
#include "tyr/planning/lifted/state_storage/hash_set/fact.hpp"
#include "tyr/planning/state_storage/hash_set/numeric.hpp"
#elif defined(TYR_STATE_STORAGE_TREE)
#include "tyr/planning/lifted/state_storage/tree_compression/atom.hpp"
#include "tyr/planning/lifted/state_storage/tree_compression/fact.hpp"
#include "tyr/planning/state_storage/tree_compression/numeric.hpp"
#endif

#include <tuple>           // for operat...
#include <utility>         // for move
#include <yggdrasil/containers/vector.hpp>      // for ygg::View
#include <yggdrasil/semantics/comparators.hpp>  // for operat...

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

namespace tyr::planning
{

template StateView<LiftedTag> materialize_state(const StateView<LiftedTag>& source, StateRepository<LiftedTag>& target);

struct StateRepository<LiftedTag>::Impl
{
    struct Definition
    {
        explicit Definition(TaskPtr<LiftedTag> task_) : task(std::move(task_)) {}

        TaskPtr<LiftedTag> task;
    };

    struct Evaluator
    {
        explicit Evaluator(AxiomEvaluatorPtr<LiftedTag> axiom_evaluator_) :
            execution_context(axiom_evaluator_ ? axiom_evaluator_->get_execution_context() : nullptr),
            context(),
            fluent_backend(context),
            derived_backend(context),
            numeric_backend(context),
            packed_states(),
            state_builder_pool(),
            axiom_evaluator(std::move(axiom_evaluator_))
        {
        }

        ygg::ExecutionContextPtr execution_context;
        StateStorageContext<LiftedTag, StateStoragePolicyTag> context;
        FactStorageBackend<LiftedTag, StateStoragePolicyTag> fluent_backend;
        AtomStorageBackend<LiftedTag, StateStoragePolicyTag> derived_backend;
        NumericStorageBackend<LiftedTag, StateStoragePolicyTag> numeric_backend;
        ygg::IndexedHashSet<State<LiftedTag>> packed_states;
        ygg::SharedObjectPool<ygg::Builder<State<LiftedTag>>, true> state_builder_pool;
        AxiomEvaluatorPtr<LiftedTag> axiom_evaluator;
    };

    Impl(ygg::uint_t index_, TaskPtr<LiftedTag> task, AxiomEvaluatorPtr<LiftedTag> axiom_evaluator, std::shared_ptr<std::atomic<ygg::uint_t>> next_index_) :
        index(index_),
        next_index(std::move(next_index_)),
        definition(std::make_shared<Definition>(std::move(task))),
        evaluator(std::move(axiom_evaluator))
    {
    }

    Impl(ygg::uint_t index_,
         std::shared_ptr<const Definition> definition_,
         AxiomEvaluatorPtr<LiftedTag> axiom_evaluator,
         std::shared_ptr<std::atomic<ygg::uint_t>> next_index_) :
        index(index_),
        next_index(std::move(next_index_)),
        definition(std::move(definition_)),
        evaluator(std::move(axiom_evaluator))
    {
    }

    ygg::uint_t index;
    std::shared_ptr<std::atomic<ygg::uint_t>> next_index;
    std::shared_ptr<const Definition> definition;
    Evaluator evaluator;
};

StateRepository<LiftedTag>::StateRepository(ygg::uint_t index,
                                            TaskPtr<LiftedTag> task,
                                            AxiomEvaluatorPtr<LiftedTag> axiom_evaluator,
                                            std::shared_ptr<std::atomic<ygg::uint_t>> next_index) :
    m_impl(std::make_unique<Impl>(index, std::move(task), std::move(axiom_evaluator), std::move(next_index)))
{
}

StateRepository<LiftedTag>::StateRepository(std::unique_ptr<Impl> impl) : m_impl(std::move(impl)) {}

StateRepository<LiftedTag>::~StateRepository() = default;

StateRepositoryPtr<LiftedTag> StateRepository<LiftedTag>::make_worker(ygg::ExecutionContextPtr execution_context) const
{
    auto axiom_evaluator = m_impl->evaluator.axiom_evaluator ? m_impl->evaluator.axiom_evaluator->make_worker(std::move(execution_context)) : nullptr;
    return StateRepositoryPtr<LiftedTag>(new StateRepository<LiftedTag>(std::make_unique<Impl>(m_impl->next_index->fetch_add(1, std::memory_order_relaxed),
                                                                                               m_impl->definition,
                                                                                               std::move(axiom_evaluator),
                                                                                               m_impl->next_index)));
}

StateView<LiftedTag> StateRepository<LiftedTag>::get_initial_state()
{
    auto state_builder = get_state_builder();

    for (const auto atom : m_impl->definition->task->get_task().get_atoms<f::FluentTag>())
        state_builder->set(m_impl->definition->task->get_fdr_context()->get_fact(atom).get_data());

    for (const auto fterm_value : m_impl->definition->task->get_task().get_fterm_values<f::FluentTag>())
        state_builder->set(fterm_value.get_fterm().get_index(), fterm_value.get_value());

    return register_state(std::move(state_builder));
}

StateView<LiftedTag> StateRepository<LiftedTag>::get_registered_state(ygg::Index<State<LiftedTag>> state_index)
{
    const auto& packed_state = m_impl->evaluator.packed_states[state_index];

    auto state_builder = get_state_builder();

    state_builder->set(state_index);
    m_impl->evaluator.fluent_backend.unpack(packed_state.template get_atoms<f::FluentTag>(), state_builder->template get_atoms<f::FluentTag>());
    m_impl->evaluator.derived_backend.unpack(packed_state.template get_atoms<f::DerivedTag>(), state_builder->template get_atoms<f::DerivedTag>());
    m_impl->evaluator.numeric_backend.unpack(packed_state.get_numeric_variables(), state_builder->get_numeric_variables());

    return StateView<LiftedTag>(shared_from_this(), std::move(state_builder));
}

StateView<LiftedTag>
StateRepository<LiftedTag>::create_state(const std::vector<ygg::Data<fp::FDRFact<f::FluentTag>>>& fluent_facts,
                                         const std::vector<std::pair<ygg::Index<fp::GroundFunctionTerm<f::FluentTag>>, ygg::float_t>>& fterm_values)
{
    auto state_builder = get_state_builder();

    for (const auto& fact : fluent_facts)
        state_builder->set(fact);
    for (const auto& [fterm, value] : fterm_values)
        state_builder->set(fterm, value);

    return register_state(std::move(state_builder));
}

StateView<LiftedTag>
StateRepository<LiftedTag>::create_state(const std::vector<fp::FDRFactView<f::FluentTag>>& fluent_facts,
                                         const std::vector<std::pair<fp::GroundFunctionTermView<f::FluentTag>, ygg::float_t>>& fterm_values)
{
    auto state_builder = get_state_builder();

    for (const auto& fact : fluent_facts)
        state_builder->set(fact.get_data());
    for (const auto& [fterm, value] : fterm_values)
        state_builder->set(fterm.get_index(), value);

    return register_state(std::move(state_builder));
}

ygg::SharedObjectPoolPtr<ygg::Builder<State<LiftedTag>>, true> StateRepository<LiftedTag>::get_state_builder()
{
    auto state_builder = m_impl->evaluator.state_builder_pool.get_or_allocate();
    state_builder->clear();

    return state_builder;
}

StateView<LiftedTag> StateRepository<LiftedTag>::register_state(ygg::SharedObjectPoolPtr<ygg::Builder<State<LiftedTag>>, true> state)
{
    if (m_impl->evaluator.axiom_evaluator)
        m_impl->evaluator.axiom_evaluator->compute_extended_state(*state);

    state->set(m_impl->evaluator.packed_states
                   .insert(ygg::Data<State<LiftedTag>>(ygg::Index<State<LiftedTag>>(m_impl->evaluator.packed_states.size()),
                                                       m_impl->evaluator.fluent_backend.insert(state->template get_atoms<f::FluentTag>()),
                                                       m_impl->evaluator.derived_backend.insert(state->template get_atoms<f::DerivedTag>()),
                                                       m_impl->evaluator.numeric_backend.insert(state->get_numeric_variables())))
                   .first);

    return StateView<LiftedTag>(shared_from_this(), std::move(state));
}

size_t StateRepository<LiftedTag>::memory_usage() const noexcept
{
    size_t bytes = 0;
    bytes += m_impl->evaluator.context.memory_usage();
    bytes += m_impl->evaluator.packed_states.memory_usage();
    return bytes;
}

const TaskPtr<LiftedTag>& StateRepository<LiftedTag>::get_task() const noexcept { return m_impl->definition->task; }

const AxiomEvaluatorPtr<LiftedTag>& StateRepository<LiftedTag>::get_axiom_evaluator() const noexcept { return m_impl->evaluator.axiom_evaluator; }

const ygg::ExecutionContextPtr& StateRepository<LiftedTag>::get_execution_context() const noexcept { return m_impl->evaluator.execution_context; }

ygg::uint_t StateRepository<LiftedTag>::get_index() const noexcept { return m_impl->index; }

size_t StateRepository<LiftedTag>::num_states() const noexcept { return m_impl->evaluator.packed_states.size(); }

static_assert(StateRepositoryConcept<StateRepository<LiftedTag>, LiftedTag>);

}
