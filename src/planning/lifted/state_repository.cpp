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
#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/lifted/axiom_evaluator.hpp"
#include "tyr/planning/lifted/state_data.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/state_repository.hpp"
#include "tyr/planning/state_storage/config.hpp"
#include "tyr/planning/task_utils.hpp"

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

#include <tuple>
#include <type_traits>
#include <utility>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/semantics/comparators.hpp>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

namespace
{
std::atomic<ygg::uint_t> next_storage_identity { 0 };
}

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

    template<bool ThreadSafe>
    struct Storage
    {
        using PackedStates =
            ygg::IndexedHashSet<State<LiftedTag>, ygg::Hash<ygg::Data<State<LiftedTag>>>, ygg::EqualTo<ygg::Data<State<LiftedTag>>>, 32, ThreadSafe>;

        Storage() : identity(next_storage_identity.fetch_add(1, std::memory_order_relaxed)) {}

        const ygg::uint_t identity;
        StateStorageContext<LiftedTag, StateStoragePolicyTag, ThreadSafe> context;
        PackedStates packed_states;
    };

    template<bool ThreadSafe>
    using StoragePtr = std::conditional_t<ThreadSafe, std::shared_ptr<Storage<ThreadSafe>>, std::unique_ptr<Storage<ThreadSafe>>>;

    template<bool ThreadSafe>
    struct Evaluator
    {
        Evaluator(StoragePtr<ThreadSafe> storage_, AxiomEvaluatorPtr<LiftedTag> axiom_evaluator_) :
            execution_context(axiom_evaluator_ ? axiom_evaluator_->get_execution_context() : nullptr),
            storage(std::move(storage_)),
            fluent_backend(storage->context),
            derived_backend(storage->context),
            numeric_backend(storage->context),
            state_builder_pool(),
            axiom_evaluator(std::move(axiom_evaluator_))
        {
        }

        ygg::ExecutionContextPtr execution_context;
        StoragePtr<ThreadSafe> storage;
        FactStorageBackend<LiftedTag, StateStoragePolicyTag, ThreadSafe> fluent_backend;
        AtomStorageBackend<LiftedTag, StateStoragePolicyTag, ThreadSafe> derived_backend;
        NumericStorageBackend<LiftedTag, StateStoragePolicyTag, ThreadSafe> numeric_backend;
        ygg::SharedObjectPool<ygg::Builder<State<LiftedTag>>, true> state_builder_pool;
        AxiomEvaluatorPtr<LiftedTag> axiom_evaluator;
    };

    Impl(ygg::uint_t index_, TaskPtr<LiftedTag> task, AxiomEvaluatorPtr<LiftedTag> axiom_evaluator, std::shared_ptr<std::atomic<ygg::uint_t>> next_index_) :
        index(index_),
        next_index(std::move(next_index_)),
        definition(std::make_shared<Definition>(std::move(task))),
        evaluator(std::make_unique<Evaluator<false>>(std::make_unique<Storage<false>>(), std::move(axiom_evaluator))),
        shared_evaluator(),
        storage_identity(evaluator->storage->identity)
    {
    }

    Impl(ygg::uint_t index_,
         std::shared_ptr<const Definition> definition_,
         AxiomEvaluatorPtr<LiftedTag> axiom_evaluator,
         std::shared_ptr<std::atomic<ygg::uint_t>> next_index_) :
        index(index_),
        next_index(std::move(next_index_)),
        definition(std::move(definition_)),
        evaluator(std::make_unique<Evaluator<false>>(std::make_unique<Storage<false>>(), std::move(axiom_evaluator))),
        shared_evaluator(),
        storage_identity(evaluator->storage->identity)
    {
    }

    Impl(ygg::uint_t index_,
         std::shared_ptr<const Definition> definition_,
         std::shared_ptr<Storage<true>> storage,
         AxiomEvaluatorPtr<LiftedTag> axiom_evaluator,
         std::shared_ptr<std::atomic<ygg::uint_t>> next_index_) :
        index(index_),
        next_index(std::move(next_index_)),
        definition(std::move(definition_)),
        evaluator(),
        shared_evaluator(std::make_unique<Evaluator<true>>(std::move(storage), std::move(axiom_evaluator))),
        storage_identity(shared_evaluator->storage->identity)
    {
    }

    template<typename Callback>
    decltype(auto) visit_evaluator(Callback&& callback)
    {
        if (evaluator)
            return std::forward<Callback>(callback)(*evaluator);
        return std::forward<Callback>(callback)(*shared_evaluator);
    }

    template<typename Callback>
    decltype(auto) visit_evaluator(Callback&& callback) const
    {
        if (evaluator)
            return std::forward<Callback>(callback)(static_cast<const Evaluator<false>&>(*evaluator));
        return std::forward<Callback>(callback)(static_cast<const Evaluator<true>&>(*shared_evaluator));
    }

    ygg::uint_t index;
    std::shared_ptr<std::atomic<ygg::uint_t>> next_index;
    std::shared_ptr<const Definition> definition;
    std::unique_ptr<Evaluator<false>> evaluator;
    std::unique_ptr<Evaluator<true>> shared_evaluator;
    const ygg::uint_t storage_identity;
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
    const auto& source_axiom_evaluator =
        m_impl->visit_evaluator([](const auto& evaluator) -> const AxiomEvaluatorPtr<LiftedTag>& { return evaluator.axiom_evaluator; });
    auto axiom_evaluator = source_axiom_evaluator ? source_axiom_evaluator->make_worker(std::move(execution_context)) : nullptr;
    return StateRepositoryPtr<LiftedTag>(new StateRepository<LiftedTag>(std::make_unique<Impl>(m_impl->next_index->fetch_add(1, std::memory_order_relaxed),
                                                                                               m_impl->definition,
                                                                                               std::move(axiom_evaluator),
                                                                                               m_impl->next_index)));
}

std::vector<StateRepositoryPtr<LiftedTag>> StateRepository<LiftedTag>::make_shared_workers(std::span<const ygg::ExecutionContextPtr> execution_contexts) const
{
    auto workers = std::vector<StateRepositoryPtr<LiftedTag>> {};
    if (execution_contexts.empty())
        return workers;

    const auto storage = std::make_shared<Impl::Storage<true>>();
    const auto repository_index = m_impl->next_index->fetch_add(1, std::memory_order_relaxed);
    const auto& source_axiom_evaluator =
        m_impl->visit_evaluator([](const auto& evaluator) -> const AxiomEvaluatorPtr<LiftedTag>& { return evaluator.axiom_evaluator; });

    workers.reserve(execution_contexts.size());
    for (const auto& execution_context : execution_contexts)
    {
        auto axiom_evaluator = source_axiom_evaluator ? source_axiom_evaluator->make_worker(execution_context) : nullptr;
        workers.emplace_back(new StateRepository<LiftedTag>(
            std::make_unique<Impl>(repository_index, m_impl->definition, storage, std::move(axiom_evaluator), m_impl->next_index)));
    }
    return workers;
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
    auto state_builder = get_state_builder();

    state_builder->set(state_index);
    m_impl->visit_evaluator(
        [&](auto& evaluator)
        {
            const auto& packed_state = evaluator.storage->packed_states[state_index];
            evaluator.fluent_backend.unpack(packed_state.template get_atoms<f::FluentTag>(), state_builder->template get_atoms<f::FluentTag>());
            evaluator.derived_backend.unpack(packed_state.template get_atoms<f::DerivedTag>(), state_builder->template get_atoms<f::DerivedTag>());
            evaluator.numeric_backend.unpack(packed_state.get_numeric_variables(), state_builder->get_numeric_variables());
        });

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
    auto state_builder = m_impl->visit_evaluator([](auto& evaluator) { return evaluator.state_builder_pool.get_or_allocate(); });
    state_builder->clear();

    return state_builder;
}

StateView<LiftedTag> StateRepository<LiftedTag>::register_state(ygg::SharedObjectPoolPtr<ygg::Builder<State<LiftedTag>>, true> state)
{
    m_impl->visit_evaluator(
        [&](auto& evaluator)
        {
            if (evaluator.axiom_evaluator)
                evaluator.axiom_evaluator->compute_extended_state(*state);

            auto& packed_states = evaluator.storage->packed_states;
            using PackedStates = std::remove_cvref_t<decltype(packed_states)>;
            if constexpr (!PackedStates::thread_safe)
            {
                state->set(packed_states
                               .insert(ygg::Data<State<LiftedTag>>(ygg::Index<State<LiftedTag>>(packed_states.size()),
                                                                   evaluator.fluent_backend.insert(state->template get_atoms<f::FluentTag>()),
                                                                   evaluator.derived_backend.insert(state->template get_atoms<f::DerivedTag>()),
                                                                   evaluator.numeric_backend.insert(state->get_numeric_variables())))
                               .first);
            }
            else
            {
                const auto candidate = ygg::Data<State<LiftedTag>>(ygg::Index<State<LiftedTag>>::max(),
                                                                   evaluator.fluent_backend.insert(state->template get_atoms<f::FluentTag>()),
                                                                   evaluator.derived_backend.insert(state->template get_atoms<f::DerivedTag>()),
                                                                   evaluator.numeric_backend.insert(state->get_numeric_variables()));
                const auto hash = PackedStates::hash(candidate);
                auto state_index = packed_states.find_with_hash(candidate, hash);
                if (!state_index)
                {
                    state_index.emplace(packed_states
                                            .complete_miss_with_hash(hash,
                                                                     candidate,
                                                                     [&](ygg::Index<State<LiftedTag>> index)
                                                                     {
                                                                         return ygg::Data<State<LiftedTag>>(index,
                                                                                                            candidate.template get_atoms<f::FluentTag>(),
                                                                                                            candidate.template get_atoms<f::DerivedTag>(),
                                                                                                            candidate.get_numeric_variables());
                                                                     })
                                            .first);
                }
                state->set(*state_index);
            }
        });

    return StateView<LiftedTag>(shared_from_this(), std::move(state));
}

size_t StateRepository<LiftedTag>::memory_usage() const noexcept
{
    return m_impl->visit_evaluator([](const auto& evaluator)
                                   { return evaluator.storage->context.memory_usage() + evaluator.storage->packed_states.memory_usage(); });
}

const TaskPtr<LiftedTag>& StateRepository<LiftedTag>::get_task() const noexcept { return m_impl->definition->task; }

const AxiomEvaluatorPtr<LiftedTag>& StateRepository<LiftedTag>::get_axiom_evaluator() const noexcept
{
    return m_impl->visit_evaluator([](const auto& evaluator) -> const AxiomEvaluatorPtr<LiftedTag>& { return evaluator.axiom_evaluator; });
}

const ygg::ExecutionContextPtr& StateRepository<LiftedTag>::get_execution_context() const noexcept
{
    return m_impl->visit_evaluator([](const auto& evaluator) -> const ygg::ExecutionContextPtr& { return evaluator.execution_context; });
}

ygg::uint_t StateRepository<LiftedTag>::get_index() const noexcept { return m_impl->index; }

ygg::uint_t StateRepository<LiftedTag>::get_storage_identity() const noexcept { return m_impl->storage_identity; }

size_t StateRepository<LiftedTag>::num_states() const noexcept
{
    return m_impl->visit_evaluator([](const auto& evaluator) { return evaluator.storage->packed_states.size(); });
}

static_assert(StateRepositoryConcept<StateRepository<LiftedTag>, LiftedTag>);

}
