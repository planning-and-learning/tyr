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

#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/planning/axiom_evaluator.hpp"
#include "tyr/planning/state_storage/policy.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/state_data.hpp"
#include "tyr/planning/state_delta.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/config.hpp"

#include <atomic>
#include <cassert>
#include <memory>
#include <span>
#include <type_traits>
#include <urd/elements/less.hpp>
#include <urd/repr/records/records.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <yggdrasil/formalism/keyed_impl.hpp>
#include <yggdrasil/formalism/part_instantiations.hpp>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::planning
{
namespace
{

/// Identifies the storage cohorts of a task kind; repositories share storage iff their identities match.
template<TaskKind Kind>
ygg::uint_t next_state_storage_identity() noexcept
{
    static auto next = std::atomic<ygg::uint_t> { 0 };
    return next.fetch_add(1, std::memory_order_relaxed);
}

void insert_initial_fluent_facts(const Task<GroundTag>& task, urd::builder<State<GroundTag>, StateRepository<GroundTag>>& state)
{
    for (const auto fact : task.get_task().get_fluent_facts())
        state.set(fact);
}

void insert_initial_fluent_facts(const Task<LiftedTag>& task, urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>& state)
{
    for (const auto atom : task.get_task().template get_atoms<formalism::FluentTag>())
        state.set(task.get_fdr_context()->get_fact(atom));
}

void prepare_builder(const Task<GroundTag>& task, urd::builder<State<GroundTag>, StateRepository<GroundTag>>& state)
{
    state.clear();
    state.resize_fluent_facts(task.get_task().get_fluent_variables().size());
}

void prepare_builder(const Task<LiftedTag>&, urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>& state) { state.clear(); }

}

template<TaskKind Kind>
struct StateRepository<Kind>::Impl
{
    /// What a cohort of workers shares: its states' records and its storages' pools (thread-safe: a private repository
    /// is a cohort of one; ponytail: it pays the pools' locks, single-threaded pools if that ever shows in a profile).
    struct Storage
    {
        explicit Storage(const Task<Kind>& task) : identity(next_state_storage_identity<Kind>()), context(task), packed_states(urd::layer { 0 }) {}

        ygg::uint_t identity;
        StateStorageContext<Kind, StateStoragePolicyTag, true> context;
        typename StateRepository<Kind>::PackedStates packed_states;
    };

    /// A worker's own: the stores of its storages over the shared pools (their scratch buffers its own), and its pool of
    /// state builders.
    struct Evaluator
    {
        explicit Evaluator(std::shared_ptr<Storage> storage_) :
            storage(std::move(storage_)),
            fact_repository(storage->context),
            atom_repository(storage->context),
            numeric_repository(storage->context),
            state_builder_pool()
        {
        }

        std::shared_ptr<Storage> storage;
        StoragePool<FactStorage<Kind>> fact_repository;
        StoragePool<AtomStorage<Kind>> atom_repository;
        StoragePool<NumericStorage<Kind>> numeric_repository;
        urd::Store<urd::slots<urd::builder<State<Kind>, StateRepository<Kind>>>, urd::concurrent> state_builder_pool;

        /// The fluent facts of `state`, stored as `delta` to `parent` if given: their key.
        urd::key<FactStorage<Kind>>
        insert_fluents(const urd::builder<State<Kind>, StateRepository<Kind>>& state, const urd::data<State<Kind>>* parent, const StateDelta* delta)
        {
            const auto& unpacked = state.fact_storage;
            return urd::key<FactStorage<Kind>>(parent ? fact_repository.insert(unpacked, parent->fact_storage.value, std::span(delta->fluents)) :
                                                          fact_repository.insert(unpacked));
        }

        /// The derived atoms of `state`, stored as a difference to `parent_state` if given: their key. Axioms recompute
        /// derived atoms, so their changes are not part of a StateDelta.
        urd::key<AtomStorage<Kind>>
        insert_derived(const urd::builder<State<Kind>, StateRepository<Kind>>& state, const urd::data<State<Kind>>* parent, const StateView<Kind>* parent_state)
        {
            const auto& unpacked = state.atom_storage;
            return urd::key<AtomStorage<Kind>>(parent ? atom_repository.insert(unpacked, parent->atom_storage.value, parent_state->get_atom_storage()) :
                                                          atom_repository.insert(unpacked));
        }

        /// The numeric values of `state`, stored as `delta` to `parent` if given: their key.
        urd::key<NumericStorage<Kind>>
        insert_numeric(urd::builder<State<Kind>, StateRepository<Kind>>& state, const urd::data<State<Kind>>* parent, const StateDelta* delta)
        {
            auto& unpacked = state.numeric_storage;
            return urd::key<NumericStorage<Kind>>(parent ? numeric_repository.insert(unpacked, parent->numeric_storage.value, std::span(delta->numerics)) :
                                                             numeric_repository.insert(unpacked));
        }

        /// `parent_state` and `delta` are either both null, or a registered state of this storage and the exact changes from it to `state`.
        /// Its record found, or inserted, by `repository`'s keyed find and insert (its states' part, this storage's pool).
        template<typename ExtendState>
        void register_state(StateRepository<Kind>& repository,
                            urd::builder<State<Kind>, StateRepository<Kind>>& state,
                            ExtendState&& extend_state,
                            const StateView<Kind>* parent_state = nullptr,
                            const StateDelta* delta = nullptr)
        {
            assert((parent_state == nullptr) == (delta == nullptr));
#ifndef NDEBUG
            if (delta)
            {
                auto expected = StateDelta {};
                compute_state_delta(StateContext<Kind> { *parent_state->context().get_task(), *parent_state, ygg::float_t { 0 } },
                                    StateContext<Kind> { repository, state, ygg::float_t { 0 } },
                                    expected);
                assert(expected == *delta && "register_state: delta does not match the changes from the parent.");
            }
#endif
            const auto* parent = parent_state ? &*repository[parent_state->pack().index()] : nullptr;
            // Identified by its fluent facts and numeric values: its derived atoms are computed only for a new state.
            auto record = urd::data<State<Kind>>(insert_fluents(state, parent, delta), {}, insert_numeric(state, parent, delta));
            auto stored = repository.find(record);

            if (!stored)
            {
                std::forward<ExtendState>(extend_state)();
                record.atom_storage = insert_derived(state, parent, parent_state);
                const auto [inserted_view, inserted] = repository.insert(record);
                if (inserted)
                {
                    state.set(inserted_view.index());
                    return;
                }
                stored = inserted_view;
            }

            atom_repository.unpack(repository[stored->index()]->atom_storage.value, state.atom_storage);
            state.set(stored->index());
        }
    };

    Impl(ygg::uint_t index_, TaskPtr<Kind> task_, bool concurrent_, std::shared_ptr<Storage> storage, std::shared_ptr<std::atomic<ygg::uint_t>> next_index_) :
        index(index_),
        next_index(std::move(next_index_)),
        task(std::move(task_)),
        concurrent(concurrent_),
        evaluator(std::move(storage))
    {
    }

    ygg::uint_t index;
    std::shared_ptr<std::atomic<ygg::uint_t>> next_index;
    TaskPtr<Kind> task;
    /// Whether its workers share its storage (make_worker): else each worker has storage of its own.
    bool concurrent;
    Evaluator evaluator;
};

/// The root of a cohort is worker 0; make_worker numbers the others.
template<TaskKind Kind>
StateRepository<Kind>::StateRepository(TaskPtr<Kind> task, bool concurrent) :
    StateRepository(std::make_unique<Impl>(0,
                                           task,
                                           concurrent,
                                           std::make_shared<typename Impl::Storage>(*task),
                                           std::make_shared<std::atomic<ygg::uint_t>>(1)))
{
}

template<TaskKind Kind>
StateRepository<Kind>::StateRepository(std::unique_ptr<Impl> impl) :
    ygg::formalism::Nests<StateRepository<Kind>, ygg::formalism::Nested<const formalism::Repository>>(impl->task->get_repository()),
    m_impl(std::move(impl))
{
}

template<TaskKind Kind>
auto StateRepository<Kind>::own_parts() const -> OwnParts<false>
{
    const auto& evaluator = m_impl->evaluator;
    return { { *this, evaluator.storage->packed_states },
             { *this, evaluator.fact_repository },
             { *this, evaluator.atom_repository },
             { *this, evaluator.numeric_repository } };
}

template<TaskKind Kind>
auto StateRepository<Kind>::own_parts() -> OwnParts<true>
{
    auto& evaluator = m_impl->evaluator;
    return { { *this, evaluator.storage->packed_states },
             { *this, evaluator.fact_repository },
             { *this, evaluator.atom_repository },
             { *this, evaluator.numeric_repository } };
}

template<TaskKind Kind>
StateRepository<Kind>::~StateRepository() = default;

template<TaskKind Kind>
StateRepositoryPtr<Kind> StateRepository<Kind>::make_worker() const
{
    const auto index = m_impl->next_index->fetch_add(1, std::memory_order_relaxed);
    auto storage = m_impl->concurrent ? m_impl->evaluator.storage : std::make_shared<typename Impl::Storage>(*m_impl->task);
    return StateRepositoryPtr<Kind>(
        new StateRepository<Kind>(std::make_unique<Impl>(index, m_impl->task, m_impl->concurrent, std::move(storage), m_impl->next_index)));
}

template<TaskKind Kind>
StateView<Kind> StateRepository<Kind>::get_initial_state(AxiomEvaluator<Kind>& axiom_evaluator)
{
    auto state_builder = get_state_builder();
    insert_initial_fluent_facts(*m_impl->task, *state_builder);

    for (const auto fterm_value : m_impl->task->get_task().template get_fterm_values<formalism::FluentTag>())
        state_builder->set(fterm_value.get_fterm().index(), fterm_value.get_value());

    return register_state(axiom_evaluator, std::move(state_builder));
}

template<TaskKind Kind>
StateView<Kind> StateRepository<Kind>::get_registered_state(urd::index<State<Kind>> state_index) const
{
    auto state_builder = get_state_builder();
    state_builder->set(state_index);
    unpack_storages(*(*this)[state_index], *state_builder);
    return StateView<Kind>(std::move(state_builder), this);
}

template<TaskKind Kind>
StateView<Kind> StateRepository<Kind>::unpack(const urd::data<State<Kind>>& packed) const
{
    auto state_builder = get_state_builder();
    unpack_storages(packed, *state_builder);
    return StateView<Kind>(std::move(state_builder), this);
}

template<TaskKind Kind>
void StateRepository<Kind>::unpack_storages(const urd::data<State<Kind>>& packed, urd::builder<State<Kind>, StateRepository<Kind>>& state) const
{
    auto& evaluator = m_impl->evaluator;
    evaluator.fact_repository.unpack(packed.fact_storage.value, state.fact_storage);
    evaluator.atom_repository.unpack(packed.atom_storage.value, state.atom_storage);
    evaluator.numeric_repository.unpack(packed.numeric_storage.value, state.numeric_storage);
}

template<TaskKind Kind>
StateView<Kind> StateRepository<Kind>::create_state(
    AxiomEvaluator<Kind>& axiom_evaluator,
    const urd::Store<urd::sequence<urd::data<formalism::FDRFact<formalism::FluentTag>>>>& fluent_facts,
    const urd::Store<urd::sequence<std::pair<urd::index<formalism::FunctionTerm<GroundTag, formalism::FluentTag>>, ygg::float_t>>>& fterm_values)
{
    auto state_builder = get_state_builder();

    for (const auto& fact : fluent_facts)
        state_builder->set(fact);
    for (const auto& [fterm, value] : fterm_values)
        state_builder->set(fterm, value);

    return register_state(axiom_evaluator, std::move(state_builder));
}

template<TaskKind Kind>
template<typename FactView>
    requires std::same_as<typename FactView::value_type, formalism::FDRFact<formalism::FluentTag>>
StateView<Kind>
StateRepository<Kind>::create_state(AxiomEvaluator<Kind>& axiom_evaluator,
                                    const urd::Store<urd::sequence<FactView>>& fluent_facts,
                                    const urd::Store<urd::sequence<formalism::FunctionTermViewValuePair<GroundTag, formalism::FluentTag>>>& fterm_values)
{
    auto state_builder = get_state_builder();

    for (const auto& fact : fluent_facts)
        state_builder->set(fact);
    for (const auto& [fterm, value] : fterm_values)
        state_builder->set(fterm.index(), value);

    return register_state(axiom_evaluator, std::move(state_builder));
}

template<TaskKind Kind>
std::shared_ptr<urd::builder<State<Kind>, StateRepository<Kind>>> StateRepository<Kind>::get_state_builder() const
{
    // Each owner keeps the repository alive; the slot goes back to the pool with the last one.
    auto state_builder = m_impl->evaluator.state_builder_pool.make_shared(this->shared_from_this());
    prepare_builder(*m_impl->task, *state_builder);
    return state_builder;
}

template<TaskKind Kind>
StateView<Kind> StateRepository<Kind>::register_state(AxiomEvaluator<Kind>& axiom_evaluator,
                                                      std::shared_ptr<urd::builder<State<Kind>, StateRepository<Kind>>> state)
{
    if (axiom_evaluator.get_task() != m_impl->task)
        throw std::invalid_argument("StateRepository::register_state(...): axiom evaluator belongs to a different task.");
    m_impl->evaluator.register_state(*this, *state, [&] { axiom_evaluator.compute_extended_state(*state, *this); });
    return StateView<Kind>(std::move(state), this);
}

template<TaskKind Kind>
StateView<Kind> StateRepository<Kind>::register_state(AxiomEvaluator<Kind>& axiom_evaluator,
                                                      std::shared_ptr<urd::builder<State<Kind>, StateRepository<Kind>>> state,
                                                      const StateView<Kind>& parent,
                                                      const StateDelta& delta)
{
    if (axiom_evaluator.get_task() != m_impl->task)
        throw std::invalid_argument("StateRepository::register_state(...): axiom evaluator belongs to a different task.");
    // A delta insert reuses the parent's unchanged subtrees by their node indices, so the parent's trees must live in
    // this storage. A parent from separate storage would need its subtrees inserted here first, which is a full insert.
    const auto shares_storage = parent.context().shares_storage_with(*this);
    const auto* parent_state = shares_storage ? &parent : nullptr;
    const auto* parent_delta = shares_storage ? &delta : nullptr;
    m_impl->evaluator.register_state(*this, *state, [&] { axiom_evaluator.compute_extended_state(*state, *this); }, parent_state, parent_delta);
    return StateView<Kind>(std::move(state), this);
}

template<TaskKind Kind>
StateView<Kind> StateRepository<Kind>::register_extended_state(std::shared_ptr<urd::builder<State<Kind>, StateRepository<Kind>>> state)
{
    m_impl->evaluator.register_state(*this, *state, [] {});
    return StateView<Kind>(std::move(state), this);
}

template<TaskKind Kind>
StateView<Kind> StateRepository<Kind>::register_extended_state(const StateBuilderView<Kind>& source)
{
    if (source.context().get_task().get() != get_task().get())
        throw std::invalid_argument("StateRepository::register_extended_state(...): source state belongs to a different task.");
    auto state = get_state_builder();
    state->assign(source);
    return register_extended_state(std::move(state));
}

template<TaskKind Kind>
size_t StateRepository<Kind>::memory_usage() const noexcept
{
    const auto& storage = *m_impl->evaluator.storage;
    return storage.context.memory_usage() + urd::memory_usage(storage.packed_states);
}

template<TaskKind Kind>
const TaskPtr<Kind>& StateRepository<Kind>::get_task() const noexcept
{ return m_impl->task; }

template<TaskKind Kind>
bool StateRepository<Kind>::shares_storage_with(const StateRepository& other) const noexcept
{ return m_impl->evaluator.storage == other.m_impl->evaluator.storage; }

template<TaskKind Kind>
bool StateRepository<Kind>::is_concurrent() const noexcept
{ return m_impl->concurrent; }

template<TaskKind Kind>
ygg::uint_t StateRepository<Kind>::get_worker_index() const noexcept
{ return m_impl->index; }

template<TaskKind Kind>
std::size_t StateRepository<Kind>::layer() const noexcept
{ return m_impl->evaluator.storage->identity; }

template<TaskKind Kind>
size_t StateRepository<Kind>::num_states() const noexcept
{
    return m_impl->evaluator.storage->packed_states.size();
}

template class StateRepository<GroundTag>;
template class StateRepository<LiftedTag>;

template StateView<GroundTag>
StateRepository<GroundTag>::create_state(AxiomEvaluator<GroundTag>&,
                                         const urd::Store<urd::sequence<formalism::FDRFactView<formalism::FluentTag>>>&,
                                         const urd::Store<urd::sequence<formalism::FunctionTermViewValuePair<GroundTag, formalism::FluentTag>>>&);
template StateView<GroundTag>
StateRepository<GroundTag>::create_state(AxiomEvaluator<GroundTag>&,
                                         const urd::Store<urd::sequence<urd::owning_view<formalism::FDRFact<formalism::FluentTag>, formalism::Repository>>>&,
                                         const urd::Store<urd::sequence<formalism::FunctionTermViewValuePair<GroundTag, formalism::FluentTag>>>&);
template StateView<LiftedTag>
StateRepository<LiftedTag>::create_state(AxiomEvaluator<LiftedTag>&,
                                         const urd::Store<urd::sequence<formalism::FDRFactView<formalism::FluentTag>>>&,
                                         const urd::Store<urd::sequence<formalism::FunctionTermViewValuePair<GroundTag, formalism::FluentTag>>>&);
template StateView<LiftedTag>
StateRepository<LiftedTag>::create_state(AxiomEvaluator<LiftedTag>&,
                                         const urd::Store<urd::sequence<urd::owning_view<formalism::FDRFact<formalism::FluentTag>, formalism::Repository>>>&,
                                         const urd::Store<urd::sequence<formalism::FunctionTermViewValuePair<GroundTag, formalism::FluentTag>>>&);

template StateView<GroundTag>
materialize_state(const StateView<GroundTag>& source, StateRepository<GroundTag>& target, AxiomEvaluator<GroundTag>& axiom_evaluator);
template StateView<LiftedTag>
materialize_state(const StateView<LiftedTag>& source, StateRepository<LiftedTag>& target, AxiomEvaluator<LiftedTag>& axiom_evaluator);

/// The keyed members of the repositories' parts: their states, and their storages' pools.
using GroundStateRepository = StateRepository<GroundTag>;
using LiftedStateRepository = StateRepository<LiftedTag>;

}

YGG_FORMALISM_PART(, tyr::planning::GroundStateRepository, 0, 1)
YGG_FORMALISM_PART(, tyr::planning::GroundStateRepository, 1, 1)
YGG_FORMALISM_PART(, tyr::planning::GroundStateRepository, 2, 1)
YGG_FORMALISM_PART(, tyr::planning::GroundStateRepository, 3, 1)
YGG_FORMALISM_PART(, tyr::planning::LiftedStateRepository, 0, 1)
YGG_FORMALISM_PART(, tyr::planning::LiftedStateRepository, 1, 1)
YGG_FORMALISM_PART(, tyr::planning::LiftedStateRepository, 2, 1)
YGG_FORMALISM_PART(, tyr::planning::LiftedStateRepository, 3, 1)
