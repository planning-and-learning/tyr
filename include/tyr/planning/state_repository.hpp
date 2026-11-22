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

#ifndef TYR_PLANNING_STATE_REPOSITORY_HPP_
#define TYR_PLANNING_STATE_REPOSITORY_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/state_view.hpp"
#include "tyr/planning/task.hpp"

#include <atomic>
#include <boost/mp11.hpp>
#include <concepts>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <urd/config.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/slots/slots.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/keyed.hpp>
#include <yggdrasil/formalism/symbol_repository.hpp>

namespace tyr::planning
{

/// The pools of a state's storages as a part of its repository (ygg::formalism::KeyedRepository), read (Writable false)
/// or written into: each storage S interned under its key PackedStorage<S> (StateRepository::pack, its
/// StateComponentRepository), which interning a state's builder writes into its record; a storage being built viewed
/// through its task's formalism repository, as a state's view reads it.
template<TaskKind Kind, bool Writable>
class StoragePart
{
    std::conditional_t<Writable, StateRepository<Kind>, const StateRepository<Kind>>* m_repository;

public:
    /// The keys it resolves: its storages' packed keys.
    using KeyTypes = boost::mp11::mp_list<PackedStorage<FactStorage<Kind>>, PackedStorage<AtomStorage<Kind>>, PackedStorage<NumericStorage<Kind>>>;
    /// The repository a storage is viewed through: the state repository.
    template<typename T>
    using Owner = StateRepository<Kind>;
    /// Whether it writes records under keys K: one of its keys, where written into.
    template<typename K>
    static constexpr bool writes = Writable && boost::mp11::mp_contains<KeyTypes, K>::value;

    explicit StoragePart(std::conditional_t<Writable, StateRepository<Kind>, const StateRepository<Kind>>& repository) noexcept : m_repository(&repository) {}

    /// A storage being built, read through its task's formalism repository (never a temporary).
    template<typename S>
    [[nodiscard]] urd::borrowed_view<S, formalism::Repository, urd::builder<S, formalism::Repository>>
    view(const urd::builder<S, formalism::Repository>& storage URD_LIFETIMEBOUND) const noexcept
    { return { &storage, m_repository->get_task()->get_repository().get() }; }

    /// A storage interned into its pool: the view of its key, and whether it was new (the pools do not say: false).
    template<typename S, typename Storage>
        requires Writable
    std::pair<urd::index_view<S, StateRepository<Kind>, urd::key_t<S>>, bool> insert(const urd::builder<S, formalism::Repository>& storage,
                                                                                   Storage& /*builders*/) const
    { return { { m_repository->pack(storage), m_repository }, false }; }
};

/// Always owned by a StateRepositoryPtr (std::make_shared): the states it hands out keep it alive. A keyed repository
/// (ygg::formalism::KeyedRepository) of its own parts: its states' records, interned whole in a pool shared by its
/// workers, and the pools of their storages (StoragePart); its records are read, viewed, found and inserted, and a
/// state's builder interned, by the keyed members (operator[], at, view, find, insert), its domain operations below
/// (register_state: the axiom closure, deltas to a parent) storing through them.
template<TaskKind Kind>
class StateRepository : public ygg::formalism::KeyedRepository<StateRepository<Kind>>, public std::enable_shared_from_this<StateRepository<Kind>>
{
private:
    struct Impl;

    explicit StateRepository(std::unique_ptr<Impl> impl);

public:
    /// The states' records, each once (references stay valid; thread-safe, so that workers share it).
    using PackedStates = urd::Store<urd::pool<urd::data<State<Kind>>, urd::index<State<Kind>>>, urd::stable_refs, urd::layered, urd::concurrent>;

    /// A private repository; with `concurrent`, one whose workers (make_worker) share canonical state storage.
    explicit StateRepository(TaskPtr<Kind> task, bool concurrent = false);
    ~StateRepository();

    StateRepository(const StateRepository&) = delete;
    StateRepository& operator=(const StateRepository&) = delete;
    StateRepository(StateRepository&&) = delete;
    StateRepository& operator=(StateRepository&&) = delete;

    StateView<Kind> get_initial_state(AxiomEvaluator<Kind>& axiom_evaluator);

    /// The kind of record it holds: a repository nesting it resolves states here (ygg::formalism::Nests).
    using SymbolTypes = boost::mp11::mp_list<State<Kind>>;
    /// The keys it resolves itself: its states' indices, its storages' packed keys.
    using KeyTypes = boost::mp11::mp_append<boost::mp11::mp_list<urd::index<State<Kind>>>, typename StoragePart<Kind, false>::KeyTypes>;
    /// Its own parts, as read (Writable false) or written into (ygg::formalism::key_parts): its states, its storages.
    template<bool Writable>
    using OwnParts = std::tuple<ygg::formalism::HeldPart<std::conditional_t<Writable, StateRepository, const StateRepository>,
                                                         std::conditional_t<Writable, PackedStates, const PackedStates>,
                                                         boost::mp11::mp_list<urd::index<State<Kind>>>>,
                                StoragePart<Kind, Writable>>;

    /// No layer below it: its workers share its states instead.
    [[nodiscard]] const StateRepository* parent() const noexcept { return nullptr; }

    /// Const: the state's builder comes from the repository's pool of scratch builders.
    StateView<Kind> get_registered_state(urd::index<State<Kind>> state_index) const;

    /// A state record unpacked, its storages read from this repository's: a registered one's (get_registered_state, its
    /// index set) or a record held outside it, packed here but not registered (no index).
    StateView<Kind> unpack(const urd::data<State<Kind>>& packed) const;

    /// A state's storage unpacked from this repository's (urd::resolution<PackedStorage<S>>).
    urd::builder<FactStorage<Kind>, formalism::Repository> unpack(const PackedStorage<FactStorage<Kind>>& packed) const;
    urd::builder<AtomStorage<Kind>, formalism::Repository> unpack(const PackedStorage<AtomStorage<Kind>>& packed) const;
    urd::builder<NumericStorage<Kind>, formalism::Repository> unpack(const PackedStorage<NumericStorage<Kind>>& packed) const;

    /// A state's storage packed into this repository's (StoragePart::insert: interning a state's builder).
    PackedStorage<FactStorage<Kind>> pack(const urd::builder<FactStorage<Kind>, formalism::Repository>& storage);
    PackedStorage<AtomStorage<Kind>> pack(const urd::builder<AtomStorage<Kind>, formalism::Repository>& storage);
    PackedStorage<NumericStorage<Kind>> pack(const urd::builder<NumericStorage<Kind>, formalism::Repository>& storage);

    StateView<Kind>
    create_state(AxiomEvaluator<Kind>& axiom_evaluator,
                 const urd::Store<urd::sequence<urd::data<formalism::FDRFact<formalism::FluentTag>>>>& fluent_facts,
                 const urd::Store<urd::sequence<std::pair<urd::index<formalism::FunctionTerm<GroundTag, formalism::FluentTag>>, ygg::float_t>>>& fterm_values);
    /// The facts as views: borrowed (stored facts, FDRFactView) or owned (a state's, computed on the fly).
    template<typename FactView>
        requires std::same_as<typename FactView::value_type, formalism::FDRFact<formalism::FluentTag>>
    StateView<Kind> create_state(AxiomEvaluator<Kind>& axiom_evaluator,
                                 const urd::Store<urd::sequence<FactView>>& fluent_facts,
                                 const urd::Store<urd::sequence<formalism::FunctionTermViewValuePair<GroundTag, formalism::FluentTag>>>& fterm_values);

    std::shared_ptr<urd::builder<State<Kind>, StateRepository<Kind>>> get_state_builder() const;

    /// The builder must come from this repository and have no retained mutable aliases.
    StateView<Kind> register_state(AxiomEvaluator<Kind>& axiom_evaluator, std::shared_ptr<urd::builder<State<Kind>, StateRepository<Kind>>> state);
    /// Like register_state, but `delta` must be the exact changes from `parent` to `state`, as produced when generating a successor.
    /// If both share storage, state component repositories may insert `state` as `delta` to `parent`.
    StateView<Kind> register_state(AxiomEvaluator<Kind>& axiom_evaluator,
                                   std::shared_ptr<urd::builder<State<Kind>, StateRepository<Kind>>> state,
                                   const StateView<Kind>& parent,
                                   const StateDelta& delta);
    /// The builder must come from this repository, already contain its axiom closure, and have no retained mutable aliases.
    StateView<Kind> register_extended_state(std::shared_ptr<urd::builder<State<Kind>, StateRepository<Kind>>> state);
    /// Copies an already extended state of this task into repository-owned storage; source storage remains borrowed and unchanged.
    StateView<Kind> register_extended_state(const StateBuilderView<Kind>& source);

    [[nodiscard]] StateRepositoryPtr<Kind> make_worker() const;

    /// All repositories sharing this storage must be quiescent while inspecting memory usage.
    [[nodiscard]] size_t memory_usage() const noexcept;

    const TaskPtr<Kind>& get_task() const noexcept;
    bool shares_storage_with(const StateRepository& other) const noexcept;
    [[nodiscard]] bool is_concurrent() const noexcept;
    /// The index of this worker among the repositories sharing its storage.
    [[nodiscard]] ygg::uint_t get_worker_index() const noexcept;
    /// The repository's number, as urd's views ask of a context: the identity of the storage that owns the states,
    /// shared by all workers of a cohort.
    [[nodiscard]] std::size_t layer() const noexcept;
    [[nodiscard]] size_t num_states() const noexcept;

private:
    friend class ygg::formalism::KeyedRepository<StateRepository>;

    /// Its own parts as read: its states, its storages; written into, likewise.
    [[nodiscard]] OwnParts<false> own_parts() const { return { { *this, *m_states }, StoragePart<Kind, false>(*this) }; }

    [[nodiscard]] OwnParts<true> own_parts() { return { { *this, *m_states }, StoragePart<Kind, true>(*this) }; }

    /// A state record's storages, read from this repository's, into a builder.
    void unpack_storages(const urd::data<State<Kind>>& packed, urd::builder<State<Kind>, StateRepository<Kind>>& state) const;

    std::unique_ptr<Impl> m_impl;
    /// Its states' pool, in its Impl's storage (shared by its workers).
    PackedStates* m_states;
};

extern template class StateRepository<GroundTag>;
extern template class StateRepository<LiftedTag>;

template<TaskKind Kind>
StateView<Kind> materialize_state(const StateView<Kind>& source, StateRepository<Kind>& target, AxiomEvaluator<Kind>& axiom_evaluator)
{
    if (source.context().get_task() != target.get_task())
        throw std::invalid_argument("materialize_state(...): source state belongs to a different task.");
    if (axiom_evaluator.get_task() != target.get_task())
        throw std::invalid_argument("materialize_state(...): axiom evaluator belongs to a different task.");
    if (&source.context() == &target)
        return source;
    if (source.context().shares_storage_with(target))
        return target.get_registered_state(source.pack().index());

    auto builder = target.get_state_builder();
    builder->assign_unextended_part(source);
    return target.register_state(axiom_evaluator, std::move(builder));
}

extern template StateView<GroundTag>
materialize_state(const StateView<GroundTag>& source, StateRepository<GroundTag>& target, AxiomEvaluator<GroundTag>& axiom_evaluator);
extern template StateView<LiftedTag>
materialize_state(const StateView<LiftedTag>& source, StateRepository<LiftedTag>& target, AxiomEvaluator<LiftedTag>& axiom_evaluator);
}

#endif
