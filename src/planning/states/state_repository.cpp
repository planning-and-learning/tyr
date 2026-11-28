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

#include "tyr/planning/states/state_repository.hpp"

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
#include "tyr/planning/axioms/axiom_evaluator.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"
#include "tyr/planning/states/state_builder.hpp"
#include "tyr/planning/states/state_data.hpp"
#include "tyr/planning/states/state_delta.hpp"
#include "tyr/planning/states/successor_hint.hpp"
#include "tyr/planning/states/storages/state_storage.hpp"
#include "tyr/planning/states/storages/policies/config.hpp"
#include "tyr/planning/states/storages/policies/policy.hpp"
#include "tyr/planning/successors/successor_builder.hpp"

#include <atomic>
#include <cassert>
#include <memory>
#include <optional>
#include <span>
#include <type_traits>
#include <urd/elements/less.hpp>
#include <urd/repr/records/records.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <yggdrasil/formalism/builder_impl.hpp>
#include <yggdrasil/formalism/builder_instantiations.hpp>
#include <yggdrasil/formalism/keyed_impl.hpp>
#include <yggdrasil/formalism/part_instantiations.hpp>
#include <yggdrasil/formalism/row_builder.hpp>
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
    for (const auto fact : task.get_task().fluent_facts())
        state.set(fact);
}

void insert_initial_fluent_facts(const Task<LiftedTag>& task, urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>& state)
{
    for (const auto atom : task.get_task().template atoms<formalism::FluentTag>())
        state.set(task.get_fdr_context()->get_fact(atom));
}

void prepare_builder(const Task<GroundTag>& task, urd::builder<State<GroundTag>, StateRepository<GroundTag>>& state)
{
    state.clear();
    state.resize_fluent_facts(task.get_task().fluent_variables().size());
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
        explicit Storage(const Task<Kind>& task) :
            identity(next_state_storage_identity<Kind>()),
            context(task),
            records()
        {
        }

        ygg::uint_t identity;
        StateStorageContext<Kind, StateStoragePolicyTag, true> context;
        typename StateRepository<Kind>::Symbols records;
    };

    /// A worker's own: the stores of its storages over the shared pools (their scratch buffers its own), and its builder
    /// storage.
    struct Evaluator
    {
        explicit Evaluator(std::shared_ptr<Storage> storage_) :
            storage(std::move(storage_)),
            fact_repository(storage->context),
            atom_repository(storage->context),
            numeric_repository(storage->context),
            temporal_repository(storage->context),
            builders()
        {
        }

        std::shared_ptr<Storage> storage;
        StoragePool<FactStorage<Kind>> fact_repository;
        StoragePool<AtomStorage<Kind>> atom_repository;
        StoragePool<NumericStorage<Kind>> numeric_repository;
        StoragePool<TemporalStorage<Kind>> temporal_repository;
        /// The builders of states, successors and their parts (StateRepositoryBuilderTypes), and the scratch records
        /// they are interned through.
        StateRepositoryBuilderStorage<Kind> builders;
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
StateRepository<Kind>::StateRepository(const TaskPtr<Kind>& task, bool concurrent) :
    StateRepository(std::make_unique<Impl>(0, task, concurrent, std::make_shared<typename Impl::Storage>(*task), std::make_shared<std::atomic<ygg::uint_t>>(1)))
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
    return { { *this, evaluator.storage->records },
             { *this, evaluator.fact_repository },
             { *this, evaluator.atom_repository },
             { *this, evaluator.numeric_repository },
             { *this, evaluator.temporal_repository } };
}

template<TaskKind Kind>
auto StateRepository<Kind>::own_parts() -> OwnParts<true>
{
    auto& evaluator = m_impl->evaluator;
    return { { *this, evaluator.storage->records },
             { *this, evaluator.fact_repository },
             { *this, evaluator.atom_repository },
             { *this, evaluator.numeric_repository },
             { *this, evaluator.temporal_repository } };
}

template<TaskKind Kind>
StateRepositoryPtr<Kind> StateRepository<Kind>::create(const TaskPtr<Kind>& task, bool concurrent)
{ return StateRepositoryPtr<Kind>(new StateRepository<Kind>(task, concurrent)); }

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
StateRepositoryBuilderStorage<Kind>& StateRepository<Kind>::builders() const
{ return m_impl->evaluator.builders; }

template<TaskKind Kind>
size_t StateRepository<Kind>::memory_usage() const noexcept
{
    const auto& storage = *m_impl->evaluator.storage;
    return storage.context.memory_usage() + urd::memory_usage(storage.records.template get<State<Kind>>());
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

template class StateRepository<GroundTag>;
template class StateRepository<LiftedTag>;

namespace
{
/// A storage of a state (held built) interned through `repository`'s generic insert_built, as a difference to the
/// parent's where `hint` gives one (its part gives its store the hint where the store takes one).
template<typename S, TaskKind Kind>
urd::key<S> intern_storage(StateRepository<Kind>& repository, const urd::built_child<S, StateRepository<Kind>>& storage, const SuccessorHint<Kind>* hint)
{
    if (hint != nullptr)
        return repository.insert_built(*storage, *hint).first.index();
    else
        return repository.insert_built(*storage).first.index();
}

/// The record of `state`'s identity: its identifying storages interned (as differences to the parent's where `hint`
/// gives one), its derived atoms not yet.
template<TaskKind Kind>
urd::data<State<Kind>>
intern_identity(StateRepository<Kind>& repository, const urd::builder<State<Kind>, StateRepository<Kind>>& state, const SuccessorHint<Kind>* hint)
{
    auto record = urd::data<State<Kind>>();
    record.fact_storage = intern_storage(repository, state.fact_storage, hint);
    record.numeric_storage = intern_storage(repository, state.numeric_storage, hint);
    record.temporal_storage = intern_storage(repository, state.temporal_storage, hint);
    return record;
}

/// `state` as the registered state `stored`: its derived atoms read from it, its index set.
template<TaskKind Kind, typename Stored>
void take_registered(const StateRepository<Kind>& repository, urd::builder<State<Kind>, StateRepository<Kind>>& state, const Stored& stored)
{
    repository.unpack_built(repository[stored.index()]->atom_storage, *state.atom_storage);
    state.set(stored.index());
}

/// The lookup by identity: whether a state of `state`'s identifying storages is registered; if so, `state` takes it.
template<TaskKind Kind>
bool find_by_identity(StateRepository<Kind>& repository, urd::builder<State<Kind>, StateRepository<Kind>>& state, const SuccessorHint<Kind>* hint)
{
    auto record = intern_identity(repository, state, hint);
    const auto stored = repository.find(record);
    if (!stored)
        return false;
    take_registered(repository, state, *stored);
    return true;
}

/// Registers `state`, which holds its axiom closure: its storages interned, its record inserted, its index set (a
/// state registered meanwhile is taken instead).
// ponytail: after a lookup the identifying storages are interned a second time (found at once, along the changed path
// with a parent). Measured within noise (A* blind, 26 strips cases); carry the lookup's keys if a profile says otherwise.
template<TaskKind Kind>
void insert_by_identity(StateRepository<Kind>& repository, urd::builder<State<Kind>, StateRepository<Kind>>& state, const SuccessorHint<Kind>* hint)
{
    auto record = intern_identity(repository, state, hint);
    record.atom_storage = intern_storage(repository, state.atom_storage, hint);
    const auto [inserted_view, inserted] = repository.insert(record);
    if (inserted)
        state.set(inserted_view.index());
    else
        take_registered(repository, state, inserted_view);
}

/// The hint of a successor `state` of `parent` by `delta`, where a delta insert applies: it reuses the parent's unchanged
/// subtrees by their node indices, so the parent's trees must live in this storage and the parent be registered. A
/// parent from separate storage would need its subtrees inserted here first, which is a full insert.
template<TaskKind Kind>
std::optional<SuccessorHint<Kind>> hint_of(const StateRepository<Kind>& repository, const StateBuilderView<Kind>& parent, const StateDelta& delta)
{
    if (parent.pack().index().is_none() || !parent.context().shares_storage_with(repository))
        return std::nullopt;
    return SuccessorHint<Kind> { &*repository[parent.pack().index()], &parent, &delta };
}

/// A state registered with every step: looked up, closed under the axioms where it is new, and registered.
template<TaskKind Kind>
void register_closed(StateRepository<Kind>& repository, AxiomEvaluator<Kind>& axiom_evaluator, urd::builder<State<Kind>, StateRepository<Kind>>& state)
{
    if (!find_by_identity(repository, state, static_cast<const SuccessorHint<Kind>*>(nullptr)))
    {
        axiom_evaluator.compute_extended_state(state, repository);
        insert_by_identity(repository, state, static_cast<const SuccessorHint<Kind>*>(nullptr));
    }
}

template<TaskKind Kind>
void check_task(const StateRepository<Kind>& repository, const AxiomEvaluator<Kind>& axiom_evaluator)
{
    if (axiom_evaluator.get_task() != repository.get_task())
        throw std::invalid_argument("state repository: the axiom evaluator belongs to a different task.");
}
}  // namespace

template<TaskKind Kind>
StateBuilderHandle<Kind> get_state_builder(const StateRepository<Kind>& repository)
{
    auto& builders = repository.builders();
    auto state = ygg::formalism::checkout<State<Kind>>(builders);
    prepare_builder(*repository.get_task(), *state);
    return state;
}

template<TaskKind Kind>
bool find_state(StateRepository<Kind>& repository, urd::builder<State<Kind>, StateRepository<Kind>>& state)
{ return find_by_identity(repository, state, static_cast<const SuccessorHint<Kind>*>(nullptr)); }

template<TaskKind Kind>
bool find_state(StateRepository<Kind>& repository,
                urd::builder<State<Kind>, StateRepository<Kind>>& state,
                const StateBuilderView<Kind>& parent,
                const StateDelta& delta)
{
    const auto hint = hint_of(repository, parent, delta);
    return find_by_identity(repository, state, hint.has_value() ? &*hint : nullptr);
}

template<TaskKind Kind>
void register_extended_state(StateRepository<Kind>& repository, urd::builder<State<Kind>, StateRepository<Kind>>& state)
{ insert_by_identity(repository, state, static_cast<const SuccessorHint<Kind>*>(nullptr)); }

template<TaskKind Kind>
void register_extended_state(StateRepository<Kind>& repository,
                             urd::builder<State<Kind>, StateRepository<Kind>>& state,
                             const StateBuilderView<Kind>& parent,
                             const StateDelta& delta)
{
    const auto hint = hint_of(repository, parent, delta);
    insert_by_identity(repository, state, hint.has_value() ? &*hint : nullptr);
}

template<TaskKind Kind>
StateBuilderHandle<Kind> register_extended_state(StateRepository<Kind>& repository, const StateBuilderView<Kind>& source)
{
    if (source.context().get_task().get() != repository.get_task().get())
        throw std::invalid_argument("register_extended_state(...): source state belongs to a different task.");
    auto state = get_state_builder(repository);
    state->assign(source);
    state->index = {};
    register_extended_state(repository, *state);
    return state;
}

template<TaskKind Kind>
StateBuilderHandle<Kind> get_initial_state(StateRepository<Kind>& repository, AxiomEvaluator<Kind>& axiom_evaluator)
{
    const auto& task = *repository.get_task();
    auto state = get_state_builder(repository);
    insert_initial_fluent_facts(task, *state);
    for (const auto fterm_value : task.get_task().template fterm_values<formalism::FluentTag>())
        state->set(fterm_value.fterm().index(), fterm_value.value());
    check_task(repository, axiom_evaluator);
    register_closed(repository, axiom_evaluator, *state);
    return state;
}

template<TaskKind Kind>
StateBuilderHandle<Kind>
create_state(StateRepository<Kind>& repository,
             AxiomEvaluator<Kind>& axiom_evaluator,
             const urd::Store<urd::sequence<urd::data<formalism::FDRFact<formalism::FluentTag>>>>& fluent_facts,
             const urd::Store<urd::sequence<std::pair<urd::index<formalism::FunctionTerm<GroundTag, formalism::FluentTag>>, ygg::float_t>>>& fterm_values)
{
    auto state = get_state_builder(repository);
    for (const auto& fact : fluent_facts)
        state->set(fact);
    for (const auto& [fterm, value] : fterm_values)
        state->set(fterm, value);
    check_task(repository, axiom_evaluator);
    register_closed(repository, axiom_evaluator, *state);
    return state;
}

template<TaskKind Kind, urd::ViewOf<formalism::FDRFact<formalism::FluentTag>> FactView>
StateBuilderHandle<Kind> create_state(StateRepository<Kind>& repository,
                                      AxiomEvaluator<Kind>& axiom_evaluator,
                                      const urd::Store<urd::sequence<FactView>>& fluent_facts,
                                      const urd::Store<urd::sequence<formalism::FunctionTermViewValuePair<GroundTag, formalism::FluentTag>>>& fterm_values)
{
    auto state = get_state_builder(repository);
    for (const auto& fact : fluent_facts)
        state->set(fact);
    for (const auto& [fterm, value] : fterm_values)
        state->set(fterm.index(), value);
    check_task(repository, axiom_evaluator);
    register_closed(repository, axiom_evaluator, *state);
    return state;
}

template<TaskKind Kind>
StateBuilderHandle<Kind> unpack(const StateRepository<Kind>& repository, const urd::data<State<Kind>>& packed)
{
    auto state = get_state_builder(repository);
    repository.unpack_built(packed.fact_storage, *state->fact_storage);
    repository.unpack_built(packed.atom_storage, *state->atom_storage);
    repository.unpack_built(packed.numeric_storage, *state->numeric_storage);
    repository.unpack_built(packed.temporal_storage, *state->temporal_storage);
    return state;
}

template<TaskKind Kind>
StateBuilderHandle<Kind> get_registered_state(const StateRepository<Kind>& repository, urd::index<State<Kind>> state_index)
{
    auto state = unpack(repository, *repository[state_index]);
    state->set(state_index);
    return state;
}

template<TaskKind Kind>
SharedStateBuilderView<Kind> share(const StateRepository<Kind>& repository, StateBuilderHandle<Kind> state)
{
    // The handle and the repository, held together: the builder goes back to the pool before the repository goes.
    using Held = std::pair<std::shared_ptr<const StateRepository<Kind>>, StateBuilderHandle<Kind>>;
    auto held = std::make_shared<Held>(repository.shared_from_this(), std::move(state));
    auto* const record = held->second.get();
    return SharedStateBuilderView<Kind>(std::shared_ptr<const urd::builder<State<Kind>, StateRepository<Kind>>>(std::move(held), record), &repository);
}

template StateBuilderHandle<GroundTag> get_state_builder(const StateRepository<GroundTag>&);
template bool find_state(StateRepository<GroundTag>&, urd::builder<State<GroundTag>, StateRepository<GroundTag>>&);
template bool find_state(StateRepository<GroundTag>&, urd::builder<State<GroundTag>, StateRepository<GroundTag>>&, const StateBuilderView<GroundTag>&, const StateDelta&);
template void register_extended_state(StateRepository<GroundTag>&, urd::builder<State<GroundTag>, StateRepository<GroundTag>>&, const StateBuilderView<GroundTag>&, const StateDelta&);
template void register_extended_state(StateRepository<GroundTag>&, urd::builder<State<GroundTag>, StateRepository<GroundTag>>&);
template StateBuilderHandle<GroundTag> register_extended_state(StateRepository<GroundTag>&, const StateBuilderView<GroundTag>&);
template StateBuilderHandle<GroundTag> get_initial_state(StateRepository<GroundTag>&, AxiomEvaluator<GroundTag>&);
template StateBuilderHandle<GroundTag>
create_state(StateRepository<GroundTag>&,
             AxiomEvaluator<GroundTag>&,
             const urd::Store<urd::sequence<urd::data<formalism::FDRFact<formalism::FluentTag>>>>&,
             const urd::Store<urd::sequence<std::pair<urd::index<formalism::FunctionTerm<GroundTag, formalism::FluentTag>>, ygg::float_t>>>&);
template StateBuilderHandle<GroundTag> create_state(StateRepository<GroundTag>&,
                                               AxiomEvaluator<GroundTag>&,
                                               const urd::Store<urd::sequence<formalism::FDRFactView<formalism::FluentTag>>>&,
                                               const urd::Store<urd::sequence<formalism::FunctionTermViewValuePair<GroundTag, formalism::FluentTag>>>&);
template StateBuilderHandle<GroundTag> create_state(StateRepository<GroundTag>&,
                                               AxiomEvaluator<GroundTag>&,
                                               const urd::Store<urd::sequence<urd::owning_view<formalism::FDRFact<formalism::FluentTag>, formalism::Repository>>>&,
                                               const urd::Store<urd::sequence<formalism::FunctionTermViewValuePair<GroundTag, formalism::FluentTag>>>&);
template StateBuilderHandle<GroundTag> unpack(const StateRepository<GroundTag>&, const urd::data<State<GroundTag>>&);
template StateBuilderHandle<GroundTag> get_registered_state(const StateRepository<GroundTag>&, urd::index<State<GroundTag>>);
template SharedStateBuilderView<GroundTag> share(const StateRepository<GroundTag>&, StateBuilderHandle<GroundTag>);

template StateBuilderHandle<LiftedTag> get_state_builder(const StateRepository<LiftedTag>&);
template bool find_state(StateRepository<LiftedTag>&, urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>&);
template bool find_state(StateRepository<LiftedTag>&, urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>&, const StateBuilderView<LiftedTag>&, const StateDelta&);
template void register_extended_state(StateRepository<LiftedTag>&, urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>&, const StateBuilderView<LiftedTag>&, const StateDelta&);
template void register_extended_state(StateRepository<LiftedTag>&, urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>&);
template StateBuilderHandle<LiftedTag> register_extended_state(StateRepository<LiftedTag>&, const StateBuilderView<LiftedTag>&);
template StateBuilderHandle<LiftedTag> get_initial_state(StateRepository<LiftedTag>&, AxiomEvaluator<LiftedTag>&);
template StateBuilderHandle<LiftedTag>
create_state(StateRepository<LiftedTag>&,
             AxiomEvaluator<LiftedTag>&,
             const urd::Store<urd::sequence<urd::data<formalism::FDRFact<formalism::FluentTag>>>>&,
             const urd::Store<urd::sequence<std::pair<urd::index<formalism::FunctionTerm<GroundTag, formalism::FluentTag>>, ygg::float_t>>>&);
template StateBuilderHandle<LiftedTag> create_state(StateRepository<LiftedTag>&,
                                               AxiomEvaluator<LiftedTag>&,
                                               const urd::Store<urd::sequence<formalism::FDRFactView<formalism::FluentTag>>>&,
                                               const urd::Store<urd::sequence<formalism::FunctionTermViewValuePair<GroundTag, formalism::FluentTag>>>&);
template StateBuilderHandle<LiftedTag> create_state(StateRepository<LiftedTag>&,
                                               AxiomEvaluator<LiftedTag>&,
                                               const urd::Store<urd::sequence<urd::owning_view<formalism::FDRFact<formalism::FluentTag>, formalism::Repository>>>&,
                                               const urd::Store<urd::sequence<formalism::FunctionTermViewValuePair<GroundTag, formalism::FluentTag>>>&);
template StateBuilderHandle<LiftedTag> unpack(const StateRepository<LiftedTag>&, const urd::data<State<LiftedTag>>&);
template StateBuilderHandle<LiftedTag> get_registered_state(const StateRepository<LiftedTag>&, urd::index<State<LiftedTag>>);
template SharedStateBuilderView<LiftedTag> share(const StateRepository<LiftedTag>&, StateBuilderHandle<LiftedTag>);


/// The keyed members of the repositories' parts: their states, and their storages' pools.
using GroundStateRepository = StateRepository<GroundTag>;
using LiftedStateRepository = StateRepository<LiftedTag>;

}

YGG_FORMALISM_BUILDER(, tyr::planning::StateRepositoryBuilderStorage<tyr::GroundTag>, 14)
YGG_FORMALISM_BUILDER(, tyr::planning::StateRepositoryBuilderStorage<tyr::LiftedTag>, 14)
YGG_FORMALISM_PART(, tyr::planning::GroundStateRepository, 0, 9)
YGG_FORMALISM_PART(, tyr::planning::GroundStateRepository, 1, 1)
YGG_FORMALISM_PART(, tyr::planning::GroundStateRepository, 2, 1)
YGG_FORMALISM_PART(, tyr::planning::GroundStateRepository, 3, 1)
YGG_FORMALISM_PART(, tyr::planning::GroundStateRepository, 4, 1)
YGG_FORMALISM_PART(, tyr::planning::LiftedStateRepository, 0, 9)
YGG_FORMALISM_PART(, tyr::planning::LiftedStateRepository, 1, 1)
YGG_FORMALISM_PART(, tyr::planning::LiftedStateRepository, 2, 1)
YGG_FORMALISM_PART(, tyr::planning::LiftedStateRepository, 3, 1)
YGG_FORMALISM_PART(, tyr::planning::LiftedStateRepository, 4, 1)
