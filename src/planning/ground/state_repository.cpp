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

#include "tyr/planning/ground/state_repository.hpp"

#include "tyr/formalism/planning/declarations.hpp"  // for ygg::Index
#include "tyr/formalism/planning/views.hpp"         // for ygg::View
#include "tyr/planning/ground/axiom_evaluator.hpp"  // for AxiomE...
#include "tyr/planning/ground/state_data.hpp"
#include "tyr/planning/ground/task.hpp"  // for Ground...
#include "tyr/planning/state_repository.hpp"
#include "tyr/planning/state_storage/config.hpp"
#include "tyr/planning/task_utils.hpp"  // for create...

#include <yggdrasil/containers/indexed_hash_set.hpp>
#include <yggdrasil/containers/raw_array_set.hpp>
#include <yggdrasil/core/config.hpp>

#if defined(TYR_STATE_STORAGE_HASHSET)
#include "tyr/planning/ground/state_storage/hash_set/atom.hpp"
#include "tyr/planning/ground/state_storage/hash_set/fact.hpp"
#include "tyr/planning/state_storage/hash_set/numeric.hpp"
#elif defined(TYR_STATE_STORAGE_TREE)
#include "tyr/planning/ground/state_storage/tree_compression/atom.hpp"
#include "tyr/planning/ground/state_storage/tree_compression/fact.hpp"
#include "tyr/planning/state_storage/tree_compression/numeric.hpp"
#endif

#include <algorithm>                 // for fill
#include <assert.h>                  // for assert
#include <boost/dynamic_bitset.hpp>  // for dynami...
#include <gtl/phmap.hpp>             // for operat...
#include <tuple>                     // for operat...
#include <utility>                   // for move
#include <valla/slot.hpp>            // for Slot
#include <valla/valla.hpp>
#include <yggdrasil/containers/vector.hpp>      // for ygg::View
#include <yggdrasil/semantics/comparators.hpp>  // for operat...

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

namespace tyr::planning
{

struct StateRepository<GroundTag>::Impl
{
    struct Definition
    {
        explicit Definition(TaskPtr<GroundTag> task_) : task(std::move(task_)) {}

        TaskPtr<GroundTag> task;
    };

    struct Evaluator
    {
        Evaluator(const Definition& definition, AxiomEvaluatorPtr<GroundTag> axiom_evaluator_) :
            context(*definition.task),
            fluent_backend(context),
            derived_backend(context),
            numeric_backend(context),
            packed_states(),
            state_builder_pool(),
            axiom_evaluator(std::move(axiom_evaluator_))
        {
        }

        StateStorageContext<GroundTag, StateStoragePolicyTag> context;
        FactStorageBackend<GroundTag, StateStoragePolicyTag> fluent_backend;
        AtomStorageBackend<GroundTag, StateStoragePolicyTag> derived_backend;
        NumericStorageBackend<GroundTag, StateStoragePolicyTag> numeric_backend;
        ygg::IndexedHashSet<State<GroundTag>> packed_states;
        ygg::SharedObjectPool<ygg::Builder<State<GroundTag>>> state_builder_pool;
        AxiomEvaluatorPtr<GroundTag> axiom_evaluator;
    };

    Impl(ygg::uint_t index_, TaskPtr<GroundTag> task, AxiomEvaluatorPtr<GroundTag> axiom_evaluator, std::shared_ptr<std::atomic<ygg::uint_t>> next_index_) :
        index(index_),
        next_index(std::move(next_index_)),
        definition(std::make_shared<Definition>(std::move(task))),
        evaluator(*definition, std::move(axiom_evaluator))
    {
    }

    Impl(ygg::uint_t index_,
         std::shared_ptr<const Definition> definition_,
         AxiomEvaluatorPtr<GroundTag> axiom_evaluator,
         std::shared_ptr<std::atomic<ygg::uint_t>> next_index_) :
        index(index_),
        next_index(std::move(next_index_)),
        definition(std::move(definition_)),
        evaluator(*definition, std::move(axiom_evaluator))
    {
    }

    ygg::uint_t index;
    std::shared_ptr<std::atomic<ygg::uint_t>> next_index;
    std::shared_ptr<const Definition> definition;
    Evaluator evaluator;
};

StateRepository<GroundTag>::StateRepository(ygg::uint_t index,
                                            TaskPtr<GroundTag> task,
                                            AxiomEvaluatorPtr<GroundTag> axiom_evaluator,
                                            std::shared_ptr<std::atomic<ygg::uint_t>> next_index) :
    m_impl(std::make_unique<Impl>(index, std::move(task), std::move(axiom_evaluator), std::move(next_index)))
{
}

StateRepository<GroundTag>::StateRepository(std::unique_ptr<Impl> impl) : m_impl(std::move(impl)) {}

StateRepository<GroundTag>::~StateRepository() = default;

StateRepositoryPtr<GroundTag> StateRepository<GroundTag>::make_worker(ygg::ExecutionContextPtr execution_context) const
{
    auto axiom_evaluator = m_impl->evaluator.axiom_evaluator ? m_impl->evaluator.axiom_evaluator->make_worker(std::move(execution_context)) : nullptr;
    return StateRepositoryPtr<GroundTag>(new StateRepository<GroundTag>(std::make_unique<Impl>(m_impl->next_index->fetch_add(1, std::memory_order_relaxed),
                                                                                               m_impl->definition,
                                                                                               std::move(axiom_evaluator),
                                                                                               m_impl->next_index)));
}

StateView<GroundTag> StateRepository<GroundTag>::get_initial_state()
{
    auto state_builder = get_state_builder();

    for (const auto fact : m_impl->definition->task->get_task().get_fluent_facts())
        state_builder->set(fact.get_data());

    for (const auto fterm_value : m_impl->definition->task->get_task().get_fterm_values<f::FluentTag>())
        state_builder->set(fterm_value.get_fterm().get_index(), fterm_value.get_value());

    return register_state(state_builder);
}

StateView<GroundTag> StateRepository<GroundTag>::get_registered_state(ygg::Index<State<GroundTag>> state_index)
{
    const auto& packed_state = m_impl->evaluator.packed_states[state_index];

    auto state_builder = get_state_builder();

    state_builder->set(state_index);
    m_impl->evaluator.fluent_backend.unpack(packed_state.template get_atoms<f::FluentTag>(), state_builder->template get_atoms<f::FluentTag>());
    m_impl->evaluator.derived_backend.unpack(packed_state.template get_atoms<f::DerivedTag>(), state_builder->template get_atoms<f::DerivedTag>());
    m_impl->evaluator.numeric_backend.unpack(packed_state.get_numeric_variables(), state_builder->get_numeric_variables());

    return StateView<GroundTag>(shared_from_this(), std::move(state_builder));
}

StateView<GroundTag>
StateRepository<GroundTag>::create_state(const std::vector<ygg::Data<fp::FDRFact<f::FluentTag>>>& fluent_facts,
                                         const std::vector<std::pair<ygg::Index<fp::GroundFunctionTerm<f::FluentTag>>, ygg::float_t>>& fterm_values)
{
    auto state_builder = get_state_builder();

    for (const auto& fact : fluent_facts)
        state_builder->set(fact);
    for (const auto& [fterm, value] : fterm_values)
        state_builder->set(fterm, value);

    return register_state(std::move(state_builder));
}

StateView<GroundTag> StateRepository<GroundTag>::create_state(const std::vector<fp::FDRFactView<f::FluentTag>>& fluent_facts,
                                                              const std::vector<fp::GroundFunctionTermViewValuePair<f::FluentTag>>& fterm_values)
{
    auto state_builder = get_state_builder();

    for (const auto& fact : fluent_facts)
        state_builder->set(fact.get_data());
    for (const auto& [fterm, value] : fterm_values)
        state_builder->set(fterm.get_index(), value);

    return register_state(std::move(state_builder));
}

ygg::SharedObjectPoolPtr<ygg::Builder<State<GroundTag>>> StateRepository<GroundTag>::get_state_builder()
{
    auto state_builder = m_impl->evaluator.state_builder_pool.get_or_allocate();
    state_builder->clear();

    state_builder->resize_fluent_facts(m_impl->definition->task->get_task().get_fluent_variables().size());
    state_builder->resize_derived_atoms(m_impl->definition->task->get_task().get_atoms<f::DerivedTag>().size());

    return state_builder;
}

StateView<GroundTag> StateRepository<GroundTag>::register_state(ygg::SharedObjectPoolPtr<ygg::Builder<State<GroundTag>>> state)
{
    if (m_impl->evaluator.axiom_evaluator)
        m_impl->evaluator.axiom_evaluator->compute_extended_state(*state);

    state->set(m_impl->evaluator.packed_states
                   .insert(ygg::Data<State<GroundTag>>(ygg::Index<State<GroundTag>>(m_impl->evaluator.packed_states.size()),
                                                       m_impl->evaluator.fluent_backend.insert(state->template get_atoms<f::FluentTag>()),
                                                       m_impl->evaluator.derived_backend.insert(state->template get_atoms<f::DerivedTag>()),
                                                       m_impl->evaluator.numeric_backend.insert(state->get_numeric_variables())))
                   .first);

    return StateView<GroundTag>(shared_from_this(), std::move(state));
}

size_t StateRepository<GroundTag>::memory_usage() const noexcept
{
    size_t bytes = 0;
    bytes += m_impl->evaluator.context.memory_usage();
    bytes += m_impl->evaluator.packed_states.memory_usage();
    return bytes;
}

const TaskPtr<GroundTag>& StateRepository<GroundTag>::get_task() const noexcept { return m_impl->definition->task; }

const AxiomEvaluatorPtr<GroundTag>& StateRepository<GroundTag>::get_axiom_evaluator() const noexcept { return m_impl->evaluator.axiom_evaluator; }

ygg::uint_t StateRepository<GroundTag>::get_index() const noexcept { return m_impl->index; }

size_t StateRepository<GroundTag>::num_states() const noexcept { return m_impl->evaluator.packed_states.size(); }

static_assert(StateRepositoryConcept<StateRepository<GroundTag>, GroundTag>);

}
