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

#ifndef TYR_PLANNING_STATES_STATE_REPOSITORY_HPP_
#define TYR_PLANNING_STATES_STATE_REPOSITORY_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/states/state_builder.hpp"
#include "tyr/planning/states/storages/state_storage.hpp"
#include "tyr/planning/states/state_view.hpp"
#include "tyr/planning/successors/distribution_data.hpp"
#include "tyr/planning/successors/outcome_node_data.hpp"
#include "tyr/planning/successors/successor_data.hpp"
#include "tyr/planning/successors/transition_data.hpp"
#include "tyr/planning/tasks/task.hpp"

#include <atomic>
#include <boost/mp11.hpp>
#include <boost/mp11/detail/mp_list.hpp>
#include <concepts>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <urd/config.hpp>
#include <urd/elements/builder.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/index.hpp>
#include <urd/kinds.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/slots/slots.hpp>
#include <urd/store.hpp>
#include <urd/tags.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/builder.hpp>
#include <yggdrasil/formalism/keyed.hpp>
#include <yggdrasil/formalism/nested.hpp>
#include <yggdrasil/formalism/part_instantiations.hpp>
#include <yggdrasil/formalism/symbol_repository.hpp>

namespace tyr::planning
{

/// The storages of a state, each once: its fluent facts, derived atoms, numeric values and temporal part. The state
/// repository's keys, pools and builders of them follow this list.
template<TaskKind Kind>
using StateStorageTypes = boost::mp11::mp_list<FactStorage<Kind>, AtomStorage<Kind>, NumericStorage<Kind>, TemporalStorage<Kind>>;

/// The builder types read through a state repository (yggdrasil's NamesList): a successor and its parts (a distribution,
/// an outcome, a transition's label, an action binding's row), a state and its storages. Each before the parts it holds:
/// a builder that went back to its pool holds its parts until it is checked out again, and a storage's pools are
/// destroyed in type order (ygg::Family), so the pools of its parts outlive its own.
template<TaskKind Kind>
struct StateRepositoryBuilderTypes :
    boost::mp11::mp_apply<ygg::formalism::Builders,
                          boost::mp11::mp_append<boost::mp11::mp_list<Successor<NondeterministicTag, Kind>,
                                                                      Successor<ProbabilisticTag, Kind>,
                                                                      Successor<GeneralTag, Kind>,
                                                                      Distribution<Kind>,
                                                                      OutcomeNode<NondeterministicTag, Kind>,
                                                                      OutcomeNode<ProbabilisticTag, Kind>,
                                                                      OutcomeNode<GeneralTag, Kind>,
                                                                      Transition,
                                                                      ygg::Row<formalism::Action<LiftedTag>>,
                                                                      State<Kind>>,
                                                 StateStorageTypes<Kind>>>
{
};

/// The pools of a state repository's builders and scratch records, interned into it.
template<TaskKind Kind>
using StateRepositoryBuilderStorage = ygg::formalism::BasicBuilderStorage<StateRepository<Kind>, StateRepositoryBuilderTypes<Kind>>;

/// The stores of records of kinds Ts that workers share (yggdrasil's SymbolStores, thread-safe).
template<typename... Ts>
using ConcurrentSymbolStores = ygg::formalism::SymbolStores<true, Ts...>;

/// Always owned by a StateRepositoryPtr (create): the states it hands out keep it alive. A keyed repository
/// (ygg::formalism::KeyedRepository) of its own parts, each a store shared by its workers: its states' records, and the
/// pools of their storages (StoragePool: a storage's key is its pool's handle); over its task's formalism repository,
/// nested read only, through which a state's storages read the task's facts. Its records are read, viewed, found and
/// inserted, and a state's builder interned (its storages first, into their pools), by the keyed members (operator[],
/// at, view, find, insert); its domain operations below (find_state, register_extended_state: deltas to a parent) store
/// through them.
template<TaskKind Kind>
class StateRepository :
    public ygg::formalism::KeyedRepository<StateRepository<Kind>>,
    public ygg::formalism::Nests<StateRepository<Kind>, ygg::formalism::Nested<const formalism::Repository>>,
    public std::enable_shared_from_this<StateRepository<Kind>>
{
private:
    struct Impl;

    explicit StateRepository(std::unique_ptr<Impl> impl);

    /// A private repository; with `concurrent`, one whose workers (make_worker) share canonical state storage.
    explicit StateRepository(const TaskPtr<Kind>& task, bool concurrent);

public:
    /// The kinds of record it stores, each once: its states, and the successors of each environment and their parts.
    using SymbolTypes = boost::mp11::mp_list<State<Kind>,
                                             Transition,
                                             OutcomeNode<NondeterministicTag, Kind>,
                                             OutcomeNode<ProbabilisticTag, Kind>,
                                             OutcomeNode<GeneralTag, Kind>,
                                             Distribution<Kind>,
                                             Successor<NondeterministicTag, Kind>,
                                             Successor<ProbabilisticTag, Kind>,
                                             Successor<GeneralTag, Kind>>;
    /// Their stores, one per kind (thread-safe, so that workers share them).
    using Symbols = boost::mp11::mp_apply<ConcurrentSymbolStores, SymbolTypes>;

    /// A repository of `task`'s states, shared (its states keep it alive); with `concurrent`, one whose workers
    /// (make_worker) share canonical state storage.
    [[nodiscard]] static StateRepositoryPtr<Kind> create(const TaskPtr<Kind>& task, bool concurrent = false);
    ~StateRepository();

    StateRepository(const StateRepository&) = delete;
    StateRepository& operator=(const StateRepository&) = delete;
    StateRepository(StateRepository&&) = delete;
    StateRepository& operator=(StateRepository&&) = delete;

    /// The keys of its records (a repository nesting it resolves them here: ygg::formalism::Nests).
    using SymbolKeys = boost::mp11::mp_transform<urd::key, SymbolTypes>;
    /// The keys of its states' storages (each its pool's handle).
    using StorageKeys = boost::mp11::mp_transform<urd::key, StateStorageTypes<Kind>>;
    /// The keys it resolves itself: its records', and its states' storages'.
    using KeyTypes = boost::mp11::mp_append<SymbolKeys, StorageKeys>;

    /// Its own parts' stores and the keys each resolves (ygg::formalism::HeldPart<StateRepository, P>): its records,
    /// and a pool per storage of its states.
    using PartStores = boost::mp11::mp_push_front<boost::mp11::mp_transform<StoragePool, StateStorageTypes<Kind>>, Symbols>;
    using PartKeys = boost::mp11::mp_push_front<boost::mp11::mp_transform<boost::mp11::mp_list, StorageKeys>, SymbolKeys>;

    /// Part P of its own, read (Writable false) or written into.
    template<bool Writable, std::size_t P>
    using Part = ygg::formalism::HeldPart<std::conditional_t<Writable, StateRepository, const StateRepository>, P>;

    /// Its own parts (ygg::formalism::key_parts).
    template<bool Writable>
    using OwnParts = std::tuple<Part<Writable, 0>, Part<Writable, 1>, Part<Writable, 2>, Part<Writable, 3>, Part<Writable, 4>>;

    /// No layer below it: its workers share its states instead.
    [[nodiscard]] const StateRepository* parent() const noexcept { return nullptr; }

    /// This worker's builder storage: builders checked out of it (ygg::formalism::checkout)
    /// are emptied, their buffers kept, so a state or successor written into reused ones allocates nothing once they have
    /// grown; its scratch records intern them (insert(builder, storage)). The repository must outlive its handles.
    StateRepositoryBuilderStorage<Kind>& builders() const;

    [[nodiscard]] StateRepositoryPtr<Kind> make_worker() const;

    /// All repositories sharing this storage must be quiescent while inspecting memory usage. Its own, as the storage
    /// pools' shared sets are not reached through their parts (KeyedRepository::memory_usage<T>).
    [[nodiscard]] size_t memory_usage() const noexcept;

    const TaskPtr<Kind>& get_task() const noexcept;
    bool shares_storage_with(const StateRepository& other) const noexcept;
    [[nodiscard]] bool is_concurrent() const noexcept;
    /// The index of this worker among the repositories sharing its storage.
    [[nodiscard]] ygg::uint_t get_worker_index() const noexcept;
    /// The repository's number, as urd's views ask of a context: the identity of the storage that owns the states,
    /// shared by all workers of a cohort.
    [[nodiscard]] std::size_t layer() const noexcept;

private:
    friend class ygg::formalism::KeyedRepository<StateRepository>;

    /// Its own parts as read; written into, likewise: its Impl's stores (state_repository.cpp).
    [[nodiscard]] OwnParts<false> own_parts() const;
    [[nodiscard]] OwnParts<true> own_parts();

    std::unique_ptr<Impl> m_impl;
};

extern template class StateRepository<GroundTag>;
extern template class StateRepository<LiftedTag>;

/// A state builder of `repository`'s storage, its storages' builders checked out with it, emptied for its task: what a
/// state is written into. It goes back with its handle, which the repository must outlive.
template<TaskKind Kind>
StateBuilderHandle<Kind> get_state_builder(const StateRepository<Kind>& repository);

/// The lookup: whether a state with the identity of `state` (a builder of `repository`: its fluent facts, numeric values
/// and temporal part) is registered already. If so, `state` is given that state's index and its derived atoms (the
/// axioms' result, read from the stored state), so its axioms need not be evaluated again. The lookup interns the
/// state's identifying storages; registering it afterwards finds them there.
template<TaskKind Kind>
bool find_state(StateRepository<Kind>& repository, urd::builder<State<Kind>, StateRepository<Kind>>& state);

/// Like find_state, `delta` the exact changes from `parent` to `state` (as generating a successor gives them): where
/// both share storage and `parent` is registered, each storage pool takes `state`'s as a difference to `parent`'s (a
/// SuccessorHint).
template<TaskKind Kind>
bool find_state(StateRepository<Kind>& repository,
                urd::builder<State<Kind>, StateRepository<Kind>>& state,
                const StateBuilderView<Kind>& parent,
                const StateDelta& delta);

/// Registers `state`, a builder of `repository` that holds its axiom closure (its derived atoms), in place: its storages
/// interned, its record inserted, its index set. No axiom is evaluated here.
template<TaskKind Kind>
void register_extended_state(StateRepository<Kind>& repository, urd::builder<State<Kind>, StateRepository<Kind>>& state);

/// Like register_extended_state, its storages stored as differences to `parent`'s by `delta` where both share storage
/// and `parent` is registered.
template<TaskKind Kind>
void register_extended_state(StateRepository<Kind>& repository,
                             urd::builder<State<Kind>, StateRepository<Kind>>& state,
                             const StateBuilderView<Kind>& parent,
                             const StateDelta& delta);

/// An already extended state of `repository`'s task, copied into a builder of it and registered; the source is read only.
template<TaskKind Kind>
StateBuilderHandle<Kind> register_extended_state(StateRepository<Kind>& repository, const StateBuilderView<Kind>& source);

/// The task's initial state, registered.
template<TaskKind Kind>
StateBuilderHandle<Kind> get_initial_state(StateRepository<Kind>& repository, AxiomEvaluator<Kind>& axiom_evaluator);

/// A state of the given fluent facts and function term values, registered.
template<TaskKind Kind>
StateBuilderHandle<Kind>
create_state(StateRepository<Kind>& repository,
             AxiomEvaluator<Kind>& axiom_evaluator,
             const urd::Store<urd::sequence<urd::data<formalism::FDRFact<formalism::FluentTag>>>>& fluent_facts,
             const urd::Store<urd::sequence<std::pair<urd::index<formalism::FunctionTerm<GroundTag, formalism::FluentTag>>, ygg::float_t>>>& fterm_values);

/// The facts as views: borrowed (stored facts, FDRFactView) or owned (a state's, computed on the fly).
template<TaskKind Kind, urd::ViewOf<formalism::FDRFact<formalism::FluentTag>> FactView>
StateBuilderHandle<Kind> create_state(StateRepository<Kind>& repository,
                                      AxiomEvaluator<Kind>& axiom_evaluator,
                                      const urd::Store<urd::sequence<FactView>>& fluent_facts,
                                      const urd::Store<urd::sequence<formalism::FunctionTermViewValuePair<GroundTag, formalism::FluentTag>>>& fterm_values);

/// A state record unpacked into a state builder, its storages read through `repository`'s generic unpack_built: a
/// registered one's (its index not set here) or one held outside, packed in its storage but not registered.
template<TaskKind Kind>
StateBuilderHandle<Kind> unpack(const StateRepository<Kind>& repository, const urd::data<State<Kind>>& packed);

/// A registered state unpacked into a state builder (its index set).
template<TaskKind Kind>
StateBuilderHandle<Kind> get_registered_state(const StateRepository<Kind>& repository, urd::index<State<Kind>> state_index);

/// The number of states `repository`'s storage holds (its generic size of State<Kind>).
template<TaskKind Kind>
[[nodiscard]] std::size_t num_states(const StateRepository<Kind>& repository)
{ return repository.template size<State<Kind>>(); }

/// A state held by its handle, shared: released with its last copy, keeping `repository` alive meanwhile (a state held
/// where nothing else keeps its repository, as in Python).
template<TaskKind Kind>
[[nodiscard]] SharedStateBuilderView<Kind> share(const StateRepository<Kind>& repository, StateBuilderHandle<Kind> state);

}

// The keyed members of the repositories' parts (their states and their storages' pools), instantiated once, in
// src/planning/state_repository.cpp (brokkr's include rule I7).
YGG_FORMALISM_PART(extern, tyr::planning::StateRepository<tyr::GroundTag>, 0, 9)
YGG_FORMALISM_PART(extern, tyr::planning::StateRepository<tyr::GroundTag>, 1, 1)
YGG_FORMALISM_PART(extern, tyr::planning::StateRepository<tyr::GroundTag>, 2, 1)
YGG_FORMALISM_PART(extern, tyr::planning::StateRepository<tyr::GroundTag>, 3, 1)
YGG_FORMALISM_PART(extern, tyr::planning::StateRepository<tyr::GroundTag>, 4, 1)
YGG_FORMALISM_PART(extern, tyr::planning::StateRepository<tyr::LiftedTag>, 0, 9)
YGG_FORMALISM_PART(extern, tyr::planning::StateRepository<tyr::LiftedTag>, 1, 1)
YGG_FORMALISM_PART(extern, tyr::planning::StateRepository<tyr::LiftedTag>, 2, 1)
YGG_FORMALISM_PART(extern, tyr::planning::StateRepository<tyr::LiftedTag>, 3, 1)
YGG_FORMALISM_PART(extern, tyr::planning::StateRepository<tyr::LiftedTag>, 4, 1)

#endif
