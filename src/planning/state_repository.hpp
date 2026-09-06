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

#ifndef TYR_SRC_PLANNING_STATE_REPOSITORY_HPP_
#define TYR_SRC_PLANNING_STATE_REPOSITORY_HPP_

#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/state_repository.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/config.hpp"

#include <atomic>
#include <memory>
#include <type_traits>
#include <utility>
#include <yggdrasil/containers/indexed_hash_set.hpp>
#include <yggdrasil/semantics/comparators.hpp>

namespace tyr::planning::detail
{

template<TaskKind Kind>
struct StateRepositoryPolicy;

template<TaskKind Kind>
inline std::atomic<ygg::uint_t> next_state_storage_identity { 0 };

}

namespace tyr::planning
{

template<TaskKind Kind>
struct StateRepository<Kind>::Impl
{
    using Policy = detail::StateRepositoryPolicy<Kind>;

    struct Definition
    {
        explicit Definition(TaskPtr<Kind> task_) : task(std::move(task_)) {}

        TaskPtr<Kind> task;
    };

    template<bool ThreadSafe>
    struct Storage
    {
        using PackedStates = ygg::IndexedHashSet<State<Kind>, ygg::Hash<ygg::Data<State<Kind>>>, ygg::EqualTo<ygg::Data<State<Kind>>>, 32, ThreadSafe>;

        explicit Storage(const Definition& definition) :
            identity(detail::next_state_storage_identity<Kind>.fetch_add(1, std::memory_order_relaxed)),
            context(Policy::template make_storage_context<ThreadSafe>(*definition.task)),
            packed_states()
        {
        }

        const ygg::uint_t identity;
        StateStorageContext<Kind, StateStoragePolicyTag, ThreadSafe> context;
        PackedStates packed_states;
    };

    template<bool ThreadSafe>
    using StoragePtr = std::conditional_t<ThreadSafe, std::shared_ptr<Storage<ThreadSafe>>, std::unique_ptr<Storage<ThreadSafe>>>;

    template<bool ThreadSafe>
    struct Evaluator
    {
        explicit Evaluator(StoragePtr<ThreadSafe> storage_) :
            storage(std::move(storage_)),
            fluent_backend(storage->context),
            derived_backend(storage->context),
            numeric_backend(storage->context),
            state_builder_pool()
        {
        }

        StoragePtr<ThreadSafe> storage;
        FactStorageBackend<Kind, StateStoragePolicyTag, ThreadSafe> fluent_backend;
        AtomStorageBackend<Kind, StateStoragePolicyTag, ThreadSafe> derived_backend;
        NumericStorageBackend<Kind, StateStoragePolicyTag, ThreadSafe> numeric_backend;
        ygg::SharedObjectPool<ygg::Builder<State<Kind>>, true> state_builder_pool;
    };

    Impl(ygg::uint_t index_, TaskPtr<Kind> task, bool concurrent, std::shared_ptr<std::atomic<ygg::uint_t>> next_index_) :
        index(index_),
        next_index(std::move(next_index_)),
        definition(std::make_shared<Definition>(std::move(task))),
        evaluator(concurrent ? nullptr : std::make_unique<Evaluator<false>>(std::make_unique<Storage<false>>(*definition))),
        shared_evaluator(concurrent ? std::make_unique<Evaluator<true>>(std::make_shared<Storage<true>>(*definition)) : nullptr),
        storage_identity(concurrent ? shared_evaluator->storage->identity : evaluator->storage->identity)
    {
    }

    Impl(ygg::uint_t index_, std::shared_ptr<const Definition> definition_, std::shared_ptr<std::atomic<ygg::uint_t>> next_index_) :
        index(index_),
        next_index(std::move(next_index_)),
        definition(std::move(definition_)),
        evaluator(std::make_unique<Evaluator<false>>(std::make_unique<Storage<false>>(*definition))),
        shared_evaluator(),
        storage_identity(evaluator->storage->identity)
    {
    }

    Impl(ygg::uint_t index_,
         std::shared_ptr<const Definition> definition_,
         std::shared_ptr<Storage<true>> storage,
         std::shared_ptr<std::atomic<ygg::uint_t>> next_index_) :
        index(index_),
        next_index(std::move(next_index_)),
        definition(std::move(definition_)),
        evaluator(),
        shared_evaluator(std::make_unique<Evaluator<true>>(std::move(storage))),
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

template<TaskKind Kind>
StateRepository<Kind>::StateRepository(ygg::uint_t index, TaskPtr<Kind> task, bool concurrent, std::shared_ptr<std::atomic<ygg::uint_t>> next_index) :
    m_impl(std::make_unique<Impl>(index, std::move(task), concurrent, std::move(next_index)))
{
}

template<TaskKind Kind>
StateRepository<Kind>::StateRepository(std::unique_ptr<Impl> impl) : m_impl(std::move(impl))
{
}

template<TaskKind Kind>
StateRepository<Kind>::~StateRepository() = default;

template<TaskKind Kind>
StateRepositoryPtr<Kind> StateRepository<Kind>::make_worker() const
{
    const auto index = m_impl->next_index->fetch_add(1, std::memory_order_relaxed);
    if (m_impl->evaluator)
        return StateRepositoryPtr<Kind>(new StateRepository<Kind>(std::make_unique<Impl>(index, m_impl->definition, m_impl->next_index)));

    return StateRepositoryPtr<Kind>(
        new StateRepository<Kind>(std::make_unique<Impl>(index, m_impl->definition, m_impl->shared_evaluator->storage, m_impl->next_index)));
}

template<TaskKind Kind>
StateView<Kind> StateRepository<Kind>::get_initial_state(AxiomEvaluator<Kind>& axiom_evaluator)
{
    auto state_builder = get_state_builder();
    detail::StateRepositoryPolicy<Kind>::insert_initial_fluent_facts(*m_impl->definition->task, *state_builder);

    for (const auto fterm_value : m_impl->definition->task->get_task().template get_fterm_values<::tyr::formalism::FluentTag>())
        state_builder->set(fterm_value.get_fterm().get_index(), fterm_value.get_value());

    return register_state(axiom_evaluator, std::move(state_builder));
}

template<TaskKind Kind>
StateView<Kind> StateRepository<Kind>::get_registered_state(ygg::Index<State<Kind>> state_index)
{
    auto state_builder = get_state_builder();

    state_builder->set(state_index);
    m_impl->visit_evaluator(
        [&](auto& evaluator)
        {
            const auto& packed_state = evaluator.storage->packed_states[state_index];
            evaluator.fluent_backend.unpack(packed_state.template get_atoms<::tyr::formalism::FluentTag>(),
                                            state_builder->template get_atoms<::tyr::formalism::FluentTag>());
            evaluator.derived_backend.unpack(packed_state.template get_atoms<::tyr::formalism::DerivedTag>(),
                                             state_builder->template get_atoms<::tyr::formalism::DerivedTag>());
            evaluator.numeric_backend.unpack(packed_state.get_numeric_variables(), state_builder->get_numeric_variables());
        });

    return StateView<Kind>(this->shared_from_this(), std::move(state_builder));
}

template<TaskKind Kind>
StateView<Kind> StateRepository<Kind>::create_state(
    AxiomEvaluator<Kind>& axiom_evaluator,
    const std::vector<ygg::Data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>>& fluent_facts,
    const std::vector<std::pair<ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>>, ygg::float_t>>& fterm_values)
{
    auto state_builder = get_state_builder();

    for (const auto& fact : fluent_facts)
        state_builder->set(fact);
    for (const auto& [fterm, value] : fterm_values)
        state_builder->set(fterm, value);

    return register_state(axiom_evaluator, std::move(state_builder));
}

template<TaskKind Kind>
StateView<Kind>
StateRepository<Kind>::create_state(AxiomEvaluator<Kind>& axiom_evaluator,
                                    const std::vector<::tyr::formalism::planning::FDRFactView<::tyr::formalism::FluentTag>>& fluent_facts,
                                    const std::vector<::tyr::formalism::planning::FunctionTermViewValuePair<::tyr::GroundTag, ::tyr::formalism::FluentTag>>& fterm_values)
{
    auto state_builder = get_state_builder();

    for (const auto& fact : fluent_facts)
        state_builder->set(fact.get_data());
    for (const auto& [fterm, value] : fterm_values)
        state_builder->set(fterm.get_index(), value);

    return register_state(axiom_evaluator, std::move(state_builder));
}

template<TaskKind Kind>
ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>, true> StateRepository<Kind>::get_state_builder()
{
    auto state_builder = m_impl->visit_evaluator([](auto& evaluator) { return evaluator.state_builder_pool.get_or_allocate(); });
    detail::StateRepositoryPolicy<Kind>::prepare_builder(*m_impl->definition->task, *state_builder);
    return state_builder;
}

template<TaskKind Kind>
StateView<Kind> StateRepository<Kind>::register_state(AxiomEvaluator<Kind>& axiom_evaluator, ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>, true> state)
{
    if (axiom_evaluator.get_task() != m_impl->definition->task)
        throw std::invalid_argument("StateRepository::register_state(...): axiom evaluator belongs to a different task.");
    axiom_evaluator.compute_extended_state(*state);
    return register_extended_state(std::move(state));
}

template<TaskKind Kind>
StateView<Kind> StateRepository<Kind>::register_extended_state(ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>, true> state)
{
    m_impl->visit_evaluator(
        [&](auto& evaluator)
        {
            auto& packed_states = evaluator.storage->packed_states;
            using PackedStates = std::remove_cvref_t<decltype(packed_states)>;
            if constexpr (!PackedStates::thread_safe)
            {
                state->set(packed_states
                               .insert(ygg::Data<State<Kind>>(ygg::Index<State<Kind>>(packed_states.size()),
                                                              evaluator.fluent_backend.insert(state->template get_atoms<::tyr::formalism::FluentTag>()),
                                                              evaluator.derived_backend.insert(state->template get_atoms<::tyr::formalism::DerivedTag>()),
                                                              evaluator.numeric_backend.insert(state->get_numeric_variables())))
                               .first);
            }
            else
            {
                const auto candidate = ygg::Data<State<Kind>>(ygg::Index<State<Kind>>::max(),
                                                              evaluator.fluent_backend.insert(state->template get_atoms<::tyr::formalism::FluentTag>()),
                                                              evaluator.derived_backend.insert(state->template get_atoms<::tyr::formalism::DerivedTag>()),
                                                              evaluator.numeric_backend.insert(state->get_numeric_variables()));
                const auto hash = PackedStates::hash(candidate);
                auto state_index = packed_states.find_with_hash(candidate, hash);
                if (!state_index)
                {
                    state_index.emplace(packed_states
                                            .complete_miss_with_hash(hash,
                                                                     candidate,
                                                                     [&](ygg::Index<State<Kind>> index)
                                                                     {
                                                                         return ygg::Data<State<Kind>>(
                                                                             index,
                                                                             candidate.template get_atoms<::tyr::formalism::FluentTag>(),
                                                                             candidate.template get_atoms<::tyr::formalism::DerivedTag>(),
                                                                             candidate.get_numeric_variables());
                                                                     })
                                            .first);
                }
                state->set(*state_index);
            }
        });

    return StateView<Kind>(this->shared_from_this(), std::move(state));
}

template<TaskKind Kind>
size_t StateRepository<Kind>::memory_usage() const noexcept
{
    return m_impl->visit_evaluator([](const auto& evaluator)
                                   { return evaluator.storage->context.memory_usage() + evaluator.storage->packed_states.memory_usage(); });
}

template<TaskKind Kind>
const TaskPtr<Kind>& StateRepository<Kind>::get_task() const noexcept
{
    return m_impl->definition->task;
}

template<TaskKind Kind>
bool StateRepository<Kind>::shares_storage_with(const StateRepository& other) const noexcept
{
    return m_impl->storage_identity == other.m_impl->storage_identity;
}

template<TaskKind Kind>
bool StateRepository<Kind>::is_concurrent() const noexcept
{
    return static_cast<bool>(m_impl->shared_evaluator);
}

template<TaskKind Kind>
ygg::uint_t StateRepository<Kind>::get_index() const noexcept
{
    return m_impl->index;
}

template<TaskKind Kind>
ygg::uint_t StateRepository<Kind>::get_storage_identity() const noexcept
{
    return m_impl->storage_identity;
}

template<TaskKind Kind>
size_t StateRepository<Kind>::num_states() const noexcept
{
    return m_impl->visit_evaluator([](const auto& evaluator) { return evaluator.storage->packed_states.size(); });
}

}

#endif
